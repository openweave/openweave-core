# OpenWeave Factory Provisioning Tool

The OpenWeave Factory Provisioning Tool provides a convenient means to provisioning persistent, per-device configuration information
onto Weave-enabled devices.  The Factory Provisioning Tool is intended to be used as part of a manufacturing line process that stamps
individual devices with unique identity and credential information prior to shipment to customers.  It can also be used by developers to
personalize pre-production hardware used during the development process.

## Theory of operation

The goal of the OpenWeave Factory Provisioning Tool is to inject select configuration information into a device's persistent configuration store, where
it can be picked up and used at runtime by the device firmware.  Persistent device configuration is typically stored within the device's internal
flash, although the details of exactly how the data is stored, and in what format, vary from platform to platform.  The Factory Provisioning Tool
itself is independent of the final storage format, and relies on code in the device's firmware to write the data in the correct way.  The tool's 
user interface is also largely independent of the type of hardware being configured, meaning that similar manufacturing processes can be employed 
across product lines that are based on different hardware platforms.

The Factory Provisioning Tool is designed work run on a host machine that is connected to a target device via some form of debug or control interface - for
example, a JTAG or SWD port.  The tool works by injecting provisioning information into the device’s RAM in a specially encoded form.  The device is then
instructed to restart, at which point code built-in to the device's firmware locates the encoded data, validates its integrity, and writes the contained 
values into persistent storage in a format appropriate for the platform.

The on-device code that detects and processes the injected provisioning data is built-in to the OpenWeave Device Layer, and can be enabled on any
supported platform.   Once enabled, the code runs automatically every time the device boots, at a point early in the device initialization process.
The code operates by scanning a platform-specific region of RAM.  On platforms with modest mounts of memory (for example, <1M), the scan encompasses all of
available RAM.

When placed in RAM, the provisioning data is encoded with an easily identifiable prefix, which allows it to be quickly found during the scan process. 
An integrity check value based on a cryptographic hash is used to confirm the validity of the data before processing.

By default, the provisioning tool selects the RAM location at which to inject the provisioning data based on the target device platform.  This choice
can be overridden via an argument to the tool.  In general, it is not required that the device firmware reserve a RAM location specifically for receiving
provisioning data.  More typically, provisioning data is written to a RAM location that is allocated for other purposes, but is generally unused early in
the device boot process.   Common choices are the top of the initial system stack, or the far end of a heap arena.

The Factory Provisioning Tool relies on a set of external development tools to interface to the target device.  The particular tools employed depend
on the type of the target device.  Two device interfaces are currently supported:

- a SEGGER J-Link debug probe connected to a device's JTAG or SWD port
- a USB serial port connected to an Espressif ESP32

The J-Link interface relies on the SEGGER J-Link Commander tool (JLinkExe), which must be separately installed on the host machine.

The ESP32 interface relies on the Espressif esptool.py command, which is provided as part of Espressif's ESP-IDF SDK.

## Types of information that can be provisioned

The OpenWeave Factory Provisioning Tool is capable of provisioning the following types of information:

- Device serial number
- Manufacturer-assigned Weave device id
- Manufacturer-assigned Weave certificate and private key
- Weave pairing code
- Product revision number
- Manufacturing date

Although generally a device will need all of the above information to operate properly, it is not required to provision all the information at the same
time.  Thus, provisioning of different types of information can happen at distinct points in the manufacturing process.  Additionally, it is possible to
replace previously provisioned values with new values in a subsequent provisioning step.

## Sources of provisioning information

Device provisioning information can be supplied to the Factory Provisioning Tool in the follow ways:

- A command line arguments
- Using a provisioning CSV file
- By fetching values from a Nest provisioning server

### Command line

In the simplest form, device provisioning information is specified directly on the command line to the OpenWeave Factory Provisioning Tool.
For example:

```
$ ./weave-factory-prov-tool --target nrf52840 --device-id 18B4300000000001 \
    --pairing-code NESTUS --mfg-date 2019/04/01
```

Binary data values, such as the Weave certificate and private key, can be specified either as base-64 strings, or as the names of files containing the
desired data in raw (binary) form.

See below for a full list of available command line arguments.

### Provisioning CSV file

To accommodate bulk provisioning of devices, the Factory Provisioning Tool can also read provisioning data from a CSV-formatted provisioning
data file.  The columns of this file are expected correspond to specific provisioning data types - that is, `Serial_Num`, `Certificate`, `Private_Key`, etc.
Rows in the file give individual values for specific devices, indexed by Weave device id (`Device_Id`).   The id of the specific device to be provisioning
must be specified on the command line. For example:

```
$ ./weave-factory-prov-tool --target nrf52840 --device-id 18B4300000000001 \
    --prov-csv-file ./dev-provisioning-data.csv
```

The following CSV columns are supported:

