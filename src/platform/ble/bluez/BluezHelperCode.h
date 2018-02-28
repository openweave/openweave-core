/*
 *
 *    Copyright (c) 2017 Nest Labs, Inc.
 *    All rights reserved.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */
/**
 *    @file
 *      This file provides Weave over Bluez Peripheral header
 *
 *
 */

#ifndef BLUEZHELPERCODE_H_
#define BLUEZHELPERCODE_H_

extern "C" {
// BlueZ headers
#include "gdbus/gdbus.h"
#if BLE_CONFIG_BLUEZ_MTU_FEATURE
#include "src/shared/queue.h"
#include "src/shared/io.h"
#endif // BLE_CONFIG_BLUEZ_MTU_FEATURE Todo: Remove this macro when enabling wobluez mtu
}

#include <list>
#include <Weave/WeaveVersion.h>
#include <SystemLayer/SystemLayer.h>
#include <BleLayer/BleLayer.h>
#include <InetLayer/InetLayer.h>
#include <Weave/Core/WeaveCore.h>
#include <Weave/Profiles/echo/WeaveEcho.h>
#include <Weave/Support/CodeUtils.h>
#include <Weave/Support/ErrorStr.h>
#include <Weave/Core/WeaveStats.h>
#include "Weave/Support/logging/WeaveLogging.h"
#include "WoBluezLayer.h"

using namespace nl::Inet;
using namespace nl::Weave;
using namespace nl::Weave::Profiles;

#define UUID_WEAVE_SHORT "0xFEAF"
#define UUID_WEAVE "0000feaf-0000-1000-8000-00805f9b34fb"
#define UUID_WEAVE_C1 "18EE2EF5-263D-4559-959F-4F9C429F9D11"
#define UUID_WEAVE_C2 "18EE2EF5-263D-4559-959F-4F9C429F9D12"
#define BLUEZ_PATH "/org/bluez"
#define BLUEZ_INTERFACE "org.bluez"
#define WEAVE_PATH "/org/bluez/weave"
#define ADAPTER_INTERFACE "org.bluez.Adapter1"
#define PROFILE_INTERFACE "org.bluez.GattManager1"
#define ADVERTISING_PATH "/org/bluez/advertising"
#define ADVERTISING_MANAGER_INTERFACE "org.bluez.LEAdvertisingManager1"
#define SERVICE_INTERFACE "org.bluez.GattService1"
#define CHARACTERISTIC_INTERFACE "org.bluez.GattCharacteristic1"
#define ADVERTISING_INTERFACE "org.bluez.LEAdvertisement1"
#define DEVICE_INTERFACE "org.bluez.Device1"
#define FLAGS_WEAVE_C1 "write"
#define FLAGS_WEAVE_C2 "read,indicate"
#define WEAVE_SRV_DATA_BLOCK_TYPE_WEAVE_ID_INFO (1)
#define WEAVE_ID_INFO_MAJ_VER (0x00)
#define WEAVE_ID_INFO_MIN_VER (0x02)

/*MAC OS uses MTU size 104, which is smallest among Android, MAC OS & IOS */
#define HCI_MAX_MTU (104)
#define BUFF_SIZE (1024)

namespace nl {
namespace Ble {
namespace Platform {
namespace BlueZ {

/* Weave Identification Information */
struct WeaveIdInfo
{
    uint8_t major;
    uint8_t minor;
    uint16_t vendorId;
    uint16_t productId;
    uint64_t deviceId;
    uint8_t pairingStatus;
} __attribute__((packed));

/* Weave Service Data*/
struct WeaveServiceData
{
    uint8_t dataBlock0Len;
    uint8_t dataBlock0Type;
    WeaveIdInfo weaveIdInfo;
} __attribute__((packed));

struct Adapter
{
    GDBusProxy * adapterProxy;
    GDBusProxy * advertisingProxy;
    GDBusProxy * profileProxy;
    std::list <GDBusProxy *> deviceProxies;
};

struct Characteristic
{
    DBusConnection * dbusConn;
    bool isNotifying;
    int valueLen;
    uint8_t * value;
    char * path;
    char * servicePath;
    char * uuid;
    char ** flags;
#if BLE_CONFIG_BLUEZ_MTU_FEATURE
    struct io * writePipeIO;
    struct io * indicatePipeIO;
#endif // BLE_CONFIG_BLUEZ_MTU_FEATURE
};

struct Service
{
    DBusConnection * dbusConn;
    bool isPrimary;
    char * path;
    char * uuid;
};

struct BluezServerEndpoint
{
    char * adapterName;
    char * adapterAddr;
    char * advertisingUUID;
    char * advertisingType;
    WeaveServiceData * weaveServiceData;
    Characteristic * weaveC1;
    Characteristic * weaveC2;
    Service * weaveService;
    uint16_t mtu;
};

extern BluezServerEndpoint * gBluezServerEndpoint;
extern BluezBleApplicationDelegate * gBluezBleApplicationDelegate;
extern BluezBlePlatformDelegate * gBluezBlePlatformDelegate;

} // namespace BlueZ
} // namespace Platform
} // namespace Ble
} // namespace nl

#endif /* BLUEZHELPERCODE_H_ */
