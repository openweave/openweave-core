# OpenWeave Tools

OpenWeave includes a set of command line tools to configure, manage, and test
OpenWeave deployments.

## Build weave-tools

The `weave-tools` target builds a tar archive of select Weave command line tools
for distribution. It includes the following tools:

Tool | Description | [Standalone build](https://openweave.io/guides/build#standalone_application) location
----|----|----
`gen-qr-code` | Generate a QR code | `/src/tools/misc`
`mock-device` | Generic Weave device simulator | `/src/test-apps`
`weave` | Generate and manage Weave certificates | `/src/tools/weave`
`weave-device-descriptor` | Encode and decode Weave device descriptor strings for pairing QR codes | `/src/test-apps`
[`weave-device-mgr`](/guides/tools/device-manager) | Manage the device pairing process | `/src/device-manager/python`
`weave-heartbeat` | Send and receive <a href="https://openweave.io/guides/weave-primer/profiles#heartbeat">Heartbeat <!-- heartbeat --></a> profile messages | `/src/test-apps`
`weave-key-export` | Send key export requests | `/src/test-apps`
`weave-ping`| Send and receive <a href="https://openweave.io/guides/weave-primer/profiles#echo">Echo <!-- echo --></a> profile messages | `/src/test-apps`

To build the target:

1.  Install all [build prerequisites](https://openweave.io/guides/build#prerequisites).
1.  Configure OpenWeave without BlueZ support:

        $ cd {path-to-openweave-core}
        $ ./configure --without-bluez

1.  Make the `weave-tools` target:

        $ make weave-tools

1.  Check the root `openweave-core` directory for the tar archive:

        $ ls weave*
        weave-tools-x86_64-unknown-linux-gnu-4.0d.tar.gz

## gen-qr-code

The `gen-qr-code` tool requires the Python `qrcode` module. Use `pip` to install
it:

```
$ pip install --user qrcode
```

Use `gen-qr-code` to generate a QR code for device pairing purposes. The input
for the tool must reside in a local file. For example, to generate a QR code
that sends the user to [https://www.google.com](https://www.google.com):

1.  Create a file with the string for the QR code:

        $ echo "https://www.google.com" >> ~/ow_qrcode

1.  Generate a 64x64 QR code of that string:

        $ ./gen-qr-code -v 1 -s 64 < ~/ow_qrcode

1.  To save the QR code as an image, specify an output file:

        $ ./gen-qr-code -v 1 -s 64 < ~/ow_qrcode > ~/ow_qrcode.png

Use the `weave-device-descriptor` tool to generate the device descriptor string
for use in a Weave device's pairing QR code.

## mock-device

The `mock-device` tool simulates a generic Weave node. Other tools and test case
scripts use this tool to encapsulate Weave functionality. Instantiate mock
devices on individual Happy nodes to test Weave functionality in a simulated
topology.

For example, to start a Weave mock device listening on an IPv6 address of
`fd00:0:1:1::1`, first add that IPv6 address to the `lo` (loopback) interface:

```
$ sudo ifconfig lo add fd00:0:1:1::1/64
```

Then start the Weave mock device:

```
$ ./mock-device -a fd00:0:1:1::1
WEAVE:ML: Binding IPv6 TCP listen endpoint to [fd00:0:1:1::1]:11095
WEAVE:ML: Listening on IPv6 TCP endpoint
WEAVE:ML: Binding general purpose IPv4 UDP endpoint to [::]:11095
WEAVE:IN: IPV6_PKTINFO: 92
WEAVE:ML: Binding general purpose IPv6 UDP endpoint to [fd00:0:1:1::1]:11095 (lo)
WEAVE:ML: Listening on general purpose IPv6 UDP endpoint
WEAVE:ML: Binding IPv6 multicast receive endpoint to [ff02::1]:11095 (lo)
WEAVE:ML: Listening on IPv6 multicast receive endpoint
WEAVE:EM: Cannot listen for BLE connections, null BleLayer
Weave Node Configuration:
  Fabric Id: 1
  Subnet Number: 1
  Node Id: 1
WEAVE:SD: init()
Weave Node Configuration:
  Fabric Id: 1
  Subnet Number: 1
  Node Id: 1
  Listening Addresses:
      fd00:0:1:1::1 (ipv6)
  Pairing Server: fd00:0:1:1::1
Mock Time Sync is disabled and not initialized
Mock System Time Offset initialized to: 3.213773 sec
Listening for requests...
Weave Node ready to service events; PID: 256904; PPID: 251571
```

Use the `--help` flag to view all available configuration options.

## weave-device-descriptor

The `weave-device-descriptor` tool encodes or decodes a device descriptor
string. These strings contain identifying information for a device that is
encoded into its Weave pairing QR code. Use the `--help` flag with the `encode`
or `decode` options for more information.

```
$ ./weave-device-descriptor encode --help
$ ./weave-device-descriptor decode --help
```

### encode

For example, to encode a device descriptor string with the following identifying
information, use the appropriate flags and values:

Field | Flag | Value
----|----|----
Vendor ID | `-V` | `1`
Product ID | `-p` | `1`
Product Revision Number | `-r` | `2`
Serial Number | `-s` | `18B4300000000004`
Manufacturing Date | `-m` | `2018/05/02`
802.15.4 MAC Address (Thread, BLE) | `-8` | `000D6F000DA80466`
Pairing Code | `-P` | `AB713H`

```
$ ./weave-device-descriptor encode -V 1 -p 1 -r 2 -s 18B4300000000004 -m 2018/05/02 \
                                   -8 000D6F000DA80466 -w 5CF370800E77 -P AB713H
1V:1$P:1$R:2$D:180502$S:18B4300000000004$L:000D6F000DA80466$W:5CF370800E77$C:AB713H$
```

Use this output string with the `gen-qr-code` tool to generate the QR code.

> Note: The Weave pairing code is a device-specific string of all uppercase alphanumeric characters (0-9 and A-Y, excluding I, O, Q and Z) with a length of 6 characters, where the first 5 characters are random and the 6th character is a check character generated by an adaptation of the Verhoeff algorithm. See the [Verhoeff module](https://github.com/openweave/openweave-core/tree/master/src/lib/support/verhoeff) for more information.

### decode

Use the `decode` option to decode an element of a device descriptor string. The
element to decode has the following syntax:

```
1 + {device-descriptor-element} + $
```

For example, to decode the `W:5CF370800E77` device descriptor element:

```
$ ./weave-device-descriptor decode 1W:5CF370800E77$
Primary WiFi MAC: 5C:F3:70:80:0E:77
```

## weave-heartbeat

Use `weave-heartbeat` to send and receive
<a href="https://openweave.io/guides/weave-primer/profiles#heartbeat">Heartbeat <!-- heartbeat --></a> profile messages between
two Weave nodes. Heartbeat provides a means to indicate liveness of one node to
the other nodes in the network, or to check if a node remains connected to the
fabric.

A successful Heartbeat requires one node to act as a server (listening for and
responding to the Heartbeat) and one node to act as a client (sending the
Heartbeat).

Test the `weave-heartbeat` tool using the loopback interface to mimic two nodes:

1.  Add the IPv6 addresses to be used for each Heartbeat node to the `lo`
    (loopback) interface:

        $ sudo ifconfig lo add fd00:0:1:1::1/64
        $ sudo ifconfig lo add fd00:0:1:1::2/64

1.  Start the Heartbeat server on the `fd00:0:1:1::1` address and assign it a
    `node-id` of 1:

        $ ./weave-heartbeat --node-addr fd00:0:1:1::1 --node-id 1 --listen
        WEAVE:ML: Binding IPv6 TCP listen endpoint to [fd00:0:1:1::1]:11095
        WEAVE:ML: Listening on IPv6 TCP endpoint
        WEAVE:ML: Binding general purpose IPv4 UDP endpoint to [::]:11095
        WEAVE:IN: IPV6_PKTINFO: 92
        WEAVE:ML: Binding general purpose IPv6 UDP endpoint to [fd00:0:1:1::1]:11095 (lo)
        WEAVE:ML: Listening on general purpose IPv6 UDP endpoint
        WEAVE:ML: Binding IPv6 multicast receive endpoint to [ff02::1]:11095 (lo)
        WEAVE:ML: Listening on IPv6 multicast receive endpoint
        WEAVE:EM: Cannot listen for BLE connections, null BleLayer
        Weave Node Configuration:
          Fabric Id: 1
          Subnet Number: 1
          Node Id: 1
          Listening Addresses:
              fd00:0:1:1::1 (ipv6)
        Listening for Heartbeats...
        Weave Node ready to service events; PID: 170883; PPID: 170418

1.  Open a second terminal window and start the Heartbeat client on the
    `fd00:0:1:1::2` IPv6 address with a `node-id` of 2 and the first node's IPv6
    address as the destination for the Heartbeat:

        $ ./weave-heartbeat --node-addr fd00:0:1:1::2 --node-id 2 --dest-addr fd00:0:1:1::1 1
        WEAVE:ML: Binding IPv6 TCP listen endpoint to [fd00:0:1:1::2]:11095
        WEAVE:ML: Listening on IPv6 TCP endpoint
        WEAVE:ML: Binding general purpose IPv4 UDP endpoint to [::]:11095
        WEAVE:IN: IPV6_PKTINFO: 92
        WEAVE:ML: Binding general purpose IPv6 UDP endpoint to [fd00:0:1:1::2]:11095 (lo)
        WEAVE:ML: Listening on general purpose IPv6 UDP endpoint
        WEAVE:ML: Binding IPv6 multicast receive endpoint to [ff02::1]:11095 (lo)
        WEAVE:ML: Listening on IPv6 multicast receive endpoint
        WEAVE:EM: Cannot listen for BLE connections, null BleLayer
        WEAVE:EM: Binding[0] (1): Allocated
        Weave Node Configuration:
          Fabric Id: 1
          Subnet Number: 1
          Node Id: 2
          Listening Addresses:
              fd00:0:1:1::2 (ipv6)
        Sending Heartbeats via UDP to node 1 (fd00:0:1:1::1) every 1000 ms
        Weave Node ready to service events; PID: 170932; PPID: 170608
        WEAVE:EM: Binding[0] (2): Configuring
        WEAVE:EM: Binding[0] (2): Preparing
        WEAVE:EM: Binding[0] (2): Ready, peer 1 ([fd00:0:1:1::1]:11095) via UDP

1.  After a successful connection, node 2 sends Heartbeats to node 1, and node 1
    logs Heartbeats received from node 2:

        Node 1
        <code></code>
        WEAVE:EM: Msg rcvd 00000013:1 1 0000000000000002 0000 C993 0 MsgId:1380A259
        WEAVE:EM: ec id: 1, AppState: 0xb8e89790
        Heartbeat from node 2 (fd00:0:1:1::2): state=1, err=No Error
        ### Node 2
        <code></code>
        WEAVE:EM: ec id: 1, AppState: 0x0
        WEAVE:EM: Msg sent 00000013:1 1 0000000000000001 0000 C993 0 MsgId:1380A259
        Heartbeat sent to node 1: state=1

### Heartbeat with a mock device

`weave-heartbeat` instantiates a `mock-device` for both server and client. The
same Heartbeat functionality can be demonstrated by using `mock-device` in place
of the first `weave-heartbeat` Heartbeat server:

```
$ ./mock-device -a fd00:0:1:1::1
```

## weave-ping

Use `weave-ping` to send and receive
<a href="https://openweave.io/guides/weave-primer/profiles#echo">Echo <!-- echo --></a> profile messages
between two Weave nodes. An Echo payload consists of arbitrary data supplied by
the requesting node and is expected to be echoed back verbatim in the response.
Echo provides a means to test network connectivity and latency.

A successful Echo requires one node to act as a server (listening for and
responding to the Echo request) and one node to act as a client (sending the
Echo request).

Test the `weave-ping` tool using the loopback interface to mimic two nodes:

1.  Add the IPv6 addresses to be used for each Echo node to the `lo` (loopback)
    interface:

        $ sudo ifconfig lo add fd00:0:1:1::1/64
        $ sudo ifconfig lo add fd00:0:1:1::2/64

1.  Start the Echo server on the `fd00:0:1:1::1` address, assigning it a
    `node-id` of 1:

        $ ./weave-ping --node-addr fd00:0:1:1::1 --node-id 1 --listen
        WEAVE:ML: Binding IPv6 TCP listen endpoint to [fd00:0:1:1::1]:11095
        WEAVE:ML: Listening on IPv6 TCP endpoint
        WEAVE:ML: Binding general purpose IPv4 UDP endpoint to [::]:11095
        WEAVE:IN: IPV6_PKTINFO: 92
        WEAVE:ML: Binding general purpose IPv6 UDP endpoint to [fd00:0:1:1::1]:11095 (lo)
        WEAVE:ML: Listening on general purpose IPv6 UDP endpoint
        WEAVE:ML: Binding IPv6 multicast receive endpoint to [ff02::1]:11095 (lo)
        WEAVE:ML: Listening on IPv6 multicast receive endpoint
        WEAVE:EM: Cannot listen for BLE connections, null BleLayer
        WEAVE:SD: init()
        Weave Node Configuration:
          Fabric Id: 1
          Subnet Number: 1
          Node Id: 1
          Listening Addresses:
              fd00:0:1:1::1 (ipv6)
        Listening for Echo requests...
        Iteration 0
        Weave Node ready to service events; PID: 120927; PPID: 113768
        WEAVE:ECH: Listening...

1.  Open a second terminal window and start the Echo client on the
    `fd00:0:1:1::2` IPv6 address with a `node-id` of 2 and the first node's IPv6
    address as the destination for the Echo request:

        $ ./weave-ping --node-addr fd00:0:1:1::2 --node-id 2 --dest-addr fd00:0:1:1::1 1
        WEAVE:ML: Binding general purpose IPv4 UDP endpoint to [::]:11095
        WEAVE:IN: IPV6_PKTINFO: 92
        WEAVE:ML: Binding general purpose IPv6 UDP endpoint to [fd00:0:1:1::2]:11095 (lo)
        WEAVE:ML: Listening on general purpose IPv6 UDP endpoint
        WEAVE:ML: Binding IPv6 multicast receive endpoint to [ff02::1]:11095 (lo)
        WEAVE:ML: Listening on IPv6 multicast receive endpoint
        WEAVE:EM: Cannot listen for BLE connections, null BleLayer
        WEAVE:SD: init()
        Weave Node Configuration:
          Fabric Id: 1
          Subnet Number: 1
          Node Id: 2
          Listening Addresses:
              fd00:0:1:1::2 (ipv6)
        Sending Echo requests to node 1 at fd00:0:1:1::1
        Iteration 0
        Weave Node ready to service events; PID: 121125; PPID: 121017
        WEAVE:ML: Con start 9A00 0000000000000001 0001
        WEAVE:ML: Con DNS complete 9A00 0
        WEAVE:ML: TCP con start 9A00 fd00:0:1:1::1 11095
        WEAVE:ML: TCP con complete 9A00 0
        WEAVE:ML: Con complete 9A00
        Connection established to node 1 (fd00:0:1:1::1)

1.  After a successful connection, node 1 logs Echo Requests from node 2, and
    node 2 logs Echo Responses from node 1:

        Node 1
        <code></code>
        WEAVE:ML: Con rcvd AA00 fd00:0:1:1::2 41675
        Connection received from node 2 (fd00:0:1:1::2)
        WEAVE:EM: Msg rcvd 00000001:1 15 0000000000000002 AA00 B8A5 0 MsgId:00000000
        WEAVE:EM: ec id: 1, AppState: 0xfce0ca80
        Echo Request from node 2 (fd00:0:1:1::2): len=15 ... sending response.
        WEAVE:EM: Msg sent 00000001:2 15 0000000000000002 AA00 B8A5 0 MsgId:00000000
        WEAVE:EM: Msg rcvd 00000001:1 15 0000000000000002 AA00 B8A6 0 MsgId:00000001
        ### Node 2
        <code></code>
        WEAVE:EM: ec id: 1, AppState: 0xd239baa0
        WEAVE:EM: Msg sent 00000001:1 15 0000000000000001 9A00 B8A5 0 MsgId:00000000
        WEAVE:EM: Msg rcvd 00000001:2 15 0000000000000001 9A00 B8A5 0 MsgId:00000000
        Echo Response from node 1 (fd00:0:1:1::1): 1/1(100.00%) len=15 time=0.228ms

### Echo with a mock device

`weave-ping` instantiates a `mock-device` for both server and client. The same
Echo functionality can be demonstrated by using `mock-device` in place of the
first `weave-ping` Echo server:

```
$ ./mock-device -a fd00:0:1:1::1
```
