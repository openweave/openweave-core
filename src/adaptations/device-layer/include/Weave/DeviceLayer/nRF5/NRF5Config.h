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

constexpr inline uint32_t NRF5ConfigKey(uint16_t fileId, uint16_t recordId)
{
    return static_cast<uint32_t>(fileId) << 16 | recordId;
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

    using Key = uint32_t;

    // Nordic FDS File Ids
    static constexpr uint16_t kConfigFileId_WeaveFactory    = 0x235A;
    static constexpr uint16_t kConfigFileId_WeaveConfig     = 0x235B;
    static constexpr uint16_t kConfigFileId_WeaveCounters   = 0x235C;

    // Key definitions for well-known keys.
    static constexpr uint32_t kConfigKey_SerialNum               = NRF5ConfigKey(kConfigFileId_WeaveFactory, 0x0001);
    static constexpr uint32_t kConfigKey_DeviceId                = NRF5ConfigKey(kConfigFileId_WeaveFactory, 0x0002);
    static constexpr uint32_t kConfigKey_DeviceCert              = NRF5ConfigKey(kConfigFileId_WeaveFactory, 0x0003);
    static constexpr uint32_t kConfigKey_DevicePrivateKey        = NRF5ConfigKey(kConfigFileId_WeaveFactory, 0x0004);
    static constexpr uint32_t kConfigKey_ManufacturingDate       = NRF5ConfigKey(kConfigFileId_WeaveFactory, 0x0005);
    static constexpr uint32_t kConfigKey_PairingCode             = NRF5ConfigKey(kConfigFileId_WeaveFactory, 0x0006);
    static constexpr uint32_t kConfigKey_FabricId                = NRF5ConfigKey(kConfigFileId_WeaveConfig,  0x0007);
    static constexpr uint32_t kConfigKey_ServiceConfig           = NRF5ConfigKey(kConfigFileId_WeaveConfig,  0x0008);
    static constexpr uint32_t kConfigKey_PairedAccountId         = NRF5ConfigKey(kConfigFileId_WeaveConfig,  0x0009);
    static constexpr uint32_t kConfigKey_ServiceId               = NRF5ConfigKey(kConfigFileId_WeaveConfig,  0x000A);
    static constexpr uint32_t kConfigKey_FabricSecret            = NRF5ConfigKey(kConfigFileId_WeaveConfig,  0x000B);
    static constexpr uint32_t kConfigKey_GroupKeyIndex           = NRF5ConfigKey(kConfigFileId_WeaveConfig,  0x000C);
    static constexpr uint32_t kConfigKey_LastUsedEpochKeyId      = NRF5ConfigKey(kConfigFileId_WeaveConfig,  0x000D);
    static constexpr uint32_t kConfigKey_FailSafeArmed           = NRF5ConfigKey(kConfigFileId_WeaveConfig,  0x000E);

    static constexpr uint16_t kGroupKeyRecordIdBase         = 0x8000;

    static WEAVE_ERROR Init();

    // General config value accessors.
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

    // Key utility functions
    static uint16_t GetFileId(uint32_t key);
    static uint16_t GetRecordKey(uint32_t key);
};

inline uint16_t NRF5Config::GetFileId(uint32_t key)
{
    return static_cast<uint16_t>(key >> 16);
}

inline uint16_t NRF5Config::GetRecordKey(uint32_t key)
{
    return static_cast<uint16_t>(key);
}


} // namespace Internal
} // namespace DeviceLayer
} // namespace Weave
} // namespace nl

#endif // NRF5_CONFIG_H
