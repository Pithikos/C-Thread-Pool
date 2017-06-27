
cmake_minimum_required(VERSION 2.8 FATAL_ERROR)
add_library(thpool thpool.c)
target_link_libraries(thpool pthread)
add_executable(example example.c)
target_link_libraries(example thpool)
