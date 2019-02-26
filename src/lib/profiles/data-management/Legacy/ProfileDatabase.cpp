/*
 *
 *    Copyright (c) 2015-2017 Nest Labs, Inc.
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
 *  @file
 *
 *  @brief
 *    Implementations for the abstract ProfileDatabase auxiliary
 *    class.
 *
 *  This file implements method definitions for the abstract Weave
 *  ProfileDatabase auxiliary class.
 */

#include <Weave/Profiles/data-management/Legacy/WdmManagedNamespace.h>

#include <stdarg.h>
#include <Weave/Support/CodeUtils.h>

#include <Weave/Profiles/data-management/ProfileDatabase.h>

namespace nl {
namespace Weave {
namespace Profiles {
namespace WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Legacy) {

using namespace ::nl;
using namespace ::nl::Inet;
using namespace ::nl::Weave;
using namespace ::nl::Weave::TLV;
using namespace ::nl::Weave::Profiles;
using namespace ::nl::Weave::Profiles::Common;

/**
 *  @brief
 *    Check a WDM-specific tag.
 *
 *  Check the number of a WDM-specific tag, from
 *  .../data-management/DMConstants.h, against the actual tag at the
 *  head of a TLV reader.
 *
 *  @param [in]     aTagNum         The tag number to be checked
 *                                  against a specific TLV element.
 *
 *  @param [in]     aReader         A reference to a TLV reader
 *                                  pointing to the element to be
 *                                  checked.
 *
 *  @return true iff aReader.GetTag() produces a tag that matches one
 *  of the expected tag forms for the given tag number.
 */

bool CheckWDMTag(uint32_t aTagNum, nl::Weave::TLV::TLVReader &aReader)
{
    bool returnVal;
    uint64_t tag = aReader.GetTag();

    switch (aTagNum)
    {
    case kTag_WDMPathList:
    case kTag_WDMPathProfile:
    case kTag_WDMPathArrayIndexSelector:
    case kTag_WDMPathArrayValueSelector:
    case kTag_WDMDataList:

        returnVal = (tag == CommonTag(aTagNum) || tag == ProfileTag(kWeaveProfile_WDM, aTagNum));

        break;
    case kTag_WDMPathProfileId:

        returnVal = (tag == CommonTag(kTag_WDMPathProfileId_Deprecated) ||
                     tag == ProfileTag(kWeaveProfile_WDM, kTag_WDMPathProfileId_Deprecated) ||
                     tag == ContextTag(aTagNum));

        break;
    case kTag_WDMPathProfileInstance:

        returnVal = (tag == CommonTag(kTag_WDMPathProfileInstance_Deprecated) ||
                     tag == ProfileTag(kWeaveProfile_WDM, kTag_WDMPathProfileInstance_Deprecated) ||
                     tag == ContextTag(aTagNum));

        break;
    case kTag_WDMDataListElementPath:

        returnVal = (tag == CommonTag(kTag_WDMDataListElementPath_Deprecated) ||
                     tag == ProfileTag(kWeaveProfile_WDM, kTag_WDMDataListElementPath_Deprecated) ||
                     tag == ContextTag(aTagNum));

        break;
    case kTag_WDMDataListElementVersion:

        returnVal = (tag == CommonTag(kTag_WDMDataListElementVersion_Deprecated) ||
                     tag == ProfileTag(kWeaveProfile_WDM, kTag_WDMDataListElementVersion_Deprecated) ||
                     tag == ContextTag(aTagNum));

        break;
    case kTag_WDMDataListElementData:

        /*
         * this is an especially gnarly one. in the bad old days, we
         * weren't checking this tag and it allowed the service to
         * send a profile tag for the application protocol of interest
         * as the tag for the data container here. this is incorrect,
         * of course, but for purposes of backwards compatibility we
         * should allow it until we're pretty sure the service is up
         * to date.
         */

        returnVal = (tag == CommonTag(kTag_WDMDataListElementData_Deprecated) ||
                     tag == ProfileTag(kWeaveProfile_WDM, kTag_WDMDataListElementData_Deprecated) ||
                     tag == ContextTag(aTagNum) ||
                     tag == ProfileTag(kWeaveProfile_NestProtect, 0) ||
                     tag == ProfileTag(kWeaveProfile_Occupancy, 0) ||
                     tag == ProfileTag(kWeaveProfile_Structure, 0) ||
                     tag == ProfileTag(kWeaveProfile_Safety, 0) ||
                     tag == ProfileTag(kWeaveProfile_SafetySummary, 0) ||
                     tag == ProfileTag(kWeaveProfile_NestThermostat, 0));

        break;
    default:

        returnVal = false;

        break;
    }

    return returnVal;
};

/**
 *  @brief
 *    Start reading a path list.
 *
 *  Given a fresh reader and a path list, start reading the list and
 *  validate the tags and types initially encountered in the
 *  process. If all goes well, the reader stops after the list
 *  container is entered.
 *
 *  @param [in]     aPathList       A path list passed as a reference
 *                                  to a ReferencedTLVData object. The
 *                                  normal use case will be where the
 *                                  list is actually still in a buffer
 *                                  after receipt.
 *
 *  @param [out]    aReader         A reference to a TLV reader used
 *                                  to read the path list. This reader
 *                                  will be left pointing just before
 *                                  the first path in the list.
 *
 *  @return #WEAVE_NO_ERROR on success; otherwise, a
 *  #WEAVE_ERROR reflecting a failure the open the path list and/or
 *  validate the relevant tags and types.
 */

WEAVE_ERROR OpenPathList(ReferencedTLVData &aPathList, nl::Weave::TLV::TLVReader &aReader)
{
    WEAVE_ERROR err;
    TLVType container;

    aReader.Init(aPathList.theData, aPathList.theLength);

    // we should be looking at an array called a WDM path list

    err = aReader.Next();
    SuccessOrExit(err);

    err = ValidateTLVType(kTLVType_Array, aReader);
    SuccessOrExit(err);

    err = ValidateWDMTag(kTag_WDMPathList, aReader);
    SuccessOrExit(err);

    err = aReader.EnterContainer(container);
    SuccessOrExit(err);

exit:
    return err;
}

/**
 *  @brief
 *    Start reading a data list.
 *
 *  Given a fresh reader and a data list, start reading the list and
 *  validate the tags and types initially encountered in the
 *  process. If all goes well, the reader stops after the list
 *  container is entered.
 *
 *  @param [in]     aDataList       A data list passed as a reference
 *                                  to a ReferencedTLVData object. The
 *                                  normal use case will be where the
 *                                  list is actually still in a buffer
 *                                  after receipt.
 *
 *  @param [out]    aReader         A reference to a TLV reader used
 *                                  to read the data list. This reader
 *                                  will be left pointing just before
 *                                  the first item in the list.
 *
 *  @return #WEAVE_NO_ERROR On success. Otherwise return a
 *  #WEAVE_ERROR reflecting a failure the open the data list and/or
 *  validate the relevant tags and types.
 */

WEAVE_ERROR OpenDataList(ReferencedTLVData &aDataList, nl::Weave::TLV::TLVReader &aReader)
{
    WEAVE_ERROR err;
    TLVType container;

    aReader.Init(aDataList.theData, aDataList.theLength);

    // we should be looking at an array called a WDM data list

    err = aReader.Next();
    SuccessOrExit(err);

    err = ValidateTLVType(kTLVType_Array, aReader);
    SuccessOrExit(err);

    err = ValidateWDMTag(kTag_WDMDataList, aReader);
    SuccessOrExit(err);

    err = aReader.EnterContainer(container);
    SuccessOrExit(err);

exit:
    return err;
}

/**
 *  @brief
 *    Start reading a data list element.
 *
 *  Given a reader positioned at a data list element, start reading the
 *  element and validate the tags and types initially encountered in
 *  the process. If all goes well, the reader ends up positioned at the
 *  data element data and the in/out path reader is positioned at the
 *  corresponding path.
 *
 *  @param [in]     aReader         A reference to a TLV reader
 *                                  positioned at a data list element.
 *
 *  @param [out]    aPathReader     A reference to a TLV reader to be
 *                                  pointed at the path component of
 *                                  the data list element.
 *
 *  @param [out]    aVersion        A reference to a 64-bit integer
 *                                  to be set either to the data list
 *                                  element version if one is present
 *                                  or else to kVersionNotSpecified.
 *
 *  @return #WEAVE_NO_ERROR on success or else a #WEAVE_ERROR
 *  associated with opening and reading the data list element.
 */

WEAVE_ERROR OpenDataListElement(nl::Weave::TLV::TLVReader &aReader, nl::Weave::TLV::TLVReader &aPathReader, uint64_t &aVersion)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    TLVType elementContainer;

