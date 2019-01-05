/*
 *
 *    Copyright (c) 2018 Nest Labs, Inc.
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
 *          Provides an implementation of the BLEManager singleton object
 *          for the nRF5 platforms.
 */

#include <Weave/DeviceLayer/internal/WeaveDeviceLayerInternal.h>
#include <Weave/DeviceLayer/internal/BLEManager.h>
#include <new>

#if WEAVE_DEVICE_CONFIG_ENABLE_WOBLE

#include "nrf_error.h"
#include "ble.h"
#include "ble_gap.h"
#include "ble_gattc.h"
#include "ble_advdata.h"
#include "ble_srv_common.h"
#include "nrf_ble_gatt.h"

using namespace ::nl;
using namespace ::nl::Ble;

namespace nl {
namespace Weave {
namespace DeviceLayer {
namespace Internal {

namespace {

struct WeaveServiceData
{
    uint8_t MajorVersion;
    uint8_t MinorVersion;
    uint8_t DeviceVendorId[2];
    uint8_t DeviceProductId[2];
    uint8_t DeviceId[8];
    uint8_t PairingStatus;
};

const uint16_t UUID16_WoBLEService = 0xFEAF;
const ble_uuid_t UUID_WoBLEService = { UUID16_WoBLEService, BLE_UUID_TYPE_BLE };

const ble_uuid128_t UUID128_WoBLEChar_RX = { { 0x11, 0x9D, 0x9F, 0x42, 0x9C, 0x4F, 0x9F, 0x95, 0x59, 0x45, 0x3D, 0x26, 0xF5, 0x2E, 0xEE, 0x18 } };
ble_uuid_t UUID_WoBLEChar_RX;
const WeaveBleUUID WeaveUUID_WoBLEChar_RX = { { 0x18, 0xEE, 0x2E, 0xF5, 0x26, 0x3D, 0x45, 0x59, 0x95, 0x9F, 0x4F, 0x9C, 0x42, 0x9F, 0x9D, 0x11 } };

const ble_uuid128_t UUID128_WoBLEChar_TX = { { 0x12, 0x9D, 0x9F, 0x42, 0x9C, 0x4F, 0x9F, 0x95, 0x59, 0x45, 0x3D, 0x26, 0xF5, 0x2E, 0xEE, 0x18 } };
ble_uuid_t UUID_WoBLEChar_TX;
const WeaveBleUUID WeaveUUID_WoBLEChar_TX = { { 0x18, 0xEE, 0x2E, 0xF5, 0x26, 0x3D, 0x45, 0x59, 0x95, 0x9F, 0x4F, 0x9C, 0x42, 0x9F, 0x9D, 0x12 } };

NRF_BLE_GATT_DEF(GATTModule);

WEAVE_ERROR RegisterVendorUUID(ble_uuid_t & uuid, const ble_uuid128_t & vendorUUID)
{
    WEAVE_ERROR err;

    err = sd_ble_uuid_vs_add(&vendorUUID, &uuid.type);
    SuccessOrExit(err);

    uuid.uuid = (((uint16_t)vendorUUID.uuid128[13]) << 16) | vendorUUID.uuid128[12];

exit:
    return err;
}

#if 0 // WIP
const uint8_t UUID_PrimaryService[] = { 0x00, 0x28 };
const uint8_t UUID_CharDecl[] = { 0x03, 0x28 };
const uint8_t UUID_ClientCharConfigDesc[] = { 0x02, 0x29 };
#endif

} // unnamed namespace


BLEManagerImpl BLEManagerImpl::sInstance;

WEAVE_ERROR BLEManagerImpl::_Init()
{
    WEAVE_ERROR err;
    uint16_t svcHandle;
    ble_add_char_params_t addCharParams;

    memset(mCons, 0, sizeof(mCons));
    mServiceMode = ConnectivityManager::kWoBLEServiceMode_Enabled;
    mFlags = kFlag_AdvertisingEnabled;
    memset(mDeviceName, 0, sizeof(mDeviceName));
    mAdvHandle = BLE_GAP_ADV_SET_HANDLE_NOT_SET;
    mNumGAPCons = 0;

    // Initialize the Weave BleLayer.
    err = BleLayer::Init(this, this, &SystemLayer);
    SuccessOrExit(err);

    // Register vendor-specific UUIDs with the soft device.
    //     NOTE: An NRF_ERROR_NO_MEM here means the soft device hasn't been configured
    //     with space for enough custom UUIDs.  Typically, this limit is set by overriding
    //     the NRF_SDH_BLE_VS_UUID_COUNT config option.
    err = RegisterVendorUUID(UUID_WoBLEChar_RX, UUID128_WoBLEChar_RX);
    SuccessOrExit(err);
    err = RegisterVendorUUID(UUID_WoBLEChar_TX, UUID128_WoBLEChar_TX);
    SuccessOrExit(err);

    // Add the WoBLE service.
    err = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY, (ble_uuid_t *)&UUID_WoBLEService, &svcHandle);
    SuccessOrExit(err);

