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
 *      This file defines the class representing the
 *      ResourceIdentifier within the Weave Data Management (WDM)
 *      profile.
 *
 */

#ifndef _WEAVE_DATA_MANAGEMENT_RESOURCE_IDENTIFIER_CURRENT_H
#define _WEAVE_DATA_MANAGEMENT_RESOURCE_IDENTIFIER_CURRENT_H

#include <string.h>

#include <Weave/Profiles/data-management/Current/WdmManagedNamespace.h>
#include <Weave/Core/WeaveCore.h>
#include <Weave/Core/WeaveTLV.h>
#include <Weave/Core/WeaveFabricState.h>
#include <Weave/Common/ResourceTypeEnum.h>

namespace nl {
namespace Weave {
namespace Profiles {
namespace WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current) {

/**
 * @class ResourceIdentifier
 *
 * @brief
 *    A structure encapsulating the ID of a resource.
 *
 * The ResourceIdentifier may be either a generalized 64-bit object ID
 * of a particular type or a UUID.  When encoded externally, the
 * following representations are supported:
 *
 * -- an unsigned 64-bit integer corresponding to the generalized
      object of type DEVICE with the ID specified in the
      representation
 *
 * -- a generalized representation encoded as a byte string of 10
      octets. The first two octets encode the resource type as a
      16-bit, little endian integer, with the remaining 8 bytes
      encoding a little endian 64-bit resource ID.
 *
 * ResourceIdentifiers also embody the conventions present throughout
 * the WDM code: an empty ResourceIdentifier corresponds to the node
 * ID of the DEVICE, and constructors are provided for the most common
 * usecases.
 */
class ResourceIdentifier
{
public:
    /**
     *  Construct a ResourceIdentifier corresponding to an unspecified
     *  ResourceID. The unspecified resource ID is a tuple consisting
     *  of an RESERVED resource type with an kNodeIdNotSpeficied
     *  resource.
     */
    ResourceIdentifier() : ResourceType(RESOURCE_TYPE_RESERVED), ResourceId(nl::Weave::kNodeIdNotSpecified) { };

    /**
     * Construct a ResourceIdentifier of type DEVICE based on a given aNodeId
     *
     * @param [in] aNodeId NodeId of the given Resource ID
     */
    ResourceIdentifier(const uint64_t & aNodeId) : ResourceType(Schema::Weave::Common::RESOURCE_TYPE_DEVICE), ResourceId(aNodeId)
    {
        NormalizeResource();
    };

    /**
     * Construct the ResourceIdentifier of the specified type with the given ID
     *
     * @param [in] aResourceType The Type of a resource to be named
     * @param [in] aResourceId   The ID of the resource to be named
     */
    ResourceIdentifier(uint16_t aResourceType, const uint64_t & aResourceId) : ResourceType(aResourceType), ResourceId(aResourceId)
    {
        NormalizeResource();
    };

    /**
     * Construct the ResourceIdentifier of the specified type with the given ID
     *
     * @param [in] aResourceType The Type of a resource to be named
     * @param [in] aResourceId   The ID of the resource to be named represented as an array of bytes
     * @param [in] aResourceIdLen The length of the ID in bytes
     */
    ResourceIdentifier(uint16_t aResourceType, const uint8_t * aResourceId, size_t aResourceIdLen) : ResourceType(aResourceType)
    {
        memset(ResourceIdBytes, 0, sizeof(ResourceIdBytes));

        if (aResourceIdLen > sizeof(ResourceIdBytes))
            aResourceIdLen = sizeof(ResourceIdBytes);

        memcpy(ResourceIdBytes, aResourceId, aResourceIdLen);
    };

    enum
    {
        /** A reserved resource type.  The enum is chosen such that it
         * does not conflict with the enum values from the
         * Schema::Weave:::Common::ResourceType enums.  At the moment,
         * two ResourceId values are possible for the RESERVED
         * resource type: a kNodeIdNotSpecified corresponds to an
         * unitialized ResourceIdentifier, and SELF_NODE_ID
         * corresponds to a resource that will remap onto SELF from
         * any other representation. */
        RESOURCE_TYPE_RESERVED = 0,
    };

    /**
     * Defines a special value for NodeId that refers to 'self'. In
     * certain WDM interactions, having a value of self for resource
     * allows for compressing out that information as it is redundant
     * to the source node id of the device expressed in the Weave.
     * message itself
     */
    enum
    {
        SELF_NODE_ID = 0xFFFFFFFFFFFFFFFEULL,
    };

    enum
    {
        MAX_STRING_LENGTH = 28, // strlen("ANNOTATION") + strlen("_") + 16 + 1 (for NUL)
    };

    /**
     * @brief
     *   Serialize the resource to a TLV representation using a context Path::ResourceID tag
     *
     * @param [in] aWriter A TLV writer to serialize the ResourceIdentifier into
     */
    WEAVE_ERROR ToTLV(nl::Weave::TLV::TLVWriter & aWriter) const;

