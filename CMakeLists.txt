cmake_minimum_required(VERSION 3.0.0)
project(C-Thread-Pool LANGUAGES C)

install(FILES thpool.h DESTINATION include/C-Thread-Pool)

add_library(thpool_static STATIC thpool.c)
install(TARGETS thpool_static DESTINATION lib)
set_target_properties(thpool_static PROPERTIES OUTPUT_NAME thpool)

add_library(thpool_shared SHARED thpool.c)
install(TARGETS thpool_shared DESTINATION lib)
set_target_properties(thpool_shared PROPERTIES OUTPUT_NAME thpool)
