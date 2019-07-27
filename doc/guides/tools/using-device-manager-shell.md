# Using the Weave Device Manager Shell

The Weave Device Manager Shell is a command-line tool that provides a convenient way for developers to interact with and manage
Weave-enabled devices over a local network connection.  The tool presents a shell-like interface with various built-in commands.
The commands can be used to discover a device, establish connectivity to it, and invoke device operations.  The Device Manager Shell
is implemented in Python, and is designed to be used in a desktop environment (generally linux or OSX).  The shell can be used interactively,
or it can be given a list of commands to execute via stdin.

At its core, the Weave Device Manager Shell is built around the WeaveDeviceManager C++ library, which is part of the openweave-core
project.  This same library is also used by the Nest mobile application when it interacts locally with devices.  Because of this, the Weave
Device Manager Shell is capable of performing the same device management activities as the Nest app, including pairing new devices.

The Weave Device Manager Shell can communicate with a device over a variety of network types.  Any form of IP-bearing network can be
used, including Ethernet, WiFi and Thread, provided the host machine has a suitably configured interface.  Additionally, both IPv4 and
IPv6 communication is supported.

Weave-over-BLE (WoBLE) communication can be used on hosts that provide compatible BLE interfaces (currently BlueZ on Linux and
Core Bluetooth on OSX).

Necessarily, the process of connecting to a device will differ depending on the type of network interface used.  However, once a connection
is established, all further interactions with the device are the same regardless of network type.

## Building

The Weave Device Manager Shell is built as part of a “standalone” build of the openweave-core source tree.  A standalone build builds
OpenWeave specifically configured for use on a desktop system.

To build the Weave Device Manager Shell, perform the following steps:

- Install prerequisites

        # On Linux:
        $ sudo apt-get install git make gcc g++ python python-pip python-setuptools libdbus-1-dev libudev-dev libical-dev libreadline-dev libssl-dev
        
        # On OS X:
        $ sudo cpan -i Text::Template

- Clone the openweave-core repository

        $ cd ~
        $ git clone https://github.com/openweave/openweave-core.git

- Build OpenWeave

        $ cd ~/openweave-core
        $ make -f Makefile-Standalone DEBUG=1 stage

Once the build completes, the Weave Device Manager Shell can be run using the following command:

        # On Linux:
        $ ~/openweave-core/output/x86_64-unknown-linux-gnu/bin/weave-device-mgr
        
        # On OS X:
        $ ~/openweave-core/output/x86_64-apple-darwin/bin/weave-device-mgr
        
Alternatively, you can add the output bin directory to your path:

        $ OPENWEAVE_BIN=`echo ${HOME}/openweave-core/output/x86_64-*/bin`
        $ export PATH=${PATH}:${OPENWEAVE_BIN}

## How To's

The following sections show how to perform various common tasks using the Weave Device Manager Shell.

### Connecting using Weave-over-BLE (WoBLE)

This how-to demonstrates the process of connecting to a device using Weave-over-BLE.

To connect to a device using WoBLE, the device must be advertising in connectable mode.  Most Weave devices do this automatically when
they power on in an unpaired state.  However some devices may require a press a button before they begin advertising.

In order to locate the correct device, one must know it's BLE device name. This can often be found in the log output of the
device when it boots.  E.g.:

         <info> weave: [DL] Configuring BLE advertising (interval 500 ms, connectable, device name NEST-0003)

Nest devices typically use a BLE name of ‘N’ or ‘NEST-’ followed by 4 hex digits.  On production devices this information can often be found
on the back of the device.  Failing that, the BLE name of a device can be found by issuing a `ble-scan` command:

        weave-device-mgr > ble-scan
        2019-05-09 12:53:48,206 WeaveBLEMgr  INFO     BLE is ready!
        2019-05-09 12:53:50,441 WeaveBLEMgr  INFO
        2019-05-09 12:53:50,471 WeaveBLEMgr  INFO     adding to scan list:
        2019-05-09 12:53:50,471 WeaveBLEMgr  INFO     
        2019-05-09 12:53:50,471 WeaveBLEMgr  INFO     Name =    NEST-0003
        2019-05-09 12:53:50,471 WeaveBLEMgr  INFO     ID =      ADC13F9D-35FD-40FB-81E6-F8EF461BC030
        2019-05-09 12:53:50,471 WeaveBLEMgr  INFO     RSSI =    -73
        2019-05-09 12:53:50,472 WeaveBLEMgr  INFO     ADV data: {
            kCBAdvDataIsConnectable = 1;
            kCBAdvDataLocalName = "NEST-0003";
            kCBAdvDataServiceUUIDs =     (
                FEAF
            );
        }
        2019-05-10 10:28:45,183 WeaveBLEMgr  INFO     scanning stopped

Once the device name has been determined, a WoBLE connection can be established as follows:

