#include "ConfigurationParser.h"

#include <fstream>
#include <iostream>

ControllerConfiguration::ControllerConfiguration() {
    verify_ssl_certificate = false;
    verify_ssl_hostname = false;
}

ConfigurationParser::ConfigurationParser(const std::string &path) {
    this->configuration_file = path;
}

ControllerConfiguration ConfigurationParser::getControllerConfiguration() {
    return this->controller;
}

ProxyConfiguration ConfigurationParser::getProxyConfiguration() {
    return this->proxy;
}

bool ConfigurationParser::parse_proxy_configuration() {
    bool result = true;

    // proxy info is optional
    if (config_data.contains("proxy")) {
        proxy.is_proxy = true;
        auto proxy_config = config_data["proxy"];
        if (proxy_config.contains("hostname")) {
            proxy.hostname = proxy_config["hostname"];
        }
        else {
            //proxy config exists, but no hostname is present
            std::cerr << "Proxy hostname not found" << std::endl;
            result = false;
        }

        if (proxy_config.contains("port") && proxy_config["port"].is_number()) {
            proxy.port = proxy_config["port"].get<int>();
        }
        else {
            // proxy config exists, but port number is missing
            std::cerr << "Proxy port not found" << std::endl;
            result = false;
        }

        if (proxy_config.contains("auth_type")) {
            auto auth_type = proxy_config["auth_type"];

            if (auth_type == "none")
                proxy.authentication_type = AuthenticationType::None;
            else if (auth_type == "basic") {
                proxy.authentication_type = AuthenticationType::Basic;
            }
            else if (auth_type == "digest") {
                proxy.authentication_type = AuthenticationType::Digest;
            }
            else if (auth_type == "bearer_token") {
                proxy.authentication_type = AuthenticationType::BearerToken;
            }
            else if (auth_type == "token") {
                proxy.authentication_type = AuthenticationType::Token;
            }
            else {
                // unknown authentication type
                std::cerr << "Unknown authentication type: " << auth_type << std::endl;
                result = false;
            }

            if (proxy.authentication_type == AuthenticationType::Basic || proxy.authentication_type == AuthenticationType::Digest) {
                if (proxy_config.contains("username")) {
                    proxy.username = proxy_config["username"];
                }
                else {
                    // auth is set to basic, but username is missing
                    std::cerr << "Username not found" << std::endl;
                    result = false;
                }
                if (proxy_config.contains("password")) {
                    proxy.password = proxy_config["password"];
                }
                else {
                    // auth is set to basic, but password is missing
                    std::cerr << "Password not found" << std::endl;
                    result = false;
                }
            }
            else if (proxy.authentication_type == AuthenticationType::BearerToken || proxy.authentication_type == AuthenticationType::Token) {
                if (proxy_config.contains("token")) {
                    proxy.token = proxy_config["token"];
                }
                else {
                    std::cerr << "Token not found" << std::endl;
                    result = false;
                }
            }
        }
        {
            // no auth data present.
            proxy.authentication_type = AuthenticationType::None;
        }
    }
    else {
        proxy.is_proxy = false;
    }

    return result;
}

bool ConfigurationParser::parse_controller_configuration() {
    bool result = true;
    if (config_data.contains("controller")) {
        auto controller_config = config_data["controller"];
        if (controller_config.contains("controller_url")) {
            controller.controller = controller_config["controller_url"];
        }
        else {
            std::cerr << "Controller url not found" << std::endl;
            result = false;
        }

        if (controller_config.contains("ca_cert_bundle")) {
            controller.ca_certificate_bundle = controller_config["ca_cert_bundle"];
        }
        if (controller_config.contains("verify_ssl_certificate") && controller_config["verify_ssl_certificate"].is_boolean()) {
            controller.verify_ssl_certificate = controller_config["verify_ssl_certificate"].get<bool>();
        }
        if (controller_config.contains("verify_ssl_hostname") && controller_config["verify_ssl_hostname"].is_boolean()) {
            controller.verify_ssl_hostname = controller_config["verify_ssl_hostname"].get<bool>();
        }

        if (controller_config.contains("auth_type")) {
            auto auth_type = controller_config["auth_type"];

            if (auth_type == "none")
                controller.authentication_type = AuthenticationType::None;
            else if (auth_type == "token") {
                controller.authentication_type = AuthenticationType::Token;
            }
            else {
                std::cerr << "Unknown authentication type: " << auth_type << std::endl;
                result = false;
            }

            if (controller.authentication_type == AuthenticationType::Token) {
                if (controller_config.contains("token")) {
                    controller.token = controller_config["token"];
                }
                else {
                    std::cerr << "Token not found" << std::endl;
                    result = false;
                }
            }
        }
    }
    else
        result = false;
    return result;
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

    if (parse_controller_configuration() && parse_proxy_configuration())
        return true;
    else
        return false;
}
