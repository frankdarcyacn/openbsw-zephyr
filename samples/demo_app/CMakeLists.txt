cmake_minimum_required(VERSION 3.20.0)

set(OPENBSW_DIR
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../openbsw
    CACHE PATH "Path to Eclipse OpenBSW")

set(INCLUDE_OPENBSW_LIBS_BSP
    OFF
    CACHE BOOL "Include openbsw/libs/bsp/ in build")

set(BUILD_EXECUTABLE
    "Zephyr_OpenBSW_DemoApp"
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

project(demo_app)

if (CONFIG_NETWORKING)
        add_compile_definitions(PLATFORM_SUPPORT_ETHERNET=1)
endif()

if (CONFIG_CAN)
        add_compile_definitions(PLATFORM_SUPPORT_CAN=1)
        add_compile_definitions(PLATFORM_SUPPORT_UDS=1)
endif()

add_subdirectory(${OPENBSW_DIR} openbsw)

target_include_directories(app
        PRIVATE
        include)

target_include_directories(etl BEFORE INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include/etl_profile)

target_sources(app 
        PRIVATE 
        src/console/console.cpp
        src/systems/DemoSystem.cpp
        src/systems/RuntimeSystem.cpp
        src/systems/SysAdminSystem.cpp
        src/uds/DiagSession.cpp
        src/uds/ReadIdentifierPot.cpp
        src/logger/logger.cpp
        src/main.cpp)

if (CONFIG_TRACING_USER)
target_sources(app PRIVATE src/TraceHooks.cpp)
endif()

if (CONFIG_CAN)
target_sources(app
        PRIVATE
        src/systems/CanSystem.cpp
        src/systems/DoCanSystem.cpp
        src/systems/TransportSystem.cpp
        src/systems/UdsSystem.cpp)
endif()

# Path to libraries for adaptation of OpenBSW to Zephyr
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../libs openbsw_zephyr_libs)

add_subdirectory(openbswConfig)

add_library(asyncPlatform ALIAS asyncZephyr)

target_link_libraries(app PUBLIC
        asyncBinding
        asyncConsole
        asyncZephyrImpl
        configuration
        common
        cpp2can
        canTransceiverZephyr
        docan
        lifecycle
        lifecycleSupport
        logger
        loggerIntegration
        stdioConsoleInput
        transport
        transportRouterSimple
        uds
        etl
        etlImpl
        bspZephyr)

if (CONFIG_NETWORKING)
target_link_libraries(app PUBLIC
        zephyrEthAdapter)
endif()
