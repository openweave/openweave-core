# Building OpenWeave for the ESP32

The OpenWeave source tree includes a set of build and configuration files that make it easy to incorporate OpenWeave as a library in an ESP32 application.  These files are structured as an ESP-IDF compatible “component”, making possible to drop OpenWeave into a new or existing or project and immediately begin coding.

tl;dr? Jump to [How to incorporate OpenWeave into an ESP32 project](#how-to-incorporate-openweave-into-an-esp32-project)

Or take a look at the [OpenWeave ESP32 demo project](https://github.com/openweave/openweave-esp32-lwip).

## How OpenWeave gets built for the ESP32

OpenWeave provides its own build system based on GNU autotools.  The OpenWeave build system is quite flexible, and can be configured to build OpenWeave for a large range of target environments.

For the ESP32, OpenWeave provides a wrapper makefile (build/esp32/components/openweave/component.mk) that allows OpenWeave to be incorporated as an external component in an ESP32 application.  The OpenWeave component.mk file works by calling out to the native OpenWeave build system as part of the standard ESP-IDF build process.  The wrapper takes care of all the necessary steps to build OpenWeave, including invoking the OpenWeave configure script and passing the correct cross-compilation arguments.  Dependencies are passed through, meaning that OpenWeave sources will only  be rebuilt when header / configuration file change in the base project (or if you make changes to OpenWeave itself).

The end result of the build is a set of libraries (.a files) containing the OpenWeave code compiled for ESP32. These libraries are automatically supplied to the linker when linking the target ESP32 application.

## Dependence on openweave-esp32-lwip

OpenWeave relies on an enhanced version of the open source Lightweight IP (LwIP) stack.  These enhancements provide support some of the unique features of OpenWeave, including the Weave IPv6 addressing architecture and Weave IP tunnels.

To make life easier for ESP32 developers, the OpenWeave team has published a version of the LwIP library that is pre-patched with the necessary OpenWeave enhancements.  This version incorporates all of Espressif’s own LwIP enhancements as well as the OpenWeave patches.  The openweave-esp32-lwip component can be incorporated into an ESP32 project by placing it in the project components directory.  

The openweave-esp32-lwip repo is available [here](https://github.com/openweave/openweave-esp32-lwip).

## Configuring OpenWeave

OpenWeave supports a large collection of configurable options which allow its functionality to be precisely tuned for a given application and target environment.  To make these options easier to use on the ESP32, OpenWeave provides an ESP-IDF compatible configuration database (build/esp32/components/openweave/Kconfig) that makes many of the options controllable via the standard ESP-IDF make menuconfig tool.

Two component configuration sections are defined:

  * __Component Config > OpenWeave Core__ -- Provides options that control or tune core Weave functionality
  * __Component Config > OpenWeave Device Layer__ -- Provides options specific to the ESP32 adaptation of OpenWeave

Help text is provided for each of the options explaining their use.

## How to incorporate OpenWeave into an ESP32 project

The easiest way to incorporate OpenWeave into an ESP32 project is to add the OpenWeave source repo as a git submodule within your ESP32 application repo.  As described above, OpenWeave depends on an enhanced version of the Espressif LwIP component.  This can also be incorporated as a submodule.

* At the top-level of your project directory, add the openweave-core repository as a submodule in the components directory.

        % git submodule add https://github.com/openweave/openweave-core.git components/openweave-core


* Add the openweave-esp32-lwip repository as a submodule.  Note that the local component directory must be named "lwip" so that is supersedes the default version of LwIP suppled by ESP-IDF.  

        % git submodule add https://github.com/openweave/openweave-esp32-lwip.git components/lwip


* In the top level project Makefile, add the following line:

        EXTRA_COMPONENT_DIRS += $(PROJECT_PATH)/components/openweave-core/build/esp32/components


* Enable / Disable Bluetooth support

   By default, OpenWeave builds with support for the Weave-over-BLE (WoBLE) protocol.  This requires Bluetooth support to be enabled on the ESP32.  To enable Bluetooth support for your project, do the following:

  * Run make menuconfig
  * Select (enable) the option __Component Config > Bluetooth > Bluetooth__

   Alternatively, one can disable WoBLE support in OpenWeave, which avoids the need to enable Bluetooth in ESP-IDF.

  * Run make menuconfig
  * Deselect (disable) the option __Component Config >  OpenWeave Device Layer > BLE Options > Enable Weave-over-BLE (WoBLE) Support__