    // Add the WoBLEChar_RX characteristic to the WoBLE service.
    memset(&addCharParams, 0, sizeof(addCharParams));
    addCharParams.uuid = UUID_WoBLEChar_RX.uuid;
    addCharParams.uuid_type = UUID_WoBLEChar_RX.type;
    addCharParams.max_len = NRF_SDH_BLE_GATT_MAX_MTU_SIZE;
    addCharParams.init_len = 1;
    addCharParams.is_var_len = true;
    addCharParams.char_props.write_wo_resp = 1; // TODO: verify which of these is actually needed.
    addCharParams.char_props.write = 1;
    addCharParams.read_access = SEC_OPEN;
    addCharParams.write_access = SEC_OPEN;
    addCharParams.cccd_write_access = SEC_NO_ACCESS;
//    addCharParams.is_value_user = true;
    err = characteristic_add(svcHandle, &addCharParams, &mWoBLECharHandle_RX);
    SuccessOrExit(err);

#if 1
    // Add the WoBLEChar_TX characteristic.
    memset(&addCharParams, 0, sizeof(addCharParams));
    addCharParams.uuid = UUID_WoBLEChar_TX.uuid;
    addCharParams.uuid_type = UUID_WoBLEChar_TX.type;
    addCharParams.max_len = NRF_SDH_BLE_GATT_MAX_MTU_SIZE;
    addCharParams.is_var_len = true;
    addCharParams.char_props.read = 1;
    addCharParams.char_props.notify = 1;
    addCharParams.read_access = SEC_OPEN;
    addCharParams.write_access = SEC_OPEN;
    addCharParams.cccd_write_access = SEC_OPEN;
//    addCharParams.is_value_user = true;
    err = characteristic_add(svcHandle, &addCharParams, &mWoBLECharHandle_TX);
    SuccessOrExit(err);
#endif

    // Initialize the nRF5 GATT module and set the allowable GATT MTU and GAP packet sizes
    // based on compile-time config values.
    // TODO: Possibly eliminate callback code.  Negotiation occurs before the WoBLE connection is
    // established, meaning that the MTU value must be separately queried after the callback has occurred,
    // making the callback of limited value.
    err = nrf_ble_gatt_init(&GATTModule, GATTModuleEventCallback);
    SuccessOrExit(err);
    err = nrf_ble_gatt_att_mtu_periph_set(&GATTModule, NRF_SDH_BLE_GATT_MAX_MTU_SIZE);
    SuccessOrExit(err);
    err = nrf_ble_gatt_data_length_set(&GATTModule, BLE_CONN_HANDLE_INVALID, NRF_SDH_BLE_GAP_DATA_LENGTH);
    SuccessOrExit(err);

    // Register a handler for BLE events.
    NRF_SDH_BLE_OBSERVER(sBLEObserver, WEAVE_DEVICE_LAYER_BLE_OBSERVER_PRIORITY, SoftDeviceBLEEventCallback, NULL);

    // Set a default device name.
    err = _SetDeviceName(NULL);
    SuccessOrExit(err);

    // TODO: call sd_ble_gap_ppcp_set() to set gap parameters

    PlatformMgr().ScheduleWork(DriveBLEState, 0);

exit:
    WeaveLogProgress(DeviceLayer, "BLEManagerImpl::Init() complete");
    return err;
}

WEAVE_ERROR BLEManagerImpl::_SetWoBLEServiceMode(WoBLEServiceMode val)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    VerifyOrExit(val != ConnectivityManager::kWoBLEServiceMode_NotSupported, err = WEAVE_ERROR_INVALID_ARGUMENT);
    VerifyOrExit(mServiceMode == ConnectivityManager::kWoBLEServiceMode_NotSupported, err = WEAVE_ERROR_UNSUPPORTED_WEAVE_FEATURE);

