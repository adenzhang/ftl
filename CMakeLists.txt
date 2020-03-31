
# cmake -G "Eclipse CDT4 - Unix Makefiles" -D_ECLIPSE_VERSION=4.2 -DCMAKE_BUILD_TYPE=Debug ../build
cmake_minimum_required( VERSION 2.6 )
project(ftl)

# check 64 bit
if( CMAKE_SIZEOF_VOID_P EQUAL 8 )
    set( ARCH x64 )
else()
    set( ARCH x86 )
endif()

if( NOT CMAKE_BUILD_TYPE )
        set(CMAKE_BUILD_TYPE Debug)
endif()

if (CMAKE_BUILD_TYPE STREQUAL Debug)
        set( BUILDTYPE debug)
else()
        set( BUILDTYPE release)
endif()
message("*** ${ARCH}/${BUILDTYPE} ***")



####################### test ###################

add_subdirectory(src)
add_subdirectory(test)

#add_subdirectory(src/try_catch)
