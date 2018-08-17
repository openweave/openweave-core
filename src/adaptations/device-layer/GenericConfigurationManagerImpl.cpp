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
 *          Provides the implementation of the Device Layer ConfigurationManager object
 *          for the ESP32.
 */

#include <Weave/DeviceLayer/internal/WeaveDeviceLayerInternal.h>
#include <Weave/DeviceLayer/internal/GenericConfigurationManagerImpl.h>


namespace nl {
namespace Weave {
namespace DeviceLayer {
namespace Internal {

template<class ImplClass, class KeyMap>
WEAVE_ERROR GenericConfigStoreImpl<ImplClass, KeyMap>::_GetFirmwareRevision(char * buf, size_t bufSize, size_t & outLen)
{
#ifdef WEAVE_DEVICE_CONFIG_DEVICE_FIRMWARE_REVISION
    if (WEAVE_DEVICE_CONFIG_DEVICE_FIRMWARE_REVISION[0] != 0)
    {
        outLen = min(bufSize, sizeof(WEAVE_DEVICE_CONFIG_DEVICE_FIRMWARE_REVISION) - 1);
        memcpy(buf, WEAVE_DEVICE_CONFIG_DEVICE_FIRMWARE_REVISION, outLen);
        return WEAVE_NO_ERROR;
    }
    else
#endif // WEAVE_DEVICE_CONFIG_DEVICE_FIRMWARE_REVISION
    {
        outLen = 0;
        return WEAVE_DEVICE_ERROR_CONFIG_NOT_FOUND;
    }
}

template<class ImplClass, class KeyMap>
WEAVE_ERROR GenericConfigStoreImpl<ImplClass, KeyMap>::_GetFirmwareBuildTime(uint16_t & year, uint8_t & month, uint8_t & dayOfMonth,
        uint8_t & hour, uint8_t & minute, uint8_t & second)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    const char * buildDateStr = __DATE__; // e.g. Feb 12 1996
    const char * buildTimeStr = __TIME__; // e.g. 23:59:01
    char monthStr[4];
    char * p;

    static const char months[] = "JanFebMarAprMayJunJulAugSepOctNovDec";

    memcpy(monthStr, buildDateStr, 3);
    monthStr[3] = 0;

    p = strstr(months, monthStr);
    VerifyOrExit(p != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);

    month = ((p - months) / 3) + 1;

    dayOfMonth = strtoul(buildDateStr + 4, &p, 10);
    VerifyOrExit(p == buildDateStr + 6, err = WEAVE_ERROR_INVALID_ARGUMENT);

    year = strtoul(buildDateStr + 7, &p, 10);
    VerifyOrExit(p == buildDateStr + 11, err = WEAVE_ERROR_INVALID_ARGUMENT);

    hour = strtoul(buildTimeStr, &p, 10);
    VerifyOrExit(p == buildTimeStr + 2, err = WEAVE_ERROR_INVALID_ARGUMENT);

    minute = strtoul(buildTimeStr + 3, &p, 10);
    VerifyOrExit(p == buildTimeStr + 5, err = WEAVE_ERROR_INVALID_ARGUMENT);

    second = strtoul(buildTimeStr + 6, &p, 10);
    VerifyOrExit(p == buildTimeStr + 8, err = WEAVE_ERROR_INVALID_ARGUMENT);

exit:
    return err;
}

template<class ImplClass, class KeyMap>
WEAVE_ERROR GenericConfigStoreImpl<ImplClass, KeyMap>::_GetDeviceId(uint64_t & deviceId)
{
    return Impl()->GetStoredValue(KeyMap::DeviceId, deviceId);
}

template<class ImplClass, class KeyMap>
WEAVE_ERROR GenericConfigStoreImpl<ImplClass, KeyMap>::_StoreDeviceId(uint64_t deviceId)
{
    return Impl()->StoreKeyValue(KeyMap::DeviceId, deviceId);
}

template<class ImplClass, class KeyMap>
WEAVE_ERROR GenericConfigStoreImpl<ImplClass, KeyMap>::_GetSerialNumber(char * buf, size_t bufSize, size_t & serialNumLen)
{
    return Impl()->GetStoredValue(KeyMap::SerialNum, buf, bufSize, serialNumLen);
}

template<class ImplClass, class KeyMap>
WEAVE_ERROR GenericConfigStoreImpl<ImplClass, KeyMap>::_StoreSerialNumber(const char * serialNum)
{
    return Impl()->StoreKeyValue(KeyMap::SerialNum, serialNum);
}

template<class ImplClass, class KeyMap>
WEAVE_ERROR GenericConfigStoreImpl<ImplClass, KeyMap>::_GetManufacturingDate(uint16_t& year, uint8_t& month, uint8_t& dayOfMonth)
{
    WEAVE_ERROR err;
    enum {
        kDateStringLength = 10 // YYYY-MM-DD
    };
    char dateStr[kDateStringLength + 1];
    size_t dateLen;
    char *parseEnd;

    err = Impl()->GetStoredValue(KeyMap::ManufacturingDate, dateStr, sizeof(dateStr), dateLen);
    SuccessOrExit(err);

    VerifyOrExit(dateLen == kDateStringLength, err = WEAVE_ERROR_INVALID_ARGUMENT);

    year = strtoul(dateStr, &parseEnd, 10);
    VerifyOrExit(parseEnd == dateStr + 4, err = WEAVE_ERROR_INVALID_ARGUMENT);

    month = strtoul(dateStr + 5, &parseEnd, 10);
    VerifyOrExit(parseEnd == dateStr + 7, err = WEAVE_ERROR_INVALID_ARGUMENT);

    dayOfMonth = strtoul(dateStr + 8, &parseEnd, 10);
    VerifyOrExit(parseEnd == dateStr + 10, err = WEAVE_ERROR_INVALID_ARGUMENT);

exit:
    if (err != WEAVE_NO_ERROR && err != WEAVE_DEVICE_ERROR_CONFIG_NOT_FOUND)
    {
        WeaveLogError(DeviceLayer, "Invalid manufacturing date: %s", dateStr);
    }
    return err;
}

template<class ImplClass, class KeyMap>
WEAVE_ERROR GenericConfigStoreImpl<ImplClass, KeyMap>::_StoreManufacturingDate(const char * mfgDate)
{
    return Impl()->StoreKeyValue(KeyMap::ManufacturingDate, mfgDate);
}

} // namespace Internal
} // namespace DeviceLayer
} // namespace Weave
} // namespace nl
