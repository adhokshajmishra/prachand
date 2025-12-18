#include "CommandHandler.h"

#include <iostream>
#include <boost/algorithm/string.hpp>
#include <sstream>

#include "../3rdparty/nlohmann/json.hpp"

std::string default_command_handler(const std::string &commands, std::vector<std::string> arguments) {
    std::stringstream ss;
    ss << "The function [" << commands << "] is not implemented.";
    return ss.str();
}

std::string shell_handler(const std::vector<std::string>& arguments) {
    std::stringstream ss;
    if (arguments.empty()) {
        ss << "No shell command was specified for execution.";
    }
    else {
        ss << "The shell command [" << arguments[0] << "] is not executed.";
    }
    return ss.str();
}

CommandHandler::CommandHandler() {
}

CommandHandler::~CommandHandler() {
}

Command parse_command(const nlohmann::json &command) {
    Command cmd;

    if (command.contains("command")
        && command["command"].is_string()
        && command.contains("arguments")
        && command["arguments"].is_array()) {

        nlohmann::json args = command["arguments"];

        cmd.command = command["command"].get<std::string>();
        for (const auto& arg : args) {
            cmd.arguments.push_back(arg.get<std::string>());
        }
    }

    return cmd;
}


/*
 * We are going to use the following format of command
 * shell!<string>   :   shell command
 * agent!<string>   :   agent command
 */
CommandExecutor& CommandHandler::operator[](std::string command) {
    if (commands_map.find(command) == commands_map.end()) {
        auto executor = CommandExecutor(command);
        commands_map[command] = executor;
    }

    return commands_map[command];
}

CommandExecutor::CommandExecutor(std::string command) {
    this->command = command;
    this->handler = std::bind(default_command_handler, command, std::placeholders::_1);
}

CommandExecutor& CommandExecutor::operator=(const std::string& command) {
    this->command = command;
    return *this;
}

CommandExecutor& CommandExecutor::operator=(std::function<std::string(const std::vector<std::string>&)> handler) {
    this->handler = handler;
    return *this;
}

std::string CommandExecutor::operator()(const std::vector<std::string>& args) {
    return this->handler(args);
}