    /*
     * a data list element should be an anonymous structure with 3
     * components, one of which (the version) is optional.
     */

    VerifyOrExit(aReader.GetTag() == AnonymousTag, err = WEAVE_ERROR_INVALID_TLV_TAG);

    err = ValidateTLVType(kTLVType_Structure, aReader);
    SuccessOrExit(err);

    err = aReader.EnterContainer(elementContainer);
    SuccessOrExit(err);

    /*
     * first get a copy of the reader that points to the
     * path and hold it for later.
     */

    err = aReader.Next();
    SuccessOrExit(err);

    VerifyOrExit((aReader.GetType() == kTLVType_Path), err = WEAVE_ERROR_WRONG_TLV_TYPE);

    aPathReader = aReader;

    /*
     * now get the version if there is one.
     */

    err = aReader.Next();
    SuccessOrExit(err);

    if (CheckWDMTag(kTag_WDMDataListElementVersion, aReader))
    {
        err = ValidateTLVType(kTLVType_UnsignedInteger, aReader);
        SuccessOrExit(err);

        aReader.Get(aVersion);

        err = aReader.Next();
        SuccessOrExit(err);
    }

    else
    {
        aVersion = kVersionNotSpecified;
    }

    err = ValidateWDMTag(kTag_WDMDataListElementData, aReader);

exit:
    return err;
}

/*
 * @brief
 *   An internal method to start encoding a path list
 *
 * @note
 *   There are a bunch of EncodePath() functions below that differ in
 *   the instance ID and tag style they use. Furthermore, they're all
 *   variadic so there's a chunk of them - the arg unpacking - that's
 *   hard to reduce. These static functions are the best bet for both
 *   reducing the resultant code size and duplication. They knock a few
 *   hundred bytes off the image and at least reduce the absolute amount
 *   of duplicated text by a bit.
 *
 *   This method starts a container with ProfileTag(kWeaveProfile_WDM, kTag_WDMPathProfile)
 *   and then insert a context-specific tag with ContextTag(kTag_WDMPathProfileId).
 *   This is the latest version for encoding a path.
 */

static WEAVE_ERROR StartEncodePath(TLVWriter &aWriter,
                                   const uint64_t &aTag,
                                   uint32_t aProfileId,
                                   TLVType &mOuterContainer,
                                   TLVType &mPath)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    err = aWriter.StartContainer(aTag, kTLVType_Path, mOuterContainer);
    SuccessOrExit(err);

