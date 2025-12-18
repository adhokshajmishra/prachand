cmake_minimum_required(VERSION 3.22)
project(embedded_webserver)

set(CMAKE_CXX_STANDARD 17)

find_package(PkgConfig REQUIRED)
pkg_check_modules(PQXX REQUIRED libpqxx)
message(STATUS "Found pqxx version: ${PQXX_VERSION}")
message(STATUS "pqxx include dir: ${PQXX_INCLUDE_DIR}")
message(STATUS "pqxx libraries: ${PQXX_LIBRARIES}")

if (PQXX_FOUND)
    add_subdirectory(${PROJECT_SOURCE_DIR}/libhttpserver)
    add_executable(c2server main.cpp ConnectionPool.cpp ConnectionPool.h
            ConfigurationParser.cpp
            ConfigurationParser.h
            RequestHandler.cpp
            RequestHandler.h)
    target_include_directories(c2server PUBLIC ${PROJECT_SOURCE_DIR}/../3rdparty)
    target_link_libraries(c2server libhttpserver)
    target_link_libraries(c2server ${PQXX_LINK_LIBRARIES})
endif ()