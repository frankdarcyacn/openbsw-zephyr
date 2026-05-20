cmake_minimum_required(VERSION 3.20.0)

set(OPENBSW_DIR
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../openbsw
    CACHE PATH "Path to Eclipse OpenBSW")

set(BUILD_EXECUTABLE
    "Zephyr_OpenBSW_HelloWorldApp"
    CACHE STRING "Name of build executable")

set(BUILD_TARGET_PLATFORM
    "ZEPHYR"
    CACHE STRING "Name of Target Platform")

set(BUILD_TARGET_RTOS
    "ZEPHYR"
    CACHE STRING "Name of Target OS")

# make sure zephyr compiler options are also set for cmake modules not depending on zephyr
add_compile_options($<TARGET_PROPERTY:zephyr_interface,INTERFACE_COMPILE_OPTIONS>)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(hello_world)

add_subdirectory(${OPENBSW_DIR} openbsw)

target_sources(app PRIVATE src/main.cpp)

target_link_libraries(app PUBLIC util)
