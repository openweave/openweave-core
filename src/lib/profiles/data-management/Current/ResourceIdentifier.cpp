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

// __STDC_FORMAT_MACROS must be defined for PRIX64 to be defined for pre-C++11 clib
#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif // __STDC_FORMAT_MACROS

// __STDC_LIMIT_MACROS must be defined for UINT8_MAX and INT32_MAX to be defined for pre-C++11 clib
#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif // __STDC_LIMIT_MACROS

// __STDC_CONSTANT_MACROS must be defined for INT64_C and UINT64_C to be defined for pre-C++11 clib
#ifndef __STDC_CONSTANT_MACROS
#define __STDC_CONSTANT_MACROS
#endif // __STDC_CONSTANT_MACROS

#include <stdlib.h>

#include <Weave/Profiles/data-management/Current/WdmManagedNamespace.h>
#include <Weave/Profiles/data-management/DataManagement.h>

namespace nl {
namespace Weave {
namespace Profiles {
namespace WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current) {

WEAVE_ERROR ResourceIdentifier::ToTLV(nl::Weave::TLV::TLVWriter & aWriter) const
{
    return ToTLV(aWriter, TLV::ContextTag(Path::kCsTag_ResourceID));
}

WEAVE_ERROR ResourceIdentifier::ToTLV(nl::Weave::TLV::TLVWriter & aWriter, const uint64_t & aTag) const
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    if (ResourceType == ResourceIdentifier::RESOURCE_TYPE_RESERVED)
    {
        VerifyOrExit(ResourceId == ResourceIdentifier::SELF_NODE_ID, err = WEAVE_ERROR_UNKNOWN_RESOURCE_ID);
        // the resource ID is SELF and thus should be omitted
    }
    else if (ResourceType == Schema::Weave::Common::RESOURCE_TYPE_DEVICE)
    {
        err = aWriter.Put(aTag, ResourceId);
        SuccessOrExit(err);
    }
    else
    {
        uint8_t generalEncoding[sizeof(uint16_t) + sizeof(uint64_t)];

        nl::Weave::Encoding::LittleEndian::Put16(generalEncoding, ResourceType);
        nl::Weave::Encoding::LittleEndian::Put64(generalEncoding + 2, ResourceId);
        err = aWriter.PutBytes(aTag, generalEncoding, sizeof(generalEncoding));
        SuccessOrExit(err);
    }

exit:
    return err;
}

WEAVE_ERROR ResourceIdentifier::FromTLV(nl::Weave::TLV::TLVReader & aReader)
{
    return FromTLV(aReader, kNodeIdNotSpecified);
}

WEAVE_ERROR ResourceIdentifier::FromTLV(nl::Weave::TLV::TLVReader & aReader, const uint64_t & aSelfNodeId)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    if (aReader.GetType() == TLV::kTLVType_ByteString)
    {
        if (aReader.GetLength() == (sizeof(uint16_t) + sizeof(uint64_t)))
        {
            uint8_t generalEncoding[sizeof(uint16_t) + sizeof(uint64_t)];

            err = aReader.GetBytes(generalEncoding, sizeof(uint16_t) + sizeof(uint64_t));
            SuccessOrExit(err);

            ResourceType = nl::Weave::Encoding::LittleEndian::Get16(&(generalEncoding[0]));
            ResourceId   = nl::Weave::Encoding::LittleEndian::Get64(&(generalEncoding[2]));
        }
        else
        {
            err = WEAVE_ERROR_WRONG_TLV_TYPE;
            ExitNow();
        }
    }
    else
    {
        err = aReader.Get(ResourceId);
        SuccessOrExit(err);

        ResourceType = Schema::Weave::Common::RESOURCE_TYPE_DEVICE;
    }

