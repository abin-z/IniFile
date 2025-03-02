message(STATUS "Configuring tests")

# 添加测试可执行文件，将 test01.cpp 作为源文件
add_executable(initest test01.cpp)

# 设置 test01 目标的头文件搜索路径
# 这里包含 Catch2 头文件，以便 test01.cpp 能够使用 Catch2 进行单元测试
target_include_directories(initest PRIVATE ${CATCH2_INCLUDE_DIR})

# 链接被测库 inifile
target_link_libraries(initest PRIVATE inifile)

# 注册 initest 作为 CTest 可识别的测试用例
# 当执行 `ctest` 时，会运行 initest 并检查其返回值
add_test(NAME IniFileTest COMMAND initest)