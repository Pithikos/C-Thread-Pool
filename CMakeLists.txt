cmake_minimum_required(VERSION 3.1)

project(thpool)
set(CMAKE_BUILD_TYPE Release)
include(ExternalProject)

set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR})

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
set(SOURCES ${CMAKE_SOURCE_DIR}/thpool.c)
set(HEADERS ${CMAKE_SOURCE_DIR}/thpool.h)

add_library(pool_shared SHARED ${SOURCES})
set_target_properties(pool_shared PROPERTIES VERSION 1.0 SOVERSION 1)
add_library(pool_static STATIC ${SOURCES})
set_target_properties(pool_shared pool_static PROPERTIES OUTPUT_NAME thpool)
target_link_libraries(pool_shared pool_static Threads::Threads)

if(MSVC)
  # Force to always compile with W4
  if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]" OR CMAKE_C_FLAGS MATCHES "/W[0-4]")
    string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
  else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /W4")
  endif()
elseif(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
  # Update if necessary
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wstrict-prototypes")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wstrict-prototypes")
endif()

install(TARGETS pool_shared pool_static DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
install(FILES ${HEADERS} DESTINATION ${CMAKE_INSTALL_PREFIX}/include)