    NormalizeResource(aSelfNodeId);
exit:
    return err;
}

void ResourceIdentifier::NormalizeResource(void)
{
    NormalizeResource(kNodeIdNotSpecified);
}

void ResourceIdentifier::NormalizeResource(const uint64_t & aSelfNodeId)
{
    if (ResourceType == Schema::Weave::Common::RESOURCE_TYPE_DEVICE)
    {
        if ((aSelfNodeId != kNodeIdNotSpecified) && (aSelfNodeId == ResourceId))
            ResourceId = SELF_NODE_ID;

        if (ResourceId == SELF_NODE_ID)
            ResourceType = RESOURCE_TYPE_RESERVED;
    }
}

const char * ResourceIdentifier::ResourceTypeAsString(void) const
{
    return ResourceTypeAsString(ResourceType);
}

const char * ResourceIdentifier::ResourceTypeAsString(uint16_t aResourceType)
{
    const char * retval;
    switch (aResourceType)
    {
    case RESOURCE_TYPE_RESERVED:
        retval = "RESERVED";
        break;
    case Schema::Weave::Common::RESOURCE_TYPE_DEVICE:
        retval = "DEVICE";
        break;
    case Schema::Weave::Common::RESOURCE_TYPE_USER:
        retval = "USER";
        break;
    case Schema::Weave::Common::RESOURCE_TYPE_ACCOUNT:
        retval = "ACCOUNT";
        break;
    case Schema::Weave::Common::RESOURCE_TYPE_AREA:
        retval = "AREA";
        break;
    case Schema::Weave::Common::RESOURCE_TYPE_FIXTURE:
        retval = "FIXTURE";
        break;
    case Schema::Weave::Common::RESOURCE_TYPE_GROUP:
        retval = "GROUP";
        break;
    case Schema::Weave::Common::RESOURCE_TYPE_ANNOTATION:
        retval = "ANNOTATION";
        break;
    case Schema::Weave::Common::RESOURCE_TYPE_STRUCTURE:
        retval = "STRUCTURE";
        break;
    case Schema::Weave::Common::RESOURCE_TYPE_GUEST:
        retval = "GUEST";
        break;
    case Schema::Weave::Common::RESOURCE_TYPE_SERVICE:
        retval = "SERVICE";
        break;
    default:
        retval = NULL;
    }
    return retval;
}

WEAVE_ERROR ResourceIdentifier::ToString(char * outBuffer, size_t outBufferLen)
{
    const char * resourceTypeString = ResourceTypeAsString(ResourceType);

    if (ResourceType == RESOURCE_TYPE_RESERVED)
    {
        if (ResourceId == kNodeIdNotSpecified)
        {
            snprintf(outBuffer, outBufferLen, "RESERVED_NOT_SPECIFIED");
        }
        else if (ResourceId == SELF_NODE_ID)
        {
            snprintf(outBuffer, outBufferLen, "RESERVED_DEVICE_SELF");
        }
        else
        {
            snprintf(outBuffer, outBufferLen, "%s_%" PRIX64, resourceTypeString, ResourceId);
        }
    }
    else if (resourceTypeString != NULL)
    {
        snprintf(outBuffer, outBufferLen, "%s_%016" PRIX64, resourceTypeString, ResourceId);
    }
    else
    {
        snprintf(outBuffer, outBufferLen, "(%04X)_%" PRIX64, ResourceType, ResourceId);
    }
    return WEAVE_NO_ERROR;
}

WEAVE_ERROR ResourceIdentifier::FromString(const char * inBuffer, size_t bufferLen)
{
    return FromString(inBuffer, bufferLen, kNodeIdNotSpecified);
}

WEAVE_ERROR ResourceIdentifier::FromString(const char * inBuffer, size_t bufferLen, const uint64_t & aSelfNodeId)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint16_t resourceType;
    const char * resourceName;
    char * endPtr;
    uint32_t r_lower, r_upper;
    const size_t hexDigitsInInt32 = 8;
    char uintbuffer[hexDigitsInInt32 + 1];

    for (resourceType = Schema::Weave::Common::RESOURCE_TYPE_DEVICE; resourceType <= Schema::Weave::Common::RESOURCE_TYPE_SERVICE;
         resourceType++)
    {
        resourceName                 = ResourceTypeAsString(resourceType);
        const size_t resourceNameLen = (resourceName == NULL ? 0 : strlen(resourceName));

        if (resourceName == NULL)
            continue;

        if ((resourceNameLen + 1) > bufferLen)
            continue;

        if ((strncmp(resourceName, inBuffer, resourceNameLen) == 0) && inBuffer[resourceNameLen] == '_')
        {
            // Ensure there is at least 1 character after the prefix.
            bufferLen -= resourceNameLen + 1;
            VerifyOrExit(bufferLen > 1, err = WEAVE_ERROR_INVALID_ARGUMENT);
            // skip over the prefix
            inBuffer += resourceNameLen + 1;
            break;
        }
    }

    VerifyOrExit(resourceType <= Schema::Weave::Common::RESOURCE_TYPE_SERVICE, err = WEAVE_ERROR_INVALID_ARGUMENT);

    // Content after the 16 hexadecimal digits will be ignored.
    if (bufferLen > (2 * hexDigitsInInt32))
        bufferLen = (2 * hexDigitsInInt32);

    if (bufferLen > hexDigitsInInt32)
    {
        memcpy(uintbuffer, inBuffer + bufferLen - hexDigitsInInt32, hexDigitsInInt32);
        uintbuffer[hexDigitsInInt32] = '\0';

        r_lower = strtoul(uintbuffer, &endPtr, 16);
        VerifyOrExit(strlen(endPtr) == 0, err = WEAVE_ERROR_INVALID_ARGUMENT);

        memset(uintbuffer, 0, sizeof(uintbuffer));
        memcpy(uintbuffer, inBuffer, bufferLen - hexDigitsInInt32);

        r_upper = strtoul(uintbuffer, &endPtr, 16);
        VerifyOrExit(strlen(endPtr) == 0, err = WEAVE_ERROR_INVALID_ARGUMENT);
    }
    else
    {
        r_upper = 0;
        memset(uintbuffer, 0, sizeof(uintbuffer));
        memcpy(uintbuffer, inBuffer, bufferLen);

        r_lower = strtoul(uintbuffer, &endPtr, 16);
        VerifyOrExit(strlen(endPtr) == 0, err = WEAVE_ERROR_INVALID_ARGUMENT);
    }

    ResourceId   = ((uint64_t) r_upper) << 32 | ((uint64_t) r_lower);
    ResourceType = resourceType;

    NormalizeResource(aSelfNodeId);

exit:
    return err;
}

}; // namespace WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current)
}; // namespace Profiles
}; // namespace Weave
}; // namespace nl