    // open a structure container and write the profile ID

    err = aWriter.StartContainer(ProfileTag(kWeaveProfile_WDM, kTag_WDMPathProfile), kTLVType_Structure, mPath);
    SuccessOrExit(err);

    err = aWriter.Put(ContextTag(kTag_WDMPathProfileId), aProfileId);
    SuccessOrExit(err);

    exit:

    return err;
}

static WEAVE_ERROR EndEncodePath(TLVWriter &aWriter, TLVType &mOuterContainer, WEAVE_ERROR mError)
{
    WEAVE_ERROR retErr = aWriter.EndContainer(mOuterContainer);

    /*
     * take the "worst" error and return it.
     */

    if (mError != WEAVE_NO_ERROR)
        retErr = mError;

    return retErr;
}

/**
 *  @brief
 *    Encode a WDM path with an integer profile instance ID.
 *
 *  @note
 *    Write a TLV path of the kind used in data management where, in
 *    particular, there is a profile designation placed at the beginning
 *    in order to allow interpretation of subsequent path elements.
 *
 *    This version of the method takes an integer profile instance ID.
 *
 *    This method inserts instance ID using ContextTag(kTag_WDMPathProfileInstance),
 *    which is the latest version for encoding a path.
 *
 *  @param [in]     aWriter         A reference to the TLV writer used
 *                                  to write out the path.
 *
 *  @param [in]     aTag            A reference to the fully-qualified
 *                                  TLV tag that applies to this path.
 *
 *  @param [in]     aProfileId      The profile ID under which
 *                                  elements of the path are to be
 *                                  interpreted.
 *
 *  @param [in]     aInstanceId     A reference to the optional
 *                                  instance identifier of the profile
 *                                  to be used. If no instance ID is
 *                                  to be used then this parameter
 *                                  should have a value of
 *                                  kInstanceIdNotSpecified.
 *
 *  @param [in]     aPathLen        The, possibly 0, length of the
 *                                  list of path elements beyond the
 *                                  initial profile specifier.
 *
 *  @param [in]     ...             The optional variable-length list
 *                                  of additional path tags.
 *
 *  @return #WEAVE_NO_ERROR On success. Otherwise return a
 *  #WEAVE_ERROR reflecting an inability to format the given path.
 */

WEAVE_ERROR EncodePath(TLVWriter &aWriter,
                       const uint64_t &aTag,
                       uint32_t aProfileId,
                       const uint64_t &aInstanceId,
                       uint32_t aPathLen,
                       ...)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    va_list pathTags;
    TLVType outerContainer;
    TLVType path;

    err = StartEncodePath(aWriter, aTag, aProfileId, outerContainer, path);
    SuccessOrExit(err);

    if (aInstanceId != kInstanceIdNotSpecified)
    {
        err = aWriter.Put(ContextTag(kTag_WDMPathProfileInstance), aInstanceId);
        SuccessOrExit(err);
    }

    err = aWriter.EndContainer(path);
    SuccessOrExit(err);

    va_start(pathTags, aPathLen);

    for (uint32_t i = 0; i < aPathLen; i++)
    {
        err = aWriter.PutNull(va_arg(pathTags, uint64_t));

        if (err != WEAVE_NO_ERROR)
            break;
    }

    va_end(pathTags);

exit:

    return EndEncodePath(aWriter, outerContainer, err);
}