    if (val != mServiceMode)
    {
        mServiceMode = val;
        PlatformMgr().ScheduleWork(DriveBLEState, 0);
    }

exit:
    return err;
}

WEAVE_ERROR BLEManagerImpl::_SetAdvertisingEnabled(bool val)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    VerifyOrExit(mServiceMode == ConnectivityManager::kWoBLEServiceMode_NotSupported, err = WEAVE_ERROR_UNSUPPORTED_WEAVE_FEATURE);

    if (GetFlag(mFlags, kFlag_AdvertisingEnabled) != val)
    {
        SetFlag(mFlags, kFlag_AdvertisingEnabled, val);
        PlatformMgr().ScheduleWork(DriveBLEState, 0);
    }

exit:
    return err;
}

WEAVE_ERROR BLEManagerImpl::_SetFastAdvertisingEnabled(bool val)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    VerifyOrExit(mServiceMode == ConnectivityManager::kWoBLEServiceMode_NotSupported, err = WEAVE_ERROR_UNSUPPORTED_WEAVE_FEATURE);

    if (GetFlag(mFlags, kFlag_FastAdvertisingEnabled) != val)
    {
        SetFlag(mFlags, kFlag_FastAdvertisingEnabled, val);
        PlatformMgr().ScheduleWork(DriveBLEState, 0);
    }

exit:
    return err;
}
WEAVE_ERROR BLEManagerImpl::_GetDeviceName(char * buf, size_t bufSize)
{
    // TODO: use sd_ble_gap_device_name_get to read device name from soft device?

    if (strlen(mDeviceName) >= bufSize)
    {
        return WEAVE_ERROR_BUFFER_TOO_SMALL;
    }
    strcpy(buf, mDeviceName);
    return WEAVE_NO_ERROR;
}

WEAVE_ERROR BLEManagerImpl::_SetDeviceName(const char * deviceName)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    ble_gap_conn_sec_mode_t secMode;

    VerifyOrExit(mServiceMode != ConnectivityManager::kWoBLEServiceMode_NotSupported, err = WEAVE_ERROR_UNSUPPORTED_WEAVE_FEATURE);

    if (deviceName != NULL && deviceName[0] != 0)
    {
        VerifyOrExit(strlen(deviceName) <= kMaxDeviceNameLength, err = WEAVE_ERROR_INVALID_ARGUMENT);
        strcpy(mDeviceName, deviceName);
        SetFlag(mFlags, kFlag_UseCustomDeviceName);
    }
    else
    {
        snprintf(mDeviceName, sizeof(mDeviceName), "%s%04" PRIX32,
                 WEAVE_DEVICE_CONFIG_BLE_DEVICE_NAME_PREFIX,
                 (uint32_t)FabricState.LocalNodeId);
        mDeviceName[kMaxDeviceNameLength] = 0;
        ClearFlag(mFlags, kFlag_UseCustomDeviceName);
    }

    // Do not allow device name characteristic to be changed
    BLE_GAP_CONN_SEC_MODE_SET_NO_ACCESS(&secMode);

    // Configure the device name within the BLE soft device.
    err = sd_ble_gap_device_name_set(&secMode, (const uint8_t *)mDeviceName, strlen(mDeviceName));
    SuccessOrExit(err);

exit:
    return err;
}

