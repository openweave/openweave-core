/*
 *
 *    Copyright (c) 2015-2017 Nest Labs, Inc.
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
 *      This file provides base implementation for NLWeaveBleDelegate interface
 *
 */

#import <Foundation/Foundation.h>

#import "NLWeaveStack.h"
#import "NLLogging.h"
#import "NLWeaveBleDelegate_Protected.h"

#include <Weave/Core/WeaveError.h>
#include <Weave/Support/CodeUtils.h>
#include <Weave/Support/logging/WeaveLogging.h>

#include <BleLayer/BleApplicationDelegate.h>
#include <BleLayer/BlePlatformDelegate.h>

#import "NLWeaveDeviceManager_Protected.h"

#define LOCAL_VERBOSE_LOG 0

namespace nl
{
    namespace Ble
    {
        class BleDelegateTrampoline;
    }
}

using nl::Weave::System::PacketBuffer;

@interface NLWeaveBleDelegate ()
{
    nl::Ble::BleDelegateTrampoline * _mTrampoline;
    dispatch_queue_t _mCbWorkQueue;
    NSMapTable * _mMapFromPeripheralToDm;

    // cached static Weave service UUID
    CBUUID * _mWeaveServiceUUID;

    nl::Ble::BleLayer * _mBleLayer;
}

+ (CBUUID *)GetShortestServiceUUID:(const nl::Ble::WeaveBleUUID *)svcId;

@end


namespace nl
{
namespace Ble
{

class BleDelegateTrampoline: public BleApplicationDelegate, public BlePlatformDelegate
{
public:
    BleDelegateTrampoline (NLWeaveBleDelegate * pBleDelegate);
    virtual bool SubscribeCharacteristic(BLE_CONNECTION_OBJECT connObj, const WeaveBleUUID *svcId, const WeaveBleUUID *charId);
    virtual bool UnsubscribeCharacteristic(BLE_CONNECTION_OBJECT connObj, const WeaveBleUUID *svcId, const WeaveBleUUID *charId);
    virtual bool CloseConnection(BLE_CONNECTION_OBJECT connObj);
    virtual uint16_t GetMTU(BLE_CONNECTION_OBJECT connObj) const;
    virtual bool SendIndication(BLE_CONNECTION_OBJECT connObj, const WeaveBleUUID *svcId, const WeaveBleUUID *charId, PacketBuffer *pBuf);
    virtual bool SendWriteRequest(BLE_CONNECTION_OBJECT connObj, const WeaveBleUUID *svcId, const WeaveBleUUID *charId, PacketBuffer *pBuf);
    virtual bool SendReadRequest(BLE_CONNECTION_OBJECT connObj, const WeaveBleUUID *svcId, const WeaveBleUUID *charId, PacketBuffer *pBuf);
    virtual bool SendReadResponse(BLE_CONNECTION_OBJECT connObj, BLE_READ_REQUEST_CONTEXT requestContext, const Ble::WeaveBleUUID *svcId, const WeaveBleUUID *charId);

    virtual void NotifyWeaveConnectionClosed(BLE_CONNECTION_OBJECT connObj);

private:

