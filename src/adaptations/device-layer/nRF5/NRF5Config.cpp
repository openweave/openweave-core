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

#include <Weave/DeviceLayer/internal/WeaveDeviceLayerInternal.h>
#include <Weave/DeviceLayer/nRF5/NRF5Config.h>

namespace nl {
namespace Weave {
namespace DeviceLayer {
namespace Internal {

WEAVE_ERROR NRF5Config::ReadConfigValue(Key key, bool & val)
{
    // TODO: implement me
    return WEAVE_DEVICE_ERROR_CONFIG_NOT_FOUND;
}

WEAVE_ERROR NRF5Config::ReadConfigValue(Key key, uint32_t & val)
{
    // TODO: implement me
    return WEAVE_DEVICE_ERROR_CONFIG_NOT_FOUND;
}

WEAVE_ERROR NRF5Config::ReadConfigValue(Key key, uint64_t & val)
{
    // TODO: implement me
    return WEAVE_DEVICE_ERROR_CONFIG_NOT_FOUND;
}

WEAVE_ERROR NRF5Config::ReadConfigValueStr(Key key, char * buf, size_t bufSize, size_t & outLen)
{
    // TODO: implement me
    return WEAVE_DEVICE_ERROR_CONFIG_NOT_FOUND;
}

WEAVE_ERROR NRF5Config::ReadConfigValueBin(Key key, uint8_t * buf, size_t bufSize, size_t & outLen)
{
    // TODO: implement me
    return WEAVE_DEVICE_ERROR_CONFIG_NOT_FOUND;
}

WEAVE_ERROR NRF5Config::WriteConfigValue(Key key, bool val)
{
    // TODO: implement me
    return WEAVE_NO_ERROR;
}

WEAVE_ERROR NRF5Config::WriteConfigValue(Key key, uint32_t val)
{
    // TODO: implement me
    return WEAVE_NO_ERROR;
}

WEAVE_ERROR NRF5Config::WriteConfigValue(Key key, uint64_t val)
{
    // TODO: implement me
    return WEAVE_NO_ERROR;
}

WEAVE_ERROR NRF5Config::WriteConfigValueStr(Key key, const char * str)
{
    // TODO: implement me
    return WEAVE_NO_ERROR;
}

WEAVE_ERROR NRF5Config::WriteConfigValueStr(Key key, const char * str, size_t strLen)
{
    // TODO: implement me
    return WEAVE_NO_ERROR;
}

WEAVE_ERROR NRF5Config::WriteConfigValueBin(Key key, const uint8_t * data, size_t dataLen)
{
    // TODO: implement me
    return WEAVE_NO_ERROR;
}

WEAVE_ERROR NRF5Config::ClearConfigValue(Key key)
{
    // TODO: implement me
    return WEAVE_NO_ERROR;
}

bool NRF5Config::ConfigValueExists(Key key)
{
    // TODO: implement me
    return false;
}

} // namespace Internal
} // namespace DeviceLayer
} // namespace Weave
} // namespace nl