void BLEManagerImpl::_OnPlatformEvent(const WeaveDeviceEvent * event)
{
    switch (event->Type)
    {
    case DeviceEventType::kWoBLESubscribe:
        HandleSubscribeReceived(event->WoBLESubscribe.ConId, &WEAVE_BLE_SVC_ID, &WeaveUUID_WoBLEChar_TX);
        {
            WeaveDeviceEvent conEstEvent;
            conEstEvent.Type = DeviceEventType::kWoBLEConnectionEstablished;
            PlatformMgr().PostEvent(&conEstEvent);
        }
        break;

    case DeviceEventType::kWoBLEUnsubscribe:
        HandleUnsubscribeReceived(event->WoBLEUnsubscribe.ConId, &WEAVE_BLE_SVC_ID, &WeaveUUID_WoBLEChar_TX);
        break;

    case DeviceEventType::kWoBLEWriteReceived:
        HandleWriteReceived(event->WoBLEWriteReceived.ConId, &WEAVE_BLE_SVC_ID, &WeaveUUID_WoBLEChar_RX, event->WoBLEWriteReceived.Data);
        break;

    case DeviceEventType::kWoBLEIndicateConfirm:
        HandleIndicationConfirmation(event->WoBLEIndicateConfirm.ConId, &WEAVE_BLE_SVC_ID, &WeaveUUID_WoBLEChar_TX);
        break;

    case DeviceEventType::kWoBLEConnectionError:
        HandleConnectionError(event->WoBLEConnectionError.ConId, event->WoBLEConnectionError.Reason);
        break;

    case DeviceEventType::kSoftDeviceBLEEvent:
        HandleSoftDeviceBLEEvent(event);
        break;

    case DeviceEventType::kGATTModuleEvent:
        HandleGATTModuleEvent(event);
        break;

    default:
        break;
    }
}

bool BLEManagerImpl::SubscribeCharacteristic(BLE_CONNECTION_OBJECT conId, const WeaveBleUUID * svcId, const WeaveBleUUID * charId)
{
    WeaveLogProgress(DeviceLayer, "BLEManagerImpl::SubscribeCharacteristic() not supported");
    return false;
}

bool BLEManagerImpl::UnsubscribeCharacteristic(BLE_CONNECTION_OBJECT conId, const WeaveBleUUID * svcId, const WeaveBleUUID * charId)
{
    WeaveLogProgress(DeviceLayer, "BLEManagerImpl::UnsubscribeCharacteristic() not supported");
    return false;
}

bool BLEManagerImpl::CloseConnection(BLE_CONNECTION_OBJECT conId)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    WeaveLogProgress(DeviceLayer, "Closing BLE GATT connection (con %u)", conId);

    // TODO: implement this

    // Release the associated connection state record.
    ReleaseConnectionState(conId);

    // Arrange to re-enable connectable advertising in case it was disabled due to the
    // maximum connection limit being reached.
    ClearFlag(mFlags, kFlag_Advertising);
    PlatformMgr().ScheduleWork(DriveBLEState, 0);

    return (err == WEAVE_NO_ERROR);
}

uint16_t BLEManagerImpl::GetMTU(BLE_CONNECTION_OBJECT conId) const
{
    WoBLEConState * conState = const_cast<BLEManagerImpl *>(this)->GetConnectionState(conId);
    return (conState != NULL) ? nrf_ble_gatt_eff_mtu_get(&GATTModule, conId) : 0;
}

bool BLEManagerImpl::SendIndication(BLE_CONNECTION_OBJECT conId, const WeaveBleUUID * svcId, const WeaveBleUUID * charId, PacketBuffer * data)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    WoBLEConState * conState = GetConnectionState(conId);

    WeaveLogProgress(DeviceLayer, "Sending indication for WoBLE TX characteristic (con %u, len %u)", conId, data->DataLength());

    VerifyOrExit(conState != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);

    VerifyOrExit(conState->PendingIndBuf == NULL, err = WEAVE_ERROR_INCORRECT_STATE);

    // TODO: implement this

    // Save a reference to the buffer until we get a indication from the ESP BLE layer that it
    // has been sent.
    conState->PendingIndBuf = data;
    data = NULL;

exit:
    if (err != WEAVE_NO_ERROR)
    {
        WeaveLogError(DeviceLayer, "BLEManagerImpl::SendIndication() failed: %s", ErrorStr(err));
        PacketBuffer::Free(data);
        return false;
    }
    return true;
}

bool BLEManagerImpl::SendWriteRequest(BLE_CONNECTION_OBJECT conId, const WeaveBleUUID * svcId, const WeaveBleUUID * charId, PacketBuffer * pBuf)
{
    WeaveLogError(DeviceLayer, "BLEManagerImpl::SendWriteRequest() not supported");
    return false;
}

bool BLEManagerImpl::SendReadRequest(BLE_CONNECTION_OBJECT conId, const WeaveBleUUID * svcId, const WeaveBleUUID * charId, PacketBuffer * pBuf)
{
    WeaveLogError(DeviceLayer, "BLEManagerImpl::SendReadRequest() not supported");
    return false;
}

