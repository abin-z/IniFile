# 指定CMake最低版本要求
cmake_minimum_required(VERSION 3.20)

# 生成 compile_commands.json 文件（用于 clangd、clang-tidy 等工具）
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 设置默认构建类型为Release，如果未指定构建类型
if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type")
endif()

# 设置C++标准版本为C++11，并确保严格要求此标准，不使用扩展功能
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 设置所有库生成位置无关代码 (Position Independent Code) 对应GCC/Clang的-fPIC
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# 设置输出路径：库文件、可执行文件和归档文件的生成目录
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/build_output/lib) # 静态库 (.a/.lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/build_output/bin) # 动态库 (.so/.dll)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/build_output/bin) # 可执行文件

project(IniFile-Cpp LANGUAGES C CXX VERSION 0.9.9)

# 启用测试功能
include(CTest)  # 启用 CTest 框架
enable_testing()  # 允许 add_test() 添加测试, 须放在 add_test() 之前

# 添加子目录
add_subdirectory(include)
add_subdirectory(examples)
add_subdirectory(external)
add_subdirectory(tests)

