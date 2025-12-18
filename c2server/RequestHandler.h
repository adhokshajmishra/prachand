#ifndef REQUESTHANDLER_H
#define REQUESTHANDLER_H

#include <pqxx/pqxx>
#include "./libhttpserver/WebServer.h"

class RequestHandler {
public:
    static HTTPMessage Enroll(const HTTPMessage& request, std::shared_ptr<pqxx::connection> connection);
    static HTTPMessage GetCommand(const HTTPMessage& request, std::shared_ptr<pqxx::connection> connection);
    static HTTPMessage GetResponse(const HTTPMessage& request, std::shared_ptr<pqxx::connection> connection);
    static HTTPMessage SetCommand(const HTTPMessage& request, std::shared_ptr<pqxx::connection> connection);
    static HTTPMessage SetResponse(const HTTPMessage& request, std::shared_ptr<pqxx::connection> connection);
    static HTTPMessage ListNodes(const HTTPMessage& request, std::shared_ptr<pqxx::connection> connection);
};
#endif
