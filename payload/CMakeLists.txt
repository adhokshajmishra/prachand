cmake_minimum_required(VERSION 3.28)
project(payload)

set(CMAKE_CXX_STANDARD 23)

set(CMAKE_FIND_LIBRARY_SUFFIXES .a)

find_package(OpenSSL REQUIRED)
message(STATUS "Found OpenSSL version: ${OpenSSL_VERSION}")
message(STATUS "OpenSSL include dir: ${OPENSSL_INCLUDE_DIR}")
message(STATUS "OpenSSL libraries: ${OPENSSL_LIBRARIES}")

if(OPENSSL_FOUND)
    add_executable(payload main.cpp
            OSWrapper.cpp
            OSWrapper.h
            config.h
            CommandHandler.cpp
            CommandHandler.h)
    target_link_libraries(payload PRIVATE OpenSSL::SSL OpenSSL::Crypto)
endif()