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
#include <BuildConfig.h>
#if CONFIG_NETWORK_LAYER_BLE

#include <BleLayer/BlePlatformDelegate.h>
#include "AndroidBlePlatformDelegate.h"

#include <stddef.h>

AndroidBlePlatformDelegate::AndroidBlePlatformDelegate(BleLayer *ble):
    SendWriteRequestCb(NULL),
    SubscribeCharacteristicCb(NULL),
    UnsubscribeCharacteristicCb(NULL),
    CloseConnectionCb(NULL),
    GetMTUCb(NULL)
{
}

bool AndroidBlePlatformDelegate::SubscribeCharacteristic(BLE_CONNECTION_OBJECT connObj, const nl::Ble::WeaveBleUUID *svcId, const nl::Ble::WeaveBleUUID *charId)
{
    bool rc = true;
    if (SubscribeCharacteristicCb)
    {
        rc = SubscribeCharacteristicCb(connObj, static_cast<const uint8_t *>(svcId->bytes),
				       static_cast<const uint8_t *>(charId->bytes));
    }
    return rc;
}

bool AndroidBlePlatformDelegate::UnsubscribeCharacteristic(BLE_CONNECTION_OBJECT connObj, const nl::Ble::WeaveBleUUID *svcId, const nl::Ble::WeaveBleUUID *charId)
{
    bool rc = true;
    if (UnsubscribeCharacteristicCb)
    {
        rc = UnsubscribeCharacteristicCb(connObj, static_cast<const uint8_t *>(svcId->bytes),
					 static_cast<const uint8_t *>(charId->bytes));
    }
    return rc;
}

uint16_t AndroidBlePlatformDelegate::GetMTU(BLE_CONNECTION_OBJECT connObj) const
{
    // TODO Android queue-based implementation
    uint16_t mtu = 0;
    if (GetMTUCb)
    {
      mtu = GetMTUCb(connObj);
    }
    return mtu;
}

bool AndroidBlePlatformDelegate::CloseConnection(BLE_CONNECTION_OBJECT connObj)
{
    bool rc = true;
    if (CloseConnectionCb)
    {
        rc = CloseConnectionCb(connObj);
    }
    return rc;
}

bool AndroidBlePlatformDelegate::SendIndication(BLE_CONNECTION_OBJECT connObj, const nl::Ble::WeaveBleUUID *svcId, const nl::Ble::WeaveBleUUID *charId, PacketBuffer *pBuf)
{
    // TODO Android queue-based implementation

    // Release delegate's reference to pBuf. pBuf will be freed when both platform delegate and Weave stack free their references to it.
    PacketBuffer::Free(pBuf);

    return false;
}

bool AndroidBlePlatformDelegate::SendWriteRequest(BLE_CONNECTION_OBJECT connObj, const nl::Ble::WeaveBleUUID *svcId, const nl::Ble::WeaveBleUUID *charId, PacketBuffer *pBuf)
{
    bool rc = true;
    // TODO Android queue-based implementation, for now, pretend it works.
    if (SendWriteRequestCb)
    {
        rc = SendWriteRequestCb(connObj, static_cast<const uint8_t *>(svcId->bytes),
				static_cast<const uint8_t *>(charId->bytes), pBuf->Start(), pBuf->DataLength());
    }

    // Release delegate's reference to pBuf. pBuf will be freed when both platform delegate and Weave stack free their references to it.
    // We release pBuf's reference here since its payload bytes were copied onto the Java heap by SendWriteRequestCb.
    PacketBuffer::Free(pBuf);

    return rc;
}

bool AndroidBlePlatformDelegate::SendReadRequest(BLE_CONNECTION_OBJECT connObj, const nl::Ble::WeaveBleUUID *svcId, const nl::Ble::WeaveBleUUID *charId, PacketBuffer *pBuf)
{
    // TODO Android queue-based implementation, for now, pretend it works.
    return true;
}

bool AndroidBlePlatformDelegate::SendReadResponse(BLE_CONNECTION_OBJECT connObj, BLE_READ_REQUEST_CONTEXT requestContext, const nl::Ble::WeaveBleUUID *svcId, const nl::Ble::WeaveBleUUID *charId)
{
    // TODO Android queue-based implementation, for now, pretend it works.
    return true;
}

void AndroidBlePlatformDelegate::SetSendWriteRequestCallback(SendWriteRequestCallback cb)
{
    SendWriteRequestCb = cb;
}

void AndroidBlePlatformDelegate::SetSubscribeCharacteristicCallback(SubscribeCharacteristicCallback cb)
{
    SubscribeCharacteristicCb = cb;
}

void AndroidBlePlatformDelegate::SetUnsubscribeCharacteristicCallback(UnsubscribeCharacteristicCallback cb)
{
    UnsubscribeCharacteristicCb = cb;
}

void AndroidBlePlatformDelegate::SetCloseConnectionCallback(CloseConnectionCallback cb)
{
    CloseConnectionCb = cb;
}

void AndroidBlePlatformDelegate::SetGetMTUCallback(GetMTUCallback cb)
{
    GetMTUCb = cb;
}
#endif /* CONFIG_NETWORK_LAYER_BLE */
