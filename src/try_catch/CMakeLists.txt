cmake_minimum_required( VERSION 2.6 )

set(PROJNAME try_catch)
project(try_catch)

add_definitions( -std=c++17)

file(GLOB SRC
    try_catch.h
    try_catch.cpp
)
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}  ${CMAKE_CURRENT_SOURCE_DIR}/.. )

add_library( ${PROJNAME} SHARED  ${SRC} )

target_include_directories(${PROJNAME} INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)
