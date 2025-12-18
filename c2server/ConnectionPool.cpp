#include "ConnectionPool.h"

#include <exception>
#include <iostream>

ConnectionPool::ConnectionPool(std::string connection_string, unsigned int size) {
    this->connection_string = connection_string;
    this->pool_size = size;
    this->current_pool_count = 0;

    for (unsigned int i = 0; i < pool_size; ++i)
    {
        try {
            std::shared_ptr<pqxx::connection> conn(new pqxx::connection(this->connection_string));
            pool.emplace(conn);
            ++current_pool_count;
        }
        catch (const std::exception& e)
        {
            std::cerr << e.what() << std::endl;
        }
    }
}

ConnectionPool::ConnectionPool(std::string connection_string, unsigned int size, std::unordered_map<std::string, std::string> prepared_queries) {
    this->connection_string = connection_string;
    this->pool_size = size;
    this->current_pool_count = 0;
    this->prepared_queries = prepared_queries;

    for (unsigned int i = 0; i < pool_size; ++i)
    {
        try {
            std::shared_ptr<pqxx::connection> conn(new pqxx::connection(this->connection_string));

            if (!this->prepared_queries.empty())
            {
                for (auto query : this->prepared_queries)
                {
                    conn->prepare(query.first, query.second);
                }
            }

            pool.emplace(conn);
            ++current_pool_count;
        }
        catch (const std::exception& e)
        {
            std::cerr << e.what() << std::endl;
        }
    }
}

std::shared_ptr<pqxx::connection> ConnectionPool::GetConnection() {
    std::unique_lock<std::mutex> lock(pool_mutex);

    while (pool.empty())
    {
        _condition.wait(lock);
    }

    auto conn = pool.front();
    pool.pop();
    --current_pool_count;
    lock.unlock();
    return conn;
}

void ConnectionPool::ReturnConnection(std::shared_ptr<pqxx::connection> connection) {
    std::unique_lock<std::mutex> lock(pool_mutex);
    pool.push(connection);
    ++current_pool_count;
    lock.unlock();
    _condition.notify_one();
}