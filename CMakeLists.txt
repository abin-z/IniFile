cmake_minimum_required(VERSION 3.20)

# ---------------------------------------------------------------------------------------
# Set default build to Release for single-config generators (Makefile/Ninja)
# ---------------------------------------------------------------------------------------
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  if((DEFINED PROJECT_IS_TOP_LEVEL AND PROJECT_IS_TOP_LEVEL)
    OR (NOT DEFINED PROJECT_IS_TOP_LEVEL AND CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR))
      set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose Release or Debug" FORCE)
  endif()
endif()

# ---------------------------------------------------------------------------------------
# Project definition
# ---------------------------------------------------------------------------------------
project(inifile LANGUAGES C CXX VERSION 0.9.9)

# ----------------------------------------
# Header-only library
# ----------------------------------------
add_library(inifile INTERFACE)
target_include_directories(inifile
  INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
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
# Options for examples/tests
# ----------------------------------------
option(INIFILE_BUILD_EXAMPLES  "Build examples" ${INIFILE_MASTER_PROJECT})
option(INIFILE_BUILD_TESTS "Build tests" ${INIFILE_MASTER_PROJECT})

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