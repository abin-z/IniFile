add_executable(inifile_basic inifile_basic.cpp)
target_link_libraries(inifile_basic PRIVATE inifile)

add_executable(inifile_read inifile_read.cpp)
target_link_libraries(inifile_read PRIVATE inifile)

add_executable(inifile_write inifile_write.cpp)
target_link_libraries(inifile_write PRIVATE inifile)

add_executable(inifile_load inifile_load.cpp)
target_link_libraries(inifile_load PRIVATE inifile)

add_executable(inifile_save inifile_save.cpp)
target_link_libraries(inifile_save PRIVATE inifile)

add_executable(inifile_custom inifile_custom.cpp)
target_link_libraries(inifile_custom PRIVATE inifile)

add_executable(inifile_custom2 inifile_custom2.cpp)
target_link_libraries(inifile_custom2 PRIVATE inifile)

add_executable(inifile_case_insensitive inifile_case_insensitive.cpp)
target_link_libraries(inifile_case_insensitive PRIVATE inifile)

add_executable(inifile_utility inifile_utility.cpp)
target_link_libraries(inifile_utility PRIVATE inifile)

add_executable(inifile_comment inifile_comment.cpp)
target_link_libraries(inifile_comment PRIVATE inifile)