    /**
     * @brief
     *   Serialize the resource to a TLV representation using a given tag
     *
     * @param [in] aWriter A TLV writer to serialize the ResourceIdentifier into
     * @param [in] aTag    A tag naming the serialized ResourceIdentifier
     */
    WEAVE_ERROR ToTLV(nl::Weave::TLV::TLVWriter & aWriter, const uint64_t & aTag) const;

    /**
     * @brief
     *   Deserialize a ResourceIdentifier from a TLV representaion into this object
     *
     * @param [in] aReader A TLV reader positioned on the ResourceIdentifier element
     *
     * @return WEAVE_NO_ERROR on success. Errors from TLVReader if the ResourceIdentifier cannot be properly read.
     */
    WEAVE_ERROR FromTLV(nl::Weave::TLV::TLVReader & aReader);

    /**
     * @brief
     *   Deserialize a ResourceIdentifier from a TLV representaion into this object
     *
     * @param [in] aReader A TLV reader positioned on the ResourceIdentifier element
     *
     * @param [in] aSelfNodeId a 64-bit ResourceID that will be remapped from the serialized representation onto SELF_NODE_ID
     *
     * @return WEAVE_NO_ERROR on success. Errors from TLVReader if the ResourceIdentifier cannot be properly read.
     */
    WEAVE_ERROR FromTLV(nl::Weave::TLV::TLVReader & aReader, const uint64_t & aSelfNodeId);

    /**
     * @brief
     *   Convert the ResourceIdentifier into a printable string.
     *
     * @param [in] buffer    A buffer to print into
     * @param [in] bufferLen The length of the buffer
     */
    WEAVE_ERROR ToString(char * buffer, size_t bufferLen);

    /**
     * @brief
     *  Parse a canonical string representation of a resource into a resource object.
     *
     * Converts the canonical string representation of a resource into
     * a resource object. Note that only a subset of resources can be
     * represented as a string, in particular, the reference
     * implementation in Weave will only parse resources of canonical
     * types as expressed in the ResourceTypeEnum.
     *
     * @param [in] inBuffer A buffer containing the resource ID to be parsed
     *
     * @param [in] inBufferLen The length (in bytes) of the string to be parsed
     *
     */
    WEAVE_ERROR FromString(const char * inBuffer, size_t inBufferLen);

    /**
     * @brief
     *  Parse a canonical string representation of a resource into a resource object.
     *
     * Converts the canonical string representation of a resource into
     * a resource object. Note that only a subset of resources can be
     * represented as a string, in particular, the reference
     * implementation in Weave will only parse resources of canonical
     * types as expressed in the ResourceTypeEnum.
     *
     * @param [in] inBuffer A buffer containing the resource ID to be parsed
     *
     * @param [in] inBufferLen The length (in bytes) of the string to be parsed
     *
     * @param [in] aSelfNodeId The 64-bit ID denoting what device ID should be mapped onto a SELF_NODE_ID
     *
     */
    WEAVE_ERROR FromString(const char * inBuffer, size_t inBufferLen, const uint64_t & aSelfNodeId);

    /**
     * @brief
     *   Produce a string representation of the ResourceType.  The resources types converted are those enumerated in
     * ResourceTypeEnum.h and the 0 (corresponding to the RESOURCE_TYPE_RESERVED)
     */

    const char * ResourceTypeAsString(void) const;

    /**
     * @brief
     *   Produce a string representation of a resource type.  The resources types converted are those enumerated in
     * ResourceTypeEnum.h and the 0 (corresponding to the RESOURCE_TYPE_RESERVED)
     *
     * @param [in] aResourceType A resource type enum to be converted to a string representation.
     */
    static const char * ResourceTypeAsString(uint16_t aResourceType);

    /**
     * @brief
     *   An accessor function for fetching the ResourceId
     */
    uint64_t GetResourceId() const { return ResourceId; };

    /**
     * @brief
     *   An accessor function for fetching the ResourceType
     */
    uint16_t GetResourceType() const { return ResourceType; };

    friend bool operator ==(const ResourceIdentifier & lhs, const ResourceIdentifier & rhs);

public:
    void NormalizeResource(void);
    void NormalizeResource(const uint64_t & aSelfNodeId);
    uint16_t ResourceType;
    union
    {
        uint64_t ResourceId;
        uint8_t ResourceIdBytes[8];
    };
};

inline bool operator ==(const ResourceIdentifier & lhs, const ResourceIdentifier & rhs)
{
    return lhs.ResourceId == rhs.ResourceId && lhs.ResourceType == rhs.ResourceType;
};

inline bool operator !=(const ResourceIdentifier & lhs, const ResourceIdentifier & rhs)
{
    return !(lhs == rhs);
}

}; // namespace WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current)
}; // namespace Profiles
}; // namespace Weave
}; // namespace nl

#endif // _WEAVE_DATA_MANAGEMENT_MESSAGE_DEF_CURRENT_H
