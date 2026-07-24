# `multicast_ex2`- sample application to test IPv6 multicast

This example was added to test the implementation of `ZephyrDatagramSocket::join()`

Set up the `zeth` interface...

```
tools/net-tools/net-setup.sh start
```

Build for native sim as follows...
```
west build -p -b native_sim openbsw-zephyr/samples/multicast_ex2
```

Run the built executable...

```
$ ./build/zephyr/zephyr.exe
WARNING: Using a test - not safe - entropy source
*** Booting Zephyr OS build v4.1.0-5811-gea6eade52b59 ***
[00:00:00.000,000] <inf> net_config: Initializing network
[00:00:00.000,000] <inf> net_config: IPv4 address: 192.0.2.1
[00:00:00.000,000] <inf> app: Bind socket to :::22222
[00:00:00.000,000] <inf> app: Socket bound to :::22222
[00:00:00.000,000] <inf> app: Join multicast group ff14:0000:0000:0000:0000:0000:0004:0000
[00:00:00.000,000] <inf> app: Joined multicast group ff14:0000:0000:0000:0000:0000:0004:0000
[00:00:00.000,000] <inf> app: Up and running
```

Check `INTERFACE_INDEX` in the test script `send_data_multicast_ipv6.py`
matches the index for `zeth` in your setup (run `ip a` to see index).

In another terminal run the python test script...
```
$ python3 send_data_multicast_ipv6.py
Sending to [ff14::0004:0000]:22222 on interface index 8

Sent 12 bytes to [ff14::0004:0000]:22222
Hex data: DEADBEEFCAFEBABE12345678
```

and the data should be received by the Zephyr app, like this...
```
[00:00:33.660,000] <inf> app: dataReceived: packet length=12.
[00:00:33.660,000] <inf> app: dataReceived: read 12 bytes from 201:db8:00:00:00:00:00:02:48047 to 00:00:00:00:00:00:00:00
[00:00:33.660,000] <inf> app: dataReceived: DEADBEEF
[00:00:33.660,000] <inf> app: dataReceived: CAFEBABE
[00:00:33.660,000] <inf> app: dataReceived: 12345678
```
