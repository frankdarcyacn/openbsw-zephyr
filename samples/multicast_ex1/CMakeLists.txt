cmake_minimum_required(VERSION 3.20.0)

set(OPENBSW_DIR
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../openbsw
    CACHE PATH "Path to Eclipse OpenBSW")

set(INCLUDE_OPENBSW_LIBS_BSP
    OFF
    CACHE BOOL "Include openbsw/libs/bsp/ in build")

# make sure zephyr compiler options are also set for cmake modules not depending on zephyr
add_compile_options($<TARGET_PROPERTY:zephyr_interface,INTERFACE_COMPILE_OPTIONS>)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(multicast_ex1)

if (CONFIG_NETWORKING)
        add_compile_definitions(PLATFORM_SUPPORT_ETHERNET=1)
endif()

include(${OPENBSW_DIR}/Filelists.cmake)

target_sources(app PRIVATE
    src/main.cpp
    src/OpenBSWZephyrLogger.cpp)

target_include_directories(app PRIVATE
        include)

# Path to libraries for adaptation of OpenBSW to Zephyr
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../libs openbsw_zephyr_libs)

add_subdirectory(openbswConfig)

add_library(commonImpl INTERFACE)

add_library(asyncPlatform ALIAS asyncZephyr)

target_link_libraries(app PUBLIC util)

if (CONFIG_NETWORKING)
target_link_libraries(app PUBLIC
        zephyrEthAdapter)
endif()
