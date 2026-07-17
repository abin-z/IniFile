cmake_minimum_required(VERSION 3.20)

project(testinifile LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(cmake/add_inifile.cmake)

add_executable(testinifile main.cpp)
target_link_libraries(testinifile PUBLIC inifile::inifile)