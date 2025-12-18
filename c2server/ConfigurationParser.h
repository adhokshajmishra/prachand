#ifndef CONFIGURATIONPARSER_H
#define CONFIGURATIONPARSER_H

#include <filesystem>
#include <string>

#include "../3rdparty/nlohmann/json.hpp"

struct ServerConfiguration {
    std::string hostname;
    unsigned int port;
    ServerConfiguration();
};

struct SslConfiguration
{
    std::string ssl_certificate_file;
    std::string ssl_private_key_file;
    std::string ssl_password;
    std::string diffie_hellman_key_file;
};

struct DatabaseConfiguration
{
    std::string hostname;
    unsigned int port;
    std::string username;
    std::string password;
    std::string database_name;

    std::string ConnectionString();
    DatabaseConfiguration();
};

struct DatabasePoolConfiguration
{
    unsigned int maximum_connections;
    DatabasePoolConfiguration();
};

class ConfigurationParser {
private:
    std::filesystem::path configuration_file;

    nlohmann::json config_data;
    ServerConfiguration server_config;
    SslConfiguration ssl_config;
    DatabasePoolConfiguration db_pool_config;
    DatabaseConfiguration db_config;
protected:
    bool parse_server_configuration();
    bool parse_ssl_configuration();
    bool parse_database_configuration();
    bool parse_database_pool_configuration();
public:
    bool parse();
    ServerConfiguration get_server_configuration();
    SslConfiguration get_ssl_configuration();
    DatabasePoolConfiguration get_database_pool_configuration();
    DatabaseConfiguration get_database_configuration();
    explicit ConfigurationParser(const std::string& path);
};

#endif