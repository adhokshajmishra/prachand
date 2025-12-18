#include <filesystem>
#include <fstream>
#include <iostream>
#include <thread>
#include <boost/beast/http/status.hpp>

#include "config.h"
#include "../3rdparty/nlohmann/json.hpp"
#include "uuid.h"
#include "OSWrapper.h"
#include "../3rdparty/cpp-httplib/httplib.h"
#include "CommandHandler.h"

bool startup_handler(httplib::Client &client, std::string &token, std::string &host_identifier) {
    if (std::filesystem::exists(agent_config_file) && std::filesystem::is_regular_file(agent_config_file)) {
        try {
            std::ifstream f(agent_config_file);
            nlohmann::json json = nlohmann::json::parse(f);
            if (json.contains("token") && json.contains("host_identifier")) {
                token = json["token"];
                host_identifier = json["host_identifier"];

                return true;
            }
            else {
                // configuration is invalid. Delete the file, and re-enroll
                std::filesystem::remove(agent_config_file);
                return false;
            }
        }
        catch (const nlohmann::json::parse_error& error)
        {
            std::cerr << "Error: could not parse JSON. Parse error: " << error.what() << std::endl;
            return false;
        }
        catch (const nlohmann::json::type_error& error)
        {
            std::cerr << "Error: invalid type encountered in JSON. Parse error: " << error.what() << std::endl;
            return false;
        }
        catch(const std::exception& ex)
        {
            std::cerr << "Error: some unknown error occurred. Parse error: " << ex.what() << std::endl;
            return false;
        }
    }
    else {
        // enroll the agent here
        nlohmann::json enrollment_data, host_details;

        auto new_uuid = uuid::v4::UUID::New();

        host_details["agent_version"] = "1.0.0.0";
        host_details["host_name"] = OSWrapper::hostname();
        host_details["hw_cpu_logical_core"] = std::to_string(std::thread::hardware_concurrency());
        host_details["hw_cpu_type"] = OSWrapper::hw_cpu_type();
        host_details["hw_model"] = OSWrapper::hw_model();
        host_details["hw_physical_memory"] = OSWrapper::hw_physical_memory();
        host_details["hw_vendor"] = OSWrapper::hw_vendor();
        host_details["hw_version"] = OSWrapper::hw_version();
        host_details["os_arch"] = OSWrapper::os_architecture();
        host_details["os_build"] = OSWrapper::os_build();
        host_details["os_major"] = OSWrapper::os_major();
        host_details["os_minor"] = OSWrapper::os_minor();
        host_details["os_name"] = OSWrapper::os_name();
        host_details["os_platform"] = OSWrapper::os_platform();

        enrollment_data["host_identifier"] = new_uuid.String();
        enrollment_data["host_details"] = host_details;

        std::cout << enrollment_data.dump() << std::endl;

        httplib::Result res;

        while (!((res = client.Post("/enroll", enrollment_data.dump(), "application/json")))) {
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }

        if (res->status == (int)boost::beast::http::status::ok) {
            try {
                nlohmann::json enrollment_response = nlohmann::json::parse(res->body);
                if (enrollment_response.contains("token")) {
                    // enrollment successful.
                    token = enrollment_response["token"].get<std::string>();
                    host_identifier = enrollment_data["host_identifier"].get<std::string>();

                    nlohmann::json agent_config;
                    agent_config["token"] = token;
                    agent_config["host_identifier"] = host_identifier;

                    // write to agent_config
                    std::ofstream config(agent_config_file);
                    config << agent_config.dump();
                    config.close();

                    return true;
                }
                else {
                    return false;
                }
            }
            catch (const nlohmann::json::parse_error& error)
            {
                std::cerr << "Error: could not parse JSON. Parse error: " << error.what() << std::endl;
                return false;
            }
            catch (const nlohmann::json::type_error& error)
            {
                std::cerr << "Error: invalid type encountered in JSON. Parse error: " << error.what() << std::endl;
                return false;
            }
            catch(const std::exception& ex)
            {
                std::cerr << "Error: some unknown error occurred. Parse error: " << ex.what() << std::endl;
                return false;
            }
        }
        else
            return false;
    }
}

int main() {
    httplib::Client client(controller);

    if (!ca_certificates_file.empty()) {
        client.set_ca_cert_path(ca_certificates_file);
    }
    client.enable_server_certificate_verification(verify_ssl_certificates);
    client.enable_server_hostname_verification(verify_server_hostname);

    httplib::Result res;

    std::string token, host_identifier;
    while (!startup_handler(client, token, host_identifier)) {
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }

    // startup is done.
    httplib::Headers headers;
    headers.insert(std::pair<std::string, std::string>("Authorization", token));
    std::string content_type = "application/json";
    nlohmann::json command_request;

    command_request["host_identifier"] = host_identifier;

    CommandHandler handler;
    handler["shell"] = shell_handler;

    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(5));
        if (!((res = client.Post("/get_command", headers, command_request.dump(), content_type)))) {
            continue;
        }

        if (res->status == httplib::StatusCode::OK_200) {
            try {
                nlohmann::json command_response = nlohmann::json::parse(res->body);
                if (command_response.contains("id") && command_response.contains("command")) {
                    unsigned long id = command_response["id"].get<unsigned long>();
                    nlohmann::json command = command_response["command"];

                    if (id == 0 || command.is_null()) {
                        continue;
                    }

                    Command cmd = parse_command(command);
                    std::string response = handler[cmd.command](cmd.arguments);

                    nlohmann::json response_data;
                    response_data["host_identifier"] = host_identifier;
                    response_data["command_id"] = id;
                    response_data["response"] = response;
                    client.Post("/set_response", headers, response_data.dump(), content_type);
                }
            }
            catch (...) {
                continue;
            }
        }
    }

    return 0;
}