    /**
        An weak reference to the parenting delegate object, can be nil.
        @note
          It has to be weak to avoid circular references
     */
    __weak NLWeaveBleDelegate * mBleDelegate;
};

BleDelegateTrampoline::BleDelegateTrampoline(NLWeaveBleDelegate * pBleDelegate): mBleDelegate(pBleDelegate)
{

}

/**
    @note
      This method is only called from BleLayer, under Weave task context
 */
bool BleDelegateTrampoline::SubscribeCharacteristic(BLE_CONNECTION_OBJECT connObj, const WeaveBleUUID *svcId, const WeaveBleUUID *charId)
{
    bool result = false;
    CBUUID * service = nil;
    CBUUID * characteristic = nil;

    VerifyOrExit(mBleDelegate, WDM_LOG_ERROR(@"BLE not configured properly\n"));

    if (NULL != svcId)
    {
        service = [NLWeaveBleDelegate GetShortestServiceUUID:svcId];
    }
    if (NULL != charId)
    {
        characteristic = [CBUUID UUIDWithData:[NSData dataWithBytes:charId->bytes length:sizeof(charId->bytes)]];
    }

    result = [mBleDelegate SubscribeCharacteristic:(__bridge id)connObj serivce:service characteristic:characteristic];

exit:
    return result;
}

/**
    @note
      This method is only called from BleLayer, under Weave task context
 */
bool BleDelegateTrampoline::UnsubscribeCharacteristic(BLE_CONNECTION_OBJECT connObj, const WeaveBleUUID *svcId, const WeaveBleUUID *charId)
{
    bool result = false;
    CBUUID * service = nil;
    CBUUID * characteristic = nil;

    VerifyOrExit(mBleDelegate, WDM_LOG_ERROR(@"BLE not configured properly\n"));

    if (NULL != svcId)
    {
        service = [NLWeaveBleDelegate GetShortestServiceUUID:svcId];
    }
    if (NULL != charId)
    {
        characteristic = [CBUUID UUIDWithData:[NSData dataWithBytes:charId->bytes length:sizeof(charId->bytes)]];
    }

    result = [mBleDelegate UnsubscribeCharacteristic:(__bridge id)connObj serivce:service characteristic:characteristic];

exit:
    return result;
}

/**
    @note
      This method is only called from BleLayer, under Weave task context
 */
bool BleDelegateTrampoline::CloseConnection(BLE_CONNECTION_OBJECT connObj)
{
    bool result = false;

    VerifyOrExit(mBleDelegate, WDM_LOG_ERROR(@"BLE not configured properly\n"));

    result = [mBleDelegate CloseConnection:(__bridge id)connObj];

exit:
    return result;
}

/**
    @note
      This method is only called from BleLayer, under Weave task context
 */
uint16_t BleDelegateTrampoline::GetMTU(BLE_CONNECTION_OBJECT connObj) const
{
    bool result = false;

    VerifyOrExit(mBleDelegate, WDM_LOG_ERROR(@"BLE not configured properly\n"));

    result = [mBleDelegate GetMTU:(__bridge id)connObj];

exit:
    return result;
}

/**
    @note
      This method is only called from BleLayer, under Weave task context
 */
bool BleDelegateTrampoline::SendIndication(BLE_CONNECTION_OBJECT connObj, const WeaveBleUUID *svcId, const WeaveBleUUID *charId, PacketBuffer *pBuf)
{
    bool result = false;
    CBUUID * service = nil;
    CBUUID * characteristic = nil;
    NSData * data = nil;

    VerifyOrExit(mBleDelegate, WDM_LOG_ERROR(@"BLE not configured properly\n"));

    if (NULL != svcId)
    {
        service = [NLWeaveBleDelegate GetShortestServiceUUID:svcId];
    }
    if (NULL != charId)
    {
        characteristic = [CBUUID UUIDWithData:[NSData dataWithBytes:charId->bytes length:sizeof(charId->bytes)]];
    }
    if (NULL != pBuf)
    {
        data = [NSData dataWithBytes:pBuf->Start() length:pBuf->DataLength()];
    }

    result = [mBleDelegate SendIndication:(__bridge id)connObj serivce:service characteristic:characteristic data:data];

exit:
    // Release delegate's reference to pBuf. pBuf will be freed when both platform delegate and Weave stack free their references to it.
    // We release pBuf's reference here since its payload bytes were copied into a new NSData object
    PacketBuffer::Free(pBuf);

    return result;
}

/**
    @note
      This method is only called from BleLayer, under Weave task context
 */
bool BleDelegateTrampoline::SendWriteRequest(BLE_CONNECTION_OBJECT connObj, const WeaveBleUUID *svcId, const WeaveBleUUID *charId, PacketBuffer *pBuf)
{
    bool result = false;
    CBUUID * service = nil;
    CBUUID * characteristic = nil;
    NSData * data = nil;

    VerifyOrExit(mBleDelegate, WDM_LOG_ERROR(@"BLE not configured properly\n"));

    if (NULL != svcId)
    {
        service = [NLWeaveBleDelegate GetShortestServiceUUID:svcId];
    }
    if (NULL != charId)
    {
        characteristic = [CBUUID UUIDWithData:[NSData dataWithBytes:charId->bytes length:sizeof(charId->bytes)]];
    }
    if (NULL != pBuf)
    {
        data = [NSData dataWithBytes:pBuf->Start() length:pBuf->DataLength()];
    }

    result = [mBleDelegate SendWriteRequest:(__bridge id)connObj serivce:service characteristic:characteristic data:data];

exit:
    // Release delegate's reference to pBuf. pBuf will be freed when both platform delegate and Weave stack free their references to it.
    // We release pBuf's reference here since its payload bytes were copied into a new NSData object
    PacketBuffer::Free(pBuf);

    return result;
}

/**
    @note
      This method is only called from BleLayer, under Weave task context
 */
bool BleDelegateTrampoline::SendReadRequest(BLE_CONNECTION_OBJECT connObj, const WeaveBleUUID *svcId, const WeaveBleUUID *charId, PacketBuffer *pBuf)
{
    bool result = false;
    CBUUID * service = nil;
    CBUUID * characteristic = nil;
    NSData * data = nil;

    VerifyOrExit(mBleDelegate, WDM_LOG_ERROR(@"BLE not configured properly\n"));

    if (NULL != svcId)
    {
        service = [NLWeaveBleDelegate GetShortestServiceUUID:svcId];
    }
    if (NULL != charId)
    {
        characteristic = [CBUUID UUIDWithData:[NSData dataWithBytes:charId->bytes length:sizeof(charId->bytes)]];
    }
    if (NULL != pBuf)
    {
        data = [NSData dataWithBytes:pBuf->Start() length:pBuf->DataLength()];
    }

    // Release delegate's reference to pBuf. pBuf will be freed when both platform delegate and Weave stack free their references to it.
    // We release pBuf's reference here since its payload bytes were copied into a new NSData object
    PacketBuffer::Free(pBuf);

    result = [mBleDelegate SendReadRequest:(__bridge id)connObj serivce:service characteristic:characteristic data:data];

exit:
    // Release delegate's reference to pBuf. pBuf will be freed when both platform delegate and Weave stack free their references to it.
    // We release pBuf's reference here since its payload bytes were copied into a new NSData object
    PacketBuffer::Free(pBuf);

    return result;
}

/**
    @note
      This method is only called from BleLayer, under Weave task context
 */
bool BleDelegateTrampoline::SendReadResponse(BLE_CONNECTION_OBJECT connObj, BLE_READ_REQUEST_CONTEXT requestContext, const WeaveBleUUID *svcId, const WeaveBleUUID *charId)
{
    bool result = false;
    CBUUID * service = nil;
    CBUUID * characteristic = nil;

    VerifyOrExit(mBleDelegate, WDM_LOG_ERROR(@"BLE not configured properly\n"));

    if (NULL != svcId)
    {
        service = [NLWeaveBleDelegate GetShortestServiceUUID:svcId];
    }
    if (NULL != charId)
    {
        characteristic = [CBUUID UUIDWithData:[NSData dataWithBytes:charId->bytes length:sizeof(charId->bytes)]];
    }

    result = [mBleDelegate SendReadResponse:(__bridge id)connObj requestContext:(__bridge id)requestContext serivce:service characteristic:characteristic];

exit:
    return result;
}

/**
    @note
      This method is only called from BleLayer, under Weave task context
 */
void BleDelegateTrampoline::NotifyWeaveConnectionClosed(BLE_CONNECTION_OBJECT connObj)
{
    VerifyOrExit(mBleDelegate, WDM_LOG_ERROR(@"BLE not configured properly\n"));

    [mBleDelegate NotifyWeaveConnectionClosed:(__bridge id)connObj];

exit:
    ;
}

} /* namespace Ble */
} /* namespace nl*/


