cmake_minimum_required(VERSION 3.20)

project(inifile LANGUAGES C CXX VERSION 0.9.9)

# 引入 GNUInstallDirs 模块, 提供标准安装目录变量
# 例如：CMAKE_INSTALL_PREFIX, CMAKE_INSTALL_LIBDIR, CMAKE_INSTALL_INCLUDEDIR 等
# 方便跨平台写 install(), 避免硬编码路径
include(GNUInstallDirs)

# ---------------------------------------------------------------------------------------
# Set default build to Release for single-config generators (Makefile/Ninja)
# ---------------------------------------------------------------------------------------
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  if((DEFINED PROJECT_IS_TOP_LEVEL AND PROJECT_IS_TOP_LEVEL)
    OR (NOT DEFINED PROJECT_IS_TOP_LEVEL AND CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR))
      set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose Release or Debug" FORCE)
  endif()
endif()

# ----------------------------------------
# Header-only library
# ----------------------------------------
add_library(inifile INTERFACE)
# Alias target
add_library(inifile::inifile ALIAS inifile)
target_include_directories(inifile
  INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_compile_features(inifile INTERFACE cxx_std_11)

# Compiler-specific warning/options
set(gcc_like_cxx "$<COMPILE_LANG_AND_ID:CXX,ARMClang,AppleClang,Clang,GNU,LCC>")
set(msvc_cxx "$<COMPILE_LANG_AND_ID:CXX,MSVC>")
target_compile_options(inifile INTERFACE
  "$<${gcc_like_cxx}:$<BUILD_INTERFACE:-Wall;-Wextra;-Wshadow;-Wformat=2;-Wunused>>"
  "$<${msvc_cxx}:$<BUILD_INTERFACE:/W3;/utf-8>>"
)

# ----------------------------------------------------------
# Detect master (top-level) project
# ----------------------------------------------------------
if(NOT DEFINED INIFILE_MASTER_PROJECT)
  if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(INIFILE_MASTER_PROJECT ON)
  else()
    set(INIFILE_MASTER_PROJECT OFF)
  endif()
endif()

if(INIFILE_MASTER_PROJECT)
  message(STATUS "[inifile] Master project: ${INIFILE_MASTER_PROJECT}")
  # Generate compile_commands.json for clangd/clang-tidy
  set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
  # Enable PIC for all targets (safest for static libs)
  set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

# ----------------------------------------
# Options for examples/tests/install
# ----------------------------------------
option(INIFILE_BUILD_EXAMPLES  "Build examples" ${INIFILE_MASTER_PROJECT})
option(INIFILE_BUILD_TESTS "Build tests" ${INIFILE_MASTER_PROJECT})
option(INIFILE_INSTALL "Generate install target" ${INIFILE_MASTER_PROJECT})

if(INIFILE_BUILD_EXAMPLES)
  message(STATUS "[inifile] Building examples...")
  add_subdirectory(examples)
endif()

if(INIFILE_BUILD_TESTS)
  message(STATUS "[inifile] Building tests...")
  include(CTest)  # enable add_test
  add_subdirectory(external/Catch2)
  add_subdirectory(tests)
endif()

if(INIFILE_INSTALL)
  message(STATUS "[inifile] Generating install target...")

  # -------------------------------
  # 安装库 target
  # - INTERFACE 库: 只需要 INCLUDES DESTINATION
  # - STATIC/SHARED 库: 需要 LIBRARY/ARCHIVE/RUNTIME DESTINATION
  # -------------------------------
  install(TARGETS inifile
    EXPORT inifileTargets               # 导出 target，用于 find_package()
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}  # 头文件路径
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}       # 共享库安装路径
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}       # 静态库安装路径
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}       # 可执行文件路径(Windows)
  )

  # -------------------------------
  # 安装头文件目录, 只拷贝头文件, 避免拷贝多余文件
  # -------------------------------
  install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp" PATTERN "*.inl"
  )

  # -------------------------------
  # 导出 target 定义文件(inifileTargets.cmake)
  # 供其他项目通过 find_package() 引用
  # -------------------------------
  install(EXPORT inifileTargets
    FILE inifileTargets.cmake           # 导出文件名
    NAMESPACE inifile::                 # 导入后 target 前缀
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/inifile # 安装目录
  )

  # -------------------------------
  # 生成 find_package 使用的入口文件
  # 配置模板 -> 输出 config 文件
  # -------------------------------
  include(CMakePackageConfigHelpers)
  configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/inifileConfig.cmake.in"  # 模板文件
    "${CMAKE_CURRENT_BINARY_DIR}/inifileConfig.cmake"           # 输出文件
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/inifile"
    # NO_SET_AND_CHECK_MACRO, NO_CHECK_REQUIRED_COMPONENTS_MACRO 可以省略
  )

  # -------------------------------
  # 生成版本文件(find_package 时用于版本检查)
  # -------------------------------
  write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/inifileConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}       # 项目版本号(库版本号)
    COMPATIBILITY SameMajorVersion   # 同主版本兼容
  )

  # -------------------------------
  # 安装 config 和版本文件
  # -------------------------------
  install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/inifileConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/inifileConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/inifile
  )
endif()