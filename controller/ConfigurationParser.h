#ifndef CONFIGURATIONPARSER_H
#define CONFIGURATIONPARSER_H

#include <filesystem>
#include <string>
#include "nlohmann/json.hpp"

enum AuthenticationType {
    None,
    Basic,
    Digest,
    BearerToken,
    Token,
};

struct ControllerConfiguration {
    std::string controller;
    std::string ca_certificate_bundle;
    bool verify_ssl_certificate;
    bool verify_ssl_hostname;
    AuthenticationType authentication_type;
    std::string token;
    ControllerConfiguration();
};

struct ProxyConfiguration {
    bool is_proxy;
    std::string hostname;
    int port;
    AuthenticationType authentication_type;
    std::string username;
    std::string password;
    std::string token;
};

class ConfigurationParser {
private:
    std::filesystem::path configuration_file;
    nlohmann::json config_data;
    ControllerConfiguration controller;
    ProxyConfiguration proxy;
protected:
    bool parse_proxy_configuration();
    bool parse_controller_configuration();
public:
    bool parse();
    explicit ConfigurationParser(const std::string& path);
    ControllerConfiguration getControllerConfiguration();
    ProxyConfiguration getProxyConfiguration();
};

#endif
