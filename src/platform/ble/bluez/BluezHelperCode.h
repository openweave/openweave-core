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

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include <stdlib.h>
#include <getopt.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <glib.h>
#include <sys/signalfd.h>


extern "C" {
//BlueZ headers
#include "gdbus/gdbus.h"
}

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

using namespace nl::Inet;
using namespace nl::Weave;
using namespace nl::Weave::Profiles;

#include "WoBluez.h"

#define UUID_WEAVE_SHORT                "0xFEAF"
#define UUID_WEAVE                      "0000feaf-0000-1000-8000-00805f9b34fb"
#define UUID_WEAVE_C1		            "18EE2EF5-263D-4559-959F-4F9C429F9D11"
#define UUID_WEAVE_C2		            "18EE2EF5-263D-4559-959F-4F9C429F9D12"
#define BLUEZ_PATH                      "/org/bluez"
#define BLUEZ_INTERFACE                 "org.bluez"
#define WEAVE_PATH                      "/org/bluez/weave"
#define ADAPTER_INTERFACE               "org.bluez.Adapter1"
#define PROFILE_INTERFACE               "org.bluez.GattManager1"
#define ADVERTISING_PATH                "/org/bluez/advertising"
#define ADVERTISING_MANAGER_INTERFACE   "org.bluez.LEAdvertisingManager1"
#define SERVICE_INTERFACE               "org.bluez.GattService1"
#define CHARACTERISTIC_INTERFACE        "org.bluez.GattCharacteristic1"
#define ADVERTISING_INTERFACE           "org.bluez.LEAdvertisement1"
#define FLAGS_WEAVE_C1                  "write"
#define FLAGS_WEAVE_C2                  "read,indicate"
#define HCI_MAX_MTU   				    300

namespace nl {
namespace Ble {
namespace Platform {
namespace BlueZ {

struct Adapter{
    GDBusProxy *adapterProxy;
    GDBusProxy *advertisingProxy;
    GDBusProxy *profileProxy;
};

struct Characteristic{
    DBusConnection *dbusConn;
    bool isNotifying;
    int valueLen;
    uint8_t *value;
    char *path;
    char *servicePath;
    char *uuid;
    char **flags;
};

struct Service{
    DBusConnection *dbusConn;
    bool isPrimary;
    char *path;
    char *uuid;
};

struct BluezServerEndpoint{
    char *adapterName;
    char *adapterAddr;
    char *advertisingUUID;
    char *advertisingType;
    Characteristic *weaveC1;
    Characteristic *weaveC2;
    Service *weaveService;
};

/**
 * Exit BluezIO thread
 *
 */
void ExitMainLoop(void);

/**
 * Run WoBle over Bluez thread
 *
 */
bool RunBluezIOThread(char *aBleName, char *aBleAddress);

} /* namespace Bluez */
} /* namespace Platform */
} /* namespace Ble */
} /* namespace nl */

#endif  /* BLUEZHELPERCODE_H_ */
