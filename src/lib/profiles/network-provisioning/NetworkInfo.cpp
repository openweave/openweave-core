/*
 *
 *    Copyright (c) 2013-2017 Nest Labs, Inc.
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
#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>


#include <Weave/Core/WeaveCore.h>
#include <Weave/Support/CodeUtils.h>
#include <Weave/Core/WeaveEncoding.h>
#include <Weave/Profiles/WeaveProfiles.h>
#include "NetworkInfo.h"

namespace nl {
namespace Weave {
namespace Profiles {
namespace NetworkProvisioning {

using namespace nl::Weave::Encoding;
using namespace nl::Weave::TLV;

NetworkInfo::NetworkInfo()
{
    NetworkType = kNetworkType_NotSpecified;
    NetworkId = -1;
    WiFiSSID = NULL;
    WiFiMode = kWiFiMode_NotSpecified;
    WiFiRole = kWiFiRole_NotSpecified;
    WiFiSecurityType = kWiFiSecurityType_NotSpecified;
    WiFiKey = NULL;
    WiFiKeyLen = 0;
    Hidden = false;
    ThreadNetworkName = NULL;
    ThreadExtendedPANId = NULL;
    ThreadNetworkKey = NULL;
    ThreadNetworkKeyLen = 0;
    WirelessSignalStrength = INT16_MIN;
}

NetworkInfo::~NetworkInfo()
{
    Clear();
}

static WEAVE_ERROR ReplaceValue(char *& dest, const char *src)
{
#if HAVE_MALLOC && HAVE_STRDUP && HAVE_FREE
    if (dest != NULL)
        free((void *) dest);
    if (src != NULL)
    {
        dest = strdup(src);
        return dest != NULL ? WEAVE_NO_ERROR : WEAVE_ERROR_NO_MEMORY;
    }
    else
    {
        dest = NULL;
        return WEAVE_NO_ERROR;
    }
#else
    return WEAVE_ERROR_NOT_IMPLEMENTED;
#endif
}

static WEAVE_ERROR ReplaceValue(uint8_t *& dest, uint32_t& destLen, const uint8_t *src, uint32_t srcLen)
{
#if HAVE_MALLOC && HAVE_STRDUP && HAVE_FREE
    if (dest != NULL)
        free((void *) dest);
    if (src != NULL)
    {
        dest = (uint8_t *) malloc(srcLen);
        if (dest == NULL)
            return WEAVE_ERROR_NO_MEMORY;
        memcpy(dest, src, srcLen);
        destLen = srcLen;
        return WEAVE_NO_ERROR;
    }
    else
    {
        dest = NULL;
        destLen = 0;
        return WEAVE_NO_ERROR;
    }
#else
    return WEAVE_ERROR_NOT_IMPLEMENTED;
#endif
}

WEAVE_ERROR NetworkInfo::CopyTo(NetworkInfo& dest)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint32_t unused;

    dest.NetworkType = NetworkType;
    dest.NetworkId = NetworkId;
    err = ReplaceValue(dest.WiFiSSID, WiFiSSID);
    SuccessOrExit(err);
    dest.WiFiMode = WiFiMode;
    dest.WiFiRole = WiFiRole;
    dest.WiFiSecurityType = WiFiSecurityType;
    err = ReplaceValue(dest.WiFiKey, dest.WiFiKeyLen, WiFiKey, WiFiKeyLen);
    SuccessOrExit(err);
    err = ReplaceValue(dest.ThreadNetworkName, ThreadNetworkName);
    SuccessOrExit(err);
    err = ReplaceValue(dest.ThreadExtendedPANId, unused, ThreadExtendedPANId, 8);
    SuccessOrExit(err);
    err = ReplaceValue(dest.ThreadNetworkKey, dest.ThreadNetworkKeyLen, ThreadNetworkKey, ThreadNetworkKeyLen);
    SuccessOrExit(err);
    dest.WirelessSignalStrength = WirelessSignalStrength;

exit:
    return err;
}

WEAVE_ERROR NetworkInfo::MergeTo(NetworkInfo& dest)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    if (NetworkType != kNetworkType_NotSpecified)
        dest.NetworkType = NetworkType;
    if (NetworkId != -1)
        dest.NetworkId = NetworkId;
    if (WiFiSSID != NULL)
    {
        err = ReplaceValue(dest.WiFiSSID, WiFiSSID);
        SuccessOrExit(err);
    }
    if (WiFiMode != kWiFiMode_NotSpecified)
        dest.WiFiMode = WiFiMode;
    if (WiFiRole != kWiFiRole_NotSpecified)
        dest.WiFiRole = WiFiRole;
    if (WiFiSecurityType != kWiFiSecurityType_NotSpecified)
        dest.WiFiSecurityType = WiFiSecurityType;
    if (WiFiKey != NULL)
    {
        err = ReplaceValue(dest.WiFiKey, dest.WiFiKeyLen, WiFiKey, WiFiKeyLen);
        SuccessOrExit(err);
    }
    if (ThreadNetworkName != NULL)
    {
        err = ReplaceValue(dest.ThreadNetworkName, ThreadNetworkName);
        SuccessOrExit(err);
    }
    if (ThreadExtendedPANId != NULL)
    {
        uint32_t unused;
        err = ReplaceValue(dest.ThreadExtendedPANId, unused, ThreadExtendedPANId, 8);
        SuccessOrExit(err);
    }
    if (ThreadNetworkKey != NULL)
    {
        err = ReplaceValue(dest.ThreadNetworkKey, dest.ThreadNetworkKeyLen, ThreadNetworkKey, ThreadNetworkKeyLen);
        SuccessOrExit(err);
    }
    if (WirelessSignalStrength != INT16_MIN)
        dest.WirelessSignalStrength = WirelessSignalStrength;

exit:
    return err;
}

WEAVE_ERROR NetworkInfo::Decode(nl::Weave::TLV::TLVReader& reader)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    TLVType outerContainer;
    uint32_t val;

    VerifyOrExit(reader.GetType() == kTLVType_Structure, err = WEAVE_ERROR_WRONG_TLV_TYPE);

    err = reader.EnterContainer(outerContainer);
    SuccessOrExit(err);

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
            break;
        case kTag_NetworkType:
            VerifyOrExit(reader.GetType() == kTLVType_UnsignedInteger, err = WEAVE_ERROR_INVALID_TLV_ELEMENT);
            err = reader.Get(val);
            SuccessOrExit(err);
            NetworkType = (::nl::Weave::Profiles::NetworkProvisioning::NetworkType) val;
            break;
        case kTag_WirelessSignalStrength:
            VerifyOrExit(reader.GetType() == kTLVType_SignedInteger, err = WEAVE_ERROR_INVALID_TLV_ELEMENT);
            err = reader.Get(WirelessSignalStrength);
            SuccessOrExit(err);
            break;
        case kTag_WiFiSSID:
            VerifyOrExit(reader.GetType() == kTLVType_UTF8String, err = WEAVE_ERROR_INVALID_TLV_ELEMENT);
            err = reader.DupString((char *&) WiFiSSID);
            SuccessOrExit(err);
            break;
        case kTag_WiFiMode:
            VerifyOrExit(reader.GetType() == kTLVType_UnsignedInteger, err = WEAVE_ERROR_INVALID_TLV_ELEMENT);
            err = reader.Get(val);
            SuccessOrExit(err);
            WiFiMode = (::nl::Weave::Profiles::NetworkProvisioning::WiFiMode) val;
            break;
        case kTag_WiFiRole:
            VerifyOrExit(reader.GetType() == kTLVType_UnsignedInteger, err = WEAVE_ERROR_INVALID_TLV_ELEMENT);
            err = reader.Get(val);
            SuccessOrExit(err);
            WiFiRole = (::nl::Weave::Profiles::NetworkProvisioning::WiFiRole) val;
            break;
        case kTag_WiFiSecurityType:
            VerifyOrExit(reader.GetType() == kTLVType_UnsignedInteger, err = WEAVE_ERROR_INVALID_TLV_ELEMENT);
            err = reader.Get(val);
            SuccessOrExit(err);
            WiFiSecurityType = (::nl::Weave::Profiles::NetworkProvisioning::WiFiSecurityType) val;
            break;
        case kTag_WiFiPreSharedKey:
            VerifyOrExit(reader.GetType() == kTLVType_ByteString, err = WEAVE_ERROR_INVALID_TLV_ELEMENT);
            err = reader.DupBytes(WiFiKey, WiFiKeyLen);
            SuccessOrExit(err);
            break;
        case kTag_ThreadNetworkName:
            VerifyOrExit(reader.GetType() == kTLVType_UTF8String, err = WEAVE_ERROR_INVALID_TLV_ELEMENT);
            err = reader.DupString(ThreadNetworkName);
            SuccessOrExit(err);
            break;
        case kTag_ThreadExtendedPANId:
            VerifyOrExit(reader.GetType() == kTLVType_ByteString, err = WEAVE_ERROR_INVALID_TLV_ELEMENT);
            VerifyOrExit(reader.GetLength() == 8, err = WEAVE_ERROR_INVALID_TLV_ELEMENT);
            err = reader.DupBytes(ThreadExtendedPANId, val);
            SuccessOrExit(err);
            break;
        case kTag_ThreadNetworkKey:
            VerifyOrExit(reader.GetType() == kTLVType_ByteString, err = WEAVE_ERROR_INVALID_TLV_ELEMENT);
            err = reader.DupBytes(ThreadNetworkKey, ThreadNetworkKeyLen);
            SuccessOrExit(err);
            break;
        default:
            // Ignore unknown elements
            break;
        }
    }

    if (err != WEAVE_END_OF_TLV)
        ExitNow();

    err = reader.ExitContainer(outerContainer);
    SuccessOrExit(err);

exit:
    if (err != WEAVE_NO_ERROR)
        Clear();
    return err;
}

WEAVE_ERROR NetworkInfo::Encode(nl::Weave::TLV::TLVWriter& writer, uint8_t encodeFlags) const
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    TLVType outerContainer;

    uint64_t tag =
            (writer.GetContainerType() == kTLVType_Array) ?
                    AnonymousTag : ProfileTag(kWeaveProfile_NetworkProvisioning, kTag_NetworkInformation);

    err = writer.StartContainer(tag, kTLVType_Structure, outerContainer);
    SuccessOrExit(err);

    if (NetworkId != -1)
    {
        err = writer.Put(ProfileTag(kWeaveProfile_NetworkProvisioning, kTag_NetworkId), (uint32_t) NetworkId);
        SuccessOrExit(err);
    }

    if (NetworkType != kNetworkType_NotSpecified)
    {
        err = writer.Put(ProfileTag(kWeaveProfile_NetworkProvisioning, kTag_NetworkType), (uint32_t) NetworkType);
        SuccessOrExit(err);
    }

    if (WiFiSSID != NULL)
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
        err = writer.Put(ProfileTag(kWeaveProfile_NetworkProvisioning, kTag_WiFiSecurityType),
                (uint32_t) WiFiSecurityType);
        SuccessOrExit(err);
    }

    if (WiFiKey != NULL && (encodeFlags & kEncodeFlag_EncodeCredentials) != 0)
    {
        err = writer.PutBytes(ProfileTag(kWeaveProfile_NetworkProvisioning, kTag_WiFiPreSharedKey), WiFiKey,
                WiFiKeyLen);
        SuccessOrExit(err);
    }

    if (ThreadNetworkName != NULL)
    {
        err = writer.PutString(ProfileTag(kWeaveProfile_NetworkProvisioning, kTag_ThreadNetworkName), ThreadNetworkName);
        SuccessOrExit(err);
    }

    if (ThreadExtendedPANId != NULL)
    {
        err = writer.PutBytes(ProfileTag(kWeaveProfile_NetworkProvisioning, kTag_ThreadExtendedPANId), ThreadExtendedPANId, 8);
        SuccessOrExit(err);
    }

    if (ThreadNetworkKey != NULL && (encodeFlags & kEncodeFlag_EncodeCredentials) != 0)
    {
        err = writer.PutBytes(ProfileTag(kWeaveProfile_NetworkProvisioning, kTag_ThreadNetworkKey), ThreadNetworkKey,
                ThreadNetworkKeyLen);
        SuccessOrExit(err);
    }

    if (WirelessSignalStrength != INT16_MIN)
    {
        err = writer.Put(ProfileTag(kWeaveProfile_NetworkProvisioning, kTag_WirelessSignalStrength),
                WirelessSignalStrength);
        SuccessOrExit(err);
    }

    err = writer.EndContainer(outerContainer);
    SuccessOrExit(err);

exit:
    return err;
}

void NetworkInfo::Clear()
{
    NetworkType = kNetworkType_NotSpecified;
    NetworkId = -1;
    WiFiMode = kWiFiMode_NotSpecified;
    WiFiRole = kWiFiRole_NotSpecified;
    WiFiSecurityType = kWiFiSecurityType_NotSpecified;
    WiFiKeyLen = 0;
    ThreadNetworkKeyLen = 0;
    WirelessSignalStrength = INT16_MIN;

#if HAVE_MALLOC && HAVE_FREE
    if (WiFiSSID != NULL)
    {
        free((void *) WiFiSSID);
        WiFiSSID = NULL;
    }
    if (WiFiKey != NULL)
    {
        free(WiFiKey);
        WiFiKey = NULL;
    }
    if (ThreadNetworkName != NULL)
    {
        free((void *)ThreadNetworkName);
        ThreadNetworkName = NULL;
    }
    if (ThreadExtendedPANId != NULL)
    {
        free((void *)ThreadExtendedPANId);
        ThreadExtendedPANId = NULL;
    }
    if (ThreadNetworkKey != NULL)
    {
        free((void *)ThreadNetworkKey);
        ThreadNetworkKey = NULL;
    }
#endif //HAVE_MALLOC && HAVE_FREE
}

WEAVE_ERROR NetworkInfo::DecodeList(nl::Weave::TLV::TLVReader& reader, uint16_t& elemCount, NetworkInfo *& elemArray)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    NetworkInfo *newArray = NULL;
    TLVType arrayOuter;
    int i;

    VerifyOrExit(reader.GetType() == kTLVType_Array, err = WEAVE_ERROR_WRONG_TLV_TYPE);

    err = reader.EnterContainer(arrayOuter);
    SuccessOrExit(err);

    if (elemArray == NULL)
    {
#if HAVE_MALLOC && HAVE_FREE
        elemArray = newArray = new NetworkInfo[elemCount];
#else
        return WEAVE_ERROR_NOT_IMPLEMENTED;
#endif
    }
    for (i = 0; i < elemCount; i++)
    {
        err = reader.Next();
        SuccessOrExit(err);

        err = elemArray[i].Decode(reader);
        SuccessOrExit(err);
    }

    err = reader.ExitContainer(arrayOuter);
    SuccessOrExit(err);

    elemCount = i;

    return WEAVE_NO_ERROR;

exit:
#if HAVE_MALLOC && HAVE_FREE
    if (newArray != NULL)
    {
        delete[] newArray;
        elemArray = NULL;
    }
#endif
    return err;
}

WEAVE_ERROR NetworkInfo::EncodeList(nl::Weave::TLV::TLVWriter& writer, uint16_t elemCount, const NetworkInfo *elemArray, uint8_t encodeFlags)
{
    WEAVE_ERROR err;
    TLVType outerContainerType;

    err = writer.StartContainer(AnonymousTag, kTLVType_Array, outerContainerType);
    SuccessOrExit(err);

    for (int i = 0; i < elemCount; i++)
    {
        err = elemArray[i].Encode(writer, encodeFlags);
        SuccessOrExit(err);
    }

    err = writer.EndContainer(outerContainerType);
    SuccessOrExit(err);

exit:
    return err;
}

WEAVE_ERROR NetworkInfo::EncodeList(nl::Weave::TLV::TLVWriter& writer, uint16_t arrayLen, const NetworkInfo *elemArray,
                                    ::nl::Weave::Profiles::NetworkProvisioning::NetworkType networkType, uint8_t encodeFlags, uint16_t& encodedElemCount)
{
    WEAVE_ERROR err;
    TLVType outerContainerType;

    encodedElemCount = 0;

    err = writer.StartContainer(AnonymousTag, kTLVType_Array, outerContainerType);
    SuccessOrExit(err);

    for (int i = 0; i < arrayLen; i++)
        if (elemArray[i].NetworkType != kNetworkType_NotSpecified &&
            (networkType == kNetworkType_NotSpecified || elemArray[i].NetworkType == networkType))
        {
            err = elemArray[i].Encode(writer, encodeFlags);
            SuccessOrExit(err);
            encodedElemCount++;
        }

    err = writer.EndContainer(outerContainerType);
    SuccessOrExit(err);

exit:
    return err;
}

} // NetworkProvisioning
} // namespace Profiles
} // namespace Weave
} // namespace nl
