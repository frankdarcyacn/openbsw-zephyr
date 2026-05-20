add_subdirectory(asyncZephyr)
add_subdirectory(bspZephyr)
if (CONFIG_NETWORKING)
    add_subdirectory(zephyrEthAdapter)
endif()

# ETL implementation
add_subdirectory(etlImpl)
