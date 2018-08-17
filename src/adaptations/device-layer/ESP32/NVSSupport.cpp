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

#include <Weave/DeviceLayer/internal/WeaveDeviceLayerInternal.h>
#include <Weave/DeviceLayer/ESP32/NVSSupport.h>

#include "nvs_flash.h"
#include "nvs.h"

namespace nl {
namespace Weave {
namespace DeviceLayer {
namespace Internal {

// NVS namespaces used by the Weave Device Layer
const char kNVSNamespace_WeaveFactory[]      = "weave-factory";
const char kNVSNamespace_WeaveConfig[]       = "weave-config";
const char kNVSNamespace_WeaveCounters[]     = "weave-counters";


// Keys in the weave-factory namespace
const NVSKey NVSKeys::SerialNum            = { kNVSNamespace_WeaveFactory, "serial-num"         };
const NVSKey NVSKeys::DeviceId             = { kNVSNamespace_WeaveFactory, "device-id"          };
const NVSKey NVSKeys::DeviceCert           = { kNVSNamespace_WeaveFactory, "device-cert"        };
const NVSKey NVSKeys::DevicePrivateKey     = { kNVSNamespace_WeaveFactory, "device-key"         };
const NVSKey NVSKeys::ManufacturingDate    = { kNVSNamespace_WeaveFactory, "mfg-date"           };
const NVSKey NVSKeys::PairingCode          = { kNVSNamespace_WeaveFactory, "pairing-code"       };

// Keys in the weave-config namespace
const NVSKey NVSKeys::FabricId             = { kNVSNamespace_WeaveConfig,  "fabric-id"          };
const NVSKey NVSKeys::ServiceConfig        = { kNVSNamespace_WeaveConfig,  "service-config"     };
const NVSKey NVSKeys::PairedAccountId      = { kNVSNamespace_WeaveConfig,  "account-id"         };
const NVSKey NVSKeys::ServiceId            = { kNVSNamespace_WeaveConfig,  "service-id"         };
const NVSKey NVSKeys::FabricSecret         = { kNVSNamespace_WeaveConfig,  "fabric-secret"      };
const NVSKey NVSKeys::GroupKeyIndex        = { kNVSNamespace_WeaveConfig,  "group-key-index"    };
const NVSKey NVSKeys::LastUsedEpochKeyId   = { kNVSNamespace_WeaveConfig,  "last-ek-id"         };
const NVSKey NVSKeys::FailSafeArmed        = { kNVSNamespace_WeaveConfig,  "fail-safe-armed"    };
const NVSKey NVSKeys::WiFiStationSecType   = { kNVSNamespace_WeaveConfig,  "sta-sec-type"       };

// Prefix for NVS keys containing Weave group keys.
const char kNVSKeyName_GroupKeyPrefix[]      = "gk-";


WEAVE_ERROR ReadNVSValue(NVSKey key, char * buf, size_t bufSize, size_t & outLen)
{
    WEAVE_ERROR err;
    nvs_handle handle;
    bool needClose = false;

    err = nvs_open(key.Namespace, NVS_READONLY, &handle);
    SuccessOrExit(err);
    needClose = true;

    outLen = bufSize;
    err = nvs_get_str(handle, key.Name, buf, &outLen);
    if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        outLen = 0;
        err = WEAVE_DEVICE_ERROR_CONFIG_NOT_FOUND;
    }
    else if (err == ESP_ERR_NVS_INVALID_LENGTH)
    {
        err = WEAVE_ERROR_BUFFER_TOO_SMALL;
    }
    SuccessOrExit(err);

    outLen -= 1; // Don't count trailing nul.

exit:
    if (needClose)
    {
        nvs_close(handle);
    }