/**
 *  @brief
 *    Encode a WDM path with a byte array instance ID.
 *
 *  @note
 *    Write a TLV path of the kind used in data management where, in
 *    particular, there is a profile designation placed at the beginning
 *    in order to allow interpretation of subsequent path elements.
 *
 *    This version of the method takes a byte-array profile instance ID
 *    along with a length.
 *
 *    This method inserts instance ID using ContextTag(kTag_WDMPathProfileInstance),
 *    which is the latest version for encoding a path.
 *
 *  @param [in]     aWriter         A reference to the TLV writer used
 *                                  to write out the path.
 *
 *  @param [in]     aTag            A reference to the fully-qualified
 *                                  TLV tag that applies to this path.
 *
 *  @param [in]     aProfileId      The profile ID under which the
 *                                  elements of the path are to be
 *                                  interpreted.
 *
 *  @param [in]     aInstanceIdLen  The length of the byte array
 *                                  that constitutes the instance
 *                                  ID. If there is no ID then this
 *                                  parameter shall have a value of 0.
 *
 *  @param [in]     aInstanceId     The optional byte array used as a
 *                                  profile instance identifier. This
 *                                  argument may be NULL in the case
 *                                  where no instance ID is specified.
 *
 *  @param [in]     aPathLen        The, possibly 0, length of the
 *                                  list of path elements beyond the
 *                                  initial profile specifier.
 *
 *  @param [in]     ...             The optional, variable-length list
 *                                  of additional path tags.
 *
 *  @return #WEAVE_NO_ERROR On success. Otherwise return a
 *  #WEAVE_ERROR reflecting an inability to format the given path.
 */

WEAVE_ERROR EncodePath(TLVWriter &aWriter,
                       const uint64_t &aTag,
                       uint32_t aProfileId,
                       const uint32_t aInstanceIdLen,
                       const uint8_t *aInstanceId,
                       uint32_t aPathLen,
                       ...)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    va_list pathTags;
    TLVType outerContainer;
    TLVType path;

    err = StartEncodePath(aWriter, aTag, aProfileId, outerContainer, path);
    SuccessOrExit(err);

    if (aInstanceId)
    {
        err = aWriter.PutBytes(ContextTag(kTag_WDMPathProfileInstance), aInstanceId, aInstanceIdLen);
        SuccessOrExit(err);
    }

    err = aWriter.EndContainer(path);
    SuccessOrExit(err);

    va_start(pathTags, aPathLen);

    for (uint32_t i = 0; i < aPathLen; i++)
    {
        err = aWriter.PutNull(va_arg(pathTags, uint64_t));

        if (err != WEAVE_NO_ERROR)
            break;
    }

    va_end(pathTags);

exit:

    return EndEncodePath(aWriter, outerContainer, err);
}

/**
 *  @brief
 *    Encode a WDM path with a string instance ID.
 *
 *  @note
 *    Write a TLV path of the kind used in data management where, in
 *    particular, there is a profile designation placed at the beginning
 *    in order to allow interpretation of subsequent path elements.
 *
 *    This version of the method takes a string profile instance ID.
 *
 *    This method inserts instance ID using ContextTag(kTag_WDMPathProfileInstance),
 *    which is the latest version for encoding a path.
 *
 *  @param [in]     aWriter         A reference to the TLV writer used
 *                                  to write out the path.
 *
 *  @param [in]     aTag            A reference to the fully-qualified
 *                                  TLV tag that applies to this path.
 *
 *  @param [in]     aProfileId      The profile ID under which
 *                                  elements of the path are to be
 *                                  interpreted.
 *
 *  @param [in]     aInstanceId     The optional string used as a
 *                                  profile instance identifier. This
 *                                  argument may be NULL if no
 *                                  instance ID is specified.
 *
 *  @param [in]     aPathLen        The, possibly 0, length of the
 *                                  list of path elements beyond the
 *                                  initial profile specifier.
 *
 *  @param [in]     ...             The optional, variable-length list
 *                                  of additional path tags.
 *
 *  @return #WEAVE_NO_ERROR On success. Otherwise return a
 *  #WEAVE_ERROR reflecting an inability to format the given path.
 */

WEAVE_ERROR EncodePath(TLVWriter &aWriter,
                       const uint64_t &aTag,
                       uint32_t aProfileId,
                       const char *aInstanceId,
                       uint32_t aPathLen,
                       ...)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    va_list pathTags;
    TLVType outerContainer;
    TLVType path;

    err = StartEncodePath(aWriter, aTag, aProfileId, outerContainer, path);
    SuccessOrExit(err);

    if (aInstanceId)
    {
        err = aWriter.PutString(ContextTag(kTag_WDMPathProfileInstance), aInstanceId);
        SuccessOrExit(err);
    }

    err = aWriter.EndContainer(path);
    SuccessOrExit(err);

    va_start(pathTags, aPathLen);

    for (uint32_t i = 0; i < aPathLen; i++)
    {
        err = aWriter.PutNull(va_arg(pathTags, uint64_t));

        if (err != WEAVE_NO_ERROR)
            break;
    }

    va_end(pathTags);

exit:

    return EndEncodePath(aWriter, outerContainer, err);
}