bool BLEManagerImpl::SendReadResponse(BLE_CONNECTION_OBJECT conId, BLE_READ_REQUEST_CONTEXT requestContext, const WeaveBleUUID * svcId, const WeaveBleUUID * charId)
{
    WeaveLogError(DeviceLayer, "BLEManagerImpl::SendReadResponse() not supported");
    return false;
}

void BLEManagerImpl::NotifyWeaveConnectionClosed(BLE_CONNECTION_OBJECT conId)
{
}

void BLEManagerImpl::DriveBLEState(void)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    // If the application has enabled WoBLE and BLE advertising...
    if (mServiceMode == ConnectivityManager::kWoBLEServiceMode_Enabled && GetFlag(mFlags, kFlag_AdvertisingEnabled))
    {
        // Start/re-start advertising if not already started, or if there is a pending change
        // to the advertising configuration.
        if (!GetFlag(mFlags, kFlag_Advertising) || GetFlag(mFlags, kFlag_AdvertisingConfigChangePending))
        {
            err = StartAdvertising();
            SuccessOrExit(err);
        }
    }

    // Otherwise, stop advertising if it is enabled.
    else if (GetFlag(mFlags, kFlag_AdvertisingEnabled))
    {
        err = StopAdvertising();
        SuccessOrExit(err);
    }

exit:
    if (err != WEAVE_NO_ERROR)
    {
        WeaveLogError(DeviceLayer, "Disabling WoBLE service due to error: %s", ErrorStr(err));
        mServiceMode = ConnectivityManager::kWoBLEServiceMode_Disabled;
    }
}

WEAVE_ERROR BLEManagerImpl::StartAdvertising(void)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    ble_gap_adv_data_t gapAdvData;
    ble_gap_adv_params_t gapAdvParams;
    uint16_t numWoBLECons;
    bool connectable;

    // Clear any "pending change" flag.
    ClearFlag(mFlags, kFlag_AdvertisingConfigChangePending);

    // Force the soft device to relinquish its references to the buffers containing the advertising
    // data.  This ensures the soft device is not accessing these buffers while we are encoding
    // new advertising data into them.
    if (GetFlag(mFlags, kFlag_Advertising))
    {
        ClearFlag(mFlags, kFlag_Advertising);

        err = sd_ble_gap_adv_stop(mAdvHandle);
        SuccessOrExit(err);

        err = sd_ble_gap_adv_set_configure(&mAdvHandle, NULL, NULL);
        SuccessOrExit(err);
    }

    // Encode the data that will be sent in the advertising packet and the scan response packet.
    err = EncodeAdvertisingData(gapAdvData);
    SuccessOrExit(err);

    // Set advertising parameters.
    memset(&gapAdvParams, 0, sizeof(gapAdvParams));
    gapAdvParams.primary_phy     = BLE_GAP_PHY_1MBPS;
    gapAdvParams.duration        = BLE_GAP_ADV_TIMEOUT_GENERAL_UNLIMITED;
    gapAdvParams.filter_policy   = BLE_GAP_ADV_FP_ANY;

    // Advertise connectable if we haven't reached the maximum number of WoBLE connections or the
    // maximum number of GAP connections.  (Note that the SoftDevice will return an error if connectable
    // advertising is requested when the max number of GAP connections exist).
    numWoBLECons = NumConnections();
    connectable = (numWoBLECons < kMaxConnections && mNumGAPCons < NRF_SDH_BLE_PERIPHERAL_LINK_COUNT);
    gapAdvParams.properties.type = connectable
            ? BLE_GAP_ADV_TYPE_CONNECTABLE_SCANNABLE_UNDIRECTED
            : BLE_GAP_ADV_TYPE_NONCONNECTABLE_SCANNABLE_UNDIRECTED;

    // Advertise in fast mode if not paired to an account and there are no WoBLE connections, or
    // if the application has requested fast advertising.
    gapAdvParams.interval =
        ((numWoBLECons == 0 && !ConfigurationMgr().IsPairedToAccount()) || GetFlag(mFlags, kFlag_AdvertisingEnabled))
        ? WEAVE_DEVICE_CONFIG_BLE_FAST_ADVERTISING_INTERVAL
        : WEAVE_DEVICE_CONFIG_BLE_SLOW_ADVERTISING_INTERVAL;

    WeaveLogProgress(DeviceLayer, "Configuring BLE advertising (interval %" PRIu32 " ms, %sconnectable, device name %s)",
             (((uint32_t)gapAdvParams.interval) * 10) / 16,
             (connectable) ? "" : "non-",
             mDeviceName);

    // Configure an "advertising set" in the BLE soft device with the data and parameters for Weave advertising.
    // If the advertising set doesn't exist, this call will create it and return its handle.
    err = sd_ble_gap_adv_set_configure(&mAdvHandle, &gapAdvData, &gapAdvParams);
    if (err != WEAVE_NO_ERROR)
    {
        WeaveLogError(DeviceLayer, "sd_ble_gap_adv_set_configure() failed: %s", ErrorStr(err));
    }
    SuccessOrExit(err);

    // Instruct the BLE soft device to start advertising using the configured advertising set.
    err = sd_ble_gap_adv_start(mAdvHandle, WEAVE_DEVICE_LAYER_BLE_CONN_CFG_TAG);
    if (err != WEAVE_NO_ERROR)
    {
        WeaveLogError(DeviceLayer, "sd_ble_gap_adv_start() failed: %s", ErrorStr(err));
    }
    SuccessOrExit(err);

    // Record that advertising has been enabled.
    SetFlag(mFlags, kFlag_Advertising);