    return err;
}

WEAVE_ERROR ReadNVSValue(NVSKey key, uint32_t & val)
{
    WEAVE_ERROR err;
    nvs_handle handle;
    bool needClose = false;

    err = nvs_open(key.Namespace, NVS_READONLY, &handle);
    SuccessOrExit(err);
    needClose = true;

    err = nvs_get_u32(handle, key.Name, &val);
    if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        err = WEAVE_DEVICE_ERROR_CONFIG_NOT_FOUND;
    }
    SuccessOrExit(err);

exit:
    if (needClose)
    {
        nvs_close(handle);
    }
    return err;
}

WEAVE_ERROR ReadNVSValue(NVSKey key, uint64_t & val)
{
    WEAVE_ERROR err;
    nvs_handle handle;
    bool needClose = false;

    err = nvs_open(key.Namespace, NVS_READONLY, &handle);
    SuccessOrExit(err);
    needClose = true;

    err = nvs_get_u64(handle, key.Name, &val);
    if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        err = WEAVE_DEVICE_ERROR_CONFIG_NOT_FOUND;
    }
    SuccessOrExit(err);

exit:
    if (needClose)
    {
        nvs_close(handle);
    }
    return err;
}

WEAVE_ERROR ReadNVSValueBin(NVSKey key, uint8_t * buf, size_t bufSize, size_t & outLen)
{
    WEAVE_ERROR err;
    nvs_handle handle;
    bool needClose = false;

    err = nvs_open(key.Namespace, NVS_READONLY, &handle);
    SuccessOrExit(err);
    needClose = true;

    outLen = bufSize;
    err = nvs_get_blob(handle, key.Name, buf, &outLen);
    if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        outLen = 0;
        err = WEAVE_DEVICE_ERROR_CONFIG_NOT_FOUND;
    }
    else if (err == ESP_ERR_NVS_INVALID_LENGTH)
    {
        err = WEAVE_ERROR_BUFFER_TOO_SMALL;
    }
    SuccessOrExit(err);

exit:
    if (needClose)
    {
        nvs_close(handle);
    }

    return err;
}

WEAVE_ERROR WriteNVSValue(NVSKey key, const char * str)
{
    WEAVE_ERROR err;
    nvs_handle handle;
    bool needClose = false;

    if (str != NULL)
    {
        err = nvs_open(key.Namespace, NVS_READWRITE, &handle);
        SuccessOrExit(err);
        needClose = true;

        err = nvs_set_str(handle, key.Name, str);
        SuccessOrExit(err);

        // Commit the value to the persistent store.
        err = nvs_commit(handle);
        SuccessOrExit(err);

        WeaveLogProgress(DeviceLayer, "WriteNVSValue: %s/%s = \"%s\"", key.Namespace, key.Name, str);
    }

    else
    {
        err = ClearNVSValue(key);
        SuccessOrExit(err);
    }

exit:
    if (needClose)
    {
        nvs_close(handle);
    }

    return err;
}

WEAVE_ERROR WriteNVSValue(NVSKey key, const char * str, size_t strLen)
{
    WEAVE_ERROR err;
    char * strCopy = NULL;

    if (str != NULL)
    {
        strCopy = strndup(str, strLen);
        VerifyOrExit(dataCopy != NULL, err = WEAVE_ERROR_NO_MEMORY);
    }

    err = WriteNVSValue(key, strCopy);

exit:
    if (strCopy != NULL)
    {
        free(strCopy);
    }
    return err;
}

WEAVE_ERROR WriteNVSValue(NVSKey key, uint32_t val)
{
    WEAVE_ERROR err;
    nvs_handle handle;
    bool needClose = false;

    err = nvs_open(key.Namespace, NVS_READWRITE, &handle);
    SuccessOrExit(err);
    needClose = true;

    err = nvs_set_u32(handle, key.Name, val);
    SuccessOrExit(err);

    // Commit the value to the persistent store.
    err = nvs_commit(handle);
    SuccessOrExit(err);

    WeaveLogProgress(DeviceLayer, "WriteNVSValue: %s/%s = %" PRIu32 " (0x%" PRIX32 ")", key.Namespace, key.Name, val, val);

exit:
    if (needClose)
    {
        nvs_close(handle);
    }

    return err;
}

