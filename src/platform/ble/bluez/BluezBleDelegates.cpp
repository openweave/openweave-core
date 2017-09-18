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
 *      This file provides the platform's implementation of the BluezBlePlatformDelegate and
 *      BluezBleApplicationDelegate objects.
 *
 *      The BluezBlePlatformDelegate provides the Weave stack with an interface
 *      by which to form and cancel GATT subscriptions, read and write
 *      GATT characteristic values, send GATT characteristic notifications,
 *      respond to GATT read requests, and close BLE connections.
 *
 *      The BluezBleApplicationDelegate provides the implementation for Weave to inform
 *      the application when it has finished using a given BLE connection,
 *      i.e when the WeaveConnection object wrapping this connection has
 *      closed. This allows the application to either close the BLE connection
 *      or continue to keep it open for non-Weave purposes.
 *
 *
 */


#include "BluezBleDelegates.h"
#include "BluezHelperCode.h"

#if CONFIG_BLE_PLATFORM_BLUEZ

namespace nl {
namespace Ble {
namespace Platform {
namespace BlueZ {

BluezBleApplicationDelegate::BluezBleApplicationDelegate()
{
}

void BluezBleApplicationDelegate::NotifyWeaveConnectionClosed(BLE_CONNECTION_OBJECT connObj)
{
    WeaveLogDetail(Ble, "NotifyWeaveConnectionClosed");
    ExitMainLoop();
}

uint16_t BluezBlePlatformDelegate::GetMTU(BLE_CONNECTION_OBJECT connObj) const
{
    uint16_t mtu = 0;
    if (GetMTUCb != NULL) {
        mtu = GetMTUCb((void *)connObj);
    }
    return mtu;
}

bool BluezBlePlatformDelegate::SubscribeCharacteristic(BLE_CONNECTION_OBJECT connObj, const nl::Ble::WeaveBleUUID *svcId, const nl::Ble::WeaveBleUUID *charId)
{
    //central support not yet implemented
    WeaveLogDetail(Ble, "SubscribeCharaceristic");
    return true;
}

bool BluezBlePlatformDelegate::UnsubscribeCharacteristic(BLE_CONNECTION_OBJECT connObj, const nl::Ble::WeaveBleUUID *svcId, const nl::Ble::WeaveBleUUID *charId)
{
    //central support not yet implemented
    WeaveLogDetail(Ble, "UnsubscribeCharaceristic");
    return true;
}

bool BluezBlePlatformDelegate::CloseConnection(BLE_CONNECTION_OBJECT connObj)
{
    //central support not yet implemented
    WeaveLogDetail(Ble, "CloseConnection");
    return true;
}

bool BluezBlePlatformDelegate::SendIndication(BLE_CONNECTION_OBJECT connObj, const nl::Ble::WeaveBleUUID *svcId, const nl::Ble::WeaveBleUUID *charId, nl::Inet::InetBuffer *pBuf)
{
    bool rc = true;
    WeaveLogDetail(Ble, "Start of SendIndication");

    if (SendIndicationCb)
    {
        rc = SendIndicationCb((void *)connObj, pBuf->Start(), pBuf->DataLength());
    }

    if (NULL != pBuf)
    {
        nl::Inet::InetBuffer::Free(pBuf);
    }

    return rc;
}

bool BluezBlePlatformDelegate::SendWriteRequest(BLE_CONNECTION_OBJECT connObj, const nl::Ble::WeaveBleUUID *svcId, const nl::Ble::WeaveBleUUID *charId, nl::Inet::InetBuffer *pBuf)
{
    //central support not yet implemented
    WeaveLogDetail(Ble, "SendWriteRequest");
    return true;
}

bool BluezBlePlatformDelegate::SendReadRequest(BLE_CONNECTION_OBJECT connObj, const nl::Ble::WeaveBleUUID *svcId, const nl::Ble::WeaveBleUUID *charId, nl::Inet::InetBuffer *pBuf)
{
    //central support not yet implemented
    WeaveLogDetail(Ble, "SendReadRequest");
    return true;
}

bool BluezBlePlatformDelegate::SendReadResponse(BLE_CONNECTION_OBJECT connObj, BLE_READ_REQUEST_CONTEXT requestContext, const nl::Ble::WeaveBleUUID *svcId, const nl::Ble::WeaveBleUUID *charId)
{
    //central support not yet implemented
    WeaveLogDetail(Ble, "SendReadResponse");
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

} /* namespace Bluez */
} /* namespace Platform */
} /* namespace Ble */
} /* namespace nl */

#endif /* CONFIG_BLE_PLATFORM_BLUEZ */
