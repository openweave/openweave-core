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
using namespace ::nl::Weave::Profiles;
using namespace ::nl::Weave::Profiles::SoftwareUpdate;
/*
 * -- definitions for ImageQuery and its supporting classes --
 *
 * the no-arg constructor for an IntegrityTypeList
 */
IntegrityTypeList::IntegrityTypeList()
{
  theLength=0;
  for (int i=0; i<3; i++) theList[i]=kIntegrityType_SHA160;
}
/*
 * parameters:
 * - uint8_t aLength, an 8-bit length value for the list (<4)
 * - uint8_t *aList, pointer to an array of itegrity type values
 * return:
 * - WEAVE_NO_ERROR if it's all good
 * - WEAVE_ERROR_INVALID_LIST_LENGTH if the length is too long
 */
WEAVE_ERROR IntegrityTypeList::init(uint8_t aLength, uint8_t *aList)
{
  if (aLength>3) return WEAVE_ERROR_INVALID_LIST_LENGTH;
  theLength=aLength;
  for (int i=0; i<theLength; i++) theList[i]=aList[i];
  return WEAVE_NO_ERROR;
}
/*
 * parameter: MessageIterator &i, an iterator over the message being packed
 * returns: error/status
 */
WEAVE_ERROR IntegrityTypeList::pack(MessageIterator &i)
{
  if (i.hasRoom(theLength+1)) {
    i.writeByte(theLength);
    for (int j=0; j<theLength; j++) i.writeByte(theList[j]);
    return WEAVE_NO_ERROR;
  }
  else
    return WEAVE_ERROR_INVALID_LIST_LENGTH;
}
/*
 * parameters:
 * - MessageIterator &i, an iterator over the message being parsed
 * - IntegrityTypeList &aList, a pointer to an object to contain the result
 * returns: error/status
 */
WEAVE_ERROR IntegrityTypeList::parse(MessageIterator &i, IntegrityTypeList &aList)
{
  i.readByte(&aList.theLength);
  if (!i.hasData(aList.theLength)) return WEAVE_ERROR_INVALID_LIST_LENGTH;
  for (int j=0; j<aList.theLength; j++) i.readByte(&aList.theList[j]);
  return WEAVE_NO_ERROR;
}
/*
 * parameter: IntegrityTypeList &another, another list to check against
 * returns: true if the lists are equal, false otherwise
 */
bool IntegrityTypeList::operator == (const IntegrityTypeList &another) const
{
  if (theLength!=another.theLength)
    return false;
  for (int i=0; i<theLength; i++) {
    if (theList[i]!=another.theList[i])
      return false;
  }
  return true;
}
/*
 * the no-arg constructor for an UpdateSchemeList
 */
UpdateSchemeList::UpdateSchemeList()
{
  theLength=0;
  for (int i=0; i<4; i++) theList[i]=kUpdateScheme_HTTP;
}
/*
 * parameters:
 * - uint8_t aLength, an 8-bit length value for the list (<5)
 * - uint8_t *aList, pointer to an array of update scheme values
 * return: error/status
 */
WEAVE_ERROR UpdateSchemeList::init(uint8_t aLength, uint8_t *aList)
{
  if (aLength>4) return WEAVE_ERROR_INVALID_LIST_LENGTH;
  theLength=aLength;
  for (int i=0; i<theLength; i++) theList[i]=aList[i];
  return WEAVE_NO_ERROR;
}
/*
 * parameters: MessageIterator &i,
 * returns: error/status
 */
WEAVE_ERROR UpdateSchemeList::pack(MessageIterator &i)
{
  if (i.hasRoom(theLength+1)) {
    i.writeByte(theLength);
    for (int j=0; j<theLength; j++) i.writeByte(theList[j]);
    return WEAVE_NO_ERROR;
  }
  else
    return WEAVE_ERROR_INVALID_LIST_LENGTH;
}
/*
 * parameters:
 * - MessageIterator &i, an iterator over the message being parsed
 * - UpdateSchemeList &aList, a pointer to an object into which to put the result
 * returns: the updated pointer p pointing after the parsed list
 */
