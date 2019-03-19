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



