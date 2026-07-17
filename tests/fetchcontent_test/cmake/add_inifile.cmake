include(FetchContent)

FetchContent_Declare(
  inifile
  GIT_REPOSITORY https://github.com/abin-z/inifile.git
  GIT_TAG main
)

FetchContent_MakeAvailable(inifile)