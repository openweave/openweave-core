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
#include <BleLayer/BlePlatformDelegate.h>
#include "WeaveDeviceManager-BlePlatformDelegate.h"

DeviceManager_BlePlatformDelegate::DeviceManager_BlePlatformDelegate(BleLayer *ble)
{
    Ble = ble;
    writeCB = NULL;
    subscribeCB = NULL;
    closeCB = NULL;
}

bool DeviceManager_BlePlatformDelegate::SubscribeCharacteristic(BLE_CONNECTION_OBJECT connObj, const nl::Ble::WeaveBleUUID *svcId, const nl::Ble::WeaveBleUUID *charId)
{
    const bool subscribe = true;

    if (subscribeCB && svcId && charId)
    {
        return subscribeCB(connObj, (void *)svcId->bytes, (void *)charId->bytes, subscribe);
    }

    return false;
}

bool DeviceManager_BlePlatformDelegate::UnsubscribeCharacteristic(BLE_CONNECTION_OBJECT connObj, const nl::Ble::WeaveBleUUID *svcId, const nl::Ble::WeaveBleUUID *charId)
{
    const bool subscribe = true;

    if (subscribeCB && svcId && charId)
    {
        return subscribeCB(connObj, (void *)svcId->bytes, (void *)charId->bytes, !subscribe);
    }

    return false;
}

bool DeviceManager_BlePlatformDelegate::CloseConnection(BLE_CONNECTION_OBJECT connObj)
{
    if (closeCB)
    {
        return closeCB(connObj);
    }

    return false;
}

uint16_t DeviceManager_BlePlatformDelegate::GetMTU(BLE_CONNECTION_OBJECT connObj) const
{
    // TODO Python queue-based implementation
    return 0;
}

bool DeviceManager_BlePlatformDelegate::SendIndication(BLE_CONNECTION_OBJECT connObj, const nl::Ble::WeaveBleUUID *svcId, const nl::Ble::WeaveBleUUID *charId, nl::Weave::System::PacketBuffer *pBuf)
{
    // TODO Python queue-based implementation

    // Release delegate's reference to pBuf. pBuf will be freed when both platform delegate and Weave stack free their references to it.
    nl::Weave::System::PacketBuffer::Free(pBuf);

    return false;
}

bool DeviceManager_BlePlatformDelegate::SendWriteRequest(BLE_CONNECTION_OBJECT connObj, const nl::Ble::WeaveBleUUID *svcId, const nl::Ble::WeaveBleUUID *charId, nl::Weave::System::PacketBuffer *pBuf)
{
    bool ret = false;

    if (writeCB && svcId && charId && pBuf)
    {
        ret = writeCB(connObj, (void *)svcId->bytes, (void *)charId->bytes, (void *)pBuf->Start(), pBuf->DataLength());
    }

    // Release delegate's reference to pBuf. pBuf will be freed when both platform delegate and Weave stack free their references to it.
    // We release pBuf's reference here since its payload bytes were copied into a new NSData object by WeaveBleMgr.py's writeCB, and
    // in both the error and succees cases this code has no further use for the pBuf PacketBuffer.
    nl::Weave::System::PacketBuffer::Free(pBuf);

    return ret;
}

bool DeviceManager_BlePlatformDelegate::SendReadRequest(BLE_CONNECTION_OBJECT connObj, const nl::Ble::WeaveBleUUID *svcId, const nl::Ble::WeaveBleUUID *charId, nl::Weave::System::PacketBuffer *pBuf)
{
    // TODO Python queue-based implementation
    return false;
}

bool DeviceManager_BlePlatformDelegate::SendReadResponse(BLE_CONNECTION_OBJECT connObj, BLE_READ_REQUEST_CONTEXT requestContext, const nl::Ble::WeaveBleUUID *svcId, const nl::Ble::WeaveBleUUID *charId)
{
    // TODO Python queue-based implementation
    return false;
}