/**
 *  @brief
 *    Encode a WDM path with deprecated tags and an integer instance ID.
 *
 *  @note
 *    Encode a path using the deprecated tag set accepted by the service
 *    before Weave release 2.0. This version of the method takes a numerical
 *    instance identifier.
 *
 *   This method starts a container with ProfileTag(kWeaveProfile_WDM, kTag_WDMPathProfile)
 *   and then inserts the profile ID with ProfileTag(kWeaveProfile_WDM, kTag_WDMPathProfileId_Deprecated).
 *   It then inserts the instance ID with ProfileTag(kWeaveProfile_WDM, kTag_WDMPathProfileInstance_Deprecated).
 *   This is one of the deprecated versions for encoding a path, and new designs shall avoid
 *   using this format.
 *
 *  @param [in]     aWriter         A reference to the TLV writer used
 *                                  to write out the path.
 *
 *  @param [in]     aTag            A reference to the fully-qualified
 *                                  TLV tag that applies to this path.
 *
 *  @param [in]     aProfileId      The profile ID under which
 *                                  elements of the path are to be
 *                                  interpreted.
 *
 *  @param [in]     aInstanceId     A reference to the optional
 *                                  instance ID of the profile to be
 *                                  used.
 *
 *  @param [in]     aPathLen        The, possibly 0, length of the
 *                                  list of path elements beyond the
 *                                  initial profile specifier.
 *
 *  @param [in]     ...             The optional variable-length list
 *                                  of additional path tags.
 *
 *  @return #WEAVE_NO_ERROR On success. Otherwise return a
 *  #WEAVE_ERROR reflecting an inability to format the given path.
 */

WEAVE_ERROR EncodeDeprecatedPath(TLVWriter &aWriter,
                                 const uint64_t &aTag,
                                 uint32_t aProfileId,
                                 const uint64_t &aInstanceId,
                                 uint32_t aPathLen,
                                 ...)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    va_list pathTags;
    TLVType outerContainer;
    TLVType path;

    err = aWriter.StartContainer(aTag, kTLVType_Path, outerContainer);
    SuccessOrExit(err);

    // write the profile and instance (if specified)

    err = aWriter.StartContainer(ProfileTag(kWeaveProfile_WDM, kTag_WDMPathProfile), kTLVType_Structure, path);
    SuccessOrExit(err);

    err = aWriter.Put(ProfileTag(kWeaveProfile_WDM, kTag_WDMPathProfileId_Deprecated), aProfileId);
    SuccessOrExit(err);

    if (aInstanceId != kInstanceIdNotSpecified)
    {
        err = aWriter.Put(ProfileTag(kWeaveProfile_WDM, kTag_WDMPathProfileInstance_Deprecated), aInstanceId);
        SuccessOrExit(err);
    }

    err = aWriter.EndContainer(path);
    SuccessOrExit(err);

    va_start(pathTags, aPathLen);

    for (uint32_t i = 0; i < aPathLen; i++)
    {
        err = aWriter.PutNull(va_arg(pathTags, uint64_t));

        if (err != WEAVE_NO_ERROR)
            break;
    }

    va_end(pathTags);

exit:

    return EndEncodePath(aWriter, outerContainer, err);
}

/**
 *  @brief
 *    Encode a WDM path with deprecated tags and a string instance ID.
 *
 *  @note
 *    Encode a path using the deprecated tag set (see DMConstants.h). This
 *    version of the method takes an instance ID string.
 *
 *   This method starts a container with ProfileTag(kWeaveProfile_WDM, kTag_WDMPathProfile)
 *   and then inserts the profile ID with ProfileTag(kWeaveProfile_WDM, kTag_WDMPathProfileId_Deprecated).
 *   It then inserts the instance ID with ProfileTag(kWeaveProfile_WDM, kTag_WDMPathProfileInstance_Deprecated).
 *   This is one of the deprecated versions for encoding a path, and new designs shall avoid
 *   using this format.
 *
 *  @param [in]     aWriter         A reference to the TLV writer used
 *                                  to write out the path.
 *
 *  @param [in]     aTag            A reference to the fully-qualified
 *                                  TLV tag that applies to this path.
 *
 *  @param [in]     aProfileId      The profile ID under which
 *                                  elements of the path are to be
 *                                  interpreted.
 *
 *  @param [in]     aInstanceId     The optional string used as a
 *                                  profile instance identifier. This
 *                                  argument may be NULL if no
 *                                  instance ID is specified.
 *
 *  @param [in]     aPathLen        The, possibly 0, length of the
 *                                  list of path elements beyond the
 *                                  initial profile specifier.
 *
 *  @param [in]     ...             The optional, variable-length list
 *                                  of additional path tags.
 *
 *  @return #WEAVE_NO_ERROR On success. Otherwise return a
 *  #WEAVE_ERROR reflecting an inability to format the given path.
 */