@implementation NLWeaveBleDelegate

- (instancetype)initDummyDelegate
{
    self = [super init];
    VerifyOrExit(self, WDM_LOG_ERROR(@"Memory allocation failure\n"));

    _mTrampoline = new nl::Ble::BleDelegateTrampoline (NULL);

exit:
    return self;
}

- (instancetype)init:(dispatch_queue_t)cbWorkQueue
{
    self = [super init];
    VerifyOrExit(self, WDM_LOG_ERROR(@"Memory allocation failure\n"));

    _mTrampoline = new nl::Ble::BleDelegateTrampoline (self);

    _mCbWorkQueue = cbWorkQueue;

    _mMapFromPeripheralToDm = [NSMapTable strongToWeakObjectsMapTable];

    _mWeaveServiceUUID = [NLWeaveBleDelegate GetShortestServiceUUID:&nl::Ble::WEAVE_BLE_SVC_ID];

exit:
    return self;
}

- (void)dealloc
{
    // This method can only be called by ARC
    WDM_LOG_METHOD_SIG();

    delete _mTrampoline;
    _mTrampoline = NULL;
}

/**
 @note
 This method is only called from BleLayer, under Weave task context
 */
- (nl::Ble::BlePlatformDelegate *)GetPlatformDelegate
{
    return _mTrampoline;
}

