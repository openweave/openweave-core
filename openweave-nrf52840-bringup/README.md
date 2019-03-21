# OpenWeave nRF52840 Bring-up Application

This is a simple test application designed to aid in the porting of OpenWeave and the OpenWeave Device Layer to the Nordic nRF52840.

<p style="color:red">!!!! WARNING - UNSTABLE CODE !!!!</p>

The code here is intended as a tool for developers to exercise OpenWeave functionality on the nRF52840.  It is *not* a demo application, and it implements no particularly interesting high-level behavior.
Furthermore, the code is very likely to be unstable, and/or require manual edits to operate correctly in some environments.  Anyone using the code should expect to get their hands dirty making things work.   

At some point, this code will be abandoned and a proper demo application will be built, similar to that which is available for the Espressif ESP32.  Once
this happens, the bring-up application will be removed from the OpenWeave source tree. 

In the meantime, this directory, and the OpenWeave branch it exists on (`feature/nrf52840-device-layer`), will be the focal point for efforts to port
the OpenWeave Device Layer to the nRF52840.

#### Building

* Download and install the [Nordic nRF5 SDK for Thread and Zigbee](https://www.nordicsemi.com/Software-and-Tools/Software/nRF5-SDK-for-Thread-and-Zigbee)
([Direct download link](https://www.nordicsemi.com/-/media/Software-and-other-downloads/SDKs/nRF5-SDK-for-Thread/nRF5-SDK-for-Thread-and-Zigbee/nRF5SDKforThreadandZigbee20029775ac.zip)) 


* Download and install the [Nordic nRF5x Command Line Tools](https://www.nordicsemi.com/Software-and-Tools/Development-Tools/nRF5-Command-Line-Tools)
([Direct download link](https://www.nordicsemi.com/-/media/Software-and-other-downloads/Desktop-software/nRF5-command-line-tools/sw/nRF-Command-Line-Tools_9_8_1_Linux-x86_64.tar)) 


* Download and install a suitable ARM gcc tool chain: [GNU Arm Embedded Toolchain 7-2018-q2-update](https://developer.arm.com/open-source/gnu-toolchain/gnu-rm/downloads/7-2018-q2-update)
([Direct download link](https://armkeil.blob.core.windows.net/developer/Files/downloads/gnu-rm/7-2018q2/gcc-arm-none-eabi-7-2018-q2-update-linux.tar.bz2)) 


* Set the following environment variables based on the locations/versions of the above packages:

        export NRF5_SDK_ROOT=${HOME}/tools/nRF52840/nRF5_SDK_for_Thread_and_Zigbee_2.0.0_29775ac
        export NRF5_TOOLS_ROOT=${HOME}/tools/nRF5x-Command-Line-Tools
        export GNU_INSTALL_ROOT=${HOME}/tools/gcc-arm-none-eabi-7-2018-q2-update/bin/
        export GNU_VERSION=7.3.1
        export PATH=${PATH}:${NRF5_TOOLS_ROOT}/nrfjprog

<p style="margin-left: 40px">You may wish to place these settings in local script file (e.g. setup-env.sh) so that they can be loaded into the environment as needed (e.g. by running 'source ./setup-env.sh').</p> 

* Run make to build the application

        $ cd openweave-nrf52840-bringup
        $ make clean
        $ make

#### Initializing the nRF52840 DK
The bring-up application is designed to run on the [Nordic nRF52840 DK](https://www.nordicsemi.com/Software-and-Tools/Development-Kits/nRF52840-DK) development kit.  Prior to installing
the application, the device's flash memory should be erased and the Nordic SoftDevice image installed.

* Connect the host machine to the J-Link Interface MCU USB connector on the nRF52840 DK.  The Interface MCU connector is the one on the short side of the board.


* Use the Makefile to erase the flash and program the Nordic SoftDevice image.

        $ cd openweave-nrf52840-bringup
        $ make erase
        $ make flash_softdevice

Once the above is complete, it shouldn't need be done again *unless* the SoftDevice image or the configuration storage (fds) area becomes corrupt.  If you are forced to erase the device,
the application will need to be flashed again.

#### Flashing the Application

To flash the bring-up app, run the following commands:

        $ cd openweave-nrf52840-bringup
        $ make flash

#### Viewing Logging Output

The bring-up application is built to use the SEGGER Real Time Transfer (RTT) facility for log output.  RTT is a feature built-in to the J-Link Interface MCU on the development kit board.
It allows bi-directional communication with an embedded application without the need for a dedicated UART.

Using the RTT facility requires downloading and installing the *SEGGER J-Link Software and Documentation Pack* ([web site](https://www.segger.com/downloads/jlink#J-LinkSoftwareAndDocumentationPack)).

* Download the Linux DEB installer by navigating to the following URL and agreeing to the license agreement. 

<p style="margin-left: 40px"><a href="https://www.segger.com/downloads/jlink/JLink_Linux_x86_64.rpm">JLink_Linux_x86_64.rpm</a></p>

* Install the J-Link software

        $ cd ~/Downloads
        $ dpkg -i JLink_Linux_V*_x86_64.rpm

Once the J-Link software is installed, log output can be viewed using the JLinkExe tool in combination with JLinkRTTClient.

* Run the JLinkExe tool with arguments to autoconnect to the nRF82480 DK board:

        $ JLinkExe -device NRF52840_XXAA -if SWD -speed 4000 -autoconnect 1

* In a second terminal, run the JLinkRTTClient:

        $ JLinkRTTClient

Logging output will appear in the second termnal.

An alternative, and often preferable way to view log output is to use the J-Link GDB server described in the following section. 

#### Debugging with GDB

SEGGER provides a GDB server tool which can be used to debug a running application on the nRF82480 DK board.
The GDB server also provides a local telnet port which can be used to communicate with the device over RTT.  This can be used
to view logging output.  (Note that you do not need to actually run gdb to see the logging output from the GDB server).

The openweave-nrf52840-bringup directory contains a pair of shell scripts that make it easy to start  the GDB server and initiate
a debugging session. 

* Start the GDB server

        $ cd openweave-nrf52840-bringup
        $ ./start-jlink-gdb-server.sh

* Initiate a debugging session

        $ cd openweave-nrf52840-bringup
        $ ./start-gdb.sh

The start-gdb.sh will launch gdb and instruct it to connect to the GDB server.  By default, gdb is started with the openweave-nrf52840-bringup
executable located in the build directory.  Alternatively one can pass the name of a different executable as an argument.


#### Setting up a Test Thread Network

The current version of the bring-up app uses hard-coded configuration values for provisioning the Thread network.  Eventually this will be
elimiated once support for the Weave Network Provisioning protocol is in place.  In the mean time, the following instructions can be used to
build  a test Thread network and test Weave communication using the bring-up app.

Creating a minimal test network requires two nRF52840 DK dev boards--one on which to run the bring-up app and a second to act as an NCP
for a host machine that will communicate with the bring-up app.  We will refer to these two devices as the _Weave device_ and the _NCP device_.

* Follow the instructions in the OpenThread code lab for building OpenThread and wpantund, and setting up an NCP device.

  [Cloning and building OpenThread and wpantund](https://codelabs.developers.google.com/codelabs/openthread-hardware/#2)
  
  [Set up the NCP Joiner](https://codelabs.developers.google.com/codelabs/openthread-hardware/#3)


* Connect the NCP device to the host machine and start wpantund

        $ sudo /usr/local/sbin/wpantund -o Config:NCP:SocketPath /dev/ttyACM0  \
        -o Config:TUN:InterfaceName utun7 \
        -o Daemon:SyslogMask " -info"

Note that the host should be connected to the NCP device via its nRF USB connector (the connector on the long edge of the board).


* Using wpanctl, instruct the NCP device to form the test network

        $ sudo /usr/local/bin/wpanctl -I utun7
        wpanctl:utun7> form -p 0x7777 -x 8877665544332211 -c 26 -k B8983AED954064CC4BACB3F6F145D198 -T router WARP


* Connect the Weave device to the host machine and follow the instructions above for initializing the device and flashing the bring-up app.


* Start the J-Link GDB server using the supplied helper script as described above.


* Reset the device by pressing the IP BOOT/RESET button.


* In the GDB server window you should see logging output from the Weave device similar to this:

        <info> app: ==================================================
        <info> app: openweave-nrf52840-bringup starting
        <info> app: ==================================================
        <info> app: Enabling SoftDevice
        <info> app: Waiting for SoftDevice to be enabled
        <info> app: SoftDevice enable complete
        <info> app: Initializing Weave stack
        <info> weave: [DL] FDS set: 0x235C/0x0002 = 12288 (0x3000)
        <info> weave: [DL] BLEManagerImpl::Init() complete
        <info> weave: [ML] Binding IPv6 TCP listen endpoint to [::]:11095
        <info> weave: [ML] Listening on IPv6 TCP endpoint
        <info> weave: [ML] Binding general purpose IPv6 UDP endpoint to [::]:11095 ()
        <info> weave: [ML] Listening on general purpose IPv6 UDP endpoint
        <info> weave: [DL] Pairing code not found; using default: NESTUS
        <info> weave: [DL] Device Configuration:
        <info> weave: [DL]   Device Id: 18B4300000000003
        <info> weave: [DL]   Serial Number: (not set)
        <info> weave: [DL]   Vendor Id: 9050 (0x235A) (Nest)
        <info> weave: [DL]   Product Id: 65279 (0xFEFF)
        <info> weave: [DL]   Fabric Id: (none)
        <info> weave: [DL]   Pairing Code: NESTUS


* Search the log output for a message showing the addresses that have been assigned to the Thread interface:

        <debug> weave: [DL] Updating Thread interface addresses
        <debug> weave: [DL]    FE80::6491:B6D2:193A:5251 (0, preferred)
        <debug> weave: [DL]    FD88:7766:5544::FF:FE00:F400 (1)
        <debug> weave: [DL]    FD88:7766:5544:0:9F2E:8721:E8A1:D7F3 (2, preferred)


* Copy the address that begins with FD88:7766:5544:0... and is marked "preferred".  This is the device's **mesh-local address**.


* On the host machine, use ping6 to verify connectivity to the device.

        $ ping6 FD88:7766:5544:0:9F2E:8721:E8A1:D7F3
        PING FD88:7766:5544:0:9F2E:8721:E8A1:D7F3(fd88:7766:5544:0:9f2e:8721:e8a1:d7f3) 56 data bytes
        64 bytes from fd88:7766:5544:0:9f2e:8721:e8a1:d7f3: icmp_seq=1 ttl=255 time=49.5 ms
        64 bytes from fd88:7766:5544:0:9f2e:8721:e8a1:d7f3: icmp_seq=2 ttl=255 time=18.4 ms
        64 bytes from fd88:7766:5544:0:9f2e:8721:e8a1:d7f3: icmp_seq=3 ttl=255 time=17.0 ms
        64 bytes from fd88:7766:5544:0:9f2e:8721:e8a1:d7f3: icmp_seq=4 ttl=255 time=18.4 ms

        
#### Testing Weave Communication

The following examples show how to use the stand-alone version of the weave-ping tool (built on linux or Mac) to exchange
Weave Echo Profile messages with a test device.

The examples show communicating with the device using its Thread mesh-local address.  As an option, one can also communicate
with the device using its IPv6 link-local address.  To do so, change the argument to the --dest-addr option to the device's link-local 
address followed by a percent sign (%) and the name of the wpan interface on the host machine.  E.g.:

        --dest-addr FE80::6491:B6D2:193A:5251%wpan7


##### Testing Unencrypted Weave Echo Messages over UDP

This shows sending unencrypted Weave Echo messages to a device using UDP and Weave Reliable Messaging (WRM).

        $ weave-ping --node-id 18b4300000000001 --udp --dest-addr FD88:7766:5544:0:9F2E:8721:E8A1:D7F3 18B4300000000003
        WEAVE:ML: Binding IPv4 TCP listen endpoint to [::]:11095
        WEAVE:ML: Listening on IPv4 TCP endpoint
        WEAVE:ML: Binding IPv6 TCP listen endpoint to [::]:11095
        WEAVE:ML: Listening on IPv6 TCP endpoint
        WEAVE:ML: Binding general purpose IPv4 UDP endpoint to [::]:11095
        WEAVE:IN: IPV6_PKTINFO: 92
        WEAVE:ML: Listening on general purpose IPv4 UDP endpoint
        WEAVE:ML: Binding general purpose IPv6 UDP endpoint to [::]:11095 ()
        WEAVE:ML: Listening on general purpose IPv6 UDP endpoint
        WEAVE:ML: Adding ens33 to interface table
        WEAVE:ML: Adding wpan7 to interface table
        WEAVE:EM: Cannot listen for BLE connections, null BleLayer
        WEAVE:SD: init()
        Weave Node Configuration:
          Fabric Id: 1
          Subnet Number: 1
          Node Id: 18B4300000000001
          Listening Addresses: any
        Sending Echo requests to node 18B4300000000003 at FD88:7766:5544:0:9F2E:8721:E8A1:D7F3
        Iteration 0
        Weave Node ready to service events; PID: 57910; PPID: 4808
        WEAVE:EM: ec id: 1, AppState: 0x4d008aa0
        WEAVE:EM: Msg sent 00000001:1 15 18B4300000000003 0000 8E18 0 MsgId:4DCC103B
        WEAVE:EM: Msg rcvd 00000001:2 15 18B4300000000003 0000 8E18 0 MsgId:207817AA
        Echo Response from node 18B4300000000003 (fd88:7766:5544:0:9f2e:8721:e8a1:d7f3): 1/1(100.00%) len=15 time=30.936ms
        WEAVE:EM: ec id: 1, AppState: 0x4d008aa0
        WEAVE:EM: Msg sent 00000001:1 15 18B4300000000003 0000 8E19 0 MsgId:4DCC103C
        WEAVE:EM: Msg rcvd 00000001:2 15 18B4300000000003 0000 8E19 0 MsgId:207817AB
        Echo Response from node 18B4300000000003 (fd88:7766:5544:0:9f2e:8721:e8a1:d7f3): 2/2(100.00%) len=15 time=19.612ms
        WEAVE:EM: ec id: 1, AppState: 0x4d008aa0
        WEAVE:EM: Msg sent 00000001:1 15 18B4300000000003 0000 8E1A 0 MsgId:4DCC103D
        WEAVE:EM: Msg rcvd 00000001:2 15 18B4300000000003 0000 8E1A 0 MsgId:207817AC
        Echo Response from node 18B4300000000003 (fd88:7766:5544:0:9f2e:8721:e8a1:d7f3): 3/3(100.00%) len=15 time=18.866ms


##### Testing Encrypted Weave Echo Messages over UDP using a CASE security session

This shows sending encrypted Weave Echo messages to a device using UDP and Weave Reliable Messaging (WRM), where the 
encryption keys have been established using the Weave Certificate Authenticated Session Establishment (CASE) protocol.

        $ weave-ping --node-id 18b4300000000001 --udp --dest-addr FD88:7766:5544:0:9F2E:8721:E8A1:D7F3 --case 18B4300000000003
        WEAVE:ML: Binding IPv4 TCP listen endpoint to [::]:11095
        WEAVE:ML: Listening on IPv4 TCP endpoint
        WEAVE:ML: Binding IPv6 TCP listen endpoint to [::]:11095
        WEAVE:ML: Listening on IPv6 TCP endpoint
        WEAVE:ML: Binding general purpose IPv4 UDP endpoint to [::]:11095
        WEAVE:IN: IPV6_PKTINFO: 92
        WEAVE:ML: Listening on general purpose IPv4 UDP endpoint
        WEAVE:ML: Binding general purpose IPv6 UDP endpoint to [::]:11095 ()
        WEAVE:ML: Listening on general purpose IPv6 UDP endpoint
        WEAVE:ML: Adding ens33 to interface table
        WEAVE:ML: Adding wpan7 to interface table
        WEAVE:EM: Cannot listen for BLE connections, null BleLayer
        WEAVE:SD: init()
        Weave Node Configuration:
          Fabric Id: 1
          Subnet Number: 1
          Node Id: 18B4300000000001
          Listening Addresses: any
        Sending Echo requests to node 18B4300000000003 at FD88:7766:5544:0:9F2E:8721:E8A1:D7F3
        Iteration 0
        Weave Node ready to service events; PID: 57931; PPID: 4808
        WEAVE:EM: ec id: 1, AppState: 0x52d28c0
        WEAVE:SM: CASE:GenerateBeginSessionRequest
        WEAVE:SM: CASE:AppendNewECDHKey
        WEAVE:SM: CASE:AppendCertInfo
        WEAVE:SM: CASE:AppendPayload
        WEAVE:SM: CASE:AppendSignature
        WEAVE:SM: CASE:GenerateSignature
        WEAVE:EM: Msg sent 00000004:10 715 18B4300000000003 0000 B2E8 0 MsgId:A363FD8F
        WEAVE:SM: StartSessionTimer
        WEAVE:EM: Msg rcvd 00000000:2 0 18B4300000000003 0000 B2E8 0 MsgId:207817B3
        WEAVE:EM: Rxd Ack; Removing MsgId:A363FD8F from Retrans Table
        WEAVE:SM: WRMPHandleAckRcvd
        WEAVE:EM: Removed Weave MsgId:A363FD8F from RetransTable
        WEAVE:EM: Msg rcvd 00000004:11 430 18B4300000000003 0000 B2E8 0 MsgId:207817B4
        WEAVE:EM: Weave MsgId:A363FD8F not in RetransTable
        WEAVE:EM: Piggybacking Ack for MsgId:207817B4 with msg
        WEAVE:EM: Msg sent 00000000:2 0 18B4300000000003 0000 B2E8 0 MsgId:A363FD90
        WEAVE:EM: Flushed pending ack for MsgId:207817B4 to Peer 18B4300000000003
        WEAVE:SM: CASE:ProcessBeginSessionResponse
        WEAVE:SM: CASE:VerifySignature
        WEAVE:SM: CASE:DecodeCertificateInfo
        WEAVE:SM: CASE:ValidateCert
        WEAVE:SM: CASE:DecodeWeaveECDSASignature
        WEAVE:SM: CASE:VerifyECDSASignature
        WEAVE:SM: CASE:DeriveSessionKeys
        WEAVE:SM: CASE:GenerateInitiatorKeyConfirm
        WEAVE:EM: Piggybacking Ack for MsgId:207817B4 with msg
        WEAVE:EM: Msg sent 00000004:12 32 18B4300000000003 0000 B2E8 0 MsgId:A363FD91
        WEAVE:ML: Message Encryption Key: Id=2A6F Type=SessionKey Peer=18B4300000000003 EncType=01 Key=8F4845838678FA1E346F84AB6B3463AD,409B7FA15507DAB756BD66BD441CAD1507133E2E
        WEAVE:EM: Msg rcvd 00000000:2 0 18B4300000000003 0000 B2E8 0 MsgId:207817B5
        WEAVE:EM: Rxd Ack; Removing MsgId:A363FD91 from Retrans Table
        WEAVE:SM: WRMPHandleAckRcvd
        WEAVE:SM: CancelSessionTimer
        Secure session established with node 18B4300000000003 (fd88:7766:5544:0:9f2e:8721:e8a1:d7f3)
        WEAVE:EM: Removed Weave MsgId:A363FD91 from RetransTable
        WEAVE:EM: ec id: 1, AppState: 0x52beaa0
        WEAVE:EM: Msg sent 00000001:1 15 18B4300000000003 0000 B2E9 0 MsgId:00000000
        WEAVE:EM: Msg rcvd 00000001:2 15 18B4300000000003 0000 B2E9 0 MsgId:00000000
        Echo Response from node 18B4300000000003 (fd88:7766:5544:0:9f2e:8721:e8a1:d7f3): 1/1(100.00%) len=15 time=24.205ms
        WEAVE:EM: ec id: 1, AppState: 0x52beaa0
        WEAVE:EM: Msg sent 00000001:1 15 18B4300000000003 0000 B2EA 0 MsgId:00000001
        WEAVE:EM: Msg rcvd 00000001:2 15 18B4300000000003 0000 B2EA 0 MsgId:00000001
        Echo Response from node 18B4300000000003 (fd88:7766:5544:0:9f2e:8721:e8a1:d7f3): 2/2(100.00%) len=15 time=34.185ms
        WEAVE:EM: ec id: 1, AppState: 0x52beaa0
        WEAVE:EM: Msg sent 00000001:1 15 18B4300000000003 0000 B2EB 0 MsgId:00000002
        WEAVE:EM: Msg rcvd 00000001:2 15 18B4300000000003 0000 B2EB 0 MsgId:00000002
        Echo Response from node 18B4300000000003 (fd88:7766:5544:0:9f2e:8721:e8a1:d7f3): 3/3(100.00%) len=15 time=23.653ms
        WEAVE:EM: ec id: 1, AppState: 0x52beaa0
        WEAVE:EM: Msg sent 00000001:1 15 18B4300000000003 0000 B2EC 0 MsgId:00000003
        WEAVE:EM: Msg rcvd 00000001:2 15 18B4300000000003 0000 B2EC 0 MsgId:00000003
        Echo Response from node 18B4300000000003 (fd88:7766:5544:0:9f2e:8721:e8a1:d7f3): 4/4(100.00%) len=15 time=24.373ms

 
##### Testing Encrypted Weave Echo Messages over TCP using a PASE security session

This shows sending encrypted Weave Echo messages to a device using TCP, where the encryption keys have been established using
the Weave Password Authenticated Session Establishment (PASE) protocol.

        $ weave-ping --node-id 18b4300000000001 --tcp --dest-addr FD88:7766:5544:0:9F2E:8721:E8A1:D7F3 --pase --pairing-code NESTUS 18B4300000000003
        WEAVE:ML: Binding general purpose IPv4 UDP endpoint to [::]:11095
        WEAVE:IN: IPV6_PKTINFO: 92
        WEAVE:ML: Listening on general purpose IPv4 UDP endpoint
        WEAVE:ML: Binding general purpose IPv6 UDP endpoint to [::]:11095 ()
        WEAVE:ML: Listening on general purpose IPv6 UDP endpoint
        WEAVE:ML: Adding ens33 to interface table
        WEAVE:ML: Adding wpan7 to interface table
        WEAVE:EM: Cannot listen for BLE connections, null BleLayer
        WEAVE:SD: init()
        Weave Node Configuration:
          Fabric Id: 1
          Subnet Number: 1
          Node Id: 18B4300000000001
          Listening Addresses: any
        Sending Echo requests to node 18B4300000000003 at FD88:7766:5544:0:9F2E:8721:E8A1:D7F3
        Iteration 0
        Weave Node ready to service events; PID: 57936; PPID: 4808
        WEAVE:ML: Con start 58A0 18B4300000000003 1001
        WEAVE:ML: Con DNS complete 58A0 0
        WEAVE:ML: TCP con start 58A0 fd88:7766:5544:0:9f2e:8721:e8a1:d7f3 11095
        WEAVE:ML: TCP con complete 58A0 0
        WEAVE:EM: ec id: 1, AppState: 0x844aa8c0
        WEAVE:EM: Msg sent 00000004:1 296 18B4300000000003 58A0 71B3 0 MsgId:00000000
        WEAVE:SM: StartSessionTimer
        WEAVE:EM: Msg rcvd 00000004:2 284 18B4300000000003 58A0 71B3 0 MsgId:00000000
        WEAVE:EM: Msg rcvd 00000004:3 144 18B4300000000003 58A0 71B3 0 MsgId:00000001
        WEAVE:EM: Msg sent 00000004:4 176 18B4300000000003 58A0 71B3 0 MsgId:00000001
        WEAVE:EM: Msg rcvd 00000004:5 32 18B4300000000003 58A0 71B3 0 MsgId:00000002
        WEAVE:ML: Message Encryption Key: Id=27A2 Type=SessionKey Peer=18B4300000000003 EncType=01 Key=907A6B3B62DE0A2D273EA43AD10B1A12,1D3DCB3F1C85A8D9A5C05089C98A052057B73CBE
        WEAVE:SM: CancelSessionTimer
        WEAVE:ML: Con complete 58A0
        Connection established to node 18B4300000000003 (fd88:7766:5544:0:9f2e:8721:e8a1:d7f3)
        WEAVE:EM: ec id: 1, AppState: 0x84496aa0
        WEAVE:EM: Msg sent 00000001:1 15 18B4300000000003 58A0 71B4 0 MsgId:00000000
        WEAVE:EM: Msg rcvd 00000001:2 15 18B4300000000003 58A0 71B4 0 MsgId:00000000
        Echo Response from node 18B4300000000003 (fd88:7766:5544:0:9f2e:8721:e8a1:d7f3): 1/1(100.00%) len=15 time=31.973ms
        WEAVE:EM: ec id: 1, AppState: 0x84496aa0
        WEAVE:EM: Msg sent 00000001:1 15 18B4300000000003 58A0 71B5 0 MsgId:00000001
        WEAVE:EM: Msg rcvd 00000001:2 15 18B4300000000003 58A0 71B5 0 MsgId:00000001
        Echo Response from node 18B4300000000003 (fd88:7766:5544:0:9f2e:8721:e8a1:d7f3): 2/2(100.00%) len=15 time=32.458ms
        WEAVE:EM: ec id: 1, AppState: 0x84496aa0
        WEAVE:EM: Msg sent 00000001:1 15 18B4300000000003 58A0 71B6 0 MsgId:00000002
        WEAVE:EM: Msg rcvd 00000001:2 15 18B4300000000003 58A0 71B6 0 MsgId:00000002
        Echo Response from node 18B4300000000003 (fd88:7766:5544:0:9f2e:8721:e8a1:d7f3): 3/3(100.00%) len=15 time=32.348ms
        WEAVE:EM: ec id: 1, AppState: 0x84496aa0
        WEAVE:EM: Msg sent 00000001:1 15 18B4300000000003 58A0 71B7 0 MsgId:00000003
        WEAVE:EM: Msg rcvd 00000001:2 15 18B4300000000003 58A0 71B7 0 MsgId:00000003
        Echo Response from node 18B4300000000003 (fd88:7766:5544:0:9f2e:8721:e8a1:d7f3): 4/4(100.00%) len=15 time=32.363ms
