/*
 *
 *    Copyright (c) 2019 Google LLC.
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

#ifndef WIRELESS_REG_CONFIG_H
#define WIRELESS_REG_CONFIG_H

#include <Weave/Core/WeaveCore.h>
#include <Weave/Support/FlagUtils.hpp>

namespace nl {
namespace Weave {
namespace Profiles {
namespace NetworkProvisioning {

/**
 * 2-character code identifying a wireless regulatory domain.
 */
struct WirelessRegDomain
{
    char Code[2];

    bool operator ==(const struct WirelessRegDomain & other) const;
    bool operator !=(const struct WirelessRegDomain & other) const;
    bool IsNull(void) const;
    bool IsWorldWide(void) const;

    static const WirelessRegDomain Null;
    static const WirelessRegDomain WorldWide;
};

/**
 * Device operating location, as relevant to wireless regulatory rules.
 */
enum WirelessOperatingLocation
{
    kWirelessOperatingLocation_NotSpecified = 0x00, /**< Reserved value.
                                                         May not be sent over-the-wire. */

    kWirelessOperatingLocation_Unknown      = 0x01, /**< Operating location unknown.
                                                         Signifies that the device's expected operating location
                                                         is not known, or may change over time. */

    kWirelessOperatingLocation_Indoors      = 0x02, /**< Operating indoors.
                                                         Signifies that the device's expected operating location
                                                         is indoors. */

    kWirelessOperatingLocation_Outdoors     = 0x03, /**< Operating outdoors.
                                                         Signifies that the device's expected operating location
                                                         is outdoors. */
};

/**
 * Container for wireless regulatory configuration information.
 */
class WirelessRegConfig
{
public:
    WirelessRegDomain * SupportedRegDomains;                /**< Array of supported regulatory domain structures */
    uint16_t NumSupportedRegDomains;                        /**< Length of SupportedRegDomains array */
    WirelessRegDomain RegDomain;                            /**< Active wireless regulatory domain
                                                                 Value of '\0' indicates not present. */
    uint8_t OpLocation;                                     /**< Active operating location
                                                                 Value of 0 indicates not present. */

    void Init(void);
    bool IsRegDomainPresent(void) const;
    bool IsOpLocationPresent(void) const;

    WEAVE_ERROR Encode(nl::Weave::TLV::TLVWriter & writer) const;
    WEAVE_ERROR Decode(nl::Weave::TLV::TLVReader & reader);
    WEAVE_ERROR DecodeInPlace(PacketBuffer * buf);
};

/**
 * Test WirelessRegDomain structure for equality
 */
inline bool WirelessRegDomain::operator ==(const struct WirelessRegDomain & other) const
{
    return memcmp(Code, other.Code, sizeof(Code)) == 0;
}

/**
 * Test WirelessRegDomain structure for inequality
 */
inline bool WirelessRegDomain::operator !=(const struct WirelessRegDomain & other) const
{
    return memcmp(Code, other.Code, sizeof(Code)) != 0;
}

/**
 * Test if the value is null.
 */
inline bool WirelessRegDomain::IsNull(void) const
{
    return Code[0] == '\0' && Code[1] == '\0';
}

/**
 * Test if the value represents the special 'world-wide' regulatory code.
 */
inline bool WirelessRegDomain::IsWorldWide(void) const
{
    return Code[0] == '0' && Code[1] == '0';
}

/**
 * Reset the WirelessRegConfig object to an empty state.
 */
inline void WirelessRegConfig::Init(void)
{
    memset(this, 0, sizeof(*this));
}

/**
 * Is RegDomain field present in WirelessRegConfig object.
 */
inline bool WirelessRegConfig::IsRegDomainPresent(void) const
{
    return !RegDomain.IsNull();
}

/**
 * Is OpLocation field present in WirelessRegConfig object.
 */
inline bool WirelessRegConfig::IsOpLocationPresent(void) const
{
    return OpLocation != kWirelessOperatingLocation_NotSpecified;
}

} // namespace NetworkProvisioning
} // namespace Profiles
} // namespace Weave
} // namespace nl

#endif // WIRELESS_REG_CONFIG_H