/**
 @note
 This method is only called from BleLayer, under Weave task context
 */
- (nl::Ble::BleApplicationDelegate *)GetApplicationDelegate
{
    return _mTrampoline;
}

-(void) SetBleLayer:(nl::Ble::BleLayer *)BleLayer
{
    _mBleLayer = BleLayer;
}

+ (CBUUID *)GetShortestServiceUUID:(const nl::Ble::WeaveBleUUID *)svcId
{
    // shorten the 16-byte UUID reported by BLE Layer to shortest possible, 2 or 4 bytes
    // this is the BLE Service UUID Base. If a 16-byte service UUID partially matches with this 12 bytes,
    // it could be shorten to 2 or 4 bytes.
    static const uint8_t bleBaseUUID[12] = { 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB };
    if (0 == memcmp(svcId->bytes + 4, bleBaseUUID, sizeof(bleBaseUUID)))
    {
        // okay, let's try to shorten it
        if ((0 == svcId->bytes[0]) && (0 == svcId->bytes[1]))
        {
            // the highest 2 bytes are both 0, so we just need 2 bytes
            return [CBUUID UUIDWithData:[NSData dataWithBytes:(svcId->bytes + 2) length:2]];
        }
        else
        {
            // we need to use 4 bytes
            return [CBUUID UUIDWithData:[NSData dataWithBytes:svcId->bytes length:4]];
        }
    }
    else
    {
        // it cannot be shorten as it doesn't match with the BLE Service UUID Base
        return [CBUUID UUIDWithData:[NSData dataWithBytes:svcId->bytes length:16]];
    }
}

/**
 * private static method to copy service and characteristic UUIDs from CBCharacteristic to a pair of WeaveBleUUID objects.
 * this is used in calls into Weave layer to decouple it from CoreBluetooth
 *
 * @param[in] characteristic the source characteristic
 * @param[in] svcId the destination service UUID
 * @param[in] charId the destination characteristic UUID
 *
 */
+ (void)fillServiceWithCharacteristicUuids:(CBCharacteristic*)characteristic svcId:(nl::Ble::WeaveBleUUID *)svcId charId:(nl::Ble::WeaveBleUUID *)charId
{
    static const size_t FullUUIDLength = 16;
    if ((FullUUIDLength != sizeof(charId->bytes)) ||
        (FullUUIDLength != sizeof(svcId->bytes)) ||
        (FullUUIDLength != characteristic.UUID.data.length))
    {
        // we're dead. we expect the data length to be the same (16-byte) across the board
        WDM_LOG_ERROR(@"[%s] UUID of characteristic is incompatible", __PRETTY_FUNCTION__);
        return;
    }

    memcpy(charId->bytes, characteristic.UUID.data.bytes, sizeof(charId->bytes));
    memset(svcId->bytes, 0, sizeof(svcId->bytes));

    // Expand service UUID back to 16-byte long as that's what the BLE Layer expects
    // this is a buffer pre-filled with BLE service UUID Base
    // byte 0 to 3 are reserved for shorter versions of BLE service UUIDs
    // For 4-byte service UUIDs, all bytes from 0 to 3 are used
    // For 2-byte service UUIDs, byte 0 and 1 shall be 0
    uint8_t serviceFullUUID[FullUUIDLength] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB };

    switch (characteristic.service.UUID.data.length)
    {
        case 2:
            // copy the 2-byte service UUID onto the right offset
            memcpy(serviceFullUUID + 2, characteristic.service.UUID.data.bytes, 2);
            break;
        case 4:
            // flow through
        case 16:
            memcpy(serviceFullUUID, characteristic.service.UUID.data.bytes, characteristic.service.UUID.data.length);
            break;
        default:
            // we're dead. we expect the data length to be the same (16-byte) across the board
            WDM_LOG_ERROR(@"[%s] Service UUIDs are incompatible", __PRETTY_FUNCTION__);
    }
    memcpy(svcId->bytes, serviceFullUUID, sizeof(svcId->bytes));
}