exit:
    return err;
}

WEAVE_ERROR BLEManagerImpl::StopAdvertising(void)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    if (GetFlag(mFlags, kFlag_Advertising))
    {
        ClearFlag(mFlags, kFlag_Advertising);

        err = sd_ble_gap_adv_stop(mAdvHandle);
        SuccessOrExit(err);
    }

exit:
    return err;
}

WEAVE_ERROR BLEManagerImpl::EncodeAdvertisingData(ble_gap_adv_data_t & gapAdvData)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    ble_advdata_t advData;
    ble_advdata_service_data_t serviceData;
    WeaveServiceData weaveServiceData;

    // Form the contents of the advertising packet.
    memset(&advData, 0, sizeof(advData));
    advData.name_type = BLE_ADVDATA_FULL_NAME;
    advData.include_appearance = false;
    advData.flags = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;
    advData.uuids_complete.uuid_cnt = 1;
    advData.uuids_complete.p_uuids = (ble_uuid_t *)&UUID_WoBLEService;
    gapAdvData.adv_data.p_data = mAdvDataBuf;
    gapAdvData.adv_data.len = sizeof(mAdvDataBuf);
    err = ble_advdata_encode(&advData, mAdvDataBuf, &gapAdvData.adv_data.len);
    SuccessOrExit(err);

    // Construct the Weave Service Data structure that will be sent in the scan response packet.
    memset(&weaveServiceData, 0, sizeof(weaveServiceData));
    weaveServiceData.MajorVersion = 0;
    weaveServiceData.MinorVersion = 1;
    Encoding::LittleEndian::Put16(weaveServiceData.DeviceVendorId, (uint16_t)WEAVE_DEVICE_CONFIG_DEVICE_VENDOR_ID);
    Encoding::LittleEndian::Put16(weaveServiceData.DeviceProductId, (uint16_t)WEAVE_DEVICE_CONFIG_DEVICE_PRODUCT_ID);
    Encoding::LittleEndian::Put64(weaveServiceData.DeviceId, FabricState.LocalNodeId);
    weaveServiceData.PairingStatus = ConfigurationMgr().IsPairedToAccount() ? 1 : 0;

    // Form the contents of the scan response packet.
    memset(&serviceData, 0, sizeof(serviceData));
    serviceData.service_uuid = UUID16_WoBLEService;
    serviceData.data.size = sizeof(weaveServiceData);
    serviceData.data.p_data = (uint8_t *)&weaveServiceData;
    memset(&advData, 0, sizeof(advData));
    advData.name_type = BLE_ADVDATA_NO_NAME;
    advData.include_appearance = false;
    advData.p_service_data_array = &serviceData;
    advData.service_data_count = 1;
    gapAdvData.scan_rsp_data.p_data = mScanRespDataBuf;
    gapAdvData.scan_rsp_data.len = sizeof(mScanRespDataBuf);
    err = ble_advdata_encode(&advData, mScanRespDataBuf, &gapAdvData.scan_rsp_data.len);
    SuccessOrExit(err);

exit:
    return err;
}

