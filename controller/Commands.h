#ifndef COMMANDS_H
#define COMMANDS_H

#include <string>
#include <vector>

#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "cpp-httplib/httplib.h"

std::string list_nodes(httplib::Client &client, const httplib::Headers &headers, const std::vector<std::string>& arguments);
std::string attach(httplib::Client &client, const httplib::Headers &headers, const std::vector<std::string>& arguments);

#endif //COMMANDS_H
