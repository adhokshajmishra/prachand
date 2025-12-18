#ifndef PRACHAND_CONTROLLER_COMMON_H
#define PRACHAND_CONTROLLER_COMMON_H

#include <iostream>
#include <string>

class ConsoleWriter : public std::ostream
{
private:
public:
    ConsoleWriter(std::streambuf* sbuf);
    ConsoleWriter(ConsoleWriter&& other);
    ~ConsoleWriter();
};

class ConsoleReader : public std::istream
{
public:
    ConsoleReader(std::streambuf* sbuf);
    ConsoleReader(ConsoleReader&& other);
    ~ConsoleReader();
    friend ConsoleReader ConsoleRead();
    friend std::string ConsolePrompt();
};

ConsoleWriter ConsoleWrite();
ConsoleReader ConsoleRead();

class ControllerContext {
private:
    std::string node_identifier;
    std::string active_module;
public:
    ControllerContext();
    virtual ~ControllerContext();
    void attach(std::string node_identifier);
    void setActiveModule(std::string module_name);
    void detach();
    bool isAttached();
    std::string attachedNode();
    std::string activeModule();
};

extern ControllerContext context;

#endif //PRACHAND_CONTROLLER_COMMON_H