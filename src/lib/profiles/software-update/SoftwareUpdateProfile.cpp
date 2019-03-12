/*
 *    Copyright (c) 2019 Google LLC
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

/**
 *    @file
 *      This file implements message descriptions for the messages in
 *      the software update profile.
 *
 */

#include <Weave/Core/WeaveCore.h>
#include <Weave/Core/WeaveMessageLayer.h>
#include <Weave/Profiles/ProfileCommon.h>
#include "SoftwareUpdateProfile.h"
using namespace ::nl::Weave;

namespace nl {
namespace Weave {
namespace Profiles {
namespace SoftwareUpdate {

/**
 * The default constructor for an IntegrityTypeList.  Constructs a logically empty list.  The list may be populated via the init()
 * method or by deserializing the list from a message.
 */
IntegrityTypeList::IntegrityTypeList()
{
    theLength = 0;
    for (int i = 0; i < kIntegrityType_Last; i++)
        theList[i] = kIntegrityType_SHA160;
}

/**
 * Explicitly initialize the IntegrityTypeList with an list of supported IntegrityTypes
 *
 * @param[in] aLength   An 8-bit value for the length of the list.  Must be less that the number of enums in @ref IntegrityTypes.
 * @param[in] aList  A pointer to an array of @ref IntegrityTypes values.  May be NULL only if aLength is 0.
 *
 * @retval WEAVE_NO_ERROR  On success.
 * @retval WEAVE_ERROR_INVALID_LIST_LENGTH If the length is too long
 */
WEAVE_ERROR IntegrityTypeList::init(uint8_t aLength, uint8_t * aList)
{
    if (aLength > kIntegrityType_Last)
        return WEAVE_ERROR_INVALID_LIST_LENGTH;

    theLength = aLength;
    for (int i = 0; i < theLength; i++)
        theList[i] = aList[i];

    return WEAVE_NO_ERROR;
}

/**
 * Serialize the object to the provided MessageIterator
 *
 * @param[in] i An iterator over the message being packed
 *
 * @retval WEAVE_NO_ERROR  On success.
 * @retval WEAVE_ERROR_BUFFER_TOO_SMALL If the list is too long to fit in the message.
 */
WEAVE_ERROR IntegrityTypeList::pack(MessageIterator & i)
{
    if (i.hasRoom(theLength + 1))
    {
        i.writeByte(theLength);
        for (int j = 0; j < theLength; j++)
            i.writeByte(theList[j]);

        return WEAVE_NO_ERROR;
    }
    else
        return WEAVE_ERROR_BUFFER_TOO_SMALL;
}

/**
 * Deserialize the object from the given MessageIterator into provided IntegrityTypeList
 *
 * @param[in] i An iterator over the message being parsed.
 * @param[in] aList A reference to an object to contain the result
 *
 * @retval WEAVE_NO_ERROR  On success.
 * @retval WEAVE_ERROR_BUFFER_TOO_SMALL Message was too short.
 * @retval WEAVE_ERROR_INVALID_LIST_LENGTH If the message contained an invalid list length (either not enough data to fill in the
 * list or too many to fit within the limits)
 */
WEAVE_ERROR IntegrityTypeList::parse(MessageIterator & i, IntegrityTypeList & aList)
{
    TRY(i.readByte(&aList.theLength));

    if (aList.theLength > kIntegrityType_Last)
    {
        aList.theLength = 0;
        return WEAVE_ERROR_INVALID_LIST_LENGTH;
    }

    if (!i.hasData(aList.theLength))
        return WEAVE_ERROR_BUFFER_TOO_SMALL;

    for (int j = 0; j < aList.theLength; j++)
        i.readByte(&aList.theList[j]);

    return WEAVE_NO_ERROR;
}

/**
 * An equality operator
 *
 * @param another A list to check against this list
 * @return true if the lists are equal, false otherwise
 */
bool IntegrityTypeList::operator ==(const IntegrityTypeList & another) const
{
    bool retval = (theLength == another.theLength);
    int i = 0;

    while (retval && (i < theLength))
    {
        retval = (theList[i] == another.theList[i]);
        i++;
    }

    return retval;
}

/**
 * The default constructor for an UpdateSchemeList.  Constructs a logically empty list.  The list may be populated via the init()
 * method or by deserializing the list from a message.
 */
UpdateSchemeList::UpdateSchemeList()
{
    theLength = 0;
    for (int i = 0; i < kUpdateScheme_Last; i++)
        theList[i] = kUpdateScheme_HTTP;
}

/**
 * Explicitly initialize the IntegrityTypeList with an list of supported IntegrityTypes
 *
 * @param[in] aLength   An 8-bit value for the length of the list.  Must be less that the number of enums in @ref UpdateSchemes.
 * @param[in] aList  A pointer to an array of @ref UpdateSchemes values.  May be NULL only if aLength is 0.
 *
 * @retval WEAVE_NO_ERROR  On success.
 * @retval WEAVE_ERROR_INVALID_LIST_LENGTH If the length is too long
 */
WEAVE_ERROR UpdateSchemeList::init(uint8_t aLength, uint8_t * aList)
{
    if (aLength > kUpdateScheme_Last)
        return WEAVE_ERROR_INVALID_LIST_LENGTH;

    theLength = aLength;
    for (int i = 0; i < theLength; i++)
        theList[i] = aList[i];

    return WEAVE_NO_ERROR;
}

/**
 * Serialize the object to the provided MessageIterator
 *
 * @param[in] i An iterator over the message being packed
 *
 * @retval WEAVE_NO_ERROR  On success.
 * @retval WEAVE_ERROR_BUFFER_TOO_SMALL If the list is too long to fit in the message.
 */
WEAVE_ERROR UpdateSchemeList::pack(MessageIterator & i)
{
    if (i.hasRoom(theLength + 1))
    {
        i.writeByte(theLength);
        for (int j = 0; j < theLength; j++)
            i.writeByte(theList[j]);

        return WEAVE_NO_ERROR;
    }
    else
        return WEAVE_ERROR_BUFFER_TOO_SMALL;
}

/**
 * Deserialize the object from the given MessageIterator into provided UpdateSchemeList
 *
 * @param[in] i An iterator over the message being parsed.
 * @param[in] aList A reference to an object to contain the result
 *
 * @retval WEAVE_NO_ERROR  On success.
 * @retval WEAVE_ERROR_BUFFER_TOO_SMALL Message was too short.
 * @retval WEAVE_ERROR_INVALID_LIST_LENGTH If the message contained an invalid list length (either not enough data to fill in the
 * list or too many to fit within the limits)
 */
WEAVE_ERROR UpdateSchemeList::parse(MessageIterator & i, UpdateSchemeList & aList)
{
    TRY(i.readByte(&aList.theLength));
    if (!i.hasData(aList.theLength))
        return WEAVE_ERROR_BUFFER_TOO_SMALL;

    if (aList.theLength > kUpdateScheme_Last)
        return WEAVE_ERROR_INVALID_LIST_LENGTH;

    for (int j = 0; j < aList.theLength; j++)
        i.readByte(&aList.theList[j]);

    return WEAVE_NO_ERROR;
}

/**
 * An equality operator
 *
 * @param another A list to check against this list
 * @return true if the lists are equal, false otherwise
 */
bool UpdateSchemeList::operator ==(const UpdateSchemeList & another) const
{
    bool retval = (theLength == another.theLength);
    int i = 0;

    while (retval && (i < theLength))
    {
        retval = (theList[i] == another.theList[i]);
        i++;
    }

    return retval;
}

/**
 * A constructor for the ProductSpec object
 *
 * @param[in] aVendor The vendor identifier for the specified product
 * @param[in] aProduct  Vendor-specific product identifier
 * @param[in] aRevision  Vendor-specific product revision number
 */
ProductSpec::ProductSpec(uint16_t aVendor, uint16_t aProduct, uint16_t aRevision)
{
    vendorId   = aVendor;
    productId  = aProduct;
    productRev = aRevision;
}

/**
 * A default constructor that creates an invalid ProductSpec object. Used in cases where the object is  being deserialized from a
 * message.
 */
ProductSpec::ProductSpec()
{
    vendorId = productId = productRev = 0;
}

/**
 * An equality operator
 *
 * @param another A ProductSpec to check against this ProductSpec
 * @return true if all the fields in both objects are equal, false otherwise
 */
bool ProductSpec::operator ==(const ProductSpec & another) const
{
    return ((vendorId == another.vendorId) && (productId == another.productId) && (productRev == another.productRev));
}

/**
 * Default constructor for ImageQuery. The ImageQuery may be populated by calling init() or by deserializing the object from a
 * message.
 */
ImageQuery::ImageQuery()
{
    version.theLength        = 0;
    version.isShort          = true;
    integrityTypes.theLength = 0;
    updateSchemes.theLength  = 0;
    packageSpec.theLength    = 0;
    packageSpec.isShort      = true;
    localeSpec.theLength     = 0;
    localeSpec.isShort       = true;
    targetNodeId             = 0;
}
/**
 * Explicitly initialize the ImageQuery object with the provided values.
 *
 * @param[in] aProductSpec Product specification.
 * @param[in] aVersion Currently installed version of software.
 * @param[in] aTypeList The integrity types supported by the client.
 * @param[in] aSchemeList The update schemes supported by the client.
 * @param[in] aPackage An optional package spec supported by the client.
 * @param[in] aLocale An optional locale spec requested by the client.
 * @param[in] aTargetNodeId An optional target node ID.
 * @param[in] aMetaData An optional TLV-encoded vendor data blob.
 *
 * @return WEAVE_NO_ERROR Unconditionally.
 */
WEAVE_ERROR ImageQuery::init(ProductSpec & aProductSpec, ReferencedString & aVersion, IntegrityTypeList & aTypeList,
                             UpdateSchemeList & aSchemeList, ReferencedString * aPackage, ReferencedString * aLocale,
                             uint64_t aTargetNodeId, ReferencedTLVData * aMetaData)
{
    productSpec     = aProductSpec;
    version         = aVersion;
    version.isShort = true;
    integrityTypes  = aTypeList;
    updateSchemes   = aSchemeList;
    if (aPackage != NULL)
    {
        packageSpec         = *aPackage;
        packageSpec.isShort = true;
    }
    if (aLocale != NULL)
        localeSpec = *aLocale;
    targetNodeId = aTargetNodeId;
    if (aMetaData != NULL)
        theMetaData = *aMetaData;

    return WEAVE_NO_ERROR;
}
/**
 * Serialize the underlying ImageQuery into the provided PacketBuffer
 *
 * @param[in] aBuffer A packet buffer into which to pack the query
 *
 * @retval WEAVE_NO_ERROR On success
 * @retval WEAVE_ERROR_BUFFER_TOO_SMALL If the ImageQuery is too large to fit in the provided buffer.
 */
WEAVE_ERROR ImageQuery::pack(PacketBuffer * aBuffer)
{
    MessageIterator i(aBuffer);
    i.append();
    // figure out what the frame control field looks like.
    uint8_t frameCtl = 0;
    if (packageSpec.theLength != 0)
        frameCtl |= kFlag_PackageSpecPresent;
    if (localeSpec.theLength != 0)
        frameCtl |= kFlag_LocaleSpecPresent;
    if (targetNodeId != 0)
        frameCtl |= kFlag_TargetNodeIdPresent;
    // now write it
    TRY(i.writeByte(frameCtl));
    // write the product spec
    TRY(i.write16(productSpec.vendorId));
    TRY(i.write16(productSpec.productId));
    TRY(i.write16(productSpec.productRev));
    // pack the version string
    TRY(version.pack(i));
    // now do the integrity types and update schemes
    TRY(integrityTypes.pack(i));
    TRY(updateSchemes.pack(i));
    // now the optional fields
    if (packageSpec.theLength != 0)
        TRY(packageSpec.pack(i));
    if (localeSpec.theLength != 0)
        TRY(localeSpec.pack(i));
    if (targetNodeId != 0)
        TRY(i.write64(targetNodeId));
    TRY(theMetaData.pack(i));

    return WEAVE_NO_ERROR;
}
/**
 * Deserialize the image query message provided in a PacketBuffer into a provided ImageQuery
 *
 * @param[in] aBuffer A pointer to a packet from which to parse the image query
 * @param[in] aQuery An object in which to put the result
 *
 * @return WEAVE_NO_ERROR On success
 * @return WEAVE_ERROR_BUFFER_TOO_SMALL If the message was too small to contain all the fields of the ImageQuery
 * @return WEAVE_ERROR_INVALID_LIST_LENGTH If the message contained an IntegrityTypeList or the UpdateSchemeList that was too long
 */
WEAVE_ERROR ImageQuery::parse(PacketBuffer * aBuffer, ImageQuery & aQuery)
{
    MessageIterator i(aBuffer);
    // get the frame control field this doesn't actually become part of
    // the message object.
    uint8_t frameCtl;
    TRY(i.readByte(&frameCtl));
    // now get the product spec.
    TRY(i.read16(&aQuery.productSpec.vendorId));
    TRY(i.read16(&aQuery.productSpec.productId));
    TRY(i.read16(&aQuery.productSpec.productRev));
    // get the version string
    TRY(ReferencedString::parse(i, aQuery.version));
    // now do integrity types and update schemes.
    TRY(IntegrityTypeList::parse(i, aQuery.integrityTypes));
    TRY(UpdateSchemeList::parse(i, aQuery.updateSchemes));
    // if a package is provided then get it
    if (frameCtl & kFlag_PackageSpecPresent)
        TRY(ReferencedString::parse(i, aQuery.packageSpec));
    // if a locale is provided then get it
    if (frameCtl & kFlag_LocaleSpecPresent)
        TRY(ReferencedString::parse(i, aQuery.localeSpec));
    // if a target node id is provided then get it
    if (frameCtl & kFlag_TargetNodeIdPresent)
        TRY(i.read64(&aQuery.targetNodeId));
    // and maybe the metadata
    ReferencedTLVData::parse(i, aQuery.theMetaData);

    return WEAVE_NO_ERROR;
}

/**
 * An equality operator
 *
 * @param another An ImageQuery to check against this ImageQuery
 * @return true if all the fields in both objects are equal, false otherwise
 */
bool ImageQuery::operator ==(const ImageQuery & another) const
{
    return ((productSpec.vendorId == another.productSpec.vendorId) && (productSpec.productId == another.productSpec.productId) &&
            (productSpec.productRev == another.productSpec.productRev) && (version == another.version) &&
            (integrityTypes == another.integrityTypes) && (updateSchemes == another.updateSchemes) &&
            (packageSpec == another.packageSpec) && (localeSpec == another.localeSpec) && (targetNodeId == another.targetNodeId) &&
            (theMetaData == another.theMetaData));
}

/**
 * A support method mapping the @ref IntegrityTypes values onto the lengths of the hashes of that type.
 *
 * @param[in] aType An @ref IntegrityTypes value
 * @return Length of the hash of the provided hash type.
 */
inline int integrityLength(uint8_t aType)
{
    switch (aType)
    {
    case kIntegrityType_SHA160:
        return kLength_SHA160;
    case kIntegrityType_SHA256:
        return kLength_SHA256;
    case kIntegrityType_SHA512:
        return kLength_SHA512;
    default:
        return 0;
    }
}
/**
 * The default constructor for IntegritySpec. The object must be initialized either via the init() method or via deserializing it
 * from a message.
 */
IntegritySpec::IntegritySpec()
{
    type = kIntegrityType_SHA160;
}
/**
 * Explicitly initialize the IntegritySpec object with provided values.
 *
 * @param[in] aType An integrity type value drawn from @ref IntegrityTypes
 * @param[in] aValue A hash value of the appropriate length represented as a packed string of bytes
 *
 * @return WEAVE_NO_ERROR On success
 * @return WEAVE_ERROR_INVALID_INTEGRITY_TYPE If the provided integrity type is not one of the values specified in @ref
 * IntegrityTypes
 */
WEAVE_ERROR IntegritySpec::init(uint8_t aType, uint8_t * aValue)
{
    uint8_t len = integrityLength(aType);
    if (len == 0)
        return WEAVE_ERROR_INVALID_INTEGRITY_TYPE;

    type = aType;
    for (int i = 0; i < len; i++)
        value[i] = aValue[i];

    return WEAVE_NO_ERROR;
}
/**
 * Serialize the IntegritySpec into provided MessageIterator
 * @param[in] i An iterator over the message being packed
 *
 * @retval WEAVE_NO_ERROR  On success.
 * @retval WEAVE_ERROR_BUFFER_TOO_SMALL If the IntegritySpec is too large to fit in the message.
 */
WEAVE_ERROR IntegritySpec::pack(MessageIterator & i)
{
    TRY(i.writeByte(type));
    for (int j = 0; j < integrityLength(type); j++)
        TRY(i.writeByte(value[j]));

    return WEAVE_NO_ERROR;
}
/**
 * Deserialize the object from the provided MessageIterator into provided IntegritySpec
 * @param[in] i An iterator over the message being parsed.
 * @param[in] aSpec A reference to an object to contain the result
 *
 * @retval WEAVE_NO_ERROR  On success.
 * @return WEAVE_ERROR_INVALID_INTEGRITY_TYPE If the provided integrity type is not one of the values specified in @ref
 * IntegrityTypes
 * @retval WEAVE_ERROR_BUFFER_TOO_SMALL If the message did not contain enough bytes for the integrity type and the associated hash
 */
WEAVE_ERROR IntegritySpec::parse(MessageIterator & i, IntegritySpec & aSpec)
{
    TRY(i.readByte(&aSpec.type));
    if (integrityLength(aSpec.type) == 0)
        return WEAVE_ERROR_INVALID_INTEGRITY_TYPE;

    for (int j = 0; j < integrityLength(aSpec.type); j++)
        TRY(i.readByte(&aSpec.value[j]));

    return WEAVE_NO_ERROR;
}

/**
 * An equality operator
 *
 * @param another An IntegritySpec to check against this IntegritySpec
 * @return true if all the fields in both objects are equal, false otherwise
 */
bool IntegritySpec::operator ==(const IntegritySpec & another) const
{
    bool retval = (type == another.type);
    int i = 0;

    while (retval && (i < integrityLength(type)))
    {
        retval = (value[i] == another.value[i]);
        i++;
    }

    return retval;
}
/**
 * Explicitly initialize the ImageQueryResponse object with the provided values.
 *
 * @param[in] aUri The URI at which the new firmware image is to be found.
 * @param[in] aVersion The version string for this image.
 * @param[in] aIntegrity The integrity spec corresponding to the new image.
 * @param[in] aScheme The update scheme to use in downloading.
 * @param[in] aPriority The update priority associated with this update.
 * @param[in] aCondition The condition under which to update.
 * @param[in] aReportStatus If true requests the client to report after download and update, otherwise the client will not report.
 *
 * @return WEAVE_NO_ERROR Unconditionally.
 */
WEAVE_ERROR ImageQueryResponse::init(ReferencedString & aUri, ReferencedString & aVersion, IntegritySpec & aIntegrity,
                                     uint8_t aScheme, UpdatePriority aPriority, UpdateCondition aCondition, bool aReportStatus)
{
    uri                 = aUri;
    versionSpec         = aVersion;
    versionSpec.isShort = true;
    integritySpec       = aIntegrity;
    updateScheme        = aScheme;
    updatePriority      = aPriority;
    updateCondition     = aCondition;
    reportStatus        = aReportStatus;

    return WEAVE_NO_ERROR;
}
/**
 * The default constructor for ImageQueryResponse.  The ImageQueryResponse may be populated by via the init() method or by
 * deserializing the object from a message.
 */
ImageQueryResponse::ImageQueryResponse() : uri()
{
    versionSpec.theLength = 0;
    versionSpec.isShort   = true;
    updateScheme          = kUpdateScheme_HTTP;
    reportStatus          = false;
}
/**
 * Serialize the ImageQueryResponse into the provided PacketBuffer
 *
 * @param[in] aBuffer  A packet buffer into which to pack the query response
 *
 * @retval WEAVE_NO_ERROR  On success.
 * @retval WEAVE_ERROR_BUFFER_TOO_SMALL If the ImageQueryResponse is too large to fit in the provided buffer.
 */
WEAVE_ERROR ImageQueryResponse::pack(PacketBuffer * aBuffer)
{
    MessageIterator i(aBuffer);
    i.append();
    TRY(uri.pack(i));
    TRY(versionSpec.pack(i));
    TRY(integritySpec.pack(i));
    TRY(i.writeByte((uint8_t) updateScheme));
    uint8_t updateOptions = (uint8_t) updatePriority | ((uint8_t) updateCondition << kOffset_UpdateCondition);
    if (reportStatus)
        updateOptions |= kMask_ReportStatus;
    TRY(i.writeByte(updateOptions));

    return WEAVE_NO_ERROR;
}
/**
 * Deserialize the image query response message provided in a PacketBuffer into a provided ImageQueryResponse
 *
 * @param[in] aBuffer A pointer to a packet from which to parse the image query
 * @param[in] aResponse An object in which to put the result
 *
 * @return WEAVE_NO_ERROR On success
 * @return WEAVE_ERROR_BUFFER_TOO_SMALL If the message was too small to contain all the fields of the ImageQuery
 * @return WEAVE_ERROR_INVALID_INTEGRITY_TYPE If the provided integrity type is not one of the values specified in @ref
 * IntegrityTypes
 */
WEAVE_ERROR ImageQueryResponse::parse(PacketBuffer * aBuffer, ImageQueryResponse & aResponse)
{
    MessageIterator i(aBuffer);
    uint8_t updateOptions;
    TRY(ReferencedString::parse(i, aResponse.uri));
    TRY(ReferencedString::parse(i, aResponse.versionSpec));
    TRY(IntegritySpec::parse(i, aResponse.integritySpec));
    TRY(i.readByte(&aResponse.updateScheme));
    TRY(i.readByte(&updateOptions));
    aResponse.updatePriority  = (UpdatePriority)(updateOptions & kMask_UpdatePriority);
    aResponse.updateCondition = (UpdateCondition)((updateOptions & kMask_UpdateCondition) >> kOffset_UpdateCondition);
    aResponse.reportStatus    = (updateOptions & kMask_ReportStatus) == kMask_ReportStatus;

    return WEAVE_NO_ERROR;
}
/**
 * An equality operator
 *
 * @param another An ImageQueryResponse to check against this ImageQueryResponse
 * @return true if all the fields in both objects are equal, false otherwise
 */
bool ImageQueryResponse::operator ==(const ImageQueryResponse & another) const
{
    return ((uri == another.uri) && (versionSpec == another.versionSpec) && (integritySpec == another.integritySpec) &&
            (updateScheme == another.updateScheme) && (updatePriority == another.updatePriority) &&
            (updateCondition == another.updateCondition) && (reportStatus == another.reportStatus));
}

} // namespace SoftwareUpdate
} // namespace Profiles
} // namespace Weave
} // namespace nl
