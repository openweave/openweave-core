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

#ifndef BLE_MANAGER_H
#define BLE_MANAGER_H

#if WEAVE_DEVICE_CONFIG_ENABLE_WOBLE

#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"

namespace nl {
namespace Weave {
namespace Device {
namespace Internal {

class BLEManager : private ::nl::Ble::BleLayer, private BlePlatformDelegate, private BleApplicationDelegate
{
public:

    typedef ConnectivityManager::WoBLEServiceMode WoBLEServiceMode;

    WEAVE_ERROR Init(void);

    WoBLEServiceMode GetWoBLEServiceMode(void);
    WEAVE_ERROR SetWoBLEServiceMode(WoBLEServiceMode val);

    bool IsAdvertisingEnabled(void);
    WEAVE_ERROR SetAdvertisingEnabled(bool val);

    bool IsFastAdvertisingEnabled(void);
    WEAVE_ERROR SetFastAdvertisingEnabled(bool val);

    WEAVE_ERROR GetDeviceName(char * buf, size_t bufSize);
    WEAVE_ERROR SetDeviceName(const char * deviceName);

    uint16_t NumConnections(void);

    void OnPlatformEvent(const WeaveDeviceEvent * event);

    BleLayer * GetBleLayer(void) const;

private:

    // NOTE: These members are private to the class and should not be used by friends.

    enum
    {
        kFlag_ESPBLELayerInitialized    = 0x0001,
        kFlag_AppRegistered             = 0x0002,
        kFlag_AttrsRegistered           = 0x0004,
        kFlag_GATTServiceStarted        = 0x0008,
        kFlag_AdvertisingConfigured     = 0x0010,
        kFlag_Advertising               = 0x0020,
        kFlag_ControlOpInProgress       = 0x0040,

        kFlag_AdvertisingEnabled        = 0x0080,
        kFlag_FastAdvertisingEnabled    = 0x0100,
        kFlag_UseCustomDeviceName       = 0x0200,
    };

    enum
    {
        kMaxConnections = BLE_LAYER_NUM_BLE_ENDPOINTS,
        kMaxDeviceNameLength = 16
    };

    struct WoBLEConState
    {
        PacketBuffer * PendingIndBuf;
        uint16_t ConId;
        uint16_t MTU : 10;
        uint16_t Allocated : 1;
        uint16_t Subscribed : 1;
        uint16_t Unused : 4;
    };

    WoBLEConState mCons[kMaxConnections];
    WoBLEServiceMode mServiceMode;
    esp_gatt_if_t mAppIf;
    uint16_t mServiceAttrHandle;
    uint16_t mRXCharAttrHandle;
    uint16_t mTXCharAttrHandle;
    uint16_t mTXCharCCCDAttrHandle;
    uint16_t mFlags;
    char mDeviceName[kMaxDeviceNameLength + 1];

    void DriveBLEState(void);
    WEAVE_ERROR InitESPBleLayer(void);
    WEAVE_ERROR ConfigureAdvertisingData(void);
    WEAVE_ERROR StartAdvertising(void);
    void HandleGATTControlEvent(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t * param);
    void HandleGATTCommEvent(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t * param);
    void HandleRXCharWrite(esp_ble_gatts_cb_param_t * param);
    void HandleTXCharRead(esp_ble_gatts_cb_param_t * param);
    void HandleTXCharCCCDRead(esp_ble_gatts_cb_param_t * param);
    void HandleTXCharCCCDWrite(esp_ble_gatts_cb_param_t * param);
    void HandleTXCharConfirm(WoBLEConState * conState, esp_ble_gatts_cb_param_t * param);
    void HandleDisconnect(esp_ble_gatts_cb_param_t * param);
    WoBLEConState * GetConnectionState(uint16_t conId, bool allocate = false);
    bool ReleaseConnectionState(uint16_t conId);

    static void HandleGATTEvent(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t * param);
    static void HandleGAPEvent(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t * param);
    static void DriveBLEState(intptr_t arg);

    // BlePlatformDelegate methods
    virtual bool SubscribeCharacteristic(uint16_t conId, const WeaveBleUUID * svcId, const WeaveBleUUID * charId);
    virtual bool UnsubscribeCharacteristic(uint16_t conId, const WeaveBleUUID * svcId, const WeaveBleUUID * charId);
    virtual bool CloseConnection(uint16_t conId);
    virtual uint16_t GetMTU(uint16_t conId) const;
    virtual bool SendIndication(uint16_t conId, const WeaveBleUUID * svcId, const WeaveBleUUID * charId, PacketBuffer * pBuf);
    virtual bool SendWriteRequest(uint16_t conId, const WeaveBleUUID * svcId, const WeaveBleUUID * charId, PacketBuffer * pBuf);
    virtual bool SendReadRequest(uint16_t conId, const WeaveBleUUID * svcId, const WeaveBleUUID * charId, PacketBuffer * pBuf);
    virtual bool SendReadResponse(uint16_t conId, BLE_READ_REQUEST_CONTEXT requestContext, const WeaveBleUUID * svcId, const WeaveBleUUID * charId);

    // BleApplicationDelegate methods
    virtual void NotifyWeaveConnectionClosed(uint16_t conId);
};

extern BLEManager BLEMgr;


inline BleLayer * BLEManager::GetBleLayer() const
{
    return (BleLayer *)(this);
}

inline BLEManager::WoBLEServiceMode BLEManager::GetWoBLEServiceMode(void)
{
    return mServiceMode;
}

inline bool BLEManager::IsAdvertisingEnabled(void)
{
    return GetFlag(mFlags, kFlag_AdvertisingEnabled);
}

inline bool BLEManager::IsFastAdvertisingEnabled(void)
{
    return GetFlag(mFlags, kFlag_FastAdvertisingEnabled);
}

} // namespace Internal
} // namespace Device
} // namespace Weave
} // namespace nl

#endif // WEAVE_DEVICE_CONFIG_ENABLE_WOBLE

#endif // BLE_MANAGER_H


