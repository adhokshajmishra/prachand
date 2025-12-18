#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>

#include "./libhttpserver/RequestRouter.h"
#include "./libhttpserver/WebServer.h"
#include "ConfigurationParser.h"
#include "ConnectionPool.h"
#include "RequestHandler.h"

#include "jwt-cpp/jwt.h"

bool authenticator(const std::string& path, HTTPMessage& request, std::shared_ptr<pqxx::connection> connection) {
    bool authenticated = false;

    // there will not be node key at time of enroll
    if (path == "/hello" || path == "/enroll") {
        return true;
    }

    // all expected requests are POST requests
    if (request.body.empty())
        return false;

    // Decode authorization header
    if (!request.header.count("Authorization"))
        return false;

    std::string jwt_token = request.header["Authorization"];
    auto decoded = jwt::decode(jwt_token);

    if (!decoded.has_payload_claim("access"))
        return false;

    if (!decoded.has_payload_claim("scope"))
        return false;

    auto identifier = decoded.get_payload_claim("access").as_string();
    auto scope = decoded.get_payload_claim("scope").as_string();

    try {
        nlohmann::json request_data = nlohmann::json::parse(request.body);

        // host identifier and node_key are mandatory for every request made by agent
        if (scope == "host") {
            std::string node_key;
            std::string host_identifier = identifier;
            pqxx::work transaction{*connection};
            pqxx::result result = transaction.exec_prepared("validate_node", host_identifier);
            transaction.commit();

            if (result.size() == 1) {
                // node key is the secret used for JWT
                node_key = result.at(0)["node_key"].as<std::string>();

                auto verifier = jwt::verify()
                                    .with_issuer("auth0")
                                    .with_claim("scope", jwt::claim(std::string("host")))
                                    .with_claim("access", jwt::claim(std::string(host_identifier)))
                                    .allow_algorithm(jwt::algorithm::hs256{node_key});
                std::error_code ec;
                verifier.verify(decoded, ec);
                if (ec) {
                    authenticated = false;
                }
                else {
                    authenticated = true;
                }
            }
        }
        else if (scope == "controller") {
            std::string controller_key;
            std::string controller_identifier = identifier;
            pqxx::work transaction{*connection};
            pqxx::result result = transaction.exec_prepared("validate_controller", controller_identifier);
            transaction.commit();

            if (result.size() == 1) {
                // node key is the secret used for JWT
                controller_key = result.at(0)["controller_key"].as<std::string>();

                auto verifier = jwt::verify()
                                    .with_issuer("auth0")
                                    .with_claim("scope", jwt::claim(std::string("controller")))
                                    .with_claim("access", jwt::claim(std::string(controller_identifier)))
                                    .allow_algorithm(jwt::algorithm::hs256{controller_key});
                std::error_code ec;
                verifier.verify(decoded, ec);
                if (ec) {
                    authenticated = false;
                }
                else {
                    authenticated = true;
                }
            }
        }
    }
    catch (const nlohmann::json::parse_error &error) {
        std::cerr << error.what() << std::endl;
    }
    catch (const nlohmann::json::type_error &error) {
        std::cerr << error.what() << std::endl;
    }
    catch (pqxx::sql_error const &error)
    {
        std::cerr << error.what() << std::endl;
    }
    catch (std::exception const &error)
    {
        std::cerr << error.what() << std::endl;
    }
    catch (...) {
        std::cerr << "Cannot authenticate request" << std::endl;
    }

    return authenticated;
}

