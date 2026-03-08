cmake_minimum_required(VERSION 3.20)
project(test_inifile LANGUAGES CXX)

find_package(inifile REQUIRED)

add_executable(test_main main.cpp)
target_link_libraries(test_main PRIVATE inifile::inifile)