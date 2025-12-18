#ifndef SHELLHANDLER_H
#define SHELLHANDLER_H

#include <functional>
#include <string>
#include <unordered_map>

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

class ShellCommandHandler {
    std::unordered_map<std::string, CommandExecutor> commands_map;
    std::unordered_map<std::string, std::string> help_messages;
public:
    ShellCommandHandler();
    virtual ~ShellCommandHandler();
    CommandExecutor& operator[](std::string command);
    CommandExecutor& operator[](const std::string& command, const std::string& help_message);

    std::string help(std::string command);
};

class ShellHandler {
    ShellCommandHandler handler;
public:
    explicit ShellHandler(ShellCommandHandler& cmd_handler);
    virtual ~ShellHandler();
    void run();
};

#endif //SHELLHANDLER_H