// This is only used by NLWeaveDeviceManager
- (void)prepareNewBleConnection:(NLWeaveDeviceManager*)dm
{
    WDM_LOG_DEBUG(@"Mapping device manager %@ to peripheral %@", dm, dm.blePeripheral);

    dispatch_async(_mCbWorkQueue, ^{
        [_mMapFromPeripheralToDm setObject:dm forKey:dm.blePeripheral];

        [dm.blePeripheral discoverServices:@[_mWeaveServiceUUID]];
    });
}

- (bool) isPeripheralValid:(CBPeripheral*)peripheral
{
    NLWeaveDeviceManager * dm = [_mMapFromPeripheralToDm objectForKey:peripheral];
    return (dm != nil) ? true : false;
}

- (void)forceBleDisconnect_Sync:(CBPeripheral*)peripheral
{
    WDM_LOG_METHOD_SIG();

    // force BleLayer to forget about this connObj
     _mBleLayer->HandleConnectionError((__bridge void*)peripheral, BLE_ERROR_REMOTE_DEVICE_DISCONNECTED);
}

- (void)notifyBleDisconnected:(CBPeripheral*)peripheral
{
    WDM_LOG_METHOD_SIG();

    NLWeaveDeviceManager * dm = [_mMapFromPeripheralToDm objectForKey:peripheral];
    if (dm == nil)
    {
        WDM_LOG_ERROR(@"[%s] Cannot find a matching device manager for peripheral %@", __PRETTY_FUNCTION__, peripheral);
    }

    if (dm == nil || dm.BleConnectionPreparationCompleteHandler == nil)
    {
        dispatch_async([[NLWeaveStack sharedStack] WorkQueue], ^{
        _mBleLayer->HandleConnectionError((__bridge void*)peripheral, BLE_ERROR_REMOTE_DEVICE_DISCONNECTED);
        });
    }
    else
    {
        dispatch_async([[NLWeaveStack sharedStack] WorkQueue], ^{
            PreparationCompleteHandler handler = dm.BleConnectionPreparationCompleteHandler;
            dm.BleConnectionPreparationCompleteHandler = nil;
            handler(dm, BLE_ERROR_REMOTE_DEVICE_DISCONNECTED);
        });
    }
}

/**
 * part of CBPeripheralDelegate.
 * called after service discovery is done. report failure to attach completed block on error, otherwise
 * proceed with characteristic discovery
 */
- (void)peripheral:(CBPeripheral *)peripheral
didDiscoverServices:(NSError *)error
{
    // we're in Core Bluetooth work queue

    WDM_LOG_METHOD_SIG();

    NLWeaveDeviceManager * dm = [_mMapFromPeripheralToDm objectForKey:peripheral];
    if (dm == nil)
    {
        WDM_LOG_ERROR(@"[%s] Cannot find a matching device manager for peripheral %@", __PRETTY_FUNCTION__, peripheral);
        return;
    }

    bool found = false;

    if (nil != error)
    {
        WDM_LOG_ERROR(@"[%s] BLE:Error finding Weave Service in the device: [%@]", __PRETTY_FUNCTION__, error.localizedDescription);
    }
    else
    {
        for (CBService *service in peripheral.services)
        {
            WDM_LOG_DEBUG(@"Found service in device: %@", service.UUID);

            if ([service.UUID.data isEqualToData:_mWeaveServiceUUID.data])
            {
                found = true;

                [peripheral
                 discoverCharacteristics:nil
                 forService:service];

                break;
            }
        }
    }

    if (!found)
    {
        WDM_LOG_ERROR(@"[%s] Cannot find Weave service on peripheral %@", __PRETTY_FUNCTION__, peripheral);
        dispatch_async([[NLWeaveStack sharedStack] WorkQueue], ^{
            PreparationCompleteHandler handler = dm.BleConnectionPreparationCompleteHandler;
            dm.BleConnectionPreparationCompleteHandler = nil;
            handler(dm, BLE_ERROR_NOT_WEAVE_DEVICE);
        });
    }
}

/**
 * part of CBPeripheralDelegate.
 * called after characteristic discovery is done. execute attach completed block and report error or success
 */
