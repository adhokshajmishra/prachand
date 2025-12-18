#ifndef COMMANDHANDLER_H
#define COMMANDHANDLER_H

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

#include "../3rdparty/nlohmann/json.hpp"

enum CommandType {
    shell,
    agent,
    unknown
};

struct Command {
    std::string command;
    std::vector<std::string> arguments;
};

Command parse_command(const nlohmann::json &command);

class CommandExecutor {
private:
    std::string command;
    std::function<std::string(const std::vector<std::string>&)> handler;
public:
    explicit CommandExecutor(std::string command = "");
    CommandExecutor& operator=(const std::string& command);
    CommandExecutor& operator=(std::function<std::string(const std::vector<std::string>&)> handler);
    std::string operator()(const std::vector<std::string>& args);
};

class CommandHandler {
    std::unordered_map<std::string, CommandExecutor> commands_map;
public:
    CommandHandler();
    CommandExecutor& operator[](std::string command);
    virtual ~CommandHandler();
};

std::string shell_handler(const std::vector<std::string>& arguments);

#endif