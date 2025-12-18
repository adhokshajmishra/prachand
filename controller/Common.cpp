#include <sstream>
#include "Common.h"

ControllerContext context;

ConsoleWriter::ConsoleWriter(std::streambuf* sbuf) : std::ios(sbuf), std::ostream(sbuf)
{
}

ConsoleWriter::ConsoleWriter(ConsoleWriter&& other) : ConsoleWriter(other.rdbuf())
{
}

ConsoleWriter::~ConsoleWriter()
{
}

ConsoleReader::ConsoleReader(std::streambuf* sbuf) : std::ios(sbuf), std::istream(sbuf)
{
}

ConsoleReader::ConsoleReader(ConsoleReader&& other) : ConsoleReader(other.rdbuf())
{
}

ConsoleReader::~ConsoleReader()
{
}

ConsoleWriter ConsoleWrite()
{
    return ConsoleWriter(std::cout.rdbuf());
}

std::string ConsolePrompt()
{
    std::stringstream ss;
    if (!context.attachedNode().empty())
        ss << "\033[1;34m" << context.attachedNode() << "\033[0m";

    if (!context.activeModule().empty())
        ss << "\033[1;36m[" << context.activeModule() << "]\033[0m";

    ss << ":> ";
    return ss.str();
}

ConsoleReader ConsoleRead()
{
    std::cout << std::endl << ConsolePrompt() << std::flush;
    return ConsoleReader(std::cin.rdbuf());
}


ControllerContext::ControllerContext() {
    node_identifier = "";
}

ControllerContext::~ControllerContext() {
}

void ControllerContext::attach(std::string node_identifier) {
    this->node_identifier = node_identifier;
}

void ControllerContext::detach() {
    this->node_identifier = "";
    this->active_module = "";
}

bool ControllerContext::isAttached() {
    if (this->node_identifier == "")
        return false;
    return true;
}

std::string ControllerContext::attachedNode() {
    return this->node_identifier;
}

void ControllerContext::setActiveModule(std::string module_name) {
    this->active_module = module_name;
}

std::string ControllerContext::activeModule() {
    return this->active_module;
}
