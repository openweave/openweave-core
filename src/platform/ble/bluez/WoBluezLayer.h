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
 *      This file defines the interface for WoBluez library
 */

#ifndef WOBLUEZLAYER_H_
#define WOBLUEZLAYER_H_

#include "BluezBleApplicationDelegate.h"
#include "BluezBlePlatformDelegate.h"

#define WEAVE_ID_INFO_PAIRING_STATUS_NOT_PAIRED (0)
#define WEAVE_ID_INFO_PAIRING_STATUS_PAIRED (1)
#define WEAVE_ID_INFO_PAIRING_STATUS_UNKNOWN (2)

namespace nl {
namespace Ble {
namespace Platform {
namespace BlueZ {

struct BluezPeripheralArgs
{
    char * bleName;
    char * bleAddress;
    uint16_t vendorId;
    uint16_t productId;
    uint64_t deviceId;
    uint8_t pairingStatus;
    BluezBleApplicationDelegate * bluezBleApplicationDelegate;
    BluezBlePlatformDelegate * bluezBlePlatformDelegate;
};

/**
 * Close BLE connection.
 *
 */
void CloseBleconnection(void);

/**
 * Exit BluezIO thread
 *
 */
void ExitBluezIOThread(void);

/**
 * Run WoBle over Bluez thread
 *
 * @return Returns 'true' if WoBluez library is able to successfully register
 * Weave gatt server along with advertiser, else 'false'
 */
bool RunBluezIOThread(BluezPeripheralArgs * arg);

/**
 * Invoke a function to run in BluezIO thread context
 *
 * @return Returns 'true' if a function is successfully scheduled to run in
 * BluezIO thread context, else 'false'
 */
bool RunOnBluezIOThread(int (*aCallback)(void *), void * aClosure);

} // namespace BlueZ
} // namespace Platform
} // namespace Ble
} // namespace nl

#endif /* WOBLUEZLAYER_H_ */
