/*
 *
 *    Copyright (c) 2014-2017 Nest Labs, Inc.
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

#ifndef MOCKBLEPLATFORMDELEGATE_H_
#define MOCKBLEPLATFORMDELEGATE_H_

#include <BleLayer/BlePlatformDelegate.h>

class MockBlePlatformDelegate :
    public nl::Ble::BlePlatformDelegate
{
    bool SubscribeCharacteristic(BLE_CONNECTION_OBJECT connObj, const nl::Ble::WeaveBleUUID *svcId, const nl::Ble::WeaveBleUUID *charId);
    bool UnsubscribeCharacteristic(BLE_CONNECTION_OBJECT connObj, const nl::Ble::WeaveBleUUID *svcId, const nl::Ble::WeaveBleUUID *charId);
    bool CloseConnection(BLE_CONNECTION_OBJECT connObj);
    uint16_t GetMTU(BLE_CONNECTION_OBJECT connObj) const;
    bool SendIndication(BLE_CONNECTION_OBJECT connObj, const nl::Ble::WeaveBleUUID *svcId, const nl::Ble::WeaveBleUUID *charId, nl::Weave::System::PacketBuffer *pBuf);
    bool SendWriteRequest(BLE_CONNECTION_OBJECT connObj, const nl::Ble::WeaveBleUUID *svcId, const nl::Ble::WeaveBleUUID *charId, nl::Weave::System::PacketBuffer *pBuf);
    bool SendReadRequest(BLE_CONNECTION_OBJECT connObj, const nl::Ble::WeaveBleUUID *svcId, const nl::Ble::WeaveBleUUID *charId, nl::Weave::System::PacketBuffer *pBuf);
    bool SendReadResponse(BLE_CONNECTION_OBJECT connObj, BLE_READ_REQUEST_CONTEXT requestContext, const nl::Ble::WeaveBleUUID *svcId, const nl::Ble::WeaveBleUUID *charId);
};

#endif /* MOCKBLEPLATFORMDELEGATE_H_ */