BLEManagerImpl::WoBLEConState * BLEManagerImpl::GetConnectionState(uint16_t conId, bool allocate)
{
    uint16_t freeIndex = kMaxConnections;

    for (uint16_t i = 0; i < kMaxConnections; i++)
    {
        if (mCons[i].Allocated == 1)
        {
            if (mCons[i].ConId == conId)
            {
                return &mCons[i];
            }
        }

        else if (i < freeIndex)
        {
            freeIndex = i;
        }
    }

    if (allocate)
    {
        if (freeIndex < kMaxConnections)
        {
            memset(&mCons[freeIndex], 0, sizeof(WoBLEConState));
            mCons[freeIndex].Allocated = 1;
            mCons[freeIndex].ConId = conId;
            return &mCons[freeIndex];
        }

        WeaveLogError(DeviceLayer, "Failed to allocate WoBLEConState");
    }

    return NULL;
}

bool BLEManagerImpl::ReleaseConnectionState(uint16_t conId)
{
    for (uint16_t i = 0; i < kMaxConnections; i++)
    {
        if (mCons[i].Allocated && mCons[i].ConId == conId)
        {
            PacketBuffer::Free(mCons[i].PendingIndBuf);
            mCons[i].Allocated = 0;
            return true;
        }
    }

    return false;
}

uint16_t BLEManagerImpl::_NumConnections(void)
{
    uint16_t numCons = 0;
    for (uint16_t i = 0; i < kMaxConnections; i++)
    {
        if (mCons[i].Allocated)
        {
            numCons++;
        }
    }

    return numCons;
}

void BLEManagerImpl::DriveBLEState(intptr_t arg)
{
    sInstance.DriveBLEState();
}

