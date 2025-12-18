#include "ShellHandler.h"

#include <iostream>
#include <sstream>
#include <utility>
#include <vector>

#include <boost/algorithm/string.hpp>

#include "Common.h"

std::string default_command_handler(const std::string &commands, const std::vector<std::string>& arguments) {
    std::stringstream ss;
    ss << "Unknown command: [" << commands << "]. Input 'help' to print help.";
    return ss.str();
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

ShellHandler::ShellHandler(ShellCommandHandler& cmd_handler) {
    handler = cmd_handler;
}

ShellHandler::~ShellHandler() = default;

CommandExecutor& ShellCommandHandler::operator[](std::string command) {
    if (commands_map.find(command) == commands_map.end()) {
        auto executor = CommandExecutor(command);
        commands_map[command] = executor;
    }

    return commands_map[command];
}

std::string ShellCommandHandler::help(std::string command) {
    if (command == "help") {
        for (auto& entry : help_messages) {
            std::cout << entry.first << "\t\t" << entry.second << std::endl;
        }
    }
    else if (help_messages.find(command) != help_messages.end()) {
        std::cout << command << "\t\t" << help_messages[command] << std::endl;
    }
    else {
        std::cout << "No help message found for command [" << command << "]. Input 'help' to print help." << std::endl;
    }

    return command;
}

ShellCommandHandler::ShellCommandHandler() {
    std::string cmd = "help";
    std::string msg = "prints help message";
    help_messages[cmd] = msg;
    CommandExecutor executor(cmd);
    auto handler = [&](std::vector<std::string> commands) -> std::string { this->help(commands[0]); return ""; };
    executor = handler;
    commands_map[cmd] = executor;
}
ShellCommandHandler::~ShellCommandHandler() = default;

CommandExecutor& ShellCommandHandler::operator[](const std::string& command, const std::string& help_message) {
    if (commands_map.find(command) == commands_map.end()) {
        auto executor = CommandExecutor(command);
        commands_map[command] = executor;
        help_messages[command] = help_message;
    }
    return commands_map[command];
}

void ShellHandler::run() {
    std::string input;
    while (true) {
        std::getline(ConsoleRead(), input);

        std::vector<std::string> cmd_tokens;
        boost::split(cmd_tokens, input, boost::is_any_of(" "));

        auto output = handler[cmd_tokens[0]](cmd_tokens);
        std::cout << output;
    }
}
