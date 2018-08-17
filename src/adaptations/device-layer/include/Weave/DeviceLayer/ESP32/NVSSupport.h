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
 *          Utilities for interacting with the the ESP32 "NVS" key-value store.
 */

#ifndef NVS_SUPPORT_H
#define NVS_SUPPORT_H

#include <Weave/DeviceLayer/internal/WeaveDeviceLayerInternal.h>

namespace nl {
namespace Weave {
namespace DeviceLayer {
namespace Internal {

struct NVSKey
{
    const char * Namespace;
    const char * Name;
};

struct NVSKeys
{
    using KeyT = NVSKey;

    static const NVSKey SerialNum;
    static const NVSKey DeviceId;
    static const NVSKey DeviceCert;
    static const NVSKey DevicePrivateKey;
    static const NVSKey ManufacturingDate;
    static const NVSKey PairingCode;
    static const NVSKey FabricId;
    static const NVSKey ServiceConfig;
    static const NVSKey PairedAccountId;
    static const NVSKey ServiceId;
    static const NVSKey FabricSecret;
    static const NVSKey GroupKeyIndex;
    static const NVSKey LastUsedEpochKeyId;
    static const NVSKey FailSafeArmed;
    static const NVSKey WiFiStationSecType;
};

extern const char kNVSNamespace_WeaveFactory[];
extern const char kNVSNamespace_WeaveConfig[];
extern const char kNVSNamespace_WeaveCounters[];
extern const char kNVSGroupKeyNamePrefix[];

// Maximum length of an NVS key name representing a Weave group key.  This name
// can either be 'fabric-secret' or 'gk-XXXXXXXX'.
constexpr size_t kMaxGroupKeyNameLength = 13;

extern WEAVE_ERROR ReadNVSValue(NVSKey key, char * buf, size_t bufSize, size_t & outLen);
extern WEAVE_ERROR ReadNVSValue(NVSKey key, uint32_t & val);
extern WEAVE_ERROR ReadNVSValue(NVSKey key, uint64_t & val);
extern WEAVE_ERROR ReadNVSValueBin(NVSKey key, uint8_t * buf, size_t bufSize, size_t & outLen);
extern WEAVE_ERROR WriteNVSValue(NVSKey key, const char * str);
extern WEAVE_ERROR WriteNVSValue(NVSKey key, const char * str, size_t strLen);
extern WEAVE_ERROR WriteNVSValue(NVSKey key, uint32_t val);
extern WEAVE_ERROR WriteNVSValue(NVSKey key, uint64_t val);
extern WEAVE_ERROR WriteNVSValueBin(NVSKey key, const uint8_t * data, size_t dataLen);
extern WEAVE_ERROR ClearNVSValue(NVSKey key);
extern WEAVE_ERROR ClearNVSNamespace(const char * ns);
extern WEAVE_ERROR EnsureNVSNamespace(const char * ns);

} // namespace Internal
} // namespace DeviceLayer
} // namespace Weave
} // namespace nl

#endif // NVS_SUPPORT_H
