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
#include "BluezBlePlatformDelegate.h"

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

// TODO: use single /ble definition of this
const WeaveBleUUID WEAVE_BLE_CHAR_1_ID = { { // 18EE2EF5-263D-4559-959F-4F9C429F9D11
                                             0x18, 0xEE, 0x2E, 0xF5, 0x26, 0x3D, 0x45, 0x59, 0x95, 0x9F, 0x4F, 0x9C, 0x42, 0x9F,
                                             0x9D, 0x11 } };

const WeaveBleUUID WEAVE_BLE_CHAR_2_ID = { { // 18EE2EF5-263D-4559-959F-4F9C429F9D12
                                             0x18, 0xEE, 0x2E, 0xF5, 0x26, 0x3D, 0x45, 0x59, 0x95, 0x9F, 0x4F, 0x9C, 0x42, 0x9F,
                                             0x9D, 0x12 } };

void WoBLEz_NewConnection(void * data)
{
    WeaveLogProgress(Ble, "WoBLEz_NewConnection: %p", data);
}

void WoBLEz_WriteReceived(void * data, const uint8_t * value, size_t len)
{
    // Peripheral behaviour
    WEAVE_ERROR err                 = WEAVE_NO_ERROR;
    nl::Inet::InetBuffer * msgBuf   = NULL;
    nl::Weave::System::Error syserr = WEAVE_SYSTEM_NO_ERROR;
    InEventParam * params           = NULL;

    msgBuf = nl::Inet::InetBuffer::New();

    VerifyOrExit(msgBuf != NULL, err = WEAVE_ERROR_NO_MEMORY);
    VerifyOrExit(msgBuf->AvailableDataLength() >= len, err = WEAVE_ERROR_BUFFER_TOO_SMALL);

    syserr = gBluezBlePlatformDelegate->NewEventParams(&params);
    SuccessOrExit(syserr);

    memcpy(msgBuf->Start(), value, len);
    msgBuf->SetDataLength(len);

    params->EventType            = InEventParam::EventTypeEnum::kEvent_WriteReceived;
    params->ConnectionObject     = data;
    params->WriteReceived.SvcId  = &(nl::Ble::WEAVE_BLE_SVC_ID);
    params->WriteReceived.CharId = &(WEAVE_BLE_CHAR_1_ID);
    params->WriteReceived.MsgBuf = msgBuf;

    syserr = gBluezBlePlatformDelegate->SendToWeaveThread(params);
    SuccessOrExit(syserr);

    params = NULL;
    msgBuf = NULL;

exit:
    if (syserr != WEAVE_SYSTEM_NO_ERROR)
    {
        WeaveLogError(Ble, "WoBLEz_WriteReceived syserr: %d", syserr);
    }

    if (params != NULL)
    {
        gBluezBlePlatformDelegate->ReleaseEventParams(params);
    }

    if (err != WEAVE_NO_ERROR)
    {
        WeaveLogError(Ble, "WoBLEz_WriteReceived failed: %d", err);
    }

    if (NULL != msgBuf)
    {
        nl::Inet::InetBuffer::Free(msgBuf);
    }
}

int WoBLEz_SendIndication(void * aClosure)
{
    BluezServerEndpoint * endpoint = gBluezServerEndpoint;
    nl::Inet::InetBuffer * msgBuf  = static_cast<nl::Inet::InetBuffer *>(aClosure);
    uint8_t * buffer               = msgBuf->Start();
    size_t len                     = msgBuf->DataLength();
#if BLE_CONFIG_BLUEZ_MTU_FEATURE
    struct iovec ioData;
#endif // BLE_CONFIG_BLUEZ_MTU_FEATURE

    g_free(endpoint->weaveC2->value);
    endpoint->weaveC2->valueLen = len;
    endpoint->weaveC2->value    = static_cast<uint8_t *>(g_memdup(buffer, len));

#if BLE_CONFIG_BLUEZ_MTU_FEATURE
    if (endpoint->weaveC2->indicatePipeIO)
    {
        ioData.iov_base = static_cast<void *>(endpoint->weaveC2->value);
        ioData.iov_len  = endpoint->weaveC2->valueLen;

        if (io_send(endpoint->weaveC2->indicatePipeIO, &ioData, 1) < 0)
        {
            WeaveLogError(Ble, "weave C2 fails to write into pipe");
        }
    }
#else  // BLE_CONFIG_BLUEZ_MTU_FEATURE,
    g_dbus_emit_property_changed(endpoint->weaveC2->dbusConn, endpoint->weaveC2->path, CHARACTERISTIC_INTERFACE, "Value");
#endif // BLE_CONFIG_BLUEZ_MTU_FEATURE

    nl::Inet::InetBuffer::Free(msgBuf);

    return G_SOURCE_REMOVE;
}

