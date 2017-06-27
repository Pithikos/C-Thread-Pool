

add_library(thpool thpool.c)

add_executable(example example.c)
target_link_libraries(example thpool)
