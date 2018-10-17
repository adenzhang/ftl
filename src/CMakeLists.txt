############## ftl ###############
set(PROJNAME ftl)
project(${PROJNAME})

file(GLOB SRC
    ftl/*h
)

add_library( ${PROJNAME} INTERFACE )

target_sources(${PROJNAME} INTERFACE
    ${SRC}
  #${CMAKE_CURRENT_SOURCE_DIR}/ftl/*.h
)
target_include_directories(${PROJNAME} INTERFACE
 $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

############## try_catch ###############

set(PROJNAME try_catch)
project(try_catch)

add_definitions( -std=c++17)

file(GLOB SRC
    try_catch/try_catch.h
    try_catch/try_catch.cpp
)


add_library( ${PROJNAME} SHARED  ${SRC} )

target_include_directories( ${PROJNAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)
target_link_libraries( ${PROJNAME} ftl )
