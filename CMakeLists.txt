cmake_minimum_required(VERSION 3.28)
project(Prachand)

set(CMAKE_CXX_STANDARD 23)

add_subdirectory(${PROJECT_SOURCE_DIR}/c2server)
add_subdirectory(${PROJECT_SOURCE_DIR}/payload)
add_subdirectory(${PROJECT_SOURCE_DIR}/controller)
add_executable(prachand main.cpp)