- (void)peripheral:(CBPeripheral *)peripheral
didDiscoverCharacteristicsForService:(CBService *)service
             error:(NSError *)error
{
    // we're in Core Bluetooth work queue

    WDM_LOG_METHOD_SIG();

    NLWeaveDeviceManager * dm = [_mMapFromPeripheralToDm objectForKey:peripheral];
    if (dm == nil)
    {
        WDM_LOG_ERROR(@"[%s] Cannot find a matching device manager for peripheral %@", __PRETTY_FUNCTION__, peripheral);
        return;
    }

    WEAVE_ERROR err = WEAVE_NO_ERROR;
    if (nil != error)
    {
        WDM_LOG_ERROR(@"[%s] BLE:Error finding Characteristics in Weave service on the device: [%@]", __PRETTY_FUNCTION__, error.localizedDescription);
        err = BLE_ERROR_NOT_WEAVE_DEVICE;
    }

    // we're good to go
    dispatch_async([[NLWeaveStack sharedStack] WorkQueue], ^{
        PreparationCompleteHandler handler = dm.BleConnectionPreparationCompleteHandler;
        dm.BleConnectionPreparationCompleteHandler = nil;
        handler(dm, err);
    });
}

/**
 * part of CBPeripheralDelegate.
 * called after writing completes. call BleLayer for both error and success
 */
- (void)peripheral:(CBPeripheral *)peripheral
didWriteValueForCharacteristic:(CBCharacteristic *)characteristic
             error:(NSError *)error
{
    // we're in Core Bluetooth work queue

#if LOCAL_VERBOSE_LOG
    WDM_LOG_METHOD_SIG();
#endif

    NLWeaveDeviceManager * dm = [_mMapFromPeripheralToDm objectForKey:peripheral];
    if (dm == nil)
    {
        WDM_LOG_ERROR(@"[%s] Cannot find a matching device manager for peripheral %@", __PRETTY_FUNCTION__, peripheral);
        return;
    }

    if (nil == error)
    {
        dispatch_async([[NLWeaveStack sharedStack] WorkQueue], ^{
            nl::Ble::WeaveBleUUID svcId;
            nl::Ble::WeaveBleUUID charId;
            [NLWeaveBleDelegate fillServiceWithCharacteristicUuids:characteristic svcId:&svcId charId:&charId];
            _mBleLayer->HandleWriteConfirmation((__bridge void*)peripheral, &svcId, &charId);
        });
    }
    else
    {
        WDM_LOG_ERROR(@"[%s] BLE:Error writing Characteristics in Weave service on the device: [%@]", __PRETTY_FUNCTION__, error.localizedDescription);

        dispatch_async([[NLWeaveStack sharedStack] WorkQueue], ^{
            _mBleLayer->HandleConnectionError((__bridge void*)peripheral, BLE_ERROR_GATT_WRITE_FAILED);
        });
    }
}

/**
 * part of CBPeripheralDelegate.
 * called after characteristic subscription update is done. call BleLayer for both error and success
 */
- (void)peripheral:(CBPeripheral *)peripheral
didUpdateNotificationStateForCharacteristic:(CBCharacteristic *)characteristic
             error:(NSError *)error
{
    // we're in Core Bluetooth work queue

    WDM_LOG_METHOD_SIG();

    NLWeaveDeviceManager * dm = [_mMapFromPeripheralToDm objectForKey:peripheral];
    if (dm == nil)
    {
        WDM_LOG_ERROR(@"[%s] Cannot find a matching device manager for peripheral %@", __PRETTY_FUNCTION__, peripheral);
        return;
    }

    bool isNotifying = characteristic.isNotifying;

    if (nil == error)
    {
        dispatch_async([[NLWeaveStack sharedStack] WorkQueue], ^{
            nl::Ble::WeaveBleUUID svcId;
            nl::Ble::WeaveBleUUID charId;
            [NLWeaveBleDelegate fillServiceWithCharacteristicUuids:characteristic svcId:&svcId charId:&charId];

            if (isNotifying)
            {
                _mBleLayer->HandleSubscribeComplete((__bridge void*)peripheral, &svcId, &charId);
            }
            else
            {
                _mBleLayer->HandleUnsubscribeComplete((__bridge void*)peripheral, &svcId, &charId);
            }
        });
    }
    else
    {
        WDM_LOG_ERROR(@"[%s] BLE:Error subscribing/unsubcribing some characteristic on the device: [%@]", __PRETTY_FUNCTION__, error.localizedDescription);

        dispatch_async([[NLWeaveStack sharedStack] WorkQueue], ^{
            if (isNotifying)
            {
                // we're still notifying, so we must failed the unsubscription
                _mBleLayer->HandleConnectionError((__bridge void*)peripheral, BLE_ERROR_GATT_UNSUBSCRIBE_FAILED);
            }
            else
            {
                // we're not notifying, so we must failed the subscription
                _mBleLayer->HandleConnectionError((__bridge void*)peripheral, BLE_ERROR_GATT_SUBSCRIBE_FAILED);
            }
        });
    }
}

