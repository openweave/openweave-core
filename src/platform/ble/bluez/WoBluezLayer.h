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

#define WEAVE_SRV_DATA_BLOCK_TYPE (1)
#define WEAVE_SRV_DATA_BLOCK_SIZE (16)
#define WEAVE_SRV_DATA_MAJ_VER (0x00)
#define WEAVE_SRV_DATA_MIN_VER (0x02)
#define WEAVE_SRV_DATA_PAIRING_STATUS_NOT_PAIRED (0)
#define WEAVE_SRV_DATA_PAIRING_STATUS_PAIRED (1)
#define WEAVE_SRV_DATA_PAIRING_STATUS_UNKNOWN (2)

namespace nl {
namespace Ble {
namespace Platform {
namespace BlueZ {

/* Weave Service Data*/
struct WeaveServiceData
{
    uint8_t mWeaveDataBlockLen;
    uint8_t mWeaveDataBlockType;
    uint8_t mWeaveSrvDataMajor;
    uint8_t mWeaveSrvDataMinor;
    uint16_t mWeaveVendorId;
    uint16_t mWeaveProductId;
    uint64_t mWeaveDeviceId;
    uint8_t mWeavePairingStatus;
} __attribute__((packed));

struct BluezPeripheralArgs
{
    char * bleName;
    char * bleAddress;
    WeaveServiceData * weaveServiceData;
    BluezBlePlatformDelegate * bluezBlePlatformDelegate;
};

/**
 * ClearWoBluezStatus to original setting.
 *
 */
void ClearWoBluezStatus(void);

/**
 * Exit BluezIO thread
 *
 */
void ExitBluezIOThread(void);

/**
 * Run WoBle over Bluez thread
 *
 * @return Returns 'true' if WoBluez library is able to successfully register
 * weave gatt server along with advertiser, else 'false'
 */
bool RunBluezIOThread(BluezPeripheralArgs * arg);

} // namespace BlueZ
} // namespace Platform
} // namespace Ble
} // namespace nl

#endif /* WOBLUEZLAYER_H_ */
