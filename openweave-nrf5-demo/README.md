# OpenWeave nRF5* Demo

A simple demo application showing how to build and use OpenWeave for the Nordic nRF52840 using Nordic's nRF5 SDK.

**!!!! WARNING : WORK IN PROGRESS !!!!**

Note that the code here is very much incomplete&mdash;to the extent that, at the moment, it does *essentially nothing*. 

Over time this will evolve into a working demo application, similar to that which is available for the Espressif ESP32.  Once
this happens, the demo application will be moved out of the openweave source tree and into its own repo. 

In the meantime, this directory, and the openweave branch it exists on (`feature/nrf52840-device-layer`), will be the focal point for efforts to port
the OpenWeave Device Layer to the Nordic nRF5 SDK.

#### Building

* Download and install the Nordic nRF5 SDK


* Download and install the Nordic nRF5x Command Line Tools


* Download and install a suitable gcc tool chain


* Set the following environment variables based on the locations/versions of the above packages:

        export NRF5_SDK_ROOT=/opt/nRF5_SDK_15.2.0_9412b96
        export NRF5_TOOLS_ROOT=/opt/nRF5x-Command-Line-Tools
        export GNU_INSTALL_ROOT=/opt/arm/gcc-arm-none-eabi-7-2018-q2-update/bin/
        export GNU_VERSION=7.3.1
        export PATH=${PATH}:${NRF5_TOOLS_ROOT}/nrfjprog


* Cross fingers

 
* Run make