/**
 * part of CBPeripheralDelegate.
 * called when indication of some characteristic arrives. call BleLayer for both error and success
 */
- (void)peripheral:(CBPeripheral *)peripheral
didUpdateValueForCharacteristic:(CBCharacteristic *)characteristic
             error:(NSError *)error
{
    // we're in Core Bluetooth work queue

#if LOCAL_VERBOSE_LOG
    WDM_LOG_METHOD_SIG();
#endif

    NLWeaveDeviceManager * dm = [_mMapFromPeripheralToDm objectForKey:peripheral];
    if (dm == nil)
    {
        WDM_LOG_ERROR(@"[%s] Cannot find a matching device manager for peripheral %@", __PRETTY_FUNCTION__, peripheral);
        return;
    }

    if (nil == error)
    {
        dispatch_async([[NLWeaveStack sharedStack] WorkQueue], ^{
            nl::Ble::WeaveBleUUID svcId;
            nl::Ble::WeaveBleUUID charId;
            [NLWeaveBleDelegate fillServiceWithCharacteristicUuids:characteristic svcId:&svcId charId:&charId];

            PacketBuffer *msgBuf;
            // build a inet buffer from the rxEv and send to blelayer.
            msgBuf = PacketBuffer::New();

            if (NULL != msgBuf)
            {
                memcpy(msgBuf->Start(), characteristic.value.bytes, characteristic.value.length);
                msgBuf->SetDataLength(characteristic.value.length);

                if (!_mBleLayer->HandleIndicationReceived((__bridge void*)peripheral, &svcId, &charId, msgBuf))
                {
                    // since this error comes from device manager core
                    // we assume it would do the right thing, like closing the connection
                    WDM_LOG_ERROR(@"[%s] Failed at handling incoming BLE data", __PRETTY_FUNCTION__);
                }
            }
            else
            {
                WDM_LOG_ERROR(@"[%s] Failed at allocating buffer for incoming BLE data", __PRETTY_FUNCTION__);

                _mBleLayer->HandleConnectionError((__bridge void*)peripheral, WEAVE_ERROR_NO_MEMORY);
            }

        });
    }
    else
    {
        WDM_LOG_ERROR(@"[%s] BLE:Error receiving indication of Characteristics on the device: [%@]", __PRETTY_FUNCTION__, error.localizedDescription);

        dispatch_async([[NLWeaveStack sharedStack] WorkQueue], ^{
            _mBleLayer->HandleConnectionError((__bridge void*)peripheral, BLE_ERROR_GATT_INDICATE_FAILED);
        });
    }
}

/**
 @note
 This method is only called from BleLayer, under Weave task context
 */
- (bool)SubscribeCharacteristic:(id)connObj serivce:(const CBUUID *)svcId characteristic:(const CBUUID *)charId
{
    WDM_LOG_METHOD_SIG();

    dispatch_async(_mCbWorkQueue, ^{
        CBPeripheral * peripheral = (CBPeripheral*)connObj;
        NLWeaveDeviceManager * dm = [_mMapFromPeripheralToDm objectForKey:peripheral];

        bool found = false;

        for (CBService * service in dm.blePeripheral.services)
        {
            if ([service.UUID.data isEqualToData:svcId.data])
            {
                for (CBCharacteristic * characteristic in service.characteristics)
                {
                    if ([characteristic.UUID.data isEqualToData:charId.data])
                    {
                        found = true;
                        dispatch_async(_mCbWorkQueue, ^{
                            [dm.blePeripheral setNotifyValue:true forCharacteristic:characteristic];
                        });
                    }
                }
            }
        }

        if (!found)
        {
            WDM_LOG_ERROR(@"[%s] Target peripheral doesn't have the specified service or characteristic", __PRETTY_FUNCTION__);

            dispatch_async([[NLWeaveStack sharedStack] WorkQueue], ^{
                _mBleLayer->HandleConnectionError((__bridge void*)dm.blePeripheral, BLE_ERROR_NOT_WEAVE_DEVICE);
            });
        }
    });

    return true;
}

/**
 @note
 This method is only called from BleLayer, under Weave task context
 */
