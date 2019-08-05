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

#include <Weave/DeviceLayer/internal/WeaveDeviceLayerInternal.h>
#include <Weave/DeviceLayer/internal/DeviceNetworkInfo.h>
#include <Weave/Core/WeaveTLV.h>
#include <Weave/Profiles/WeaveProfiles.h>

using namespace ::nl;
using namespace ::nl::Weave;
using namespace ::nl::Weave::TLV;
using namespace ::nl::Weave::Profiles::NetworkProvisioning;

using Profiles::kWeaveProfile_NetworkProvisioning;

namespace nl {
namespace Weave {
namespace DeviceLayer {
namespace Internal {

void DeviceNetworkInfo::Reset()
{
    memset(this, 0, sizeof(*this));
    NetworkType = kNetworkType_NotSpecified;
#if WEAVE_DEVICE_CONFIG_ENABLE_WIFI_STATION
    WiFiMode = kWiFiMode_NotSpecified;
    WiFiRole = kWiFiRole_NotSpecified;
    WiFiSecurityType = kWiFiSecurityType_NotSpecified;
#endif // WEAVE_DEVICE_CONFIG_ENABLE_WIFI_STATION
#if WEAVE_DEVICE_CONFIG_ENABLE_THREAD
    ThreadPANId = kThreadPANId_NotSpecified;
    ThreadChannel = kThreadChannel_NotSpecified;
#endif // WEAVE_DEVICE_CONFIG_ENABLE_THREAD
    WirelessSignalStrength = INT16_MIN;
}

WEAVE_ERROR DeviceNetworkInfo::Encode(nl::Weave::TLV::TLVWriter & writer) const
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    TLVType outerContainer;

    uint64_t tag = (writer.GetContainerType() == kTLVType_Array)
            ? AnonymousTag
            : ProfileTag(kWeaveProfile_NetworkProvisioning, kTag_NetworkInformation);

    err = writer.StartContainer(tag, kTLVType_Structure, outerContainer);
    SuccessOrExit(err);

    if (FieldPresent.NetworkId)
    {
        err = writer.Put(ProfileTag(kWeaveProfile_NetworkProvisioning, kTag_NetworkId), (uint32_t) NetworkId);
        SuccessOrExit(err);
    }

    if (NetworkType != kNetworkType_NotSpecified)
    {
        err = writer.Put(ProfileTag(kWeaveProfile_NetworkProvisioning, kTag_NetworkType), (uint32_t) NetworkType);
        SuccessOrExit(err);
    }

#if WEAVE_DEVICE_CONFIG_ENABLE_WIFI_STATION

    if (WiFiSSID[0] != 0)
    {
        err = writer.PutString(ProfileTag(kWeaveProfile_NetworkProvisioning, kTag_WiFiSSID), WiFiSSID);
        SuccessOrExit(err);
    }

    if (WiFiMode != kWiFiMode_NotSpecified)
    {
        err = writer.Put(ProfileTag(kWeaveProfile_NetworkProvisioning, kTag_WiFiMode), (uint32_t) WiFiMode);
        SuccessOrExit(err);
    }

    if (WiFiRole != kWiFiRole_NotSpecified)
    {
        err = writer.Put(ProfileTag(kWeaveProfile_NetworkProvisioning, kTag_WiFiRole), (uint32_t) WiFiRole);
        SuccessOrExit(err);
    }

    if (WiFiSecurityType != kWiFiSecurityType_NotSpecified)
    {
        err = writer.Put(ProfileTag(kWeaveProfile_NetworkProvisioning, kTag_WiFiSecurityType), (uint32_t) WiFiSecurityType);
        SuccessOrExit(err);
    }

    if (WiFiKeyLen != 0)
    {
        err = writer.PutBytes(ProfileTag(kWeaveProfile_NetworkProvisioning, kTag_WiFiPreSharedKey), WiFiKey, WiFiKeyLen);
        SuccessOrExit(err);
    }

#endif // WEAVE_DEVICE_CONFIG_ENABLE_WIFI_STATION

#if WEAVE_DEVICE_CONFIG_ENABLE_THREAD

    if (ThreadNetworkName[0] != 0)
    {
        err = writer.PutString(ProfileTag(kWeaveProfile_NetworkProvisioning, kTag_ThreadNetworkName), ThreadNetworkName);
        SuccessOrExit(err);
    }

    if (FieldPresent.ThreadExtendedPANId)
    {
        err = writer.PutBytes(ProfileTag(kWeaveProfile_NetworkProvisioning, kTag_ThreadExtendedPANId),
                ThreadExtendedPANId, kThreadExtendedPANIdLength);
        SuccessOrExit(err);
    }