WEAVE_ERROR EncodeDeprecatedPath(TLVWriter &aWriter,
                                 const uint64_t &aTag,
                                 uint32_t aProfileId,
                                 const char *aInstanceId,
                                 uint32_t aPathLen,
                                 ...)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    va_list pathTags;
    TLVType outerContainer;
    TLVType path;

    err = aWriter.StartContainer(aTag, kTLVType_Path, outerContainer);
    SuccessOrExit(err);

    // write the profile and instance (if specified)

    err = aWriter.StartContainer(ProfileTag(kWeaveProfile_WDM, kTag_WDMPathProfile), kTLVType_Structure, path);
    SuccessOrExit(err);

    err = aWriter.Put(ProfileTag(kWeaveProfile_WDM, kTag_WDMPathProfileId_Deprecated), aProfileId);
    SuccessOrExit(err);

    if (aInstanceId != NULL)
    {
        err = aWriter.PutString(ProfileTag(kWeaveProfile_WDM, kTag_WDMPathProfileInstance_Deprecated), aInstanceId);
        SuccessOrExit(err);
    }

    err = aWriter.EndContainer(path);
    SuccessOrExit(err);

    va_start(pathTags, aPathLen);

    for (uint32_t i = 0; i < aPathLen; i++)
    {
        err = aWriter.PutNull(va_arg(pathTags, uint64_t));

        if (err != WEAVE_NO_ERROR)
            break;
    }

    va_end(pathTags);

exit:

    return EndEncodePath(aWriter, outerContainer, err);
}

/*
 * in both of the following methods we set the version to 0.  we're
 * assuming that, for our purposes, it's OK to start the version at 0
 * and count up. if something else is required, it can be handled in
 * the concrete subclass.
 */

/**
 *  @brief
 *    The default constructor for ProfileData.
 *
 *  Initialize a fresh ProfileData item by setting its version to 0.
 */

ProfileDatabase::ProfileData::ProfileData(void)
{
    mVersion = 0;
}

/**
 *  @brief
 *    The destructor for ProfileData
 *
 *  Like the constructor this just clears the data verion to 0.
 */

ProfileDatabase::ProfileData::~ProfileData(void)
{
    mVersion = 0;
}

/**
 *  @brief
 *    Store a data list item being read.
 *
 *  This virtual method is used to store a particular data list item in
 *  an object of a concrete ProfileData subclass. The implementation
 *  here in the super-class may be used if the object is simple and
 *  "shallow", having only paths that are one element long. For a more
 *  complicated schema, implementers should override this method.
 *
 *  @param [in]     aPathReader     A reference to a TLV reader
 *                                  positioned at the path component
 *                                  of the data list item.
 *
 *  @param [in]     aVersion        The 64-bit version component of
 *                                  the data list item.
 *
 *  @param [in]     aDataReader     A reference to a TLV reader
 *                                  positioned at the data component
 *                                  of the data list item.
 *
 *  @return #WEAVE_NO_ERROR On success. Otherwise return a
 *  #WEAVE_ERROR indicating a failure to store the data of interest.
 */

WEAVE_ERROR ProfileDatabase::ProfileData::Store(TLVReader &aPathReader, uint64_t aVersion, TLVReader &aDataReader)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    TLVType container;

    err = aPathReader.Next();

    if (err == WEAVE_END_OF_TLV)
    {
        /*
         * the path given was the top-level profile path so the we
         * have to do the entire bucket, as follows. we should be
         * looking at a structure.
         */

        VerifyOrExit((aDataReader.GetType() == kTLVType_Structure), err = WEAVE_ERROR_WRONG_TLV_TYPE);

        err = aDataReader.EnterContainer(container);
        SuccessOrExit(err);

        err = aDataReader.Next();
        while (err == WEAVE_NO_ERROR)
        {
            err = StoreItem(aDataReader.GetTag(), aDataReader);
            SuccessOrExit(err);

            err = aDataReader.Next();
        }

        // don't return an error if we're just out of TLV

        if (err == WEAVE_END_OF_TLV)
            err = WEAVE_NO_ERROR;

        aDataReader.ExitContainer(container);
    }

    else
    {
        /*
         * in this case, the path contained an additional tag
         * accessing a particular data item directly.
         */

        err = StoreItem(aPathReader.GetTag(), aDataReader);
        SuccessOrExit(err);
    }

    /*
     * whatever happened above, we set the version here as long as it
     * didn't go horribly wrong.
     */

    mVersion = aVersion;

exit:
    return err;
}

/**
 *  @brief
 *    Store a data list.
 *
 *  Given a TLV-encoded data list, this method goes through the process
 *  of parsing that list and calling the concrete methods provided by
 *  ProfileDatabase subclass implementers to put the referenced data
 *  where it belongs.
 *
 *  @param [in]     aDataList       A reference to a ReferencedTLVData object
 *                                  containing the data of interest in
 *                                  TLV-encoded form.
 *
 *  @return #WEAVE_NO_ERROR On success. Otherwise return a
 *  #WEAVE_ERROR indicating a failure to store the data of interest.
 */

