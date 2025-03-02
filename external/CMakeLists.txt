# 设置 Catch2 头文件目录路径，并存入 CMake 缓存  
# ${CMAKE_CURRENT_LIST_DIR} 表示当前 CMakeLists.txt 所在目录  
# 这样 CATCH2_INCLUDE_DIR 指向 Catch2 头文件所在的目录，供后续 include 使用  
set(CATCH2_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/catch2_single" CACHE INTERNAL "")