    if (FieldPresent.ThreadMeshPrefix)
    {
        err = writer.PutBytes(ProfileTag(kWeaveProfile_NetworkProvisioning, kTag_ThreadMeshPrefix),
                ThreadMeshPrefix, kThreadMeshPrefixLength);
        SuccessOrExit(err);
    }

    if (FieldPresent.ThreadNetworkKey)
    {
        err = writer.PutBytes(ProfileTag(kWeaveProfile_NetworkProvisioning, kTag_ThreadNetworkKey),
                ThreadNetworkKey, kThreadNetworkKeyLength);
        SuccessOrExit(err);
    }

    if (FieldPresent.ThreadPSKc)
    {
        err = writer.PutBytes(ProfileTag(kWeaveProfile_NetworkProvisioning, kTag_ThreadNetworkPSKc),
                ThreadNetworkPSKc, kThreadPSKcLength);
        SuccessOrExit(err);
    }

    if (ThreadPANId != kThreadPANId_NotSpecified)
    {
        VerifyOrExit(ThreadPANId <= UINT16_MAX, err = WEAVE_ERROR_INVALID_ARGUMENT);
        err = writer.Put(ProfileTag(kWeaveProfile_NetworkProvisioning, kTag_ThreadPANId), ThreadPANId);
        SuccessOrExit(err);
    }

    if (ThreadChannel != kThreadChannel_NotSpecified)
    {
        err = writer.Put(ProfileTag(kWeaveProfile_NetworkProvisioning, kTag_ThreadChannel), ThreadChannel);
        SuccessOrExit(err);
    }

#endif // WEAVE_DEVICE_CONFIG_ENABLE_THREAD

    if (WirelessSignalStrength != INT16_MIN)
    {
        err = writer.Put(ProfileTag(kWeaveProfile_NetworkProvisioning, kTag_WirelessSignalStrength), WirelessSignalStrength);
        SuccessOrExit(err);
    }

