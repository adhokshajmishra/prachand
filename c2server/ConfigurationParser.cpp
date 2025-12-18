#include <exception>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include "ConfigurationParser.h"

DatabaseConfiguration::DatabaseConfiguration() {
    this->port = 0;
}

std::string encodeURIComponent(std::string decoded)
{

    std::ostringstream oss;
    std::regex r("[!'\\(\\)*-.0-9A-Za-z_~]");

    for (char &c : decoded)
    {
        if (std::regex_match((std::string){c}, r))
        {
            oss << c;
        }
        else
        {
            oss << "%" << std::uppercase << std::setw(2) << std::setfill('0') << std::hex << (int)(c & 0xff);
        }
    }
    return oss.str();
}

std::string DatabaseConfiguration::ConnectionString() {
    std::stringstream connection_string;

    connection_string << "postgresql://" << encodeURIComponent(this->username);

    if (!this->password.empty()) {
        connection_string << ":" << encodeURIComponent(this->password);
    }

    connection_string << "@" << this->hostname;

    if (this->port > 0)
        connection_string << ":" << std::to_string(this->port);

    connection_string << "/" << this->database_name;

    return connection_string.str();
}

DatabasePoolConfiguration::DatabasePoolConfiguration() {
    this->maximum_connections = 0;
}

ConfigurationParser::ConfigurationParser(const std::string& path) {
    this->configuration_file = path;
}

ServerConfiguration::ServerConfiguration() {
    this->port = 1234;
}

bool ConfigurationParser::parse() {
    try {
        if (!std::filesystem::exists(this->configuration_file))
        {
            std::cerr << "Error: configuration file does not exist." << std::endl;
            return false;
        }
        if (std::filesystem::is_symlink(this->configuration_file))
        {
            this->configuration_file = std::filesystem::read_symlink(this->configuration_file);

            if (!std::filesystem::exists(this->configuration_file))
            {
                std::cerr << "Error: configuration file is a symlink resolving to non-existing path." << std::endl;
                return false;
            }
        }
        if (!std::filesystem::is_regular_file(this->configuration_file))
        {
            std::cerr << "Error: configuration file is not a regular file." << std::endl;
            return false;
        }
        if (std::filesystem::is_empty(this->configuration_file))
        {
            std::cerr << "Error: configuration file is empty." << std::endl;
            return false;
        }
    }
    catch (const std::filesystem::filesystem_error& error)
    {
        std::cerr << "Error (" << error.code() << ") while validating configuration file path: " << error.what() << std::endl;
        return false;
    }

    std::ifstream config_file(this->configuration_file);
    std::string str((std::istreambuf_iterator<char>(config_file)), std::istreambuf_iterator<char>());

    try {
        this->config_data = nlohmann::json::parse(str);
    }
    catch (const nlohmann::json::parse_error& error)
    {
        std::cerr << "Error: could not parse JSON. Parse error: " << error.what() << std::endl;
        return false;
    }
    catch (const nlohmann::json::type_error& error)
    {
        std::cerr << "Error: invalid type encountered in JSON. Parse error: " << error.what() << std::endl;
        return false;
    }
    catch(const std::exception& ex)
    {
        std::cerr << "Error: some unknown error occurred. Parse error: " << ex.what() << std::endl;
        return false;
    }

    if (parse_server_configuration() &&
        parse_ssl_configuration() &&
        parse_database_configuration() &&
        parse_database_pool_configuration())
        return true;
    else
        return false;
}

ServerConfiguration ConfigurationParser::get_server_configuration() {
    return this->server_config;
}

DatabaseConfiguration ConfigurationParser::get_database_configuration() {
    return this->db_config;
}

DatabasePoolConfiguration ConfigurationParser::get_database_pool_configuration() {
    return this->db_pool_config;
}

SslConfiguration ConfigurationParser::get_ssl_configuration() {
    return this->ssl_config;
}

bool ConfigurationParser::parse_server_configuration() {
    if (config_data.contains("server_configuration"))
    {
        auto server = config_data["server_configuration"];

        if (server.contains("hostname"))
        {
            server_config.hostname = server["hostname"].get<std::string>();
        }
        else
        {
            server_config.hostname = "127.0.0.1";
        }

        if (server.contains("port"))
        {
            server_config.port = server["port"].get<unsigned int>();
        }
        else
        {
            server_config.port = 1234;
        }
        return true;
    }
    else
        return false;
}

bool ConfigurationParser::parse_ssl_configuration() {
    if (config_data.contains("ssl_configuration")) {
        auto ssl = config_data["ssl_configuration"];

        if (ssl.contains("ssl_certificate")) {
            ssl_config.ssl_certificate_file = ssl["ssl_certificate"].get<std::string>();
        }
        else
            return false;
        if (ssl.contains("ssl_private_key")) {
            ssl_config.ssl_private_key_file = ssl["ssl_private_key"].get<std::string>();
        }
        else
            return false;
        if (ssl.contains("dh_key")) {
            ssl_config.diffie_hellman_key_file = ssl["dh_key"].get<std::string>();
        }
        else
            return false;
        if (ssl.contains("ssl_password")) {
            ssl_config.ssl_password = ssl["ssl_password"].get<std::string>();
        }
        else
            return false;

        return true;
    }
    else
        return false;
}

bool ConfigurationParser::parse_database_configuration() {
    if (config_data.contains("database_configuration")) {
        auto db = config_data["database_configuration"];
        if (db.contains("hostname")) {
            db_config.hostname = db["hostname"].get<std::string>();
        }
        else
            return false;
        if (db.contains("port")) {
            db_config.port = db["port"].get<unsigned int>();
        }
        else
            return false;
        if (db.contains("username")) {
            db_config.username = db["username"].get<std::string>();
        }
        else
            return false;
        if (db.contains("password")) {
            db_config.password = db["password"].get<std::string>();
        }
        else
            return false;
        if (db.contains("database_name")) {
            db_config.database_name = db["database_name"].get<std::string>();
        }
        else
            return false;
        return true;
    }
    else
        return false;
}

bool ConfigurationParser::parse_database_pool_configuration() {
    if (config_data.contains("database_pool_configuration")) {
        auto dbpool = config_data["database_pool_configuration"];
        if (dbpool.contains("maximum_connections")) {
            db_pool_config.maximum_connections = dbpool.at("maximum_connections").get<unsigned int>();
            return true;
        }
        return false;
    }
    return false;
}