| Name | Format | Description |
| --- | --- | --- |
| `Device_Id` | 16 hex digits | Weave device id.  _Must be present._ |
| `Serial_Num` | string | Device serial number. |
| `Certificate` | base-64 string | Manufacturer-assigned Weave device certificate. |
| `Private_Key` | base-64 string | Manufacturer-assigned Weave private key. |
| `Pairing_Code` | string | Weave pairing code. |
| `Product_Rev` | integer | Product revision number. |
| `Mfg_Date` | _YYYY/MM/DD_ | Device manufacturing date. |

Columns may appear in the CSV file in any order.  All columns are optional, with the exception of `Device_Id`.   Any values _not_ present in the CSV file
are simply not provisioned on the device.

The user may specify individual provisioning values on the command line in addition to the CSV file, in which case the command line value take
precedent over those in the file.

The CSV file format support by the Factory Provisioning Tool is compatible with the output of the `weave` tool's `gen-provisioning-data` command.

### Nest provisioning server

The Factory Provisioning Tool supports fetching select provisioning information from a Nest provisioning server using an HTTPS based protocol.
The provisioning server protocol can be used to fetch the Manufacturer-assigned Weave device certificate, the corresponding private key, and the
Weave pairing code from the provisioning server.

The network location of the provisioning server is specified by supplying a base URL on the provisioning tool command line. The desired provisioning
information is selected by specify the Weave device id on the command line. For example:

```
$ ./weave-factory-prov-tool --target nrf52840 --device-id 18B4300000000001 \
    --prov-server https://192.168.172.2:8000/
```

The user may specify individual provisioning values on the command line in addition to the provisioning server URL.  In this case, the values given on
the command line take precedent over values returned by the server.

## Enabling / disabling factory provisioning support

Support for OpenWeave factory provisioning in device firmware is controlled by the `WEAVE_DEVICE_CONFIG_ENABLE_FACTORY_PROVISIONING`
compile-time configuration option.  This option is __enabled__ by default.  The feature can be disabled by overriding the option in the application's
`WeaveProjectConfig.h` file.  For example:

```
#define WEAVE_DEVICE_CONFIG_ENABLE_FACTORY_PROVISIONING 0
```

In general it is safe to enable factory provisioning in production device firmware _provided that the device debug interface is appropriately disabled
on production devices._  This can be achieved either via hardware means (for example, by blowing fuses in the SoC), or in software (for example, via a
secure boot loader that blocks debug access as part of the boot process).

## Running the Factory Provisioning Tool

The OpenWeave Factory Provisioning Tool supports the following command line options:

| Option | Description |
| --- | --- |
| --target _\<string\>_ | Target device type. Choices are: __nrf52840__, __esp32__ |
| --load-addr _\<hex-digits\>_ | Address in device memory at which provisioning data will be written. |
| --verbose, -v | Adjust output verbosity level. Use multiple arguments to increase verbosity. |
| --serial-num _\<string\>_ | Set the device serial number. |
| --device-id _\<hex-digits\>_ | Set the manufacturer-assigned device id. |
| --device-cert _\<base-64\>_ \| _\<file-name\>_ | Set the manufacturer-assigned Weave device certificate.|
| --device-key _\<base-64\>_ \| _\<file-name\>_ | Set the manufacturer-assigned Weave device private key. |
| --pairing-code _\<string\>_ | Set the device pairing code. |
| --product-rev _\<integer\>_ | Set the product revision for the device. |
| --mfg-date _\<YYYY/MM/DD\>_ \| __today__ \| __now__ | Set the device's manufacturing date. |
| --jlink-cmd _\<path-name\>_ | Path to JLink command. Defaults to 'JLinkExe'. |
| --jlink-if __SWD__ \| __JTAG__ | J-Link interface type. Defaults to SWD. |
| --jlink-speed _\<integer\>_ \| __adaptive__ \| __auto__ | J-Link interface speed. |
| --jlink-sn _\<string\>_ | J-Link probe serial number. |
| --esptool-cmd _\<path-name\>_ | Path to esptool command. Defaults to 'esptool.py'. |
| --port _\<path-name\>_ | COM port device name for ESP32. Defaults to /tty/USB0. |
| --speed _\<integer\>_ | Baud rate for COM port. Defaults to 115200. |
| --prov-csv-file _\<file-name\>_ | Read device provisioning data from a provisioning CSV file. |
| --prov-server _\<url\>_ | Read device provisioning data from a provisioning server. |
| --disable-server-validation | When using HTTPS, disable validation of the certificate presented by the provisioning server. |

## Examples

The following command sets the device id, serial number, product revision, and pairing code to specific values.  The manufacturing
date is set to the current date.  And the device certificate and private key are set to test values given in a CSV file supplied with the
`openweave-core` source repository.

```
$ ./weave-factory-prov-tool --target nrf52840 --device-id 18B4300000000042 \
    --serial-num JAYS_DEVICE_42 --product-rev 1 --pairing-code NESTUS --mfg-date today \
    --prov-csv-file ~/projects/openweave-core/certs/development/device/test-dev-provisioning-data.csv
```