* Scan for the target device and establish a BLE connection

        weave-device-mgr > ble-scan-connect NEST-0003
        2019-03-22 12:43:05,744 WeaveBLEMgr  INFO     BLE is ready!
        2019-03-22 12:43:06,101 WeaveBLEMgr  INFO     adding to scan list:
        2019-03-22 12:43:06,101 WeaveBLEMgr  INFO     
        2019-03-22 12:43:06,101 WeaveBLEMgr  INFO     Name =    NEST-0003                                                                       
        2019-03-22 12:43:06,101 WeaveBLEMgr  INFO     ID =      63B7C6D9-DF17-4E0A-89B6-58FDD58DE718                                            
        2019-03-22 12:43:06,101 WeaveBLEMgr  INFO     RSSI =    -59                                                                             
        2019-03-22 12:43:06,101 WeaveBLEMgr  INFO     ADV data: {
            kCBAdvDataIsConnectable = 1;
            kCBAdvDataLocalName = "NEST-0003";
            kCBAdvDataServiceUUIDs =     (
                FEAF
            );
        }
        2019-03-22 12:43:06,101 WeaveBLEMgr  INFO     
        2019-03-22 12:43:06,102 WeaveBLEMgr  INFO     scanning stopped
        2019-03-22 12:43:06,102 WeaveBLEMgr  INFO     trying to connect to NEST-0003
        2019-03-22 12:43:07,204 WeaveBLEMgr  INFO     Discovering services
        2019-03-22 12:43:07,208 WeaveBLEMgr  INFO     Discovering services
        2019-03-22 12:43:07,363 WeaveBLEMgr  INFO     connect success
        weave-device-mgr > 

* Establish a Weave-over-BLE connection with the device: 

        weave-device-mgr > connect --ble
        WEAVE:BLE: subscribe complete, ep = 0x1080c5d30
        WEAVE:BLE: peripheral chose BTP version 3; central expected between 2 and 3
        WEAVE:BLE: using BTP fragment sizes rx 20 / tx 101.
        WEAVE:BLE: local and remote recv window size = 3
        WEAVE:ML: WoBle con complete BBD0
        WEAVE:ML: Con complete BBD0
        WEAVE:DM: Connected to device
        WEAVE:DM: Initializing Weave BLE connection
        WEAVE:EM: ec id: 1, AppState: 0x6b018800
        WEAVE:DM: Sending IdentifyRequest to device
        WEAVE:EM: Msg sent 0000000E:1 16 FFFFFFFFFFFFFFFF BBD0 ACD9 0 MsgId:00000000
        WEAVE:EM: Msg rcvd 0000000E:2 29 18B4300000000003 BBD0 ACD9 0 MsgId:00000000
        WEAVE:DM: Received identify response from device 18B4300000000003 (ble con BBD0)
        Connected to device.
        
        weave-device-mgr (18B4300000000003 @ ::) >

Once the WoBLE connection has been established, the Device Manager Shell prompt will change to show the Weave
node id of the connected device.   At this point the connection can be used to issue commands to the device, as shown
below.

NOTE: The above example shows connecting to a device without authenticating.  While this is allowed, production devices
will restrict what commands can be sent over such a connection.  See the sections on connecting using a pairing code or
an access token for how to establish an authenticated connection. 

### Connecting using a device WiFi Soft-AP network

This how-to demonstrates the process of connecting to a device that presents a WiFi Soft-AP network.

Some devices will present WiFi Soft-AP network when they first boot in an unpaired state.  To talk to such a device, the
computer running the Weave Device Manager Shell must be manually connected to the soft-AP network prior to connecting
the Device Manager Shell.

Most devices present an open WiFi network that requires no password to connect.  Other devices will use a short fixed WiFi
password that is printed on the device.  Either way, the security of the interaction is ensured by the Weave protocol itself,
not by the security of the WiFi network.

The name of the device's Soft-AP network is unique to each device.   Nest devices will typically use a name that begins with
'NEST-' followed by 4 hex digits.  On some devices the network name will be printed on the device.  For development devices,
the name of the Soft-AP network can often be found in the device's log output; e.g:

        I (9126) weave[DL]: Configuring WiFi AP: SSID NEST-2F50, channel 1

Devices in an unpaired state will automatically present a Soft-AP network.  Usually this will time-out after a period of
inactivity.  On some devices, a button press can be used to get the device to re-enable its Soft-AP network.  Other devices
will require a power-cycle.

Devices that have already been paired will generally not present a Soft-AP network on boot.  Often, though, a button press
can be used to get the device to re-enable its Soft-AP network.  This is convenient in cases where the device's WiFi configuration
must be changed; e.g. when the password for the home WiFi network changes.

To connect to a device using its Wifi Soft-AP network:





### Connecting using an existing WiFi or Thread network

This how-to demonstrates the process of connecting to a device over a WiFi or Thread network.

This process is typically used on devices that present a WiFi soft-AP when they first boot in an unpaired state.  To talk
to a device this way, the computer running the Weave Device Manager Shell must be manually connected to the device's soft-AP
network prior to connecting the Device Manager Shell.  Most devices present an open WiFi network that requires no password
to connect.  Other devices use a short fixed WiFi password that is printed on the device.  Either way, the security of the
interaction is ensured by the Weave protocol itself, not by the security of the WiFi network.

This process can also be used to connect to a device over an existing WiFi or Thread network to which the device itself is
already connected.  This scenario arises when 