WEAVE_ERROR ProfileDatabase::Store(ReferencedTLVData &aDataList)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    TLVReader dataRdr;
    uint64_t version;
    TLVReader pathRdr;

    OpenDataList(aDataList, dataRdr);

    err = dataRdr.Next();
    while (err == WEAVE_NO_ERROR)
    {
        err = OpenDataListElement(dataRdr, pathRdr, version);
        SuccessOrExit(err);

        err = StoreInternal(pathRdr, version, dataRdr);
        SuccessOrExit(err);

        err = CloseDataListElement(dataRdr);
        SuccessOrExit(err);

        err = dataRdr.Next();
    }

    err = CloseList(dataRdr);

exit:
    return err;
}

/**
 *  @brief
 *    Retrieve a data list given a path list.
 *
 *  Given a list of paths, retrieve a data list containing data list
 *  elements for each path in the path list the data that is the terminal of that
 *  path.
 *
 *  @param [in]     aPathList       A reference to a ReferencedTLVData
 *                                  object containing a TLV-encoded
 *                                  list of paths representing data to
 *                                  retrieve. This parameter is kept constant
 *                                  throughout the execution of this function.
 *
 *  @param [out]    aDataList       A reference to a ReferencedTLVData
 *                                  object in which to write the
 *                                  retrieved results. Data length is adjusted only
 *                                  after successful execution of this function.
 *
 *  @return #WEAVE_NO_ERROR On success. Otherwise return a
 *  #WEAVE_ERROR indicating a failure to retrieve the data list of
 *  interest.
 */

WEAVE_ERROR ProfileDatabase::Retrieve(ReferencedTLVData &aPathList, ReferencedTLVData &aDataList)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    TLVWriter writer;
    writer.Init(aDataList.theData, aDataList.theMaxLength);

    err = Retrieve(aPathList, writer);
    SuccessOrExit(err);

    aDataList.theLength = writer.GetLengthWritten();

exit:
    return err;
}

/**
 *  @brief
 *    Write out a data list given a path list.
 *
 *  Given a list of paths and a TLV writer, write out a data list
 *  containing data list elements for each path in the path list and the data that is the
 *  terminal of that path.
 *
 *  @param [in]     aPathList       A reference to a ReferencedTLVData
 *                                  object containing a list of TLV
 *                                  paths representing data to
 *                                  retrieve. This parameter is kept constant
 *                                  throughout the execution of this function.
 *
 *  @param [in]     aWriter         A reference to the TLV writer to
 *                                  use in writing out the retrieved
 *                                  path list. Internal state for the writer could be
 *                                  unrecoverable in case of an error.
 *
 *
 *  @return #WEAVE_NO_ERROR On success. Otherwise return a
 *  #WEAVE_ERROR indicating a failure to retrieve the data or write
 *  out the data list of interest.
 */

WEAVE_ERROR ProfileDatabase::Retrieve(ReferencedTLVData &aPathList, TLVWriter &aWriter)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    TLVReader rdr;
    ProfileData *profileData;

    // aPathList is copied at here into rdr, and then left alone, so
    // it stays constant no matter what will happen
    err = OpenPathList(aPathList, rdr);
    SuccessOrExit(err);

    // There is no useful guarantee for the state of aWriter shall anything go wrong
    err = StartDataList(aWriter);
    SuccessOrExit(err);

    err = rdr.Next();
    while (err == WEAVE_NO_ERROR)
    {
        TLVType containerTypePathList;

        err = rdr.EnterContainer(containerTypePathList);
        SuccessOrExit(err);

        err = rdr.Next();
        SuccessOrExit(err);

        err = LookupDataFromProfileDescriptor(rdr, &profileData);
        SuccessOrExit(err);

        /*
         * OK. now we're poised for victory. there's a reader to read
         * the path. there's a writer to write out the data. as the
         * Nike ads used to say, "Just Do It".
         */

        err = StartDataListElement(aWriter);
        SuccessOrExit(err);

        err = profileData->Retrieve(rdr, aWriter);
        SuccessOrExit(err);

        err = EndDataListElement(aWriter);
        SuccessOrExit(err);

        /*
         * now we actually close the path we're working on and move on
         * to the next path list element, if any.
         */

        err = rdr.ExitContainer(containerTypePathList);
        SuccessOrExit(err);

        err = rdr.Next();
    }

    if (err == WEAVE_END_OF_TLV)
        err = WEAVE_NO_ERROR;

    EndList(aWriter);

    // Note this isn't really necessary, as rdr should already be depleted by now.
    // We reach this line when we get WEAVE_END_OF_TLV from rdr.Next().
    CloseList(rdr);

exit:

    return err;
}

