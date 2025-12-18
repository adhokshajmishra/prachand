cmake_minimum_required(VERSION 3.28)
project(controller)

set(CMAKE_CXX_STANDARD 23)

find_package(OpenSSL REQUIRED)
message(STATUS "Found OpenSSL version: ${OpenSSL_VERSION}")
message(STATUS "OpenSSL include dir: ${OPENSSL_INCLUDE_DIR}")
message(STATUS "OpenSSL libraries: ${OPENSSL_LIBRARIES}")

if(OPENSSL_FOUND)
    add_executable(controller main.cpp
            ConfigurationParser.cpp
            ConfigurationParser.h
            ShellHandler.cpp
            ShellHandler.h
            Commands.cpp
            Commands.h
            Common.cpp
            Common.h
    )

    target_include_directories(controller PUBLIC ${PROJECT_SOURCE_DIR}/../3rdparty)
    target_link_libraries(controller PRIVATE OpenSSL::SSL OpenSSL::Crypto)
endif ()