WEAVE_ERROR UpdateSchemeList::parse(MessageIterator &i, UpdateSchemeList &aList)
{
  i.readByte(&aList.theLength);
  if (!i.hasData(aList.theLength)) return WEAVE_ERROR_INVALID_LIST_LENGTH;
  for (int j=0; j<aList.theLength; j++) i.readByte(&aList.theList[j]);
  return WEAVE_NO_ERROR;
}
/*
 * parameter: UpdateSchemeList &another, another list to check against
 * returns: true if the lists are equal, false otherwise
 */
bool UpdateSchemeList::operator == (const UpdateSchemeList &another) const
{
  if (theLength!=another.theLength)
    return false;
  for (int i=0; i<theLength; i++) {
    if (theList[i]!=another.theList[i])
      return false;
  }
  return true;
}
/*
 * parameters:
 * - uint16_t aVendor, the vendor identifier for the specified product
 * - uint16_t aProduct, that vendor's product identifier
 * - uint16_t aRevision, the vendor-specific product revision number
 */
ProductSpec::ProductSpec(uint16_t aVendor, uint16_t aProduct, uint16_t aRevision)
{
  vendorId=aVendor;
  productId=aProduct;
  productRev=aRevision;
}
/*
 * the no-arg constructor for a ProductSpec
 */
ProductSpec::ProductSpec()
{
  vendorId=productId=productRev=0;
}

bool ProductSpec::operator == (const ProductSpec &another) const
{
  return ((vendorId == another.vendorId) && (productId == another.productId) && (productRev == another.productRev));
}

/*
 * the no-arg constructor for an ImageQuery
 */
ImageQuery::ImageQuery()
{
  version.theLength=0;
  version.isShort=true;
  integrityTypes.theLength=0;
  updateSchemes.theLength=0;
  packageSpec.theLength=0;
  packageSpec.isShort=true;
  localeSpec.theLength=0;
  localeSpec.isShort=true;
  targetNodeId=0;
}
/*
 * parameters:
 * - ProductSpec &aProductSPec, the sending device's product specification
 * - ReferencedString &aVersion, the sending device's software version
 * - IntegrityTypeList &aTypeList, the integrity types supported by the sender
 * - UpdateSchemeList &aSchemeList, the update schemes supported by the sender
 * - ReferencedString *aPackage, the sending device's package spec (optional)
 * - ReferencedString *aLocale, the sending device's locale spec (optional)
 * - ReferencedTLVData *aMetaData, optional TLV-encoded vendor data
 return: error/status
*/
WEAVE_ERROR ImageQuery::init(ProductSpec &aProductSpec,
                             ReferencedString &aVersion,
                             IntegrityTypeList &aTypeList,
                             UpdateSchemeList &aSchemeList,
                             ReferencedString *aPackage,
                             ReferencedString *aLocale,
                             uint64_t aTargetNodeId,
                             ReferencedTLVData *aMetaData)
{
  productSpec=aProductSpec;
  version=aVersion;
  version.isShort=true;
  integrityTypes=aTypeList;
  updateSchemes=aSchemeList;
  if (aPackage!=NULL) {
    packageSpec=*aPackage;
    packageSpec.isShort=true;
  }
  if (aLocale!=NULL) localeSpec=*aLocale;
  targetNodeId=aTargetNodeId;
  if (aMetaData!=NULL) theMetaData=*aMetaData;
  return WEAVE_NO_ERROR;
}
/*
 * parameter: PacketBuffer *aBuffer, a packet into which the pack the query
 * return: /error/status
 */
WEAVE_ERROR ImageQuery::pack(PacketBuffer *aBuffer)
{
  MessageIterator i(aBuffer);
  i.append();
  // figure out what the frame control field looks like.
  uint8_t frameCtl=0;
  if (packageSpec.theLength!=0)
    frameCtl|=kFlag_PackageSpecPresent;
  if (localeSpec.theLength!=0)
    frameCtl|=kFlag_LocaleSpecPresent;
  if (targetNodeId!=0)
    frameCtl|=kFlag_TargetNodeIdPresent;
  // now write it
  TRY(i.writeByte(frameCtl));
  // write the product spec
  TRY(i.write16(productSpec.vendorId));
  TRY(i.write16(productSpec.productId));
  TRY(i.write16(productSpec.productRev));
  // pack the version string
  TRY(version.pack(i));
  // now do the intergrity types and update schemes
  TRY(integrityTypes.pack(i));
  TRY(updateSchemes.pack(i));
  // now the optional fields
  if (packageSpec.theLength!=0) TRY(packageSpec.pack(i));
  if (localeSpec.theLength!=0) TRY(localeSpec.pack(i));
  if (targetNodeId!=0) TRY(i.write64(targetNodeId));
  theMetaData.pack(i);
  return WEAVE_NO_ERROR;
}
/*
 * parameters:
 * - PacketBuffer *aBuffer, a pointer to a packet from which to parse the query
 * - ImageQuery &aQuery, an object in which to put the result
 * return: error/status
 */