void BLEManagerImpl::HandleSoftDeviceBLEEvent(const WeaveDeviceEvent * event)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    const ble_evt_t * bleEvent = &event->Platform.SoftDeviceBLEEvent;
    bool driveBLEState = false;

    // TODO: implement me

    switch (bleEvent->header.evt_id)
    {
    case BLE_GAP_EVT_CONNECTED:
        WeaveLogProgress(DeviceLayer, "BLE GAP connection established (con %" PRIu16 ")", bleEvent->evt.gap_evt.conn_handle);

        mNumGAPCons++;

        // The SoftDevice automatically disables advertising whenever a connection is established.  So adjust the
        // current state accordingly.
        ClearFlag(mFlags, kFlag_Advertising);

        driveBLEState = true;

        break;

    case BLE_GAP_EVT_DISCONNECTED:
        WeaveLogProgress(DeviceLayer, "BLE GAP connection terminated (con %" PRIu16 ")", bleEvent->evt.gap_evt.conn_handle);

        if (mNumGAPCons > 0)
        {
            mNumGAPCons--;
        }

        // Force a reconfiguration of advertising in case we switched to non-connectable mode when
        // the connection was established.
        SetFlag(mFlags, kFlag_AdvertisingConfigChangePending);

        driveBLEState = true;

        break;

    case BLE_GAP_EVT_SEC_PARAMS_REQUEST:
        WeaveLogProgress(DeviceLayer, "BLE_GAP_EVT_SEC_PARAMS_REQUEST");
        // Pairing not supported
        err = sd_ble_gap_sec_params_reply(bleEvent->evt.gap_evt.conn_handle,
                                          BLE_GAP_SEC_STATUS_PAIRING_NOT_SUPP,
                                          NULL,
                                          NULL);
        SuccessOrExit(err);
        break;

    case BLE_GAP_EVT_PHY_UPDATE_REQUEST:
    {
        WeaveLogProgress(DeviceLayer, "BLE GAP PHY update request (con %" PRIu16 ")", bleEvent->evt.gap_evt.conn_handle);
        const ble_gap_phys_t phys = { BLE_GAP_PHY_AUTO, BLE_GAP_PHY_AUTO };
        err = sd_ble_gap_phy_update(bleEvent->evt.gap_evt.conn_handle, &phys);
        SuccessOrExit(err);
        break;
    }

    case BLE_GATTS_EVT_SYS_ATTR_MISSING:
        WeaveLogProgress(DeviceLayer, "BLE_GATTS_EVT_SYS_ATTR_MISSING");
        err = sd_ble_gatts_sys_attr_set(bleEvent->evt.gatts_evt.conn_handle, NULL, 0, 0);
        SuccessOrExit(err);
        break;

    case BLE_GATTC_EVT_TIMEOUT:
        WeaveLogProgress(DeviceLayer, "BLE GATT Client timeout (con %" PRIu16 ")", bleEvent->evt.gattc_evt.conn_handle);
        err = sd_ble_gap_disconnect(bleEvent->evt.gattc_evt.conn_handle,
                                    BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
        SuccessOrExit(err);
        break;

    case BLE_GATTS_EVT_TIMEOUT:
        WeaveLogProgress(DeviceLayer, "BLE GATT Server timeout (con %" PRIu16 ")", bleEvent->evt.gatts_evt.conn_handle);
        err = sd_ble_gap_disconnect(bleEvent->evt.gatts_evt.conn_handle,
                                    BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
        APP_ERROR_CHECK(err);
        break;

    default:
        WeaveLogProgress(DeviceLayer, "BLE SoftDevice event 0x%02x", bleEvent->header.evt_id);
        break;
    }

exit:
    if (err != WEAVE_NO_ERROR)
    {
        WeaveLogError(DeviceLayer, "Disabling WoBLE service due to error: %s", ErrorStr(err));
        sInstance.mServiceMode = ConnectivityManager::kWoBLEServiceMode_Disabled;
        driveBLEState = true;
    }

    if (driveBLEState)
    {
        PlatformMgr().ScheduleWork(DriveBLEState, 0);
    }
}

void BLEManagerImpl::HandleGATTModuleEvent(const WeaveDeviceEvent * event)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    const nrf_ble_gatt_evt_t * gattModuleEvent = &event->Platform.GATTModuleEvent;
    bool driveBLEState = false;

    // TODO: implement me

    switch (gattModuleEvent->evt_id)
    {
    case NRF_BLE_GATT_EVT_ATT_MTU_UPDATED:
        WeaveLogProgress(DeviceLayer, "GATT MTU updated (con %" PRIu16 ", mtu %" PRIu16 ")", gattModuleEvent->conn_handle, gattModuleEvent->params.att_mtu_effective);
        // TODO: save MTU for WoBLE connections???
        break;
    case NRF_BLE_GATT_EVT_DATA_LENGTH_UPDATED:
        WeaveLogProgress(DeviceLayer, "GAP packet data length updated (con %" PRIu16 ", len %" PRIu16 ")", gattModuleEvent->conn_handle, gattModuleEvent->params.data_length);
        break;
    default:
        WeaveLogProgress(DeviceLayer, "GATT module event 0x%02x", gattModuleEvent->evt_id);
        break;
    }

exit:
    if (err != WEAVE_NO_ERROR)
    {
        WeaveLogError(DeviceLayer, "Disabling WoBLE service due to error: %s", ErrorStr(err));
        sInstance.mServiceMode = ConnectivityManager::kWoBLEServiceMode_Disabled;
        driveBLEState = true;
    }

    if (driveBLEState)
    {
        PlatformMgr().ScheduleWork(DriveBLEState, 0);
    }
}

void BLEManagerImpl::SoftDeviceBLEEventCallback(const ble_evt_t * bleEvent, void * context)
{
    WeaveDeviceEvent event;
    BaseType_t yieldRequired;

    // Copy the BLE event into a WeaveDeviceEvent.
    event.Type = DeviceEventType::kSoftDeviceBLEEvent;
    event.Platform.SoftDeviceBLEEvent = *bleEvent;

    // Post the event to the Weave queue.
    PlatformMgrImpl().PostEventFromISR(&event, yieldRequired);

    portYIELD_FROM_ISR(yieldRequired);
}

void BLEManagerImpl::GATTModuleEventCallback(nrf_ble_gatt_t * gattModule, const nrf_ble_gatt_evt_t * gattModuleEvent)
{
    WeaveDeviceEvent event;
    BaseType_t yieldRequired;

    // Copy the GATT module event into a WeaveDeviceEvent.
    event.Type = DeviceEventType::kGATTModuleEvent;
    event.Platform.GATTModuleEvent = *gattModuleEvent;

    // Post the event to the Weave queue.
    PlatformMgrImpl().PostEventFromISR(&event, yieldRequired);

    portYIELD_FROM_ISR(yieldRequired);
}

} // namespace Internal
} // namespace DeviceLayer
} // namespace Weave
} // namespace nl

#endif // WEAVE_DEVICE_CONFIG_ENABLE_WOBLE

