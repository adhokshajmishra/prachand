#include <cstdlib>
#include <iostream>

#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "cpp-httplib/httplib.h"

#include "Commands.h"
#include "ConfigurationParser.h"
#include "ShellHandler.h"

int main() {
    ConfigurationParser config("controller.conf");
    if (!config.parse()) {
        std::cerr << "Failed to parse configuration." << std::endl;
        return 1;
    }

    ControllerConfiguration controller = config.getControllerConfiguration();
    ProxyConfiguration proxy = config.getProxyConfiguration();

    httplib::Client cli(controller.controller);
    httplib::Headers headers;
    cli.enable_server_certificate_verification(controller.verify_ssl_certificate);
    cli.enable_server_hostname_verification(controller.verify_ssl_hostname);

    if (proxy.is_proxy) {
        cli.set_proxy(proxy.hostname, proxy.port);

        if (proxy.authentication_type == AuthenticationType::Basic) {
            cli.set_proxy_basic_auth(proxy.username, proxy.password);
        }
        else if (proxy.authentication_type == AuthenticationType::Digest) {
            cli.set_proxy_digest_auth(proxy.username, proxy.password);
        }
        else if (proxy.authentication_type == AuthenticationType::BearerToken) {
            cli.set_proxy_bearer_token_auth(proxy.token);
        }
    }

    if (controller.authentication_type == AuthenticationType::Token) {
        headers.insert(std::pair<std::string, std::string>("Authorization", controller.token));
    }

    // intialisation is done.
    std::string command;
    httplib::Result res;
    std::string content_type = "application/json";

    bool is_shell_mode = false;

    ShellCommandHandler handler;

    handler["lsnode", "list enrolled nodes"] = [&cli, &headers](const std::vector<std::string>& args) -> std::string {
        return list_nodes(cli, headers, args);
    };

    handler["quit", "exit the controller"] = [&cli, &headers](const std::vector<std::string>& args) -> std::string {
        exit(0);
        return "";
    };

    handler["attach", "attach to node using identifier"] = [&cli, &headers](const std::vector<std::string>& args) -> std::string {
        return attach(cli, headers, args);
    };

    ShellHandler shell(handler);
    shell.run();
    return 0;
}