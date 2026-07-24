# Using OpenBSW with Zephyr

This repo provides adaptation libraries and sample code to help
build applications for [Zephyr](https://www.zephyrproject.org/)
that incorporate [OpenBSW](https://github.com/eclipse-openbsw/openbsw) libraries.

Two primary example applications are provided.

* [hello_world](samples/hello_world/README.md) is a very simple example that shows the bare minimum configuration needed to build OpenBSW and Zephyr together.
* [demo_app](samples/demo_app/README.md) is a full example that demonstrates some of OpenBSW's features working on Zephyr.
  See [samples/demo_app/README.md](samples/demo_app/README.md) for its description.

## Prerequisites

Set up the dependencies and tools needed to build Zephyr by following its
[Getting Started Guide](https://docs.zephyrproject.org/latest/develop/getting_started/index.html).
When this is complete, you should have a python environment in which [Zephyr's `west` tool](https://docs.zephyrproject.org/latest/develop/west/index.html) is available.
`west` is a multipurpose tool used extensively in development and test of applications using Zephyr, as outlined below.

## Workspace Setup

Create a top directory (eg. named `openbsw_zephyr_workspace`) and clone this repo in it.

```
mkdir openbsw_zephyr_workspace
cd openbsw_zephyr_workspace
git clone https://github.com/eclipse-openbsw/openbsw-zephyr.git openbsw-zephyr
```

Initialize the workspace (this just creates `.west/config`)...

```
openbsw_zephyr_workspace> west init -l openbsw-zephyr
```

Update the workspace - the following command pulls code from multiple git repos
and lays it out in the workspace as specified in `openbsw-zephyr/west.yml`...

```
openbsw_zephyr_workspace> west update
```

## Build, run and debug on your host

Among the [boards supported by Zephyr](https://docs.zephyrproject.org/latest/boards/index.html)
is `native_sim` which provides a simulation of an embedded board to allow application development
on your host system.

To build the `hello_world` sample app to run on your host system...

```
openbsw_zephyr_workspace> west build -p -b native_sim openbsw-zephyr/samples/hello_world
```

Assuming this is successful, you can run the `hello_world` executable...
```
openbsw_zephyr_workspace> build/zephyr/zephyr.exe
```

If you wish to build another sample in the same `build` directory then you can delete the old build first, or, use `west` option `-p always` (or just `-p`).

To build the `demo_app` sample to run on your host system...
```
openbsw_zephyr_workspace> west build -p -b native_sim openbsw-zephyr/samples/demo_app
```
And as above, you can run the `demo_app` executable as follows...

Note: Before running the `demo_app` it is recommended to set up connectivity as described [here](#setting-up-native-sim-communication).
```
openbsw_zephyr_workspace> build/zephyr/zephyr.exe
```

To start the application in a `gdb` session...
```
openbsw_zephyr_workspace> west debug
```

## Build, flash and debug on an embedded target board

To build `hello_world` for an actual board, let's use the `s32k148_evb` board as an example...
```
openbsw_zephyr_workspace> west build -p -b s32k148_evb openbsw-zephyr/samples/hello_world
```

Once built, to flash the image on the board...
```
openbsw_zephyr_workspace> west flash
```

To debug the application running on the board...
```
openbsw_zephyr_workspace> west debug
```

To connect to the board's console via serial port...
```
minicom -D /dev/ttyACM0
```

To do the same with `demo_app` on the board,
simply replace `hello_world` with `demo_app` in the above `west build` command.

If you have a `s32k148_evb` with the `nxp_adtja1101` automotive ethernet shield,
you can create an image for that
by adding that shield and the networking stack as extra configuration, as follow...
```
west build -p -b s32k148_evb --shield nxp_adtja1101 openbsw-zephyr/samples/demo_app -- -DEXTRA_CONF_FILE=boards/s32k148_evb_network.conf
```

See [samples/demo_app/README.md](samples/demo_app/README.md) for info about `demo_app`
and building it for different boards.

## Setting up native-sim communication

### Setting up CAN on Ubuntu/WSL

For `native_sim`, CAN is implemented using a Virtual `SocketCAN` interface named `vcan0`
which can be set up on Ubuntu as described
[here](https://eclipse-openbsw.github.io/openbsw/sphinx_docs/doc/dev/learning/can/index.html).
Other posix hosts may be set up similarly.

However, if you are using Ubuntu in WSL,
the default Ubuntu image distributed for WSL does not include `SocketCAN` support.
The solution is to rebuild the kernel with `SocketCAN` support as described
[here](https://eclipse-openbsw.github.io/openbsw/sphinx_docs/doc/dev/learning/setup/setup_wsl_socketcan.html).

If you are using a development board connected to your PC via a USB-CAN adapter,
then you should be able to make that available in WSL (with Ubuntu built with `SocketCAN` support)
as an interface named `can0`using
[usbipd](https://github.com/dorssel/usbipd-win/)
as described
[here](https://eclipse-openbsw.github.io/openbsw/sphinx_docs/doc/dev/learning/setup/setup_wsl_usb.html).

### Setting up ethernet communication on Ubuntu/WSL

Run the `net-setup.sh` script from the zephyr tools folder at the root of the openbsw_zephyr_workspace in a separate terminal...
```
openbsw_zephyr_workspace/tools/net-tools> ./net-setup.sh
```
By default this uses the configuration in `zeth.conf`
which sets up a new network interface called `zeth` with IP address `192.0.2.2`.
The interface will be removed when the script is stopped with `CTRL-C`.

## Legals

Distributed under the [Apache 2.0 License](LICENSE).

Also see [NOTICE](NOTICE.md).
