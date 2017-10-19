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
 *      This file provides the platform's implementation of the BluezBlePlatformDelegate
 *
 *      The BluezBlePlatformDelegate provides the Weave stack with an interface
 *      by which to form and cancel GATT subscriptions, read and write
 *      GATT characteristic values, send GATT characteristic notifications,
 *      respond to GATT read requests, and close BLE connections.
 */

#include "BluezBlePlatformDelegate.h"
#include "BluezHelperCode.h"

#if CONFIG_BLE_PLATFORM_BLUEZ

namespace nl {
namespace Ble {
namespace Platform {
namespace BlueZ {

BluezBlePlatformDelegate ::BluezBlePlatformDelegate(BleLayer * ble) : Ble(ble), SendIndicationCb(NULL), GetMTUCb(NULL) { };

uint16_t BluezBlePlatformDelegate::GetMTU(BLE_CONNECTION_OBJECT connObj) const
{
    uint16_t mtu = 0;
    if (GetMTUCb != NULL)
    {
        mtu = GetMTUCb((void *) connObj);
    }
    return mtu;
}

bool BluezBlePlatformDelegate::SubscribeCharacteristic(BLE_CONNECTION_OBJECT connObj, const nl::Ble::WeaveBleUUID * svcId,
                                                       const nl::Ble::WeaveBleUUID * charId)
{
    WeaveLogError(Ble, "SubscribeCharacteristic: Not implemented");
    return true;
}

bool BluezBlePlatformDelegate::UnsubscribeCharacteristic(BLE_CONNECTION_OBJECT connObj, const nl::Ble::WeaveBleUUID * svcId,
                                                         const nl::Ble::WeaveBleUUID * charId)
{
    WeaveLogError(Ble, "UnsubscribeCharacteristic: Not implemented");
    return true;
}

bool BluezBlePlatformDelegate::CloseConnection(BLE_CONNECTION_OBJECT connObj)
{
    WeaveLogError(Ble, "CloseConnection: Not implemented");
    return true;
}

bool BluezBlePlatformDelegate::SendIndication(BLE_CONNECTION_OBJECT connObj, const nl::Ble::WeaveBleUUID * svcId,
                                              const nl::Ble::WeaveBleUUID * charId, nl::Inet::InetBuffer * pBuf)
{
    bool rc = true;
    WeaveLogDetail(Ble, "Start of SendIndication");

    if (SendIndicationCb)
    {
        rc = SendIndicationCb((void *) connObj, pBuf->Start(), pBuf->DataLength());
    }

    if (NULL != pBuf)
    {
        nl::Inet::InetBuffer::Free(pBuf);
    }

    return rc;
}

bool BluezBlePlatformDelegate::SendWriteRequest(BLE_CONNECTION_OBJECT connObj, const nl::Ble::WeaveBleUUID * svcId,
                                                const nl::Ble::WeaveBleUUID * charId, nl::Inet::InetBuffer * pBuf)
{
    WeaveLogError(Ble, "SendWriteRequest: Not implemented");
    return true;
}

bool BluezBlePlatformDelegate::SendReadRequest(BLE_CONNECTION_OBJECT connObj, const nl::Ble::WeaveBleUUID * svcId,
                                               const nl::Ble::WeaveBleUUID * charId, nl::Inet::InetBuffer * pBuf)
{
    WeaveLogError(Ble, "SendReadRequest: Not implemented");
    return true;
}

bool BluezBlePlatformDelegate::SendReadResponse(BLE_CONNECTION_OBJECT connObj, BLE_READ_REQUEST_CONTEXT requestContext,
                                                const nl::Ble::WeaveBleUUID * svcId, const nl::Ble::WeaveBleUUID * charId)
{
    WeaveLogError(Ble, "SendReadResponse: Not implemented");
    return true;
}

void BluezBlePlatformDelegate::SetSendIndicationCallback(SendIndicationCallback cb)
{
    SendIndicationCb = cb;
}

void BluezBlePlatformDelegate::SetGetMTUCallback(GetMTUCallback cb)
{
    GetMTUCb = cb;
}

} // namespace BlueZ
} // namespace Platform
} // namespace Ble
} // namespace nl

#endif /* CONFIG_BLE_PLATFORM_BLUEZ */