/**
 *  @brief
 *    Find a ProfileData object in the database.
 *
 *  This utility method is used to find ProfileData objects in a
 *  particular ProfileDatabase. It depends largely on lookup methods
 *  provided by the implementer of the concrete ProfileDatabase
 *  subclass.
 *
 *  @note
 *    This is the top-level method for finding the right profile
 *    data structure to match an existing path. The path reader
 *    should be pointing at the profile descriptor, the one immediately
 *    after entering the path container.
 *
 *  @param [in]     aDescReader     A reference to a TLV reader
 *                                  positioned at a WDM path - i.e. a
 *                                  TLV path that has, as its first
 *                                  element, a profile description.
 *
 *  @param [out]    aProfileData    A pointer, intended to return a
 *                                  pointer to the ProfileData object
 *                                  of interest.
 *
 *  @return #WEAVE_NO_ERROR on success, otherwise return a
 *  #WEAVE_ERROR indicating a failure to look up a matching
 *  ProfileData object.
 */

WEAVE_ERROR ProfileDatabase::LookupDataFromProfileDescriptor(nl::Weave::TLV::TLVReader &aDescReader, ProfileData **aProfileData)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    TLVType profileContainer;

    uint32_t profileId;

    TLVReader instanceIdRdr;

    /*
     * the first element of a path under WDM should be a structure
     * with 2 elements, one of which (the instance) is optional.
     */

    err = ValidateTLVType(kTLVType_Structure, aDescReader);
    SuccessOrExit(err);

    err = ValidateWDMTag(kTag_WDMPathProfile, aDescReader);
    SuccessOrExit(err);

    /*
     * parse the path profile and get the profile data object
     */

    err = aDescReader.EnterContainer(profileContainer);
    SuccessOrExit(err);

    /*
     * the first element here should be a profile ID
     */

    err = aDescReader.Next();
    SuccessOrExit(err);

    err = ValidateTLVType(kTLVType_UnsignedInteger, aDescReader);
    SuccessOrExit(err);

    err = ValidateWDMTag(kTag_WDMPathProfileId, aDescReader);
    SuccessOrExit(err);

    err = aDescReader.Get(profileId);
    SuccessOrExit(err);

    /*
     * and the second may be an instance. if there's one there, then
     * point the NHL lookup method at a reader for it.
     */

    err = aDescReader.Next();

    if (err == WEAVE_END_OF_TLV)
    {
        err = LookupProfileData(profileId, NULL, aProfileData);
    }

    else if (err == WEAVE_NO_ERROR)
    {
        instanceIdRdr = aDescReader;

        err = LookupProfileData(profileId, &instanceIdRdr, aProfileData);
    }

    SuccessOrExit(err);

    /*
     * now get out. skip over the instance, if any, with the path
     * reader and force an exit from the container with profile info
     * in it.
     */

    aDescReader.ExitContainer(profileContainer);

exit:
    return err;
}

/**
 *  @brief
 *    Find a ProfileData object in the database.
 *
 *  This utility method is used to find ProfileData objects in a
 *  particular ProfileDatabase. It depends largely on lookup methods
 *  provided by the implementer of the concrete ProfileDatabase
 *  subclass.
 *
 *  @note
 *    This is the top-level method for finding the right profile
 *    data structure to match an existing path. The path reader
 *    should be pointing at the whole path container.
 *
 *  @param [in]     aPathReader     A reference to a TLV reader
 *                                  positioned at a WDM path - i.e. a
 *                                  TLV path that has, as its first
 *                                  element, a profile description.
 *
 *  @param [out]    aProfileData    A pointer, intended to return a
 *                                  pointer to the ProfileData object
 *                                  of interest.
 *
 *  @return #WEAVE_NO_ERROR on success, otherwise return a
 *  #WEAVE_ERROR indicating a failure to look up a matching
 *  ProfileData object.
 */

WEAVE_ERROR ProfileDatabase::LookupProfileData(TLVReader &aPathReader, ProfileData **aProfileData)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    TLVType pathContainer;

    err = aPathReader.EnterContainer(pathContainer);
    SuccessOrExit(err);

    /*
     * the first element of a path under WDM should be a structure
     * with 2 elements, one of which (the instance) is optional.
     */

    err = aPathReader.Next();
    SuccessOrExit(err);

    err = LookupDataFromProfileDescriptor(aPathReader, aProfileData);
    SuccessOrExit(err);

    /*
     * we explicitly DON'T exit the path container here because it may
     * contain additional elements.
     */

exit:
    return err;
}

WEAVE_ERROR ProfileDatabase::StoreInternal(TLVReader &aPathReader, uint64_t aVersion, TLVReader &aDataReader)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    ProfileData *profileData;
    TLVType containerTypeDataElem;

    err = aPathReader.EnterContainer(containerTypeDataElem);
    SuccessOrExit(err);

    err = aPathReader.Next();
    SuccessOrExit(err);

    err = LookupDataFromProfileDescriptor(aPathReader, &profileData);
    SuccessOrExit(err);

    err = profileData->Store(aPathReader, aVersion, aDataReader);
    SuccessOrExit(err);

    err = aPathReader.ExitContainer(containerTypeDataElem);

exit:
    return err;
}

}; // WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Legacy)
}; // Profiles
}; // Weave
}; // nl
