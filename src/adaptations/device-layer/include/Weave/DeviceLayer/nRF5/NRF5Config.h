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
 *          Utilities for interacting with the Nordic Flash Data Storage (FDS) API.
 */

#ifndef NRF5_CONFIG_H
#define NRF5_CONFIG_H

#include <Weave/DeviceLayer/internal/WeaveDeviceLayerInternal.h>

namespace nl {
namespace Weave {
namespace DeviceLayer {
namespace Internal {

constexpr inline uint32_t MakeKey(uint16_t fileId, uint16_t recordId)
{
    return static_cast<uint32_t>(fileId) << 16 | recordId;
}

inline uint16_t FileIdFromKey(uint32_t key)
{
    return static_cast<uint16_t>(key >> 16);
}

inline uint16_t RecordIdFromKey(uint32_t key)
{
    return static_cast<uint16_t>(key);
}


/**
 * Provides functions and definitions for accessing device configuration information on the ESP32.
 *
 * This class is designed to be mixed-in to concrete implementation classes as a means to
 * provide access to configuration information to generic base classes.
 */
class NRF5Config
{
public:

    typedef uint32_t Key;

    // Nordic FDS File Ids
    static constexpr uint16_t kConfigFileId_WeaveFactory    = 0x235A;
    static constexpr uint16_t kConfigFileId_WeaveConfig     = 0x235B;
    static constexpr uint16_t kConfigFileId_WeaveCounters   = 0x235C;

    // Key definitions for well-known keys.
    static constexpr Key kConfigKey_SerialNum               = MakeKey(kConfigFileId_WeaveFactory, 0x0001);
    static constexpr Key kConfigKey_DeviceId                = MakeKey(kConfigFileId_WeaveFactory, 0x0001);
    static constexpr Key kConfigKey_DeviceCert              = MakeKey(kConfigFileId_WeaveFactory, 0x0001);
    static constexpr Key kConfigKey_DevicePrivateKey        = MakeKey(kConfigFileId_WeaveFactory, 0x0001);
    static constexpr Key kConfigKey_ManufacturingDate       = MakeKey(kConfigFileId_WeaveFactory, 0x0001);
    static constexpr Key kConfigKey_PairingCode             = MakeKey(kConfigFileId_WeaveFactory, 0x0001);
    static constexpr Key kConfigKey_FabricId                = MakeKey(kConfigFileId_WeaveConfig,  0x0001);
    static constexpr Key kConfigKey_ServiceConfig           = MakeKey(kConfigFileId_WeaveConfig,  0x0001);
    static constexpr Key kConfigKey_PairedAccountId         = MakeKey(kConfigFileId_WeaveConfig,  0x0001);
    static constexpr Key kConfigKey_ServiceId               = MakeKey(kConfigFileId_WeaveConfig,  0x0001);
    static constexpr Key kConfigKey_FabricSecret            = MakeKey(kConfigFileId_WeaveConfig,  0x0001);
    static constexpr Key kConfigKey_GroupKeyIndex           = MakeKey(kConfigFileId_WeaveConfig,  0x0001);
    static constexpr Key kConfigKey_LastUsedEpochKeyId      = MakeKey(kConfigFileId_WeaveConfig,  0x0001);
    static constexpr Key kConfigKey_FailSafeArmed           = MakeKey(kConfigFileId_WeaveConfig,  0x0001);

    static constexpr uint16_t kGroupKeyRecordIdBase         = 0x8000;

    // Config value accessors.
    static WEAVE_ERROR ReadConfigValue(Key key, bool & val);
    static WEAVE_ERROR ReadConfigValue(Key key, uint32_t & val);
    static WEAVE_ERROR ReadConfigValue(Key key, uint64_t & val);
    static WEAVE_ERROR ReadConfigValueStr(Key key, char * buf, size_t bufSize, size_t & outLen);
    static WEAVE_ERROR ReadConfigValueBin(Key key, uint8_t * buf, size_t bufSize, size_t & outLen);
    static WEAVE_ERROR WriteConfigValue(Key key, bool val);
    static WEAVE_ERROR WriteConfigValue(Key key, uint32_t val);
    static WEAVE_ERROR WriteConfigValue(Key key, uint64_t val);
    static WEAVE_ERROR WriteConfigValueStr(Key key, const char * str);
    static WEAVE_ERROR WriteConfigValueStr(Key key, const char * str, size_t strLen);
    static WEAVE_ERROR WriteConfigValueBin(Key key, const uint8_t * data, size_t dataLen);
    static WEAVE_ERROR ClearConfigValue(Key key);
    static bool ConfigValueExists(Key key);
};


} // namespace Internal
} // namespace DeviceLayer
} // namespace Weave
} // namespace nl

#endif // NRF5_CONFIG_H
