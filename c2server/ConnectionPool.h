#ifndef FLEET_CONNECTIONPOOL_H
#define FLEET_CONNECTIONPOOL_H

#include <condition_variable>
#include <string>
#include <queue>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <pqxx/pqxx>

class ConnectionPool {
private:
    std::mutex pool_mutex;
    std::condition_variable _condition;
    std::string connection_string;
    unsigned int pool_size, current_pool_count;
    std::unordered_map<std::string, std::string> prepared_queries;
    std::queue<std::shared_ptr<pqxx::connection>> pool;
public:
    ConnectionPool(std::string connection, unsigned int size);
    ConnectionPool(std::string connection, unsigned int size, std::unordered_map<std::string, std::string> prepared_queries);
    std::shared_ptr<pqxx::connection> GetConnection();
    void ReturnConnection(std::shared_ptr<pqxx::connection> connection);
};


#endif //FLEET_CONNECTIONPOOL_H
