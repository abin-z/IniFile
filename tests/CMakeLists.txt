# 如果是 MSVC 编译器，设置源文件为 UTF-8 编码
if (MSVC)
  add_compile_options("/utf-8")
endif()

# 添加测试可执行文件，将 test_inifile.cpp 作为源文件
add_executable(initest test_inifile.cpp)

if (MSVC)
  # 在 MSVC 下启用 /bigobj，以支持段数较多的大型对象文件
  target_compile_options(initest PRIVATE /bigobj)
endif()

# 链接 Catch2 测试框架库
target_link_libraries(initest PRIVATE Catch2)

# 链接被测库 inifile
target_link_libraries(initest PRIVATE inifile)

# 允许 add_test() 添加测试
enable_testing()

# 注册 initest 作为 CTest 可识别的测试用例
# 当执行 `ctest` 时，会运行 initest 并检查其返回值
add_test(NAME IniFileTest COMMAND initest)