- (bool)UnsubscribeCharacteristic:(id)connObj serivce:(const CBUUID *)svcId characteristic:(const CBUUID *)charId
{
    WDM_LOG_METHOD_SIG();

    dispatch_async(_mCbWorkQueue, ^{
        CBPeripheral * peripheral = (CBPeripheral*)connObj;
        NLWeaveDeviceManager * dm = [_mMapFromPeripheralToDm objectForKey:peripheral];

        bool found = false;

        for (CBService * service in dm.blePeripheral.services)
        {
            if ([service.UUID.data isEqualToData:svcId.data])
            {
                for (CBCharacteristic * characteristic in service.characteristics)
                {
                    if ([characteristic.UUID.data isEqualToData:charId.data])
                    {
                        found = true;
                        dispatch_async(_mCbWorkQueue, ^{
                            [dm.blePeripheral setNotifyValue:false forCharacteristic:characteristic];
                        });
                    }
                }
            }
        }

        if (!found)
        {
            WDM_LOG_ERROR(@"[%s] Target peripheral doesn't have the specified service or characteristic", __PRETTY_FUNCTION__);

            dispatch_async([[NLWeaveStack sharedStack] WorkQueue], ^{
                _mBleLayer->HandleConnectionError((__bridge void*)dm.blePeripheral, BLE_ERROR_NOT_WEAVE_DEVICE);
            });
        }
    });

    return true;
}

/**
 @note
 This method is only called from BleLayer, under Weave task context
 */
- (bool)CloseConnection:(id)connObj
{
    WDM_LOG_METHOD_SIG();
    return true;
}

/**
 @note
 This method is only called from BleLayer, under Weave task context
 */
- (uint16_t)GetMTU:(id)connObj
{
    WDM_LOG_METHOD_SIG();
    return 0;
}

/**
 @note
 This method is only called from BleLayer, under Weave task context
 */
- (bool)SendIndication:(id)connObj serivce:(const CBUUID *)svcId characteristic:(const CBUUID *)charId data:(const NSData*)buf
{
    WDM_LOG_METHOD_SIG();
    return false;
}

/**
 @note
 This method is only called from BleLayer, under Weave task context
 */
- (bool)SendWriteRequest:(id)connObj serivce:(const CBUUID *)svcId characteristic:(CBUUID *)charId data:(NSData*)buf
{
#if LOCAL_VERBOSE_LOG
    WDM_LOG_METHOD_SIG();
#endif

    dispatch_async(_mCbWorkQueue, ^{
        CBPeripheral * peripheral = (CBPeripheral*)connObj;
        NLWeaveDeviceManager * dm = [_mMapFromPeripheralToDm objectForKey:peripheral];

        bool found = false;

        for (CBService * service in dm.blePeripheral.services)
        {
            if ([service.UUID.data isEqualToData:svcId.data])
            {
                for (CBCharacteristic * characteristic in service.characteristics)
                {
                    if ([characteristic.UUID.data isEqualToData:charId.data])
                    {
                        found = true;
                        dispatch_async(_mCbWorkQueue, ^{
                            [dm.blePeripheral writeValue:buf forCharacteristic:characteristic type:CBCharacteristicWriteWithResponse];
                        });
                    }
                }
            }
        }

        if (!found)
        {
            WDM_LOG_ERROR(@"[%s] Target peripheral doesn't have the specified service or characteristic", __PRETTY_FUNCTION__);

            dispatch_async([[NLWeaveStack sharedStack] WorkQueue], ^{
                _mBleLayer->HandleConnectionError((__bridge void*)dm.blePeripheral, BLE_ERROR_NOT_WEAVE_DEVICE);
            });
        }
    });

    return true;
}

/**
 @note
 This method is only called from BleLayer, under Weave task context
 */
- (bool)SendReadRequest:(id)connObj serivce:(const CBUUID *)svcId characteristic:(const CBUUID *)charId data:(const NSData*)buf
{
    WDM_LOG_METHOD_SIG();
    return false;
}

/**
 @note
 This method is only called from BleLayer, under Weave task context
 */
- (bool)SendReadResponse:(id)connObj requestContext:(id)readContext serivce:(const CBUUID *)svcId characteristic:(const CBUUID *)charId
{
    WDM_LOG_METHOD_SIG();
    return false;
}

/**
 @note
 This method is only called from BleLayer, under Weave task context
 */
- (void)NotifyWeaveConnectionClosed:(id)connObj
{
    WDM_LOG_METHOD_SIG();
}

@end