bool WoBLEz_ScheduleSendIndication(void * data, nl::Inet::InetBuffer * msgBuf)
{
    const char * msg               = NULL;
    bool success                   = false;
    BluezServerEndpoint * endpoint = static_cast<BluezServerEndpoint *>(data);

    VerifyOrExit(endpoint != NULL, msg = "endpoint is NULL in WoBLEz_SendIndication");
    VerifyOrExit(endpoint == gBluezServerEndpoint, msg = "Unexpected endpoint in WoBLEz_SendIndication");
    VerifyOrExit(endpoint->weaveC2 != NULL, msg = "weaveC2 is NULL in WoBLEz_SendIndication");

    success = RunOnBluezIOThread(WoBLEz_SendIndication, msgBuf);

exit:

    if (NULL != msg)
    {
        WeaveLogError(Ble, msg);
    }

    if (!success && msgBuf != NULL)
    {
        nl::Inet::InetBuffer::Free(msgBuf);
    }

    return success;
}

void WoBLEz_ConnectionClosed(void * data)
{
    InEventParam * params        = NULL;
    nl::Weave::System::Error err = WEAVE_SYSTEM_NO_ERROR;

    WeaveLogProgress(Ble, "WoBLEz_ConnectionClosed: %p", data);

    err = gBluezBlePlatformDelegate->NewEventParams(&params);
    SuccessOrExit(err);

    params->EventType            = InEventParam::EventTypeEnum::kEvent_ConnectionError;
    params->ConnectionObject     = data;
    params->ConnectionError.mErr = BLE_ERROR_REMOTE_DEVICE_DISCONNECTED;

    err = gBluezBlePlatformDelegate->SendToWeaveThread(params);
    SuccessOrExit(err);

    params = NULL;

exit:
    if (err != WEAVE_SYSTEM_NO_ERROR)
    {
        WeaveLogError(Ble, "WoBLEz_ConnectionClosed err: %d", err);
    }

    if (params != NULL)
    {
        gBluezBlePlatformDelegate->ReleaseEventParams(params);
    }
}

void WoBLEz_SubscriptionChange(void * data)
{
    InEventParam * params          = NULL;
    nl::Weave::System::Error err   = WEAVE_SYSTEM_NO_ERROR;
    BluezServerEndpoint * endpoint = static_cast<BluezServerEndpoint *>(data);
    const char * msg               = NULL;

    VerifyOrExit(endpoint != NULL, msg = "endpoint is NULL in WoBLEz_SubscriptionChange");
    VerifyOrExit(endpoint == gBluezServerEndpoint, msg = "Unexpected endpoint in WoBLEz_SubscriptionChange");
    VerifyOrExit(endpoint->weaveC2 != NULL, msg = "weaveC2 is NULL in WoBLEz_SubscriptionChange");

    err = gBluezBlePlatformDelegate->NewEventParams(&params);
    SuccessOrExit(err);

    params->EventType = endpoint->weaveC2->isNotifying ? InEventParam::EventTypeEnum::kEvent_SubscribeReceived
                                                       : InEventParam::EventTypeEnum::kEvent_UnsubscribeReceived;
    params->ConnectionObject          = data;
    params->SubscriptionChange.SvcId  = &(nl::Ble::WEAVE_BLE_SVC_ID);
    params->SubscriptionChange.CharId = &(WEAVE_BLE_CHAR_2_ID);

    err = gBluezBlePlatformDelegate->SendToWeaveThread(params);
    SuccessOrExit(err);

    params = NULL;

exit:

    if (err != WEAVE_SYSTEM_NO_ERROR)
    {
        WeaveLogError(Ble, "WoBLEz_ConnectionClosed err: %d", err);
    }

    if (params != NULL)
    {
        gBluezBlePlatformDelegate->ReleaseEventParams(params);
    }

    if (NULL != msg)
    {
        WeaveLogError(Ble, msg);
    }
}

void WoBLEz_IndicationConfirmation(void * data)
{
    InEventParam * params        = NULL;
    nl::Weave::System::Error err = WEAVE_SYSTEM_NO_ERROR;

    err = gBluezBlePlatformDelegate->NewEventParams(&params);
    SuccessOrExit(err);

    params->EventType                     = InEventParam::EventTypeEnum::kEvent_IndicationConfirmation;
    params->ConnectionObject              = data;
    params->IndicationConfirmation.SvcId  = &(nl::Ble::WEAVE_BLE_SVC_ID);
    params->IndicationConfirmation.CharId = &(WEAVE_BLE_CHAR_2_ID);

    err = gBluezBlePlatformDelegate->SendToWeaveThread(params);
    SuccessOrExit(err);

    params = NULL;

exit:
    if (err != WEAVE_SYSTEM_NO_ERROR)
    {
        WeaveLogError(Ble, "WoBLEz_IndicationConfirmation err: %d", err);
    }

    if (params != NULL)
    {
        gBluezBlePlatformDelegate->ReleaseEventParams(params);
    }
}

} // namespace BlueZ
} /* namespace Platform */
} /* namespace Ble */
} /* namespace nl */

#endif /* CONFIG_BLE_PLATFORM_BLUEZ */