WEAVE_ERROR ImageQuery::parse(PacketBuffer *aBuffer, ImageQuery &aQuery)
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
  if (frameCtl & kFlag_PackageSpecPresent) TRY(ReferencedString::parse(i, aQuery.packageSpec));
  // if a locale is provided then get it
  if (frameCtl & kFlag_LocaleSpecPresent) TRY(ReferencedString::parse(i, aQuery.localeSpec));
  // if a target node id is provided then get it
  if (frameCtl & kFlag_TargetNodeIdPresent) TRY(i.read64(&aQuery.targetNodeId));
  // and maybe the metadata
  ReferencedTLVData::parse(i, aQuery.theMetaData);
  return WEAVE_NO_ERROR;
}
/*
 * the only way to tell if two of these things are equal is to try
 * all of their components.
 * parameter: ImageQuery &another, another query to check against
 * returns: true if the queries are equal, false otherwise
 */
bool ImageQuery::operator == (const ImageQuery &another) const
{
  return ((productSpec.vendorId==another.productSpec.vendorId) &&
          (productSpec.productId==another.productSpec.productId) &&
          (productSpec.productRev==another.productSpec.productRev) &&
          (version==another.version) &&
          (integrityTypes==another.integrityTypes) &&
          (updateSchemes==another.updateSchemes) &&
          (packageSpec==another.packageSpec) &&
          (localeSpec==another.localeSpec) &&
          (targetNodeId==another.targetNodeId) &&
          (theMetaData==another.theMetaData));
}
/*
 * -- definitions for ImageQueryResponse and its supporting classes --
 *
 * parameter: uint8_t type, an IntegrityType value
 * return: the integity check output length for that type
 */