WEAVE_ERROR WriteNVSValue(NVSKey key, uint64_t val)
{
    WEAVE_ERROR err;
    nvs_handle handle;
    bool needClose = false;

    err = nvs_open(key.Namespace, NVS_READWRITE, &handle);
    SuccessOrExit(err);
    needClose = true;

    err = nvs_set_u64(handle, key.Name, val);
    SuccessOrExit(err);

    // Commit the value to the persistent store.
    err = nvs_commit(handle);
    SuccessOrExit(err);

    WeaveLogProgress(DeviceLayer, "WriteNVSValue: %s/%s = %" PRIu64 " (0x%" PRIX64 ")", key.Namespace, key.Name, val, val);

exit:
    if (needClose)
    {
        nvs_close(handle);
    }

    return err;
}

WEAVE_ERROR WriteNVSValueBin(NVSKey key, const uint8_t * data, size_t dataLen)
{
    WEAVE_ERROR err;
    nvs_handle handle;
    bool needClose = false;

    if (data != NULL)
    {
        err = nvs_open(key.Namespace, NVS_READWRITE, &handle);
        SuccessOrExit(err);
        needClose = true;

        err = nvs_set_blob(handle, key.Name, data, dataLen);
        SuccessOrExit(err);

        // Commit the value to the persistent store.
        err = nvs_commit(handle);
        SuccessOrExit(err);

        WeaveLogProgress(DeviceLayer, "WriteNVSValue: %s/%s = (blob length %" PRId32 ")", key.Namespace, key.Name, dataLen);
    }

    else
    {
        err = ClearNVSValue(key);
        SuccessOrExit(err);
    }

exit:
    if (needClose)
    {
        nvs_close(handle);
    }

    return err;
}

WEAVE_ERROR ClearNVSValue(NVSKey key)
{
    WEAVE_ERROR err;
    nvs_handle handle;
    bool needClose = false;

    err = nvs_open(key.Namespace, NVS_READWRITE, &handle);
    SuccessOrExit(err);
    needClose = true;

    err = nvs_erase_key(handle, key.Name);
    if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        ExitNow(err = WEAVE_NO_ERROR);
    }
    SuccessOrExit(err);

    // Commit the value to the persistent store.
    err = nvs_commit(handle);
    SuccessOrExit(err);

    WeaveLogProgress(DeviceLayer, "ClearNVSValue: %s/%s", key.Namespace, key.Name);

exit:
    if (needClose)
    {
        nvs_close(handle);
    }

    return err;
}

WEAVE_ERROR ClearNVSNamespace(const char * ns)
{
    WEAVE_ERROR err;
    nvs_handle handle;
    bool needClose = false;

    err = nvs_open(ns, NVS_READWRITE, &handle);
    SuccessOrExit(err);
    needClose = true;

    err = nvs_erase_all(handle);
    SuccessOrExit(err);

    err = nvs_commit(handle);
    SuccessOrExit(err);

exit:
    if (needClose)
    {
        nvs_close(handle);
    }
    return err;
}

WEAVE_ERROR EnsureNVSNamespace(const char * ns)
{
    WEAVE_ERROR err;
    nvs_handle handle;
    bool needClose = false;

    err = nvs_open(ns, NVS_READONLY, &handle);
    if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        err = nvs_open(ns, NVS_READWRITE, &handle);
        SuccessOrExit(err);
        needClose = true;

        err = nvs_commit(handle);
        SuccessOrExit(err);
    }
    SuccessOrExit(err);
    needClose = true;

exit:
    if (needClose)
    {
        nvs_close(handle);
    }
    return err;
}

} // namespace Internal
} // namespace DeviceLayer
} // namespace Weave
} // namespace nl
