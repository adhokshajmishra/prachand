#ifndef OSWRAPPER_H
#define OSWRAPPER_H

#include <string>

class OSWrapper {
public:
    static std::string hostname();
    static std::string os_platform();
    static std::string os_name();
    static std::string os_architecture();
    static std::string os_major();
    static std::string os_minor();
    static std::string os_build();
    static std::string hw_vendor();
    static std::string hw_version();
    static std::string hw_model();
    static std::string hw_cpu_type();
    static std::string hw_physical_memory();
};

#endif //OSWRAPPER_H