int main() {
    std::unordered_map<std::string, std::string> prepared_queries;
    prepared_queries["add_node"] = "insert into nodes(host_identifier, "
                                   "os_arch, os_build, os_major, os_minor, os_name, os_platform, "
                                   "agent_version, node_key, node_invalid, "
                                   "hardware_vendor, hardware_model, hardware_version, hostname, "
                                   "hardware_cpu_logical_core, hardware_cpu_type, hardware_physical_memory, "
                                   "enrolled_on, last_seen) "
                                   "values ($1, $2, $3, $4, $5, $6, $7, $8, $9, $10, "
                                   "$11, $12, $13, $14, $15, $16, $17, $18, $19)";
    prepared_queries["update_node_key"] = "update nodes set node_key = $1 where host_identifier = $2";
    prepared_queries["validate_node"] = "select * from nodes where host_identifier = $1;";
    prepared_queries["validate_node_key"] = "select * from nodes where node_key = $1";
    prepared_queries["validate_controller"] = "select * from controllers where controller_identifier = $1;";
    prepared_queries["get_command"] = "select * from command_queue where host_identifier = $1 and (sent = false or sent is NULL) order by id asc limit 1";
    prepared_queries["mark_command_as_sent"] = "update command_queue set sent_time = $1, sent = true where id = $2";
    prepared_queries["get_response"] = "select * from response_queue where host_identifier = $1 "
                                       "and command_id = $2";
    prepared_queries["set_command"] = "insert into command_queue(host_identifier, command, queue_time) values ($1, $2, $3) returning id";
    prepared_queries["set_response"] = "insert into response_queue(host_identifier, command_id, timestamp, response) "
                                       "values ($1, $2, $3, $4)";
    prepared_queries["list_nodes"] = "select * from nodes where last_seen > $1 and id > $2 order by id asc limit $3";

    ConfigurationParser config_parser("config.json");
    if (!config_parser.parse()) {
        std::cerr << "Failed to parse configuration file" << std::endl;
        return 1;
    }

    ServerConfiguration server_configuration = config_parser.get_server_configuration();
    SslConfiguration ssl_server_configuration = config_parser.get_ssl_configuration();
    DatabaseConfiguration database_configuration = config_parser.get_database_configuration();
    DatabasePoolConfiguration db_pool_configuration = config_parser.get_database_pool_configuration();

    std::shared_ptr<ConnectionPool> connection_pool(new ConnectionPool(database_configuration.ConnectionString(),
        db_pool_configuration.maximum_connections,
        prepared_queries));

    RequestRouter router(default_req_handler,
    [connection_pool](const std::string& destination, HTTPMessage& request) -> bool
                     {
                        auto conn = connection_pool->GetConnection();
                        bool result = authenticator(destination, request, conn);
                         connection_pool->ReturnConnection(conn);
                        return result;
                     },
                     [](const std::string& destination, HTTPMessage& request) -> bool
                     {
                         return true;
                     });

    router["/enroll"].post([connection_pool](const HTTPMessage& req) -> HTTPMessage {
        auto connection = connection_pool->GetConnection();
        HTTPMessage response = RequestHandler::Enroll(req, connection);
        connection_pool->ReturnConnection(connection);
        return response;
    });
    router["/get_response"].post([connection_pool](const HTTPMessage& req) -> HTTPMessage {
        auto connection = connection_pool->GetConnection();
        HTTPMessage response = RequestHandler::GetResponse(req, connection);
        connection_pool->ReturnConnection(connection);
        return response;
    });
    router["/get_command"].post([connection_pool](const HTTPMessage& req) -> HTTPMessage {
        auto connection = connection_pool->GetConnection();
        HTTPMessage response = RequestHandler::GetCommand(req, connection);
        connection_pool->ReturnConnection(connection);
        return response;
    });
    router["/set_response"].post([connection_pool](const HTTPMessage& req) -> HTTPMessage {
        auto connection = connection_pool->GetConnection();
        HTTPMessage response = RequestHandler::SetResponse(req, connection);
        connection_pool->ReturnConnection(connection);
        return response;
    });
    router["/set_command"].post([connection_pool](const HTTPMessage& req) -> HTTPMessage {
        auto connection = connection_pool->GetConnection();
        HTTPMessage response = RequestHandler::SetCommand(req, connection);
        connection_pool->ReturnConnection(connection);
        return response;
    });
    router["/list_nodes"].post([connection_pool](const HTTPMessage& req) -> HTTPMessage {
        auto connection = connection_pool->GetConnection();
        HTTPMessage response = RequestHandler::ListNodes(req, connection);
        connection_pool->ReturnConnection(connection);
        return response;
    });
    router["/hello"].get([](const HTTPMessage& req) -> HTTPMessage {
        HTTPMessage response;
        response.body = "hello world!";
        response.status = boost::beast::http::status::ok;
        return response;
    });

    WebServer server(router, server_configuration.hostname, server_configuration.port);
    server.setTlsCertificates(ssl_server_configuration.ssl_certificate_file,
        ssl_server_configuration.ssl_private_key_file,
        ssl_server_configuration.diffie_hellman_key_file,
        ssl_server_configuration.ssl_password);

    server.run();
    return 0;
}
