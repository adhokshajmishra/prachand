#include <boost/beast/http.hpp>
#include <random>
#include "../3rdparty/nlohmann/json.hpp"
#include "RequestHandler.h"
#include "jwt-cpp/jwt.h"

std::string random_string(size_t length)
{
    const char charset[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
    const size_t max_index = (sizeof(charset) - 1);

    std::random_device rd;
    std::mt19937 generator(rd());
    std::uniform_int_distribution<> distrib(0, max_index - 1);

    std::string str(length, 0);
    std::generate_n(str.begin(), length, [&distrib, &generator, &charset]() -> char
                    { return charset[distrib(generator)]; });

    return str;
}

HTTPMessage RequestHandler::Enroll(const HTTPMessage &request, std::shared_ptr<pqxx::connection> connection) {
    HTTPMessage response;
    nlohmann::json enrollment_response;
    try {
        auto enrollment_request = nlohmann::json::parse(request.body);
        if (enrollment_request.contains("host_identifier") && enrollment_request.contains("host_details")) {
            std::string host_identifier = enrollment_request["host_identifier"];
            std::string node_key = random_string(32);
            bool node_unique = false;
            auto host_details = enrollment_request["host_details"];

            // Does the host identifier exist already?
            pqxx::work transaction{*connection};
            pqxx::result result = transaction.exec_prepared("validate_node", host_identifier);
            transaction.commit();

            if (result.size() == 1)
            {
                /*
                 * The host identifier already exists. We only update the node key, and move on.
                 */
                bool unique_node = false;
                while (unique_node == false)
                {
                    result = transaction.exec_prepared("validate_node_key", node_key);
                    transaction.commit();
                    if (result.size() == 1)
                    {
                        node_key = random_string(32);
                        unique_node = false;
                    }
                    else
                    {
                        unique_node = true;
                    }
                }

                result = transaction.exec_prepared("update_node_key", node_key, host_identifier);
                transaction.commit();
                // prepare JWT token

                auto token = jwt::create()
                                    .set_type("JWS")
                                    .set_issuer("auth0")
                                    .set_payload_claim("access", jwt::claim(std::string(host_identifier)))
                                    .set_payload_claim("scope", jwt::claim(std::string("host")))
                                    .sign(jwt::algorithm::hs256{node_key});
                enrollment_response["token"] = token;
                enrollment_response["node_invalid"] = false;
                response.status = boost::beast::http::status::ok;
                response.body = enrollment_response.dump();

                response.header["Content-Type"] = "application/json";
                return response;
            }
            else if (result.size() == 0) {
                std::string os_arch, os_build, os_major, os_minor, os_name, os_platform,
                        agent_version, hardware_vendor, hardware_model, hardware_version,
                        hostname, hardware_cpu_logical_core, hardware_cpu_type, hardware_physical_memory;
                long enrolled_on, last_seen;
                std::time_t t = std::time(0);
                enrolled_on = t;
                last_seen = t;

                os_arch = host_details.contains("os_arch") ? host_details["os_arch"] : "unknown";
                os_build = host_details.contains("os_build") ? host_details["os_build"] : "unknown";
                os_major = host_details.contains("os_major") ? host_details["os_major"] : "unknown";
                os_minor = host_details.contains("os_minor") ? host_details["os_minor"] : "unknown";
                os_name = host_details.contains("os_name") ? host_details["os_name"] : "unknown";
                os_platform = host_details.contains("os_platform") ? host_details["os_platform"] : "unknown";

                hardware_vendor = host_details.contains("hw_vendor") ? host_details["hw_vendor"] : "unknown";
                hardware_model = host_details.contains("hw_model") ? host_details["hw_model"] : "unknown";
                hardware_version = host_details.contains("hw_version") ? host_details["hw_version"] : "unknown";
                hardware_cpu_logical_core = host_details.contains("hw_cpu_logical_core") ? host_details["hw_cpu_logical_core"] : "unknown";
                hardware_cpu_type = host_details.contains("hw_cpu_type") ? host_details["hw_cpu_type"] : "unknown";
                hardware_physical_memory = host_details.contains("hw_physical_memory") ? host_details["hw_physical_memory"] : "unknown";

                hostname = host_details.contains("host_name") ? host_details["host_name"] : "unknown";
                agent_version = host_details.contains("agent_version") ? host_details["agent_version"] : "unknown";

                std::string node_invalid = "false";
                transaction.exec_prepared("add_node", host_identifier, os_arch, os_build, os_major, os_minor, os_name, os_platform,
                                   agent_version, node_key, node_invalid,
                                   hardware_vendor, hardware_model, hardware_version, hostname,
                                   hardware_cpu_logical_core, hardware_cpu_type, hardware_physical_memory,
                                   enrolled_on, last_seen);
                transaction.commit();

                auto token = jwt::create()
                                    .set_type("JWS")
                                    .set_issuer("auth0")
                                    .set_payload_claim("access", jwt::claim(std::string(host_identifier)))
                                    .set_payload_claim("scope", jwt::claim(std::string("host")))
                                    .sign(jwt::algorithm::hs256{node_key});
                enrollment_response["token"] = token;
                enrollment_response["node_invalid"] = false;
                response.status = boost::beast::http::status::ok;
                response.body = enrollment_response.dump();

                response.header["Content-Type"] = "application/json";
                return response;
            }
            else {
                // how did we get two entries ?
                response.status = boost::beast::http::status::forbidden;
                response.body = "{\n"
                                "  \"message\":\"Enrollment has been denied.\",\n"
                                "  \"node_key\":\"\",\n"
                                "  \"node_invalid\":\"true\"\n"
                                "}";
                response.header["Content-Type"] = "application/json";
                return response;
            }

        }
        else {
            // incomplete data
            response.status = boost::beast::http::status::bad_request;
            response.body = "{\n"
                            "  \"message\":\"Enrollment failed due to bad request\",\n"
                            "  \"node_key\":\"\",\n"
                            "  \"node_invalid\":\"true\"\n"
                            "}";
            response.header["Content-Type"] = "application/json";
            return response;
        }
    }
    catch (const nlohmann::json::parse_error &error) {
        response.status = boost::beast::http::status::bad_request;
        response.header["Content-Type"] = "application/json";
        response.body = "{\n"
                        "  \"message\":\"Enrollment failed due to bad request\",\n"
                        "  \"node_key\":\"\",\n"
                        "  \"node_invalid\":\"true\"\n"
                        "}";
        return response;
    }
    catch (const nlohmann::json::type_error &error) {
        response.status = boost::beast::http::status::bad_request;
        response.header["Content-Type"] = "application/json";
        response.body = "{\n"
                        "  \"message\":\"Enrollment failed due to bad request\",\n"
                        "  \"node_key\":\"\",\n"
                        "  \"node_invalid\":\"true\"\n"
                        "}";
        return response;
    }
    catch (pqxx::sql_error const &e)
    {
        response.status = boost::beast::http::status::internal_server_error;
        response.body = "{\n"
                        "  \"message\":\"Enrollment failed due to database error.\",\n"
                        "  \"node_key\":\"\",\n"
                        "  \"node_invalid\":\"true\"\n"
                        "}";
        response.header["Content-Type"] = "application/json";
        return response;
    }
    catch (std::exception const &e)
    {
        response.status = boost::beast::http::status::internal_server_error;
        response.body = "{\n"
                        "  \"message\":\"Enrollment failed due to server error.\",\n"
                        "  \"node_key\":\"\",\n"
                        "  \"node_invalid\":\"true\"\n"
                        "}";
        response.header["Content-Type"] = "application/json";
        return response;
    }
    catch (...) {
        response.status = boost::beast::http::status::bad_request;
        response.header["Content-Type"] = "application/json";
        response.body = "{\n"
                        "  \"message\":\"Enrollment failed due to bad request\",\n"
                        "  \"node_key\":\"\",\n"
                        "  \"node_invalid\":\"true\"\n"
                        "}";
        return response;
    }
}

HTTPMessage RequestHandler::GetCommand(const HTTPMessage &request, std::shared_ptr<pqxx::connection> connection) {
    HTTPMessage response;
    try {
        nlohmann::json request_data = nlohmann::json::parse(request.body);

        std::string host_identifier = request_data["host_identifier"];

        // do we have some command for this agent?
        pqxx::work transaction{*connection};
        pqxx::result result = transaction.exec_prepared("get_command", host_identifier);
        transaction.commit();

        // we are going to get maximum one record at a time due to 'limit 1' in query
        if (result.size() == 1) {
            std::string command_data = result.at(0)["command"].as<std::string>();
            nlohmann::json command = nlohmann::json::parse(command_data);
            unsigned long id = result.at(0)["id"].as<unsigned long>();

            nlohmann::json response_data;
            response_data["id"] = id;
            response_data["command"] = command;
            response.header["Content-Type"] = "application/json";
            response.body = response_data.dump();
            response.status = boost::beast::http::status::ok;

            // mark the command as sent
            std::time_t sent_time = std::time(0);
            result = transaction.exec_prepared("mark_command_as_sent", sent_time, id);
            transaction.commit();
        }
        else {
            nlohmann::json response_data;
            response_data["id"] = 0;
            response_data["command"] = "";
            response.header["Content-Type"] = "application/json";
            response.body = response_data.dump();
            response.status = boost::beast::http::status::ok;
        }

        return response;
    }
    catch (const nlohmann::json::parse_error &error) {
        response.status = boost::beast::http::status::bad_request;
        response.header["Content-Type"] = "application/json";
        response.body = "{\n"
                        "  \"message\":\"Request failed due to bad request\"\n"
                        "}";
        return response;
    }
    catch (const nlohmann::json::type_error &error) {
        response.status = boost::beast::http::status::bad_request;
        response.header["Content-Type"] = "application/json";
        response.body = "{\n"
                        "  \"message\":\"Request failed due to bad request\"\n"
                        "}";
        return response;
    }
    catch (pqxx::sql_error const &e)
    {
        response.status = boost::beast::http::status::internal_server_error;
        response.body = "{\n"
                        "  \"message\":\"Request failed due to database error.\"\n"
                        "}";
        response.header["Content-Type"] = "application/json";
        return response;
    }
    catch (std::exception const &e)
    {
        response.status = boost::beast::http::status::internal_server_error;
        response.body = "{\n"
                        "  \"message\":\"Request failed due to server error.\"\n"
                        "}";
        response.header["Content-Type"] = "application/json";
        return response;
    }
    catch (...) {
        response.status = boost::beast::http::status::bad_request;
        response.header["Content-Type"] = "application/json";
        response.body = "{\n"
                        "  \"message\":\"Request failed due to bad request\"\n"
                        "}";
        return response;
    }
    return response;
}

HTTPMessage RequestHandler::GetResponse(const HTTPMessage &request, std::shared_ptr<pqxx::connection> connection) {
    HTTPMessage response;
    try {
        nlohmann::json request_data = nlohmann::json::parse(request.body);

        // we cannot retrieve response without command_id
        // command id should be numeric
        if (!request_data.contains("host_identifier")
            || !request_data["host_identifier"].is_string()
            || !request_data.contains("command_id")
            || !request_data["command_id"].is_number_unsigned()) {
            response.status = boost::beast::http::status::bad_request;
            response.header["Content-Type"] = "application/json";
            response.body = "{\n"
                            "  \"message\":\"Request failed due to bad request\"\n"
                            "}";
            return response;
        }

        std::string host_identifier = request_data["host_identifier"];
        unsigned long command_id = request_data["command_id"].get<unsigned long>();

        // do we have some response for this agent?
        pqxx::work transaction{*connection};
        pqxx::result result = transaction.exec_prepared("get_response", host_identifier, command_id);
        transaction.commit();

        // we are going to get maximum one record at a time due to 'limit 1' in query
        if (result.size() > 0) {
            nlohmann::json response_data = nlohmann::json::object();
            std::string command_output = result.at(0)["response"].as<std::string>();
            response_data["host_identifier"] = host_identifier;
            response_data["command_id"] = command_id;
            response_data["response"] = command_output;
            response.header["Content-Type"] = "application/json";
            response.body = response_data.dump();
            response.status = boost::beast::http::status::ok;
        }
        else {
            // we have no response
            nlohmann::json response_data = nlohmann::json::object();
            response_data["host_identifier"] = host_identifier;
            response_data["command_id"] = command_id;
            response_data["response"] = "No repsonse was found";
            response.header["Content-Type"] = "application/json";
            response.body = response_data.dump();
            response.status = boost::beast::http::status::not_found;
        }
        return response;
    }
    catch (const nlohmann::json::parse_error &error) {
        response.status = boost::beast::http::status::bad_request;
        response.header["Content-Type"] = "application/json";
        response.body = "{\n"
                        "  \"message\":\"Request failed due to bad request\"\n"
                        "}";
        return response;
    }
    catch (const nlohmann::json::type_error &error) {
        response.status = boost::beast::http::status::bad_request;
        response.header["Content-Type"] = "application/json";
        response.body = "{\n"
                        "  \"message\":\"Request failed due to bad request\"\n"
                        "}";
        return response;
    }
    catch (pqxx::sql_error const &e)
    {
        response.status = boost::beast::http::status::internal_server_error;
        response.body = "{\n"
                        "  \"message\":\"Request failed due to database error.\"\n"
                        "}";
        response.header["Content-Type"] = "application/json";
        return response;
    }
    catch (std::exception const &e)
    {
        response.status = boost::beast::http::status::internal_server_error;
        response.body = "{\n"
                        "  \"message\":\"Request failed due to server error.\"\n"
                        "}";
        response.header["Content-Type"] = "application/json";
        return response;
    }
    catch (...) {
        response.status = boost::beast::http::status::bad_request;
        response.header["Content-Type"] = "application/json";
        response.body = "{\n"
                        "  \"message\":\"Request failed due to bad request\"\n"
                        "}";
        return response;
    }
    return response;
}

HTTPMessage RequestHandler::SetCommand(const HTTPMessage &request, std::shared_ptr<pqxx::connection> connection) {
    HTTPMessage response;
    try {
        nlohmann::json request_data = nlohmann::json::parse(request.body);

        std::string host_identifier;
        nlohmann::json command;

        if (request_data.contains("host_identifier")
            && request_data.contains("command")
            && request_data.contains("arguments")) {
            host_identifier = request_data["host_identifier"].get<std::string>();
            command["command"] = request_data["command"];
            command["arguments"] = request_data["arguments"];
        }
        std::time_t queue_time = std::time(0);

        pqxx::work transaction{*connection};
        pqxx::result result = transaction.exec_prepared("set_command", host_identifier, command.dump(), queue_time);
        transaction.commit();

        // we inserted only one row. We better get only one record
        if (result.size() == 1) {
            unsigned long id = result.at(0)["id"].as<unsigned long>();
            nlohmann::json response_data;
            response_data["id"] = id;

            response.header["Content-Type"] = "application/json";
            response.body = response_data.dump();
            response.status = boost::beast::http::status::ok;

            return response;
        }
        else {
            nlohmann::json response_data;
            std::stringstream ss;
            ss << "Expected 1 record, got " << result.size() << " records";
            response_data["message"] = ss.str();
            response.header["Content-Type"] = "application/json";
            response.body = response_data.dump();
            response.status = boost::beast::http::status::internal_server_error;

            return response;
        }
    }
    catch (const nlohmann::json::parse_error &error) {
        nlohmann::json error_data;
        error_data["message"] = error.what();
        response.status = boost::beast::http::status::bad_request;
        response.header["Content-Type"] = "application/json";
        response.body = error_data.dump();
        return response;
    }
    catch (const nlohmann::json::type_error &error) {
        nlohmann::json error_data;
        error_data["message"] = error.what();
        response.status = boost::beast::http::status::bad_request;
        response.header["Content-Type"] = "application/json";
        response.body = error_data.dump();
        return response;
    }
    catch (pqxx::sql_error const &e)
    {
        response.status = boost::beast::http::status::internal_server_error;
        response.body = "{\n"
                        "  \"message\":\"Request failed due to database error.\"\n"
                        "}";
        response.header["Content-Type"] = "application/json";
        return response;
    }
    catch (std::exception const &e)
    {
        response.status = boost::beast::http::status::internal_server_error;
        response.body = "{\n"
                        "  \"message\":\"Request failed due to server error.\"\n"
                        "}";
        response.header["Content-Type"] = "application/json";
        return response;
    }
    catch (...) {
        response.status = boost::beast::http::status::bad_request;
        response.header["Content-Type"] = "application/json";
        response.body = "{\n"
                        "  \"message\":\"Request failed due to bad request\"\n"
                        "}";
        return response;
    }
    return response;
}

HTTPMessage RequestHandler::SetResponse(const HTTPMessage &request, std::shared_ptr<pqxx::connection> connection) {
    HTTPMessage response;
    try {
        nlohmann::json request_data = nlohmann::json::parse(request.body);
        std::string host_identifier = request_data["host_identifier"];

        // we cannot store response without command_id and response fields being present
        if (!request_data.contains("command_id") || !request_data.contains("response")) {
            response.status = boost::beast::http::status::bad_request;
            response.header["Content-Type"] = "application/json";
            response.body = "{\n"
                            "  \"message\":\"Request failed due to bad request\"\n"
                            "}";
            return response;
        }

        // command id should be numeric
        if (!request_data["command_id"].is_number_unsigned()) {
            response.status = boost::beast::http::status::bad_request;
            response.header["Content-Type"] = "application/json";
            response.body = "{\n"
                            "  \"message\":\"Request failed due to bad request\"\n"
                            "}";
            return response;
        }

        unsigned long cmd_id = request_data["command_id"].get<unsigned long>();
        std::string cmd_response = request_data["response"];
        std::time_t response_time = std::time(0);

        pqxx::work transaction{*connection};

        // Do we already have a response for this command?
        pqxx::result result = transaction.exec_prepared("get_response", host_identifier, cmd_id);
        transaction.commit();

        if (result.size() != 0) {
            response.status = boost::beast::http::status::conflict;
            response.header["Content-Type"] = "application/json";
            response.body = "{\n"
                            "  \"message\":\"Request closed due to duplicate request\"\n"
                            "}";
            return response;
        }

        result = transaction.exec_prepared("set_response", host_identifier,
            cmd_id, response_time, cmd_response);
        transaction.commit();
    }
    catch (const nlohmann::json::parse_error &error) {
        response.status = boost::beast::http::status::bad_request;
        response.header["Content-Type"] = "application/json";
        response.body = "{\n"
                        "  \"message\":\"Request failed due to bad request\"\n"
                        "}";
        return response;
    }
    catch (const nlohmann::json::type_error &error) {
        response.status = boost::beast::http::status::bad_request;
        response.header["Content-Type"] = "application/json";
        response.body = "{\n"
                        "  \"message\":\"Request failed due to bad request\"\n"
                        "}";
        return response;
    }
    catch (pqxx::sql_error const &e)
    {
        response.status = boost::beast::http::status::internal_server_error;
        response.body = "{\n"
                        "  \"message\":\"Request failed due to database error.\"\n"
                        "}";
        response.header["Content-Type"] = "application/json";
        return response;
    }
    catch (std::exception const &e)
    {
        response.status = boost::beast::http::status::internal_server_error;
        response.body = "{\n"
                        "  \"message\":\"Request failed due to server error.\"\n"
                        "}";
        response.header["Content-Type"] = "application/json";
        return response;
    }
    catch (...) {
        response.status = boost::beast::http::status::bad_request;
        response.header["Content-Type"] = "application/json";
        response.body = "{\n"
                        "  \"message\":\"Request failed due to bad request\"\n"
                        "}";
        return response;
    }
    return response;
}

HTTPMessage RequestHandler::ListNodes(const HTTPMessage& request, std::shared_ptr<pqxx::connection> connection) {
    HTTPMessage response;
    try {
        nlohmann::json request_data = nlohmann::json::parse(request.body);

        unsigned long long last_seen = 0, id = 0, limit = 10;
        if (request_data.contains("last_seen") && request_data["last_seen"].is_number_unsigned()) {
            last_seen = request_data["last_seen"].get<unsigned long long>();
        }

        if (request_data.contains("id") && request_data["id"].is_number_unsigned()) {
            id = request_data["id"].get<unsigned long long>();
        }
        if (request_data.contains("limit") && request_data["limit"].is_number_unsigned()) {
            limit = request_data["limit"].get<unsigned long long>();
        }

        pqxx::work transaction{*connection};
        pqxx::result result = transaction.exec_prepared("list_nodes", last_seen, id, limit);
        transaction.commit();

        nlohmann::json response_data;
        nlohmann::json nodes = nlohmann::json::array();
        for (auto i = 0; i < result.size(); i++) {
            nlohmann::json node_entry;

            node_entry["id"] = result.at(i)["id"].as<unsigned long long>();
            node_entry["host_identifier"] = result.at(i)["host_identifier"].as<std::string>();
            node_entry["hostname"] = result.at(i)["hostname"].as<std::string>();
            node_entry["os_name"] = result.at(i)["os_name"].as<std::string>();
            node_entry["agent_version"] = result.at(i)["agent_version"].as<std::string>();

            nodes.push_back(node_entry);
        }

        response_data["nodes"] = nodes;
        response.status = boost::beast::http::status::ok;
        response.header["Content-Type"] = "application/json";
        response.body = response_data.dump();
        return response;
    }
    catch (const nlohmann::json::parse_error &error) {
        response.status = boost::beast::http::status::bad_request;
        response.header["Content-Type"] = "application/json";
        response.body = "{\n"
                        "  \"message\":\"Request failed due to bad request\"\n"
                        "}";
        return response;
    }
    catch (const nlohmann::json::type_error &error) {
        response.status = boost::beast::http::status::bad_request;
        response.header["Content-Type"] = "application/json";
        response.body = "{\n"
                        "  \"message\":\"Request failed due to bad request\"\n"
                        "}";
        return response;
    }
    catch (pqxx::sql_error const &e)
    {
        response.status = boost::beast::http::status::internal_server_error;
        response.body = "{\n"
                        "  \"message\":\"Request failed due to database error.\"\n"
                        "}";
        response.header["Content-Type"] = "application/json";
        return response;
    }
    catch (std::exception const &e)
    {
        response.status = boost::beast::http::status::internal_server_error;
        response.body = "{\n"
                        "  \"message\":\"Request failed due to server error.\"\n"
                        "}";
        response.header["Content-Type"] = "application/json";
        return response;
    }
    catch (...) {
        response.status = boost::beast::http::status::bad_request;
        response.header["Content-Type"] = "application/json";
        response.body = "{\n"
                        "  \"message\":\"Request failed due to bad request\"\n"
                        "}";
        return response;
    }
    return response;
}