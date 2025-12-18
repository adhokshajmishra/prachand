#ifndef CONFIG_H
#define CONFIG_H

#include <string>

/*
 * This header file has common variables used elsewhere in payload.
 * Everything which is supposed to be "configurable" is kept here.
 */

// enable HTTPS support
#define CPPHTTPLIB_OPENSSL_SUPPORT

inline std::string agent_config_file = "agent.config";
inline std::string controller = "https://localhost:8080";
inline std::string ca_certificates_file;

inline bool verify_ssl_certificates = false;
inline bool verify_server_hostname = false;

#endif //CONFIG_H
