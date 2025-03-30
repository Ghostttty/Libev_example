########################################################################
# Prevent in-tree builds
########################################################################

if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
	message(FATAL_ERROR "Prevented in-tree build. This is bad practice.")
endif(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})

########################################################################
# Project setup
########################################################################

cmake_minimum_required(VERSION 3.28.0)
project(libev_server VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

message( STATUS "CMAKE_SYSTEM: " ${CMAKE_SYSTEM} )
message( STATUS "CMAKE_SYSTEM_PROCESSOR: " ${CMAKE_SYSTEM_PROCESSOR} )
message( STATUS "CMAKE_C_COMPILER: " ${CMAKE_C_COMPILER} )

set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -pipe -Wall -Wextra -Werror -pedantic")

########################################################################
# Find libev library
########################################################################

find_library(LIBEV_LIBRARY
    NAMES ev
    PATHS /usr/lib /usr/local/lib
)

find_path(LIBEV_INCLUDE_DIR
    NAMES ev.h
    PATHS /usr/include /usr/local/include
)

########################################################################
# SRC
########################################################################

add_executable(libev_server ./src/main.cpp)

if (LIBEV_LIBRARY AND LIBEV_INCLUDE_DIR)
    target_include_directories(libev_server PRIVATE ${LIBEV_INCLUDE_DIR})
    target_link_libraries(libev_server PRIVATE ${LIBEV_LIBRARY})
else()
    message(FATAL_ERROR "libev-dev not found!")
endif()

########################################################################
# Include directories
########################################################################

include_directories(include)