    err = writer.EndContainer(outerContainer);
    SuccessOrExit(err);

exit:
    return err;
}

WEAVE_ERROR DeviceNetworkInfo::Decode(nl::Weave::TLV::TLVReader & reader)
{
    WEAVE_ERROR err;
    TLVType outerContainer;
    uint32_t val;

    if (reader.GetType() == kTLVType_NotSpecified)
    {
        err = reader.Next();
        SuccessOrExit(err);
    }

    VerifyOrExit(reader.GetTag() == ProfileTag(kWeaveProfile_NetworkProvisioning, kTag_NetworkInformation) ||
                 reader.GetTag() == AnonymousTag,
                 err = WEAVE_ERROR_INVALID_TLV_ELEMENT);

    VerifyOrExit(reader.GetType() == kTLVType_Structure, err = WEAVE_ERROR_WRONG_TLV_TYPE);

    err = reader.EnterContainer(outerContainer);
    SuccessOrExit(err);

    Reset();

    while ((err = reader.Next()) == WEAVE_NO_ERROR)
    {
        uint64_t elemTag = reader.GetTag();

        if (!IsProfileTag(elemTag) || ProfileIdFromTag(elemTag) != kWeaveProfile_NetworkProvisioning)
            continue;

        switch (TagNumFromTag(elemTag))
        {
        case kTag_NetworkId:
            VerifyOrExit(reader.GetType() == kTLVType_UnsignedInteger, err = WEAVE_ERROR_INVALID_TLV_ELEMENT);
            err = reader.Get(NetworkId);
            SuccessOrExit(err);
            FieldPresent.NetworkId = true;
            break;
        case kTag_NetworkType:
            VerifyOrExit(reader.GetType() == kTLVType_UnsignedInteger, err = WEAVE_ERROR_INVALID_TLV_ELEMENT);
            err = reader.Get(val);
            SuccessOrExit(err);
            NetworkType = (NetworkType_t) val;
            break;
        case kTag_WirelessSignalStrength:
            VerifyOrExit(reader.GetType() == kTLVType_SignedInteger, err = WEAVE_ERROR_INVALID_TLV_ELEMENT);
            err = reader.Get(WirelessSignalStrength);
            SuccessOrExit(err);
            break;
#if WEAVE_DEVICE_CONFIG_ENABLE_WIFI_STATION
        case kTag_WiFiSSID:
            VerifyOrExit(reader.GetType() == kTLVType_UTF8String, err = WEAVE_ERROR_INVALID_TLV_ELEMENT);
            err = reader.GetString(WiFiSSID, sizeof(WiFiSSID));
            SuccessOrExit(err);
            break;
        case kTag_WiFiMode:
            VerifyOrExit(reader.GetType() == kTLVType_UnsignedInteger, err = WEAVE_ERROR_INVALID_TLV_ELEMENT);
            err = reader.Get(val);
            SuccessOrExit(err);
            WiFiMode = (WiFiMode_t) val;
            break;
        case kTag_WiFiRole:
            VerifyOrExit(reader.GetType() == kTLVType_UnsignedInteger, err = WEAVE_ERROR_INVALID_TLV_ELEMENT);
            err = reader.Get(val);
            SuccessOrExit(err);
            WiFiRole = (WiFiRole_t) val;
            break;
        case kTag_WiFiPreSharedKey:
            VerifyOrExit(reader.GetType() == kTLVType_ByteString, err = WEAVE_ERROR_INVALID_TLV_ELEMENT);
            val = reader.GetLength();
            VerifyOrExit(val <= kMaxWiFiKeyLength, err = WEAVE_ERROR_INVALID_TLV_ELEMENT);
            WiFiKeyLen = (uint16_t)val;
            err = reader.GetBytes(WiFiKey, sizeof(WiFiKey));
            SuccessOrExit(err);
            break;
        case kTag_WiFiSecurityType:
            VerifyOrExit(reader.GetType() == kTLVType_UnsignedInteger, err = WEAVE_ERROR_INVALID_TLV_ELEMENT);
            err = reader.Get(val);
            SuccessOrExit(err);
            WiFiSecurityType = (WiFiSecurityType_t) val;
            break;
#else // WEAVE_DEVICE_CONFIG_ENABLE_WIFI_STATION
        case kTag_WiFiSSID:
        case kTag_WiFiMode:
        case kTag_WiFiRole:
        case kTag_WiFiPreSharedKey:
        case kTag_WiFiSecurityType:
            ExitNow(err = WEAVE_ERROR_UNSUPPORTED_WEAVE_FEATURE);
            break;
#endif //WEAVE_DEVICE_CONFIG_ENABLE_WIFI_STATION
#if WEAVE_DEVICE_CONFIG_ENABLE_THREAD
        case kTag_ThreadNetworkName:
            VerifyOrExit(reader.GetType() == kTLVType_UTF8String, err = WEAVE_ERROR_INVALID_TLV_ELEMENT);
            err = reader.GetString(ThreadNetworkName, sizeof(ThreadNetworkName));
            SuccessOrExit(err);
            break;
        case kTag_ThreadExtendedPANId:
            VerifyOrExit(reader.GetType() == kTLVType_ByteString, err = WEAVE_ERROR_INVALID_TLV_ELEMENT);
            val = reader.GetLength();
            VerifyOrExit(val == kThreadExtendedPANIdLength, err = WEAVE_ERROR_INVALID_TLV_ELEMENT);
            err = reader.GetBytes(ThreadExtendedPANId, sizeof(ThreadExtendedPANId));
            SuccessOrExit(err);
            FieldPresent.ThreadExtendedPANId = true;
            break;
        case kTag_ThreadMeshPrefix:
            VerifyOrExit(reader.GetType() == kTLVType_ByteString, err = WEAVE_ERROR_INVALID_TLV_ELEMENT);
            val = reader.GetLength();
            VerifyOrExit(val == kThreadMeshPrefixLength, err = WEAVE_ERROR_INVALID_TLV_ELEMENT);
            err = reader.GetBytes(ThreadMeshPrefix, sizeof(ThreadMeshPrefix));
            SuccessOrExit(err);
            FieldPresent.ThreadMeshPrefix = true;
            break;
        case kTag_ThreadNetworkKey:
            VerifyOrExit(reader.GetType() == kTLVType_ByteString, err = WEAVE_ERROR_INVALID_TLV_ELEMENT);
            val = reader.GetLength();
            VerifyOrExit(val == kThreadNetworkKeyLength, err = WEAVE_ERROR_INVALID_TLV_ELEMENT);
            err = reader.GetBytes(ThreadNetworkKey, sizeof(ThreadNetworkKey));
            SuccessOrExit(err);
            FieldPresent.ThreadNetworkKey = true;
            break;
        case kTag_ThreadPSKc:
            VerifyOrExit(reader.GetType() == kTLVType_ByteString, err = WEAVE_ERROR_INVALID_TLV_ELEMENT);
            val = reader.GetLength();
            VerifyOrExit(val == kThreadPSKcLength, err = WEAVE_ERROR_INVALID_TLV_ELEMENT);
            err = reader.GetBytes(ThreadPSKc, sizeof(ThreadPSKc));
            SuccessOrExit(err);
            FieldPresent.ThreadPSKc = true;
            break;
        case kTag_ThreadPANId:
            VerifyOrExit(reader.GetType() == kTLVType_UnsignedInteger, err = WEAVE_ERROR_INVALID_TLV_ELEMENT);
            err = reader.Get(val);
            SuccessOrExit(err);
            VerifyOrExit(val <= UINT16_MAX, err = WEAVE_ERROR_INVALID_TLV_ELEMENT);
            ThreadPANId = val;
            break;
        case kTag_ThreadChannel:
            VerifyOrExit(reader.GetType() == kTLVType_UnsignedInteger, err = WEAVE_ERROR_INVALID_TLV_ELEMENT);
            err = reader.Get(val);
            SuccessOrExit(err);
            VerifyOrExit(val <= UINT8_MAX, err = WEAVE_ERROR_INVALID_TLV_ELEMENT);
            ThreadChannel = val;
            break;
#else // WEAVE_DEVICE_CONFIG_ENABLE_THREAD
        case kTag_ThreadNetworkName:
        case kTag_ThreadExtendedPANId:
        case kTag_ThreadMeshPrefix:
        case kTag_ThreadNetworkKey:
        case kTag_ThreadPSKc:
        case kTag_ThreadPANId:
        case kTag_ThreadChannel:
            ExitNow(err = WEAVE_ERROR_UNSUPPORTED_WEAVE_FEATURE);
            break;
#endif // WEAVE_DEVICE_CONFIG_ENABLE_THREAD
        default:
            // Ignore unknown elements for compatibility with future formats.
            break;
        }
    }

    if (err != WEAVE_END_OF_TLV)
        ExitNow();

    err = reader.ExitContainer(outerContainer);
    SuccessOrExit(err);

exit:
    return err;
}

WEAVE_ERROR DeviceNetworkInfo::MergeTo(DeviceNetworkInfo & dest)
{
    if (NetworkType != kNetworkType_NotSpecified)
    {
        dest.NetworkType = NetworkType;
    }
    if (FieldPresent.NetworkId)
    {
        dest.NetworkId = NetworkId;
        dest.FieldPresent.NetworkId = true;
    }

#if WEAVE_DEVICE_CONFIG_ENABLE_WIFI_STATION
    if (WiFiSSID[0] != 0)
    {
        memcpy(dest.WiFiSSID, WiFiSSID, sizeof(WiFiSSID));
    }
    if (WiFiMode != kWiFiMode_NotSpecified)
    {
        dest.WiFiMode = WiFiMode;
    }
    if (WiFiRole != kWiFiRole_NotSpecified)
    {
        dest.WiFiRole = WiFiRole;
    }
    if (WiFiSecurityType != kWiFiSecurityType_NotSpecified)
    {
        dest.WiFiSecurityType = WiFiSecurityType;
    }
    if (WiFiKeyLen != 0)
    {
        memcpy(dest.WiFiKey, WiFiKey, WiFiKeyLen);
        dest.WiFiKeyLen = WiFiKeyLen;
    }
#endif // WEAVE_DEVICE_CONFIG_ENABLE_WIFI_STATION

#if WEAVE_DEVICE_CONFIG_ENABLE_THREAD
    if (ThreadNetworkName[0] != 0)
    {
        memcpy(dest.ThreadNetworkName, ThreadNetworkName, sizeof(ThreadNetworkName));
    }
    if (FieldPresent.ThreadExtendedPANId)
    {
        memcpy(dest.ThreadExtendedPANId, ThreadExtendedPANId, sizeof(ThreadExtendedPANId));
    }
    if (FieldPresent.ThreadMeshPrefix)
    {
        memcpy(dest.ThreadMeshPrefix, ThreadMeshPrefix, sizeof(ThreadMeshPrefix));
    }
    if (FieldPresent.ThreadNetworkKey)
    {
        memcpy(dest.ThreadNetworkKey, ThreadNetworkKey, sizeof(ThreadNetworkKey));
    }
    if (FieldPresent.ThreadPSKc)
    {
        memcpy(dest.ThreadPSKc, ThreadPSKc, sizeof(ThreadPSKc));
    }
    if (ThreadPANId != kThreadPANId_NotSpecified)
    {
        dest.ThreadPANId = ThreadPANId;
    }
    if (ThreadChannel != kThreadChannel_NotSpecified)
    {
        dest.ThreadChannel = ThreadChannel;
    }
#endif // WEAVE_DEVICE_CONFIG_ENABLE_THREAD

    if (WirelessSignalStrength != INT16_MIN)
    {
        dest.WirelessSignalStrength = WirelessSignalStrength;
    }

    return WEAVE_NO_ERROR;
}

WEAVE_ERROR DeviceNetworkInfo::EncodeArray(nl::Weave::TLV::TLVWriter & writer, const DeviceNetworkInfo * elems, size_t count)
{
    WEAVE_ERROR err;
    TLVType outerContainerType;

    err = writer.StartContainer(AnonymousTag, kTLVType_Array, outerContainerType);
    SuccessOrExit(err);

    for (size_t i = 0; i < count; i++)
    {
        err = elems[i].Encode(writer);
        SuccessOrExit(err);
    }

    err = writer.EndContainer(outerContainerType);
    SuccessOrExit(err);

exit:
    return err;
}

} // namespace Internal
} // namespace DeviceLayer
} // namespace Weave
} // namespace nl
