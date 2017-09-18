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
 *      This file defines WoBluez peripheral interface implementation that hands the wrapper talking with BleLayer via the
 *      corresponding platform interface function when the application passively receives an incoming BLE connection.
 *
 */

#include "WoBluez.h"
#include "BluezHelperCode.h"

#include <getopt.h>
#include <inttypes.h>
#include <limits.h>
#include <InetLayer/InetLayer.h>
#include <Weave/Core/WeaveCore.h>
#include <Weave/Profiles/echo/WeaveEcho.h>
#include <Weave/Support/CodeUtils.h>
#include <Weave/Support/ErrorStr.h>

#if CONFIG_BLE_PLATFORM_BLUEZ

namespace nl {
namespace Ble {
namespace Platform {
namespace BlueZ {

//TODO: use single /ble definition of this
const WeaveBleUUID WEAVE_BLE_CHAR_1_ID = {
    {
        // 18EE2EF5-263D-4559-959F-4F9C429F9D11
        0x18,
        0xEE,
        0x2E,
        0xF5,
        0x26,
        0x3D,
        0x45,
        0x59,
        0x95,
        0x9F,
        0x4F,
        0x9C,
        0x42,
        0x9F,
        0x9D,
        0x11
    }
};

const WeaveBleUUID WEAVE_BLE_CHAR_2_ID = {
    {
        // 18EE2EF5-263D-4559-959F-4F9C429F9D12
        0x18,
        0xEE,
        0x2E,
        0xF5,
        0x26,
        0x3D,
        0x45,
        0x59,
        0x95,
        0x9F,
        0x4F,
        0x9C,
        0x42,
        0x9F,
        0x9D,
        0x12
    }
};

void WoBLEz_NewConnection(void *data)
{
    WeaveLogDetail(Ble, "WoBLEz_NewConnection: %p", data);
}

void WoBLEz_WriteReceived(void *data, const uint8_t *value, size_t len)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    // Peripheral behaviour
    nl::Inet::InetBuffer *msgBuf = NULL;
    msgBuf = nl::Inet::InetBuffer::New();
    VerifyOrExit(msgBuf != NULL, err = WEAVE_ERROR_NO_MEMORY);
    VerifyOrExit(msgBuf->AvailableDataLength() >= len, err = WEAVE_ERROR_BUFFER_TOO_SMALL);

    memcpy(msgBuf->Start(), value, len);
    msgBuf->SetDataLength(len);

    if (!MessageLayer.mBle->HandleWriteReceived(data, &(nl::Ble::WEAVE_BLE_SVC_ID), &WEAVE_BLE_CHAR_1_ID, msgBuf))
    {
        WeaveLogDetail(Ble, "HandleWriteReceived failed to HandleWriteReceived ");
    }

exit:

    if (err != WEAVE_NO_ERROR)
    {
        WeaveLogDetail(Ble, "WoBLEz_WriteReceived failed: %d", err);
    }

    if (NULL != msgBuf)
    {
        nl::Inet::InetBuffer::Free(msgBuf);
    }
}


bool WoBLEz_SendIndication(void *data, uint8_t *buffer, size_t len)
{
    const char * msg = NULL;
    bool success = false;
    BluezServerEndpoint *endpoint = static_cast<BluezServerEndpoint *>(data);
    VerifyOrExit(endpoint != NULL, msg = "endpoint is NULL in WoBLEz_SendIndication");
    VerifyOrExit(endpoint->weaveC2 != NULL, msg = "weaveC2 is NULL in WoBLEz_SendIndication" );

    g_free(endpoint->weaveC2->value);
    endpoint->weaveC2->valueLen = len;
    endpoint->weaveC2->value = static_cast<uint8_t*>(g_memdup(buffer, len));
    g_dbus_emit_property_changed(endpoint->weaveC2->dbusConn, endpoint->weaveC2->path, CHARACTERISTIC_INTERFACE, "Value");
    success = true;

exit:

    if (NULL != msg)
    {
        WeaveLogDetail(Ble, msg);
    }

    return success;
}

void WoBLEz_ConnectionClosed(void *data)
{
    WeaveLogDetail(Ble, "WoBLEz_ConnectionClosed: %p", data);
    MessageLayer.mBle->HandleConnectionError(data, BLE_ERROR_REMOTE_DEVICE_DISCONNECTED);
}

void WoBLEz_SubscriptionChange(void *data)
{
    const char * msg = NULL;
    bool enabled;
    BluezServerEndpoint *endpoint = static_cast<BluezServerEndpoint *>(data);
    VerifyOrExit(endpoint != NULL, msg = "endpoint is NULL in WoBLEz_SendIndication");
    VerifyOrExit(endpoint->weaveC2 != NULL, msg = "weaveC2 is NULL in WoBLEz_SendIndication" );

    enabled = endpoint->weaveC2->isNotifying;

    if (enabled)
    {
        if (!MessageLayer.mBle->HandleSubscribeReceived(data, &(nl::Ble::WEAVE_BLE_SVC_ID), &WEAVE_BLE_CHAR_2_ID))
        {
            WeaveLogDetail(Ble, "HandleSubscribeReceived failed");
        }
    }
    else
    {
        if (!MessageLayer.mBle->HandleUnsubscribeReceived(data, &(nl::Ble::WEAVE_BLE_SVC_ID), &WEAVE_BLE_CHAR_2_ID))
        {
            WeaveLogDetail(Ble, "HandleUnsubscribeReceived failed");
        }
    }

exit:

    if (NULL != msg)
    {
        WeaveLogDetail(Ble, msg);
    }
}

void WoBLEz_IndicationConfirmation(void *data)
{

    if (!MessageLayer.mBle->HandleIndicationConfirmation(data, &(nl::Ble::WEAVE_BLE_SVC_ID), &WEAVE_BLE_CHAR_2_ID))
    {
        WeaveLogDetail(Ble, "HandleIndicationConfirmation failed");
    }
}

} /* namespace Bluez */
} /* namespace Platform */
} /* namespace Ble */
} /* namespace nl */

#endif /* CONFIG_BLE_PLATFORM_BLUEZ */
