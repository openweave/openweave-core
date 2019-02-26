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
 *          General utility functions available on all platforms.
 */

#include <Weave/DeviceLayer/internal/WeaveDeviceLayerInternal.h>

namespace nl {
namespace Weave {
namespace DeviceLayer {

WEAVE_ERROR ParseCompilerDateStr(const char * dateStr, uint16_t & year, uint8_t & month, uint8_t & dayOfMonth)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    char monthStr[4];
    char * p;

    static const char months[] = "JanFebMarAprMayJunJulAugSepOctNovDec";

    VerifyOrExit(strlen(dateStr) == 11, err = WEAVE_ERROR_INVALID_ARGUMENT);

    memcpy(monthStr, dateStr, 3);
    monthStr[3] = 0;

    p = strstr(months, monthStr);
    VerifyOrExit(p != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);

    month = ((p - months) / 3) + 1;

    dayOfMonth = strtoul(dateStr + 4, &p, 10);
    VerifyOrExit(p == dateStr + 6, err = WEAVE_ERROR_INVALID_ARGUMENT);

    year = strtoul(dateStr + 7, &p, 10);
    VerifyOrExit(p == dateStr + 11, err = WEAVE_ERROR_INVALID_ARGUMENT);

exit:
    return err;
}

WEAVE_ERROR Parse24HourTimeStr(const char * timeStr, uint8_t & hour, uint8_t & minute, uint8_t & second)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    char * p;

    VerifyOrExit(strlen(timeStr) == 8, err = WEAVE_ERROR_INVALID_ARGUMENT);

    hour = strtoul(timeStr, &p, 10);
    VerifyOrExit(p == timeStr + 2, err = WEAVE_ERROR_INVALID_ARGUMENT);

    minute = strtoul(timeStr + 3, &p, 10);
    VerifyOrExit(p == timeStr + 5, err = WEAVE_ERROR_INVALID_ARGUMENT);

    second = strtoul(timeStr + 6, &p, 10);
    VerifyOrExit(p == timeStr + 8, err = WEAVE_ERROR_INVALID_ARGUMENT);

exit:
    return err;
}

const char * CharacterizeIPv6Address(const IPAddress & ipAddr)
{
    if (ipAddr.IsIPv6LinkLocal())
    {
        return "Link-local IPv6 address";
    }
    else if (ipAddr.IsIPv6ULA())
    {
        if (FabricState.FabricId != kFabricIdNotSpecified && ipAddr.GlobalId() == nl::Weave::WeaveFabricIdToIPv6GlobalId(FabricState.FabricId))
        {
            switch (ipAddr.Subnet())
            {
            case kWeaveSubnetId_PrimaryWiFi:
                return "Weave WiFi IPv6 ULA";
            case kWeaveSubnetId_Service:
                return "Weave Service IPv6 ULA";
            case kWeaveSubnetId_ThreadMesh:
                return "Weave Thread IPv6 ULA";
            case kWeaveSubnetId_ThreadAlarm:
                return "Weave Thread Alarm IPv6 ULA";
            case kWeaveSubnetId_WiFiAP:
                return "Weave WiFi AP IPv6 ULA";
            case kWeaveSubnetId_MobileDevice:
                return "Weave Mobile IPv6 ULA";
            default:
                return "Weave IPv6 ULA";
            }
        }
    }
    else if ((ntohl(ipAddr.Addr[0]) & 0xE0000000U) == 0x20000000U)
    {
        return "Global IPv6 address";
    }
    return "IPv6 address";
}

} // namespace DeviceLayer
} // namespace Weave
} // namespace nl
