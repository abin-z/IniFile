add_executable(inifile_read inifile_read.cpp)
target_link_libraries(inifile_read PRIVATE inifile)

add_executable(inifile_write inifile_write.cpp)
target_link_libraries(inifile_write PRIVATE inifile)

add_executable(inifile_load inifile_load.cpp)
target_link_libraries(inifile_load PRIVATE inifile)

add_executable(inifile_save inifile_save.cpp)
target_link_libraries(inifile_save PRIVATE inifile)

add_executable(inifile_basic inifile_basic.cpp)
target_link_libraries(inifile_basic PRIVATE inifile)

add_executable(inifile_custom inifile_custom.cpp)
target_link_libraries(inifile_custom PRIVATE inifile)