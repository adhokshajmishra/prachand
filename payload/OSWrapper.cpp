#include "OSWrapper.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

#include <boost/algorithm/string.hpp>
#include <sys/sysinfo.h>
#include <sys/utsname.h>
#include <unistd.h>

struct OSInfo {
    std::string name;
    std::string major;
    std::string minor;
    std::string build;

    bool is_populated;
};

OSInfo os_info;

std::string read_entire_file(const std::string& path) {
    std::ifstream t(path);
    std::stringstream buffer;
    buffer << t.rdbuf();
    return buffer.str();
}

std::string get_cpu_type() {
    std::ifstream infile("/proc/cpuinfo");
    std::string line;

    std::string implementor, variant, part;

    std::string implementor_token = "CPU implementer\t: ";
    std::string variant_token = "CPU variant\t: ";
    std::string part_token = "CPU part\t: ";

    while (std::getline(infile, line)) {
        if (line.rfind(implementor_token, 0) != std::string::npos) {
            implementor = line;
            implementor.erase(0, implementor_token.length());
        }
        if (line.rfind(variant_token, 0) != std::string::npos) {
            variant = line.substr(0, line.find(variant_token));
        }
        if (line.rfind(part_token, 0) != std::string::npos) {
            part = line.substr(0, line.find(part_token));
        }

        if (!implementor.empty() && !variant.empty() && !part.empty()) {
            break;
        }
    }

    if (implementor == "0x61") {
        implementor = "Apple";
    }

    return implementor;
}

void get_os_info() {
    std::ifstream infile("/etc/os-release");
    std::string line;

    std::string name, major, minor, build;

    std::string name_token = "PRETTY_NAME=";
    std::string version_token = "VERSION_ID=";
    std::string major_token = "";
    std::string minor_token = "";
    std::string build_token = "";

    while (std::getline(infile, line)) {
        if (line.rfind(name_token, 0) != std::string::npos) {
            name = line;
            name.erase(0, name_token.length());
            boost::erase_all(name, "\"");
        }
        if (line.rfind(version_token, 0) != std::string::npos) {
            std::string version = line;
            version.erase(0, version_token.length());
            std::stringstream ss(version);
            std::string token;
            bool first = true;
            while (std::getline(ss, token, '.')) {
                if (first) {
                    first = false;
                    major = token;
                    boost::erase_all(major, "\"");
                }
                else {
                    minor = token;
                    boost::erase_all(minor, "\"");
                    break;
                }
            }
        }
    }

    os_info.name = name;
    os_info.major = major;
    os_info.minor = minor;
    os_info.build = build;
    os_info.is_populated = true;
}

std::string OSWrapper::hostname() {
    char hostname[1024];
    gethostname(hostname, 1024);
    return std::string(hostname);
}

std::string OSWrapper::os_platform() {
#ifdef __linux__
    return std::string("Linux");
#elif __APPLE__
    return std::string("MacOS");
#endif
}

std::string OSWrapper::os_name() {
    if (!os_info.is_populated) {
        get_os_info();
    }

    return os_info.name;
}

std::string OSWrapper::os_architecture() {
    utsname uts;
    uname(&uts);

    return std::string(uts.machine);
}

std::string OSWrapper::hw_vendor() {
    std::string result = read_entire_file("/sys/devices/virtual/dmi/id/sys_vendor");
    result.erase(std::remove(result.begin(), result.end(), '\n'), result.cend());
    return result;
}

std::string OSWrapper::hw_version() {
    std::string result = read_entire_file("/sys/devices/virtual/dmi/id/product_version");
    result.erase(std::remove(result.begin(), result.end(), '\n'), result.cend());
    return result;
}

std::string OSWrapper::hw_model() {
    std::string result = read_entire_file("/sys/devices/virtual/dmi/id/product_name");
    result.erase(std::remove(result.begin(), result.end(), '\n'), result.cend());
    return result;
}

std::string OSWrapper::hw_cpu_type() {
    return get_cpu_type();
}

std::string OSWrapper::hw_physical_memory() {
    struct sysinfo info;
    sysinfo(&info);

    return std::to_string(info.totalram);
}

std::string OSWrapper::os_major() {
    if (!os_info.is_populated) {
        get_os_info();
    }

    return os_info.major;
}

std::string OSWrapper::os_minor() {
    if (!os_info.is_populated) {
        get_os_info();
    }

    return os_info.minor;
}

std::string OSWrapper::os_build() {
    if (!os_info.is_populated) {
        get_os_info();
    }

    return os_info.build;
}