inline int integrityLength(uint8_t aType)
{
  switch (aType) {
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
/*
 * the no-arg contructor for an IntegritySpec
 */
IntegritySpec::IntegritySpec()
{
  type=kIntegrityType_SHA160;
}
/*
 * parameters:
 * - uint8_t aType, an integrity type
 * - uint8_t * aValue, an integrity check code of the appropriate length
 * represented as a packed string of bytes
 * return: error/status
 */
WEAVE_ERROR IntegritySpec::init(uint8_t aType, uint8_t* aValue)
{
  uint8_t len=integrityLength(aType);
  if (len==0) return WEAVE_ERROR_INVALID_INTEGRITY_TYPE;
  type=aType;
  for (int i=0; i<len; i++) value[i]=aValue[i];
  return WEAVE_NO_ERROR;
}
/*
 * parameters: MessageIterator &i, an iterator ofver the message
 * being packed
 * returns: error/status
 */
WEAVE_ERROR IntegritySpec::pack(MessageIterator &i)
{
  TRY(i.writeByte(type));
  for (int j=0; j<integrityLength(type); j++) TRY(i.writeByte(value[j]));
  return WEAVE_NO_ERROR;
}
/*
 * parameters:
 * - MessageIterator &i, an iterator over the message being parsed
 * - IntegritySpec &aSpec, a pointer to an object to contain the result
 * returns: error/status
 */
WEAVE_ERROR IntegritySpec::parse(MessageIterator &i, IntegritySpec &aSpec)
{
  TRY(i.readByte(&aSpec.type));
  for (int j=0; j<integrityLength(aSpec.type); j++) TRY(i.readByte(&aSpec.value[j]));
  return WEAVE_NO_ERROR;
}
/*
 * parameter: IntegritySpec &another, another spec to check against
 * returns: true if the integrity specs are equal, false otherwise
 */
bool IntegritySpec::operator == (const IntegritySpec &another) const
{
  if (type!=another.type)
    return false;
  for (int i=0; i<integrityLength(type); i++) {
    if (value[i]!=another.value[i])
      return false;
  }
  return true;
}
/*
 * parameters:
 * - ReferencedString &aUri, the URI at which the new firmware image is to be found
 * - Referenced &aVersion, the version string for this image
 * - IntegritySpec &aIntegrity, the integrity spec corresponding to the new image
 * - uint8_t aScheme, the update scheme to use in downloading
 * - UpdatePriority aPriority, the update priority associated with this update
 * - UpdateCondition aCondition, the condition under which to update
 * - bool aReportStatus, if true requests the client to report after download and
 * update, otherwise the client will not report
 * return: error/status
 */
WEAVE_ERROR ImageQueryResponse::init(ReferencedString &aUri, ReferencedString &aVersion,
                                     IntegritySpec &aIntegrity, uint8_t aScheme,
                                     UpdatePriority aPriority, UpdateCondition aCondition,
                                     bool aReportStatus)
{
  // we assume success here (woohoo! optional)
  uri=aUri;
  versionSpec=aVersion;
  versionSpec.isShort=true;
  integritySpec=aIntegrity;
  updateScheme=aScheme;
  updatePriority=aPriority;
  updateCondition=aCondition;
  reportStatus=aReportStatus;
  return WEAVE_NO_ERROR;
}
/*
 * the no-arg constructor for ImageQueryResponse objects
 */
ImageQueryResponse::ImageQueryResponse() :
    uri()
{
  versionSpec.theLength=0;
  versionSpec.isShort=true;
  updateScheme=kUpdateScheme_HTTP;
  reportStatus=false;
}
/*
 * parameter: PacketBuffer *aBuffer, a packet into which the pack the response
 * return: /error/status
 */
WEAVE_ERROR ImageQueryResponse::pack(PacketBuffer* aBuffer)
{
  MessageIterator i(aBuffer);
  i.append();
  TRY(uri.pack(i));
  TRY(versionSpec.pack(i));
  TRY(integritySpec.pack(i));
  TRY(i.writeByte((uint8_t)updateScheme));
  uint8_t updateOptions=(uint8_t)updatePriority|((uint8_t)updateCondition<<kOffset_UpdateCondition);
  if (reportStatus) updateOptions|=kMask_ReportStatus;
  TRY(i.writeByte(updateOptions));
  return WEAVE_NO_ERROR;
}
/*
 * parameters:
 * - PacketBuffer* aBuffer, a message buffer from which to parse the message
 * - ImageQueryResponse &aResponse, a place to put the result
 * returns: error/status
 */
WEAVE_ERROR ImageQueryResponse::parse(PacketBuffer *aBuffer, ImageQueryResponse &aResponse)
{
  MessageIterator i(aBuffer);
  uint8_t updateOptions;
  TRY(ReferencedString::parse(i, aResponse.uri));
  TRY(ReferencedString::parse(i, aResponse.versionSpec));
  TRY(IntegritySpec::parse(i, aResponse.integritySpec));
  TRY(i.readByte(&aResponse.updateScheme));
  TRY(i.readByte(&updateOptions));
  aResponse.updatePriority=(UpdatePriority)(updateOptions&kMask_UpdatePriority);
  aResponse.updateCondition=(UpdateCondition)((updateOptions&kMask_UpdateCondition)>>kOffset_UpdateCondition);
  aResponse.reportStatus=(updateOptions&kMask_ReportStatus)==kMask_ReportStatus;
  return WEAVE_NO_ERROR;
}
/*
 * parameter: ImageQueryResponse &another, another response to check against
 * returns: true if the responses are equal, false otherwise
 */
bool ImageQueryResponse::operator ==(const ImageQueryResponse &another) const
{
  return ((uri==another.uri) &&
          (versionSpec==another.versionSpec) &&
          (integritySpec==another.integritySpec) &&
          (updateScheme==another.updateScheme) &&
          (updatePriority==another.updatePriority) &&
          (updateCondition==another.updateCondition) &&
          (reportStatus==another.reportStatus));
}
