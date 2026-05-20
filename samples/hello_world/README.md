# `hello_world`- sample application using OpenBSW with Zephyr

This sample code provides a minimal example of building OpenBSW with Zephyr.
In [main.cpp](src/main.cpp) it uses OpenBSW's `util::string::ConstString` to define `"Hello World!"`
and prints that to `stdout` along with `CONFIG_BOARD_TARGET`.
This sample serves to explain the minimum build configuration needed to bring an OpenBSW library into Zephyr.

As outlined in the [top-level README.md](../../README.md),
once the Zephyr build environment is set up
this application can be built for a variety of boards as follows...
```
openbsw_zephyr_workspace> west build -p -b <board> openbsw-zephyr/samples/hello_world
```
where `<board>` is any board supported by Zephyr with a standard library.

## CMake

Please take a look at the `CMakeLists.txt` file in this directory and note the following points.

1. OpenBSW is found by including by the following lines...
```
set(OPENBSW_DIR
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../openbsw
    CACHE PATH "Path to Eclipse OpenBSW")

...

add_subdirectory(${OPENBSW_DIR} openbsw)
```

2. Zephyr is found by the following line...
```
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
```

3. In order to build OpenBSW libraries for different Zephyr-supported target boards, the following line is needed...
```
add_compile_options($<TARGET_PROPERTY:zephyr_interface,INTERFACE_COMPILE_OPTIONS>)
```

4. Finally OpenBSW's `util` library is made a dependency in order to include it in the build...
```
target_link_libraries(app PUBLIC util)
```


## Kconfig configuration

Since OpenBSW's `util` library is written in `C++`, and is dependent on the standard library,
the following are added to `prj.conf`...
```
CONFIG_CPP=y
CONFIG_REQUIRES_FULL_LIBCPP=y
```
