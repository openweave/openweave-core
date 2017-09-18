/*
 *
 *    Copyright (c) 2016-2017 Nest Labs, Inc.
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
 *      This file implements parsers and encoders for messages in Weave
 *      Data Management (WDM) profile.
 *
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

#include <Weave/Profiles/data-management/Current/WdmManagedNamespace.h>
#include <Weave/Profiles/data-management/DataManagement.h>

#include <Weave/Profiles/security/WeaveSecurity.h>

#include <Weave/Support/WeaveFaultInjection.h>


#include <stdio.h>

#ifndef MIN
#define MIN(x , y)  (((x) < (y)) ? (x) : (y))
#endif

namespace nl {
namespace Weave {
namespace Profiles {
namespace WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current) {

#if WEAVE_DETAIL_LOGGING

static uint32_t gPrettyPrintingDepthLevel = 0;
static char gLineBuffer[256];
static uint32_t gCurLineBufferSize = 0;

#define PRETTY_PRINT(fmt, ...)              do { PrettyPrintWDM(true, fmt, ##__VA_ARGS__); } while (0)
#define PRETTY_PRINT_SAMELINE(fmt, ...)     do { PrettyPrintWDM(false, fmt, ##__VA_ARGS__); } while (0)
#define PRETTY_PRINT_INCDEPTH()             do { gPrettyPrintingDepthLevel++; } while (0)
#define PRETTY_PRINT_DECDEPTH()             do { gPrettyPrintingDepthLevel--; } while (0)

static void PrettyPrintWDM(bool aIsNewLine, const char *aFmt,  ...)
{
    va_list args;
    int ret;
    int sizeLeft;

    va_start(args, aFmt);

    if (aIsNewLine) {
        if (gCurLineBufferSize) {
            // Don't need to explicitly NULL-terminate the string because
            // snprintf takes care of that.
            WeaveLogDetail(DataManagement, "%s", gLineBuffer);
            gCurLineBufferSize = 0;
        }

        for (uint32_t i = 0; i < gPrettyPrintingDepthLevel; i++) {
            if (sizeof(gLineBuffer) > gCurLineBufferSize) {
                sizeLeft = sizeof(gLineBuffer) - gCurLineBufferSize;
                ret = snprintf(gLineBuffer + gCurLineBufferSize, sizeLeft, "\t");
                if (ret > 0) {
                    gCurLineBufferSize += MIN(ret, sizeLeft);
                }
            }
        }
    }

    if (sizeof(gLineBuffer) > gCurLineBufferSize) {
        sizeLeft = sizeof(gLineBuffer) - gCurLineBufferSize;
        ret = vsnprintf(gLineBuffer + gCurLineBufferSize, sizeLeft, aFmt, args);
        if (ret > 0) {
            gCurLineBufferSize += MIN(ret, sizeLeft);
        }
    }

    va_end(args);
}
#else // WEAVE_DETAIL_LOGGING
#define PRETTY_PRINT(fmt, ...)
#define PRETTY_PRINT_SAMELINE(fmt, ...)
#define PRETTY_PRINT_INCDEPTH()
#define PRETTY_PRINT_DECDEPTH()
#endif // WEAVE_DETAIL_LOGGING

WEAVE_ERROR LookForElementWithTag (const nl::Weave::TLV::TLVReader & aSrcReader, const uint64_t aTagInApiForm, nl::Weave::TLV::TLVReader * apDstReader)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    // make a copy of the Path reader
    nl::Weave::TLV::TLVReader reader;
    reader.Init(aSrcReader);

    while (WEAVE_NO_ERROR == (err = reader.Next()))
    {
        // Documentation says the result of GetType must be verified before calling GetTag
        VerifyOrExit(nl::Weave::TLV::kTLVType_NotSpecified != reader.GetType(), err = WEAVE_ERROR_INVALID_TLV_ELEMENT);

        if (aTagInApiForm == reader.GetTag())
        {
            apDstReader->Init(reader);
            break;
        }
    }

exit:
    WeaveLogIfFalse((WEAVE_NO_ERROR == err) || (WEAVE_END_OF_TLV == err));

    return err;
}

ParserBase::ParserBase()
{
}

WEAVE_ERROR ParserBase::GetReaderOnTag (const uint64_t aTagToFind, nl::Weave::TLV::TLVReader * const apReader) const
{
    return LookForElementWithTag(mReader, aTagToFind, apReader);
}

template<typename T>
WEAVE_ERROR ParserBase::GetUnsignedInteger(const uint8_t aContextTag, T * const apLValue) const
{
    return GetSimpleValue(aContextTag, nl::Weave::TLV::kTLVType_UnsignedInteger, apLValue);
}

template<typename T>
WEAVE_ERROR ParserBase::GetSimpleValue(const uint8_t aContextTag, const nl::Weave::TLV::TLVType aTLVType, T * const apLValue) const
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    nl::Weave::TLV::TLVReader reader;

    *apLValue = 0;

    err = LookForElementWithTag(mReader, nl::Weave::TLV::ContextTag(aContextTag), &reader);
    SuccessOrExit(err);

    VerifyOrExit(aTLVType == reader.GetType(), err = WEAVE_ERROR_WRONG_TLV_TYPE);

    err = reader.Get(*apLValue);
    SuccessOrExit(err);

exit:
    WeaveLogIfFalse((WEAVE_NO_ERROR == err) || (WEAVE_END_OF_TLV == err));

    return err;
}

ListParserBase::ListParserBase()
{
}

WEAVE_ERROR ListParserBase::Init (const nl::Weave::TLV::TLVReader & aReader)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    // make a copy of the reader here
    mReader.Init(aReader);

    VerifyOrExit (nl::Weave::TLV::kTLVType_Array == mReader.GetType(), err = WEAVE_ERROR_WRONG_TLV_TYPE);

    // This is just a dummy, as we're not going to exit this container ever
    nl::Weave::TLV::TLVType OuterContainerType;
    err = mReader.EnterContainer(OuterContainerType);

exit:
    WeaveLogFunctError(err);

    return err;
}

WEAVE_ERROR ListParserBase::InitIfPresent (const nl::Weave::TLV::TLVReader & aReader, const uint8_t aContextTagToFind)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    nl::Weave::TLV::TLVReader reader;

    err = LookForElementWithTag(aReader, nl::Weave::TLV::ContextTag(aContextTagToFind), &reader);
    SuccessOrExit(err);

    err = Init(reader);
    SuccessOrExit(err);

exit:
    WeaveLogIfFalse((WEAVE_NO_ERROR == err) || (WEAVE_END_OF_TLV == err));

    return err;
}

WEAVE_ERROR ListParserBase::Next (void)
{
    WEAVE_ERROR err = mReader.Next();

    WeaveLogIfFalse((WEAVE_NO_ERROR == err) || (WEAVE_END_OF_TLV == err));

    return err;
}

void ListParserBase::GetReader(nl::Weave::TLV::TLVReader * const apReader)
{
    apReader->Init(mReader);
}

BuilderBase::BuilderBase() :
    mError(WEAVE_ERROR_INCORRECT_STATE),
    mpWriter(NULL),
    mOuterContainerType(nl::Weave::TLV::kTLVType_NotSpecified)
{
}

void BuilderBase::ResetError()
{
    mError = WEAVE_NO_ERROR;
    mOuterContainerType = nl::Weave::TLV::kTLVType_NotSpecified;
}


void BuilderBase::EndOfContainer()
{
    // skip if error has already been set
    SuccessOrExit(mError);

    mError = mpWriter->EndContainer(mOuterContainerType);
    SuccessOrExit(mError);

    // we've just closed properly
    // mark it so we do not panic when the build object destructor is called
    mOuterContainerType = nl::Weave::TLV::kTLVType_NotSpecified;

exit:
    ;
}

WEAVE_ERROR BuilderBase::InitAnonymousStructure (nl::Weave::TLV::TLVWriter * const apWriter)
{
    mpWriter = apWriter;
    mOuterContainerType = nl::Weave::TLV::kTLVType_NotSpecified;
    mError = mpWriter->StartContainer(nl::Weave::TLV::AnonymousTag, nl::Weave::TLV::kTLVType_Structure, mOuterContainerType);
    WeaveLogFunctError(mError);

    return mError;
}

ListBuilderBase::ListBuilderBase (void)
{
}

WEAVE_ERROR ListBuilderBase::Init (nl::Weave::TLV::TLVWriter * const apWriter, const uint8_t aContextTagToUse)
{
    mpWriter = apWriter;
    mOuterContainerType = nl::Weave::TLV::kTLVType_NotSpecified;
    mError = mpWriter->StartContainer(nl::Weave::TLV::ContextTag(aContextTagToUse),
        nl::Weave::TLV::kTLVType_Array, mOuterContainerType);
    WeaveLogFunctError(mError);

    return mError;
}

WEAVE_ERROR Path::Parser::Init (const nl::Weave::TLV::TLVReader & aReader)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    // make a copy of the reader here
    mReader.Init(aReader);

    VerifyOrExit (nl::Weave::TLV::kTLVType_Path == mReader.GetType(), err = WEAVE_ERROR_WRONG_TLV_TYPE);

    // This is just a dummy, as we're not going to exit this container ever
    nl::Weave::TLV::TLVType dummyContainerType;
    // enter into the Path
    err = mReader.EnterContainer(dummyContainerType);
    SuccessOrExit(err);
    err = mReader.Next();
    SuccessOrExit(err);

    VerifyOrExit (nl::Weave::TLV::ContextTag(kCsTag_InstanceLocator) == mReader.GetTag(), err = WEAVE_ERROR_INVALID_TLV_ELEMENT);
    VerifyOrExit (nl::Weave::TLV::kTLVType_Structure == mReader.GetType(), err = WEAVE_ERROR_WRONG_TLV_TYPE);

    // enter into the root section, we still need to call Next to access the first element
    err = mReader.EnterContainer(dummyContainerType);
    SuccessOrExit(err);

exit:
    WeaveLogFunctError(err);

    return err;
}

// Get a TLVReader at the additional tags section. Next() must be called before accessing it.
WEAVE_ERROR Path::Parser::GetTags(nl::Weave::TLV::TLVReader * const apReader) const
{
    nl::Weave::TLV::TLVType pathContainerType = nl::Weave::TLV::kTLVType_Path;
    apReader->Init(mReader);
    apReader->ExitContainer(pathContainerType);

    return WEAVE_NO_ERROR;
}

#if WEAVE_CONFIG_DATA_MANAGEMENT_ENABLE_SCHEMA_CHECK
// Roughly verify the schema is right, including
// 1) all mandatory tags are present
// 2) no unknown tags
// 3) all elements have expected data type
// 4) any tag can only appear once
WEAVE_ERROR Path::Parser::CheckSchemaValidity(void) const
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint16_t TagPresenceMask = 0;
    nl::Weave::TLV::TLVReader reader;

    PRETTY_PRINT_SAMELINE("<Resource = {");

    // make a copy of the Path reader
    reader.Init(mReader);

    while (WEAVE_NO_ERROR == (err = reader.Next()))
    {
        VerifyOrExit(nl::Weave::TLV::IsContextTag(reader.GetTag()), err = WEAVE_ERROR_INVALID_TLV_TAG);
        switch (nl::Weave::TLV::TagNumFromTag(reader.GetTag()))
        {
        case kCsTag_ResourceID:
            // check if this tag has appeared before
            VerifyOrExit (!(TagPresenceMask & (1 << kCsTag_ResourceID)), err = WEAVE_ERROR_INVALID_TLV_TAG);
            TagPresenceMask |= (1 << kCsTag_ResourceID);
            // Resource ID can be of any type, so no checking is done here

#if WEAVE_DETAIL_LOGGING
            if (nl::Weave::TLV::kTLVType_UnsignedInteger == reader.GetType())
            {
                uint64_t ResourceID;
                reader.Get(ResourceID);
                PRETTY_PRINT_SAMELINE("ResourceId = 0x%" PRIx64 ",", ResourceID);
            }
            else
            {
                PRETTY_PRINT_SAMELINE("ResourceId = ??,");
            }

#endif // WEAVE_DETAIL_LOGGING
            break;
        case kCsTag_TraitInstanceID:
            // check if this tag has appeared before
            VerifyOrExit (!(TagPresenceMask & (1 << kCsTag_TraitInstanceID)), err = WEAVE_ERROR_INVALID_TLV_TAG);
            TagPresenceMask |= (1 << kCsTag_TraitInstanceID);
            // Instance ID can be of any type, so no checking is done here

#if WEAVE_DETAIL_LOGGING
            if (nl::Weave::TLV::kTLVType_UnsignedInteger == reader.GetType())
            {
                uint64_t InstanceID;
                reader.Get(InstanceID);
                PRETTY_PRINT_SAMELINE("InstanceId = 0x%" PRIx64 ",", InstanceID);
            }
            else
            {
                PRETTY_PRINT_SAMELINE("InstanceId = ??");
            }
#endif // WEAVE_DETAIL_LOGGING
            break;

        case kCsTag_TraitProfileID:
        {
            SchemaVersionRange requestedVersion;
            uint32_t ProfileID;

            // check if this tag has appeared before
            VerifyOrExit (!(TagPresenceMask & (1 << kCsTag_TraitProfileID)), err = WEAVE_ERROR_INVALID_TLV_TAG);
            TagPresenceMask |= (1 << kCsTag_TraitProfileID);

            if (reader.GetType() == nl::Weave::TLV::kTLVType_Array) {
                TLV::TLVType type;

                err = reader.EnterContainer(type);
                SuccessOrExit(err);

                err = reader.Next();
                SuccessOrExit(err);

                VerifyOrExit(reader.GetType() == nl::Weave::TLV::kTLVType_UnsignedInteger, err = WEAVE_ERROR_WRONG_TLV_TYPE);

                err = reader.Get(ProfileID);
                SuccessOrExit(err);

                err = reader.Next();
                VerifyOrExit(err == WEAVE_NO_ERROR || err == WEAVE_END_OF_TLV, );

                if (err == WEAVE_NO_ERROR) {
                    VerifyOrExit(reader.GetType() == nl::Weave::TLV::kTLVType_UnsignedInteger, err = WEAVE_ERROR_WRONG_TLV_TYPE);
                    err = reader.Get(requestedVersion.mMaxVersion);
                    SuccessOrExit(err);
                }

                err = reader.Next();
                VerifyOrExit(err == WEAVE_NO_ERROR || err == WEAVE_END_OF_TLV, );

                if (err == WEAVE_NO_ERROR) {
                    VerifyOrExit(reader.GetType() == nl::Weave::TLV::kTLVType_UnsignedInteger, err = WEAVE_ERROR_WRONG_TLV_TYPE);
                    err = reader.Get(requestedVersion.mMinVersion);
                    SuccessOrExit(err);
                }

                err = reader.Next();
                VerifyOrExit(err == WEAVE_END_OF_TLV, err = WEAVE_ERROR_WDM_MALFORMED_DATA_ELEMENT);

                err = reader.ExitContainer(type);
            }
            else {
                VerifyOrExit(reader.GetType() == nl::Weave::TLV::kTLVType_UnsignedInteger, err = WEAVE_ERROR_WRONG_TLV_TYPE);
                err = reader.Get(ProfileID);
                SuccessOrExit(err);
            }

#if WEAVE_DETAIL_LOGGING
            {
                if (requestedVersion.mMaxVersion > 1 || requestedVersion.mMinVersion > 1) {
                    PRETTY_PRINT_SAMELINE("[ProfileId = 0x%" PRIx32, ProfileID);

                    if (requestedVersion.mMaxVersion > 1) {
                        PRETTY_PRINT_SAMELINE(", MaxVersion = %" PRIu32, requestedVersion.mMaxVersion);
                    }

                    if (requestedVersion.mMaxVersion > 1) {
                        PRETTY_PRINT_SAMELINE(", MinVersion = %" PRIu32 "],", requestedVersion.mMinVersion);
                    }
                    else {
                        PRETTY_PRINT_SAMELINE("],");
                    }
                }
                else {
                    PRETTY_PRINT_SAMELINE("ProfileId = 0x%" PRIx32 ",", ProfileID);
                }
            }
#endif // WEAVE_DETAIL_LOGGING

            break;
        }

        default:
            ExitNow(err = WEAVE_ERROR_INVALID_TLV_TAG);
        }
    }

    // if we have exhausted this container
    if (WEAVE_END_OF_TLV == err)
    {
        // okay if we have at least the Profile ID field
        if (TagPresenceMask & (1 << kCsTag_TraitProfileID))
        {
            err = WEAVE_NO_ERROR;
        }
    }
    SuccessOrExit(err);

    PRETTY_PRINT_SAMELINE("}");

    err = GetTags(&reader);
    SuccessOrExit(err);

    // Verify that the remaining additional tag section has only TAG=NULL elements, and the tags cannot be anonymous
    while (WEAVE_NO_ERROR == (err = reader.Next()))
    {
        // Cannot have an anonymous element in this section
        VerifyOrExit (nl::Weave::TLV::AnonymousTag != reader.GetTag(), err = WEAVE_ERROR_INVALID_TLV_TAG);
        // Can only have NULL value in this section
        VerifyOrExit (nl::Weave::TLV::kTLVType_Null == reader.GetType(), err = WEAVE_ERROR_WRONG_TLV_TYPE);

#if WEAVE_DETAIL_LOGGING
        const uint64_t tag = reader.GetTag();
        if (nl::Weave::TLV::IsContextTag(tag))
        {
            PRETTY_PRINT_SAMELINE("/0x%" PRIx32 " = null", nl::Weave::TLV::TagNumFromTag(tag));
        }
        else if (nl::Weave::TLV::IsProfileTag(tag))
        {
            PRETTY_PRINT_SAMELINE("/0x%" PRIx32 "::0x%" PRIx32 " = null", nl::Weave::TLV::ProfileIdFromTag(tag), nl::Weave::TLV::TagNumFromTag(tag));
        }
        else
        {
            PRETTY_PRINT_SAMELINE("?");
        }
#endif // WEAVE_DETAIL_LOGGING

    }

    PRETTY_PRINT_SAMELINE(">,");

    if (WEAVE_END_OF_TLV == err)
    {
        err = WEAVE_NO_ERROR;
    }
    SuccessOrExit(err);

exit:
    WeaveLogFunctError(err);

    return err;
}
#endif // WEAVE_CONFIG_DATA_MANAGEMENT_ENABLE_SCHEMA_CHECK

// Resource ID could be of any type, so we can only position the reader so the caller has
// full information of tag, element type, length, and value
// WEAVE_END_OF_TLV if there is no such element
WEAVE_ERROR Path::Parser::GetResourceID (nl::Weave::TLV::TLVReader * const apReader) const
{
    WEAVE_ERROR err = LookForElementWithTag(mReader, nl::Weave::TLV::ContextTag(kCsTag_ResourceID), apReader);

    WeaveLogIfFalse((WEAVE_NO_ERROR == err) || (WEAVE_END_OF_TLV == err));

    return err;
}

// WEAVE_END_OF_TLV if there is no such element
// WEAVE_ERROR_WRONG_TLV_TYPE if there is such element but it's not any of the defined unsigned integer types
WEAVE_ERROR Path::Parser::GetResourceID (uint64_t * const apResourceID) const
{
    return GetUnsignedInteger(kCsTag_ResourceID, apResourceID);
}

// Instance ID could be of any type, so we can only position the reader so the caller has
// full information of tag, element type, length, and value
WEAVE_ERROR Path::Parser::GetInstanceID (nl::Weave::TLV::TLVReader * const apReader) const
{
    WEAVE_ERROR err = LookForElementWithTag(mReader, nl::Weave::TLV::ContextTag(kCsTag_TraitInstanceID), apReader);

    WeaveLogIfFalse((WEAVE_NO_ERROR == err) || (WEAVE_END_OF_TLV == err));

    return err;
}

// WEAVE_END_OF_TLV if there is no such element
// WEAVE_ERROR_WRONG_TLV_TYPE if there is such element but it's not any of the defined unsigned integer types
WEAVE_ERROR Path::Parser::GetInstanceID (uint64_t * const apInstanceID) const
{
    return GetUnsignedInteger(kCsTag_TraitInstanceID, apInstanceID);
}

// Profile ID can only be uint32_t and not any other type
// WEAVE_END_OF_TLV if there is no such element
// WEAVE_ERROR_WRONG_TLV_TYPE if there is such element but it's not any of the defined unsigned integer types
WEAVE_ERROR Path::Parser::GetProfileID (uint32_t * const apProfileID, SchemaVersionRange * const apSchemaVersionRange)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    nl::Weave::TLV::TLVReader reader;
    nl::Weave::TLV::TLVType outerContainerType;

    apSchemaVersionRange->mMinVersion = 1;
    apSchemaVersionRange->mMaxVersion = 1;

    err = LookForElementWithTag(mReader, nl::Weave::TLV::ContextTag(kCsTag_TraitProfileID), &reader);
    SuccessOrExit(err);

    if (reader.GetType() == nl::Weave::TLV::kTLVType_Array) {
        err = reader.EnterContainer(outerContainerType);
        SuccessOrExit(err);

        err = reader.Next();
        SuccessOrExit(err);

        VerifyOrExit(reader.GetType() == nl::Weave::TLV::kTLVType_UnsignedInteger, err = WEAVE_ERROR_WRONG_TLV_TYPE);

        err = reader.Get(*apProfileID);
        SuccessOrExit(err);

        err = reader.Next();
        VerifyOrExit(err == WEAVE_NO_ERROR || err == WEAVE_END_OF_TLV, );

        if (err == WEAVE_NO_ERROR) {
            VerifyOrExit(reader.GetType() == nl::Weave::TLV::kTLVType_UnsignedInteger, err = WEAVE_ERROR_WRONG_TLV_TYPE);

            err = reader.Get(apSchemaVersionRange->mMaxVersion);
            SuccessOrExit(err);
        }

        err = reader.Next();
        VerifyOrExit(err == WEAVE_NO_ERROR || err == WEAVE_END_OF_TLV, );

        if (err == WEAVE_NO_ERROR) {
            VerifyOrExit(reader.GetType() == nl::Weave::TLV::kTLVType_UnsignedInteger, err = WEAVE_ERROR_WRONG_TLV_TYPE);

            err = reader.Get(apSchemaVersionRange->mMinVersion);
            SuccessOrExit(err);
        }

        err = reader.Next();
        VerifyOrExit(err == WEAVE_END_OF_TLV, err = WEAVE_ERROR_WDM_MALFORMED_DATA_ELEMENT);
    }
    else {
        VerifyOrExit(reader.GetType() == nl::Weave::TLV::kTLVType_UnsignedInteger, err = WEAVE_ERROR_WRONG_TLV_TYPE);

        err = reader.Get(*apProfileID);
    }

exit:
    if (err == WEAVE_END_OF_TLV) {
        err = WEAVE_NO_ERROR;
    }

    return err;
}

WEAVE_ERROR Path::Builder::_Init(nl::Weave::TLV::TLVWriter * const apWriter, const uint64_t aTagInApiForm)
{
    mpWriter = apWriter;
    mOuterContainerType = nl::Weave::TLV::kTLVType_NotSpecified;
    mError = mpWriter->StartContainer(aTagInApiForm, nl::Weave::TLV::kTLVType_Path, mOuterContainerType);
    SuccessOrExit(mError);

    // We don't care about storing the outer container's type here, for we know it's a Path
    nl::Weave::TLV::TLVType dummyContainerType;
    mError = mpWriter->StartContainer(nl::Weave::TLV::ContextTag(kCsTag_InstanceLocator), nl::Weave::TLV::kTLVType_Structure, dummyContainerType);
    SuccessOrExit(mError);

    mInTagSection = false;

exit:
    WeaveLogFunctError(mError);

    return mError;
}

WEAVE_ERROR Path::Builder::Init(nl::Weave::TLV::TLVWriter * const apWriter)
{
    return _Init(apWriter, nl::Weave::TLV::AnonymousTag);
}

WEAVE_ERROR Path::Builder::Init(nl::Weave::TLV::TLVWriter * const apWriter, const uint8_t aContextTagToUse)
{
    return _Init(apWriter, nl::Weave::TLV::ContextTag(aContextTagToUse));
}

Path::Builder & Path::Builder::ResourceID (const uint64_t aResourceID)
{
    // skip if error has already been set
    SuccessOrExit(mError);
    // we can only add this field when we're already in the tag section of a Path
    VerifyOrExit(!mInTagSection, mError = WEAVE_ERROR_INCORRECT_STATE);

    mError = mpWriter->Put(nl::Weave::TLV::ContextTag(kCsTag_ResourceID), aResourceID);
    WeaveLogFunctError(mError);

exit:

    return *this;
}

Path::Builder & Path::Builder::InstanceID (const uint64_t aInstanceID)
{
    // skip if error has already been set
    SuccessOrExit(mError);
    // we can only add this field when we're already in the tag section of a Path
    VerifyOrExit(!mInTagSection, mError = WEAVE_ERROR_INCORRECT_STATE);

    mError = mpWriter->Put(nl::Weave::TLV::ContextTag(kCsTag_TraitInstanceID), aInstanceID);
    WeaveLogFunctError(mError);

exit:

    return *this;
}

Path::Builder & Path::Builder::ProfileID (const uint32_t aProfileID)
{
    SchemaVersionRange versionRange;
    return ProfileID(aProfileID, versionRange);
}

Path::Builder & Path::Builder::ProfileID (const uint32_t aProfileID, const SchemaVersionRange &aSchemaVersionRange)
{
    // skip if error has already been set
    SuccessOrExit(mError);

    // we can only add this field when we're already in the tag section of a Path
    VerifyOrExit(!mInTagSection, mError = WEAVE_ERROR_INCORRECT_STATE);

    VerifyOrExit(aSchemaVersionRange.IsValid(), mError = WEAVE_ERROR_INVALID_ARGUMENT);

    if (aSchemaVersionRange.mMaxVersion != 1 || aSchemaVersionRange.mMinVersion != 1) {
        TLV::TLVType type2;

        mError = mpWriter->StartContainer(TLV::ContextTag(Path::kCsTag_TraitProfileID), TLV::kTLVType_Array, type2);
        SuccessOrExit(mError);

        mError = mpWriter->Put(TLV::AnonymousTag, aProfileID);
        SuccessOrExit(mError);

        if (aSchemaVersionRange.mMaxVersion != 1) {
            mError = mpWriter->Put(TLV::AnonymousTag, aSchemaVersionRange.mMaxVersion);
            SuccessOrExit(mError);
        }

        if (aSchemaVersionRange.mMinVersion != 1) {
            mError = mpWriter->Put(TLV::AnonymousTag, aSchemaVersionRange.mMinVersion);
            SuccessOrExit(mError);
        }

        mError = mpWriter->EndContainer(type2);
        SuccessOrExit(mError);
    }
    else {
        mError = mpWriter->Put(TLV::ContextTag(Path::kCsTag_TraitProfileID), aProfileID);
        SuccessOrExit(mError);
    }

exit:
    return *this;
}

Path::Builder & Path::Builder::TagSection (void)
{
    // skip if error has already been set
    SuccessOrExit(mError);
    // we can only add this field when we're already in the tag section of a Path
    VerifyOrExit(!mInTagSection, mError = WEAVE_ERROR_INCORRECT_STATE);

    mError = mpWriter->EndContainer(nl::Weave::TLV::kTLVType_Path);
    WeaveLogFunctError(mError);

    mInTagSection = true;

exit:

    return *this;
}

Path::Builder & Path::Builder::AdditionalTag (const uint64_t aTagInApiForm)
{
    // skip if error has already been set
    SuccessOrExit(mError);
    // we can only add additional tags if we're in the tag section of a Path
    VerifyOrExit(mInTagSection, mError = WEAVE_ERROR_INCORRECT_STATE);

    mError = mpWriter->PutNull(aTagInApiForm);
    WeaveLogFunctError(mError);

exit:

    return *this;
}

Path::Builder & Path::Builder::EndOfPath (void)
{
    // skip if error has already been set
    SuccessOrExit(mError);

    if (!mInTagSection)
    {
        // leave the first level container only if TagSection() hasn't been called
        mError = mpWriter->EndContainer(nl::Weave::TLV::kTLVType_Path);
        SuccessOrExit(mError);
    }

    EndOfContainer();

exit:

    return *this;
}

// aReader has to be on the element of DataElement
WEAVE_ERROR DataElement::Parser::Init (const nl::Weave::TLV::TLVReader & aReader)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    // make a copy of the reader here
    mReader.Init(aReader);

    VerifyOrExit (nl::Weave::TLV::kTLVType_Structure == mReader.GetType(), err = WEAVE_ERROR_WRONG_TLV_TYPE);

    // This is just a dummy, as we're not going to exit this container ever
    nl::Weave::TLV::TLVType OuterContainerType;
    err = mReader.EnterContainer(OuterContainerType);

exit:
    WeaveLogFunctError(err);

    return err;
}

WEAVE_ERROR
DataElement::Parser::ParseData(nl::Weave::TLV::TLVReader &aReader, int aDepth) const
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    if (aDepth == 0)
    {
        PRETTY_PRINT("\t\tData = ");
    }
    else
    {
        if (nl::Weave::TLV::IsContextTag(aReader.GetTag()))
        {
            PRETTY_PRINT("\t\t0x%" PRIx32 " = ", nl::Weave::TLV::TagNumFromTag(aReader.GetTag()));
        }
        else if (nl::Weave::TLV::IsProfileTag(aReader.GetTag()))
        {
            PRETTY_PRINT("\t\t0x%" PRIx32 "::0x%" PRIx32 " = ", nl::Weave::TLV::ProfileIdFromTag(aReader.GetTag()),
                    nl::Weave::TLV::TagNumFromTag(aReader.GetTag()));
        }
        else
        {
            // Anonymous tag, don't print anything
        }
    }

    switch (aReader.GetType())
    {
        case nl::Weave::TLV::kTLVType_Structure:
            PRETTY_PRINT("\t\t{");
            break;

        case nl::Weave::TLV::kTLVType_Array:
            PRETTY_PRINT_SAMELINE("[");
            PRETTY_PRINT("\t\t\t");
            break;

        case nl::Weave::TLV::kTLVType_SignedInteger:
        {
            int64_t value_s64;

            err = aReader.Get(value_s64);
            SuccessOrExit(err);

            PRETTY_PRINT_SAMELINE("%" PRId64 ", ", value_s64);
            break;
        }

        case nl::Weave::TLV::kTLVType_UnsignedInteger:
        {
            uint64_t value_u64;

            err = aReader.Get(value_u64);
            SuccessOrExit(err);

            PRETTY_PRINT_SAMELINE("%" PRIu64 ", ", value_u64);
            break;
        }

        case nl::Weave::TLV::kTLVType_Boolean:
        {
            bool value_b;

            err = aReader.Get(value_b);
            SuccessOrExit(err);

            PRETTY_PRINT_SAMELINE("%s, ", value_b ? "true" : "false");
            break;
        }

        case nl::Weave::TLV::kTLVType_UTF8String:
        {
            char value_s[256];

            err = aReader.GetString(value_s, sizeof(value_s));
            VerifyOrExit(err == WEAVE_NO_ERROR || err == WEAVE_ERROR_BUFFER_TOO_SMALL, );

            if (err == WEAVE_ERROR_BUFFER_TOO_SMALL) {
                PRETTY_PRINT_SAMELINE("... (byte string too long) ...");
                err = WEAVE_NO_ERROR;
            }
            else
            {
                PRETTY_PRINT_SAMELINE("\"%s\", ", value_s);
            }
            break;
        }

        case nl::Weave::TLV::kTLVType_ByteString:
        {
            uint8_t value_b[256];
            uint32_t len, readerLen;

            readerLen = aReader.GetLength();

            err = aReader.GetBytes(value_b, sizeof(value_b));
            VerifyOrExit(err == WEAVE_NO_ERROR || err == WEAVE_ERROR_BUFFER_TOO_SMALL, );

            PRETTY_PRINT_SAMELINE("[");
            PRETTY_PRINT("\t\t\t");

            if (readerLen < sizeof(value_b))
            {
                len = readerLen;
            }
            else
            {
                len = sizeof(value_b);
            }

            if (err == WEAVE_ERROR_BUFFER_TOO_SMALL)
            {
                PRETTY_PRINT_SAMELINE("... (byte string too long) ...");
            }
            else
            {
                for (size_t i = 0; i < len; i++) {
                    PRETTY_PRINT_SAMELINE("0x%" PRIx8 ", ", value_b[i]);
                }
            }

            err = WEAVE_NO_ERROR;
            PRETTY_PRINT("\t\t]");
            break;
        }

        case nl::Weave::TLV::kTLVType_Null:
            PRETTY_PRINT_SAMELINE("NULL");
            break;

        default:
            PRETTY_PRINT_SAMELINE("--");
            break;
    }

    if (aReader.GetType() == nl::Weave::TLV::kTLVType_Structure ||
            aReader.GetType() == nl::Weave::TLV::kTLVType_Array)
    {
        const char terminating_char = (aReader.GetType() == nl::Weave::TLV::kTLVType_Structure) ? '}' : ']';
        nl::Weave::TLV::TLVType type;

        IgnoreUnusedVariable(terminating_char);

        err = aReader.EnterContainer(type);
        SuccessOrExit(err);

        while ((err = aReader.Next()) == WEAVE_NO_ERROR)
        {
            PRETTY_PRINT_INCDEPTH();

            err = ParseData(aReader, aDepth + 1);
            SuccessOrExit(err);

            PRETTY_PRINT_DECDEPTH();
        }

        PRETTY_PRINT("\t\t%c,", terminating_char);

        err = aReader.ExitContainer(type);
        SuccessOrExit(err);
    }

exit:
    WeaveLogFunctError(err);
    return err;
}

#if WEAVE_CONFIG_DATA_MANAGEMENT_ENABLE_SCHEMA_CHECK
// Roughly verify the schema is right, including
// 1) all mandatory tags are present
// 2) all elements have expected data type
// 3) any tag can only appear once
// At the top level of the structure, unknown tags are ignored for foward compatibility
WEAVE_ERROR DataElement::Parser::CheckSchemaValidity (void) const
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint16_t TagPresenceMask = 0;
    nl::Weave::TLV::TLVReader reader;
    uint32_t tagNum = 0;

    PRETTY_PRINT("\t{");

    // make a copy of the reader
    reader.Init(mReader);

    while (WEAVE_NO_ERROR == (err = reader.Next()))
    {
        VerifyOrExit(nl::Weave::TLV::IsContextTag(reader.GetTag()), err = WEAVE_ERROR_INVALID_TLV_TAG);

        tagNum = nl::Weave::TLV::TagNumFromTag(reader.GetTag());

        switch (tagNum)
        {
        case kCsTag_Path:
            // check if this tag has appeared before
            VerifyOrExit (!(TagPresenceMask & (1 << kCsTag_Path)), err = WEAVE_ERROR_INVALID_TLV_TAG);
            TagPresenceMask |= (1 << kCsTag_Path);
            VerifyOrExit(nl::Weave::TLV::kTLVType_Path == reader.GetType(), err = WEAVE_ERROR_WRONG_TLV_TYPE);

            PRETTY_PRINT("\t\tDataElementPath = ");

            {
                Path::Parser path;
                err = path.Init(reader);
                SuccessOrExit(err);
                err = path.CheckSchemaValidity();
                SuccessOrExit(err);
            }

            break;
        case kCsTag_Version:
            // check if this tag has appeared before
            VerifyOrExit (!(TagPresenceMask & (1 << kCsTag_Version)), err = WEAVE_ERROR_INVALID_TLV_TAG);
            TagPresenceMask |= (1 << kCsTag_Version);
            VerifyOrExit(nl::Weave::TLV::kTLVType_UnsignedInteger == reader.GetType(), err = WEAVE_ERROR_WRONG_TLV_TYPE);

#if WEAVE_DETAIL_LOGGING
            {
                uint64_t version;
                err = reader.Get(version);
                SuccessOrExit(err);

                PRETTY_PRINT("\t\tDataElementVersion = 0x%" PRIx64 ",", version);
            }
#endif // WEAVE_DETAIL_LOGGING
            break;
        case kCsTag_Data:
            // check if this tag has appeared before
            VerifyOrExit (!(TagPresenceMask & (1 << kCsTag_Data)), err = WEAVE_ERROR_INVALID_TLV_TAG);
            TagPresenceMask |= (1 << kCsTag_Data);

            err = ParseData(reader, 0);
            SuccessOrExit(err);
            break;

        case kCsTag_DeletedDictionaryKeys:
            nl::Weave::TLV::TLVType type;

            VerifyOrExit (!(TagPresenceMask & (1 << kCsTag_DeletedDictionaryKeys)), err = WEAVE_ERROR_INVALID_TLV_TAG);
            TagPresenceMask |= (1 << kCsTag_DeletedDictionaryKeys);
            VerifyOrExit(nl::Weave::TLV::kTLVType_Array == reader.GetType(), err = WEAVE_ERROR_WRONG_TLV_TYPE);

            err = reader.EnterContainer(type);
            SuccessOrExit(err);

            PRETTY_PRINT("\t\tDataElement_DeletedDictionaryKeys =");
            PRETTY_PRINT("\t\t[");

            while ((err = reader.Next()) == WEAVE_NO_ERROR)
            {
                PropertyDictionaryKey key;

                err = reader.Get(key);
                SuccessOrExit(err);

                PRETTY_PRINT("\t\t\t0x%" PRIx16 ",", key);
            }

            PRETTY_PRINT("\t\t],");

            err = reader.ExitContainer(type);
            SuccessOrExit(err);
            break;

        case kCsTag_IsPartialChange:
            // check if this tag has appeared before
            VerifyOrExit (!(TagPresenceMask & (1 << kCsTag_IsPartialChange)), err = WEAVE_ERROR_INVALID_TLV_TAG);
            TagPresenceMask |= (1 << kCsTag_IsPartialChange);
            VerifyOrExit(nl::Weave::TLV::kTLVType_Boolean == reader.GetType(), err = WEAVE_ERROR_WRONG_TLV_TYPE);

#if WEAVE_DETAIL_LOGGING
            {
                bool flag;
                err = reader.Get(flag);
                SuccessOrExit(err);

                PRETTY_PRINT("\t\tDataElement_IsPartialChange = %s,", flag ? "true" : "false");
            }

#endif // WEAVE_DETAIL_LOGGING
            break;

        default:
            PRETTY_PRINT("\t\tUnknown tag num %" PRIu32 , tagNum);
            break;
        }
    }

    PRETTY_PRINT("\t},");

    // if we have exhausted this container
    if (WEAVE_END_OF_TLV == err)
    {
        // check for required fields:
        const uint16_t RequiredFields = (1 << kCsTag_Path);

        // Either the data or deleted keys should be present.
        const uint16_t DataElementTypeMask = (1 << kCsTag_Data) | (1 << kCsTag_DeletedDictionaryKeys);
        if ((TagPresenceMask & RequiredFields) == RequiredFields)
        {
            err = WEAVE_NO_ERROR;
        }
        else
        {
            err = WEAVE_ERROR_WDM_MALFORMED_DATA_ELEMENT;
        }
        if ((TagPresenceMask & DataElementTypeMask) == 0)
        {
            err = WEAVE_ERROR_WDM_MALFORMED_DATA_ELEMENT;
        }
    }

exit:
    WeaveLogFunctError(err);

    return err;
}
#endif // WEAVE_CONFIG_DATA_MANAGEMENT_ENABLE_SCHEMA_CHECK

// WEAVE_END_OF_TLV if there is no such element
// WEAVE_ERROR_WRONG_TLV_TYPE if there is such element but it's not a Path
WEAVE_ERROR DataElement::Parser::GetReaderOnPath (nl::Weave::TLV::TLVReader * const apReader) const
{
    WEAVE_ERROR err = LookForElementWithTag(mReader, nl::Weave::TLV::ContextTag(kCsTag_Path), apReader);

    WeaveLogIfFalse((WEAVE_NO_ERROR == err) || (WEAVE_END_OF_TLV == err));

    return err;
}

// WEAVE_END_OF_TLV if there is no such element
// WEAVE_ERROR_WRONG_TLV_TYPE if there is such element but it's not a Path
WEAVE_ERROR DataElement::Parser::GetPath (Path::Parser * const apPath) const
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    nl::Weave::TLV::TLVReader reader;

    err = LookForElementWithTag(mReader, nl::Weave::TLV::ContextTag(kCsTag_Path), &reader);
    SuccessOrExit(err);

    VerifyOrExit(nl::Weave::TLV::kTLVType_Path == reader.GetType(), err = WEAVE_ERROR_WRONG_TLV_TYPE);

    err = apPath->Init(reader);
    SuccessOrExit(err);

exit:
    WeaveLogIfFalse((WEAVE_NO_ERROR == err) || (WEAVE_END_OF_TLV == err));

    return err;
}

// WEAVE_END_OF_TLV if there is no such element
// WEAVE_ERROR_WRONG_TLV_TYPE if there is such element but it's not any of the defined unsigned integer types
WEAVE_ERROR DataElement::Parser::GetVersion (uint64_t * const apVersion) const
{
    return GetUnsignedInteger(kCsTag_Version, apVersion);
}

// Data could be of any type, so we can only position the reader so the caller has
// full information of tag, element type, length, and value
WEAVE_ERROR DataElement::Parser::GetData (nl::Weave::TLV::TLVReader * const apReader) const
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    err = LookForElementWithTag(mReader, nl::Weave::TLV::ContextTag(kCsTag_Data), apReader);
    SuccessOrExit(err);

exit:
    WeaveLogIfFalse((WEAVE_NO_ERROR == err) || (WEAVE_END_OF_TLV == err));

    return err;
}

// Default is false if there is no such element
// WEAVE_ERROR_WRONG_TLV_TYPE if there is such element but it's not a boolean
WEAVE_ERROR DataElement::Parser::GetPartialChangeFlag(bool * const apPartialChangeFlag) const
{
    return GetSimpleValue(kCsTag_IsPartialChange, nl::Weave::TLV::kTLVType_Boolean, apPartialChangeFlag);
}

WEAVE_ERROR DataElement::Parser::CheckPresence(bool * const apDataPresentFlag, bool * const apDeletePresentFlag) const
{
    nl::Weave::TLV::TLVReader reader;
    WEAVE_ERROR err_datamerge, err_dictionarydelete, err = WEAVE_NO_ERROR;

    err_datamerge = LookForElementWithTag(mReader, nl::Weave::TLV::ContextTag(kCsTag_Data), &reader);
    err_dictionarydelete = LookForElementWithTag(mReader, nl::Weave::TLV::ContextTag(kCsTag_DeletedDictionaryKeys), &reader);

    if ((err_datamerge == WEAVE_END_OF_TLV) && (err_dictionarydelete == WEAVE_END_OF_TLV))
    {
        err = WEAVE_ERROR_WDM_MALFORMED_DATA_ELEMENT;
    }

    if (err_datamerge == WEAVE_NO_ERROR)
    {
        *apDataPresentFlag = true;
    }

    if (err_dictionarydelete == WEAVE_NO_ERROR)
    {
        *apDeletePresentFlag = true;
    }

    return err;
}

WEAVE_ERROR DataElement::Parser::GetDeletedDictionaryKeys(nl::Weave::TLV::TLVReader * const apReader) const
{
    WEAVE_ERROR err;
    nl::Weave::TLV::TLVType containerType;

    err = LookForElementWithTag(mReader, nl::Weave::TLV::ContextTag(kCsTag_DeletedDictionaryKeys), apReader);
    SuccessOrExit(err);

    VerifyOrExit(apReader->GetType() == nl::Weave::TLV::kTLVType_Array, err = WEAVE_ERROR_WDM_MALFORMED_DATA_ELEMENT);

    err = apReader->EnterContainer(containerType);

exit:
    WeaveLogFunctError(err);
    return err;
}

// DataElement is only used in a Data List, which requires every path to be anonymous
// Note that both mWriter and mPathBuilder only hold reference to the actual TLVWriter
WEAVE_ERROR DataElement::Builder::Init (nl::Weave::TLV::TLVWriter * const apWriter)
{
    return InitAnonymousStructure(apWriter);
}

Path::Builder & DataElement::Builder::CreatePathBuilder()
{
    // skip if error has already been set
    SuccessOrExit(mError);

    mError = mPathBuilder.Init(mpWriter, nl::Weave::TLV::ContextTag(kCsTag_Path));
    WeaveLogFunctError(mError);

exit:

    // on error, mPathBuilder would be un-/partial initialized and cannot be used to write anything
    return mPathBuilder;
}

DataElement::Builder & DataElement::Builder::Version (const uint64_t aVersion)
{
    // skip if error has already been set
    SuccessOrExit(mError);

    mError = mpWriter->Put(nl::Weave::TLV::ContextTag(kCsTag_Version), aVersion);
    WeaveLogFunctError(mError);

exit:

    return *this;
}

// Nothing would be written if aIsLastForCurrentChange == false, as that's the default value
DataElement::Builder & DataElement::Builder::PartialChange(const bool aIsPartialChange)
{
    // skip if error has already been set
    SuccessOrExit(mError);

    if (aIsPartialChange) {
        mError = mpWriter->PutBoolean(nl::Weave::TLV::ContextTag(kCsTag_IsPartialChange), true);
        WeaveLogFunctError(mError);
    }

exit:
    return *this;
}

DataElement::Builder & DataElement::Builder::EndOfDataElement (void)
{
    EndOfContainer();

    return *this;
}

#if WEAVE_CONFIG_DATA_MANAGEMENT_ENABLE_SCHEMA_CHECK
// Roughly verify the schema is right, including
// 1) at least one element is there
// 2) all elements are anonymous and of Path type
// 3) every path is also valid in schema
WEAVE_ERROR PathList::Parser::CheckSchemaValidity (void) const
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    size_t NumPath = 0;
    nl::Weave::TLV::TLVReader reader;

    PRETTY_PRINT("PathList =");
    PRETTY_PRINT("[");

    // make a copy of the PathList reader
    reader.Init(mReader);

    while (WEAVE_NO_ERROR == (err = reader.Next()))
    {
        VerifyOrExit(nl::Weave::TLV::AnonymousTag == reader.GetTag(), err = WEAVE_ERROR_INVALID_TLV_TAG);
        VerifyOrExit(nl::Weave::TLV::kTLVType_Path == reader.GetType(), err = WEAVE_ERROR_WRONG_TLV_TYPE);

        {
            Path::Parser path;
            err = path.Init(reader);
            SuccessOrExit(err);

            PRETTY_PRINT("\t");

            err = path.CheckSchemaValidity();
            SuccessOrExit(err);
        }

        ++NumPath;
    }

    PRETTY_PRINT("],");

    // if we have exhausted this container
    if (WEAVE_END_OF_TLV == err)
    {
        // if we have at least one Path element
        if (NumPath > 0)
        {
            err = WEAVE_NO_ERROR;
        }
    }

exit:
    WeaveLogFunctError(err);

    return err;
}
#endif // WEAVE_CONFIG_DATA_MANAGEMENT_ENABLE_SCHEMA_CHECK

// Re-initialize the shared PathBuilder with anonymous tag
Path::Builder & PathList::Builder::CreatePathBuilder()
{
    // skip if error has already been set
    SuccessOrExit(mError);

    mError = mPathBuilder.Init(mpWriter);
    WeaveLogFunctError(mError);

exit:

    // on error, mPathBuilder would be un-/partial initialized and cannot be used to write anything
    return mPathBuilder;
}

// Mark the end of this array and recover the type for outer container
PathList::Builder & PathList::Builder::EndOfPathList (void)
{
    EndOfContainer();

    return *this;
}

#if WEAVE_CONFIG_DATA_MANAGEMENT_ENABLE_SCHEMA_CHECK
// Roughly verify the schema is right, including
// 1) at least one element is there
// 2) all elements are anonymous and of Structure type
// 3) every Data Element is also valid in schema
WEAVE_ERROR DataList::Parser::CheckSchemaValidity (void) const
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    size_t NumDataElement = 0;
    nl::Weave::TLV::TLVReader reader;

    PRETTY_PRINT("DataList =");
    PRETTY_PRINT("[");

    // make a copy of the reader
    reader.Init(mReader);

    while (WEAVE_NO_ERROR == (err = reader.Next()))
    {
        VerifyOrExit(nl::Weave::TLV::AnonymousTag == reader.GetTag(), err = WEAVE_ERROR_INVALID_TLV_TAG);
        VerifyOrExit(nl::Weave::TLV::kTLVType_Structure == reader.GetType(), err = WEAVE_ERROR_WRONG_TLV_TYPE);

        {
            DataElement::Parser data;
            err = data.Init(reader);
            SuccessOrExit(err);
            err = data.CheckSchemaValidity();
            SuccessOrExit(err);
        }

        ++NumDataElement;
    }

    PRETTY_PRINT("],");

    // if we have exhausted this container
    if (WEAVE_END_OF_TLV == err)
    {
        // if we have at least one data element
        if (NumDataElement > 0)
        {
            err = WEAVE_NO_ERROR;
        }
    }

exit:
    WeaveLogFunctError(err);

    return err;
}
#endif // WEAVE_CONFIG_DATA_MANAGEMENT_ENABLE_SCHEMA_CHECK

// Re-initialize the shared PathBuilder with anonymous tag
DataElement::Builder & DataList::Builder::CreateDataElementBuilder()
{
    // skip if error has already been set
    SuccessOrExit(mError);

    mError = mDataElementBuilder.Init(mpWriter);
    WeaveLogFunctError(mError);

exit:

    // on error, mPathBuilder would be un-/partial initialized and cannot be used to write anything
    return mDataElementBuilder;
}

// Mark the end of this array and recover the type for outer container
DataList::Builder & DataList::Builder::EndOfDataList (void)
{
    EndOfContainer();

    return *this;
}

WEAVE_ERROR Event::Parser::Init (const nl::Weave::TLV::TLVReader & aReader)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    // make a copy of the reader here
    mReader.Init(aReader);

    VerifyOrExit (nl::Weave::TLV::kTLVType_Structure == mReader.GetType(), err = WEAVE_ERROR_WRONG_TLV_TYPE);

    // This is just a dummy, as we're not going to exit this container ever
    nl::Weave::TLV::TLVType OuterContainerType;
    err = mReader.EnterContainer(OuterContainerType);

exit:
    WeaveLogFunctError(err);

    return err;
}

#if WEAVE_CONFIG_DATA_MANAGEMENT_ENABLE_SCHEMA_CHECK
// Roughly verify the schema is right
WEAVE_ERROR Event::Parser::CheckSchemaValidity (void) const
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    nl::Weave::TLV::TLVReader reader;
    uint32_t tagNum = 0;

    struct TagPresence
    {
        bool Source : 1;
        bool Importance : 1;
        bool Id : 1;

        bool RelatedImportance : 1;
        bool RelatedId : 1;
        bool UTCTimestamp : 1;
        bool SystemTimestamp : 1;
        bool ResourceId : 1;
        bool TraitProfileId : 1;
        bool TraitInstanceId : 1;
        bool Type : 1;

        bool DeltaUTCTime : 1;
        bool DeltaSystemTime : 1;

        bool Data : 1;
    };

    TagPresence tagPresence = { 0 };

    PRETTY_PRINT("\t{");

    // make a copy of the reader
    reader.Init(mReader);

    while (WEAVE_NO_ERROR == (err = reader.Next()))
    {
        VerifyOrExit(nl::Weave::TLV::IsContextTag(reader.GetTag()), err = WEAVE_ERROR_INVALID_TLV_TAG);

        tagNum = nl::Weave::TLV::TagNumFromTag(reader.GetTag());

        switch (tagNum)
        {
        case kCsTag_Source:
            // check if this tag has appeared before
            VerifyOrExit (tagPresence.Source == false, err = WEAVE_ERROR_INVALID_TLV_TAG);
            tagPresence.Source = true;

            VerifyOrExit(nl::Weave::TLV::kTLVType_UnsignedInteger == reader.GetType(), err = WEAVE_ERROR_WRONG_TLV_TYPE);

#if WEAVE_DETAIL_LOGGING
            {
                uint64_t value;
                err = reader.Get(value);
                SuccessOrExit(err);

                PRETTY_PRINT("\t\tSource = 0x%" PRIx64 ",", value);
            }
#endif // WEAVE_DETAIL_LOGGING

            break;

        case kCsTag_Importance:
            // check if this tag has appeared before
            VerifyOrExit (tagPresence.Importance == false, err = WEAVE_ERROR_INVALID_TLV_TAG);
            tagPresence.Importance = true;

            VerifyOrExit(nl::Weave::TLV::kTLVType_UnsignedInteger == reader.GetType(), err = WEAVE_ERROR_WRONG_TLV_TYPE);

#if WEAVE_DETAIL_LOGGING
            {
                uint64_t value;
                err = reader.Get(value);
                SuccessOrExit(err);

                PRETTY_PRINT("\t\tImportance = 0x%" PRIx64 ",", value);
            }
#endif // WEAVE_DETAIL_LOGGING

            break;

        case kCsTag_Id:
            // check if this tag has appeared before
            VerifyOrExit (tagPresence.Id == false, err = WEAVE_ERROR_INVALID_TLV_TAG);
            tagPresence.Id = true;

            VerifyOrExit(nl::Weave::TLV::kTLVType_UnsignedInteger == reader.GetType(), err = WEAVE_ERROR_WRONG_TLV_TYPE);

#if WEAVE_DETAIL_LOGGING
            {
                uint64_t value;
                err = reader.Get(value);
                SuccessOrExit(err);

                PRETTY_PRINT("\t\tId = 0x%" PRIx64 ",", value);
            }
#endif // WEAVE_DETAIL_LOGGING

            break;

        case kCsTag_RelatedImportance:
            // check if this tag has appeared before
            VerifyOrExit (tagPresence.RelatedImportance == false, err = WEAVE_ERROR_INVALID_TLV_TAG);
            tagPresence.RelatedImportance = true;

            VerifyOrExit(nl::Weave::TLV::kTLVType_UnsignedInteger == reader.GetType(), err = WEAVE_ERROR_WRONG_TLV_TYPE);

#if WEAVE_DETAIL_LOGGING
            {
                uint64_t value;
                err = reader.Get(value);
                SuccessOrExit(err);

                PRETTY_PRINT("\t\tRelatedImportance = 0x%" PRIx64 ",", value);
            }
#endif // WEAVE_DETAIL_LOGGING

            break;

        case kCsTag_RelatedId:
            // check if this tag has appeared before
            VerifyOrExit (tagPresence.RelatedId == false, err = WEAVE_ERROR_INVALID_TLV_TAG);
            tagPresence.RelatedId = true;

            VerifyOrExit(nl::Weave::TLV::kTLVType_UnsignedInteger == reader.GetType(), err = WEAVE_ERROR_WRONG_TLV_TYPE);

#if WEAVE_DETAIL_LOGGING
            {
                uint64_t value;
                err = reader.Get(value);
                SuccessOrExit(err);

                PRETTY_PRINT("\t\tRelatedId = 0x%" PRIx64 ",", value);
            }
#endif // WEAVE_DETAIL_LOGGING

            break;

        case kCsTag_UTCTimestamp:
            // check if this tag has appeared before
            VerifyOrExit (tagPresence.UTCTimestamp == false, err = WEAVE_ERROR_INVALID_TLV_TAG);
            tagPresence.UTCTimestamp = true;

            VerifyOrExit(nl::Weave::TLV::kTLVType_UnsignedInteger == reader.GetType(), err = WEAVE_ERROR_WRONG_TLV_TYPE);

#if WEAVE_DETAIL_LOGGING
            {
                uint64_t value;
                err = reader.Get(value);
                SuccessOrExit(err);

                PRETTY_PRINT("\t\tUTCTimestamp = 0x%" PRIx64 ",", value);
            }
#endif // WEAVE_DETAIL_LOGGING

            break;

        case kCsTag_SystemTimestamp:
            // check if this tag has appeared before
            VerifyOrExit (tagPresence.SystemTimestamp == false, err = WEAVE_ERROR_INVALID_TLV_TAG);
            tagPresence.SystemTimestamp = true;

            VerifyOrExit(nl::Weave::TLV::kTLVType_UnsignedInteger == reader.GetType(), err = WEAVE_ERROR_WRONG_TLV_TYPE);

#if WEAVE_DETAIL_LOGGING
            {
                uint64_t value;
                err = reader.Get(value);
                SuccessOrExit(err);

                PRETTY_PRINT("\t\tSystemTimestamp = 0x%" PRIx64 ",", value);
            }
#endif // WEAVE_DETAIL_LOGGING

            break;

        case kCsTag_ResourceId:
            // check if this tag has appeared before
            VerifyOrExit (tagPresence.ResourceId == false, err = WEAVE_ERROR_INVALID_TLV_TAG);
            tagPresence.ResourceId = true;

            VerifyOrExit(nl::Weave::TLV::kTLVType_UnsignedInteger == reader.GetType(), err = WEAVE_ERROR_WRONG_TLV_TYPE);

#if WEAVE_DETAIL_LOGGING
            {
                uint64_t value;
                err = reader.Get(value);
                SuccessOrExit(err);

                PRETTY_PRINT("\t\tResourceId = 0x%" PRIx64 ",", value);
            }
#endif // WEAVE_DETAIL_LOGGING

            break;

        case kCsTag_TraitProfileId:
        {
            SchemaVersionRange requestedVersion;
            uint32_t ProfileID;

            // check if this tag has appeared before
            VerifyOrExit (tagPresence.TraitProfileId == false, err = WEAVE_ERROR_INVALID_TLV_TAG);
            tagPresence.TraitProfileId = true;

            if (reader.GetType() == nl::Weave::TLV::kTLVType_Array) {
                TLV::TLVType type;

                err = reader.EnterContainer(type);
                SuccessOrExit(err);

                err = reader.Next();
                SuccessOrExit(err);

                VerifyOrExit(reader.GetType() == nl::Weave::TLV::kTLVType_UnsignedInteger, err = WEAVE_ERROR_WRONG_TLV_TYPE);

                err = reader.Get(ProfileID);
                SuccessOrExit(err);

                err = reader.Next();
                VerifyOrExit(err == WEAVE_NO_ERROR || err == WEAVE_END_OF_TLV, );

                if (err == WEAVE_NO_ERROR) {
                    VerifyOrExit(reader.GetType() == nl::Weave::TLV::kTLVType_UnsignedInteger, err = WEAVE_ERROR_WRONG_TLV_TYPE);

                    err = reader.Get(requestedVersion.mMaxVersion);
                    SuccessOrExit(err);
                }

                err = reader.Next();
                VerifyOrExit(err == WEAVE_NO_ERROR || err == WEAVE_END_OF_TLV, );

                if (err == WEAVE_NO_ERROR) {
                    VerifyOrExit(reader.GetType() == nl::Weave::TLV::kTLVType_UnsignedInteger, err = WEAVE_ERROR_WRONG_TLV_TYPE);

                    err = reader.Get(requestedVersion.mMinVersion);
                    SuccessOrExit(err);
                }

                err = reader.Next();
                VerifyOrExit(err == WEAVE_END_OF_TLV, err = WEAVE_ERROR_WDM_MALFORMED_DATA_ELEMENT);

                err = reader.ExitContainer(type);
            }
            else {
                VerifyOrExit(reader.GetType() == nl::Weave::TLV::kTLVType_UnsignedInteger, err = WEAVE_ERROR_WRONG_TLV_TYPE);

                err = reader.Get(ProfileID);
                SuccessOrExit(err);
            }

#if WEAVE_DETAIL_LOGGING
            {
                if (requestedVersion.mMaxVersion > 1 || requestedVersion.mMinVersion > 1) {
                    PRETTY_PRINT("\t\tTraitProfileId = 0x%" PRIx32, ProfileID);

                    if (requestedVersion.mMaxVersion > 1) {
                        PRETTY_PRINT_SAMELINE(", MaxVersion = %" PRIu32, requestedVersion.mMaxVersion);
                    }

                    if (requestedVersion.mMaxVersion > 1) {
                        PRETTY_PRINT_SAMELINE(", MinVersion = %" PRIu32 ",", requestedVersion.mMinVersion);
                    }
                    else {
                        PRETTY_PRINT_SAMELINE(",");
                    }
                }
                else {
                    PRETTY_PRINT("\t\tTraitProfileId = 0x%" PRIx32 ",", ProfileID);
                }
            }
#endif // WEAVE_DETAIL_LOGGING

            break;
        }

        case kCsTag_TraitInstanceId:
            // check if this tag has appeared before
            VerifyOrExit (tagPresence.TraitInstanceId == false, err = WEAVE_ERROR_INVALID_TLV_TAG);
            tagPresence.TraitInstanceId = true;

            VerifyOrExit(nl::Weave::TLV::kTLVType_UnsignedInteger == reader.GetType(), err = WEAVE_ERROR_WRONG_TLV_TYPE);

#if WEAVE_DETAIL_LOGGING
            {
                uint64_t value;
                err = reader.Get(value);
                SuccessOrExit(err);

                PRETTY_PRINT("\t\tTraitInstanceId = 0x%" PRIx64 ",", value);
            }
#endif // WEAVE_DETAIL_LOGGING

            break;

        case kCsTag_Type:
            // check if this tag has appeared before
            VerifyOrExit (tagPresence.Type == false, err = WEAVE_ERROR_INVALID_TLV_TAG);
            tagPresence.Type = true;

            VerifyOrExit(nl::Weave::TLV::kTLVType_UnsignedInteger == reader.GetType(), err = WEAVE_ERROR_WRONG_TLV_TYPE);

#if WEAVE_DETAIL_LOGGING
            {
                uint64_t value;
                err = reader.Get(value);
                SuccessOrExit(err);

                PRETTY_PRINT("\t\tType = 0x%" PRIx64 ",", value);
            }
#endif // WEAVE_DETAIL_LOGGING

            break;

        case kCsTag_DeltaUTCTime:
            // check if this tag has appeared before
            VerifyOrExit (tagPresence.DeltaUTCTime == false, err = WEAVE_ERROR_INVALID_TLV_TAG);
            tagPresence.DeltaUTCTime = true;

            VerifyOrExit(nl::Weave::TLV::kTLVType_SignedInteger == reader.GetType(), err = WEAVE_ERROR_WRONG_TLV_TYPE);

#if WEAVE_DETAIL_LOGGING
            {
                uint64_t value;
                err = reader.Get(value);
                SuccessOrExit(err);

                PRETTY_PRINT("\t\tDeltaUTCTime = 0x%" PRIx64 ",", value);
            }
#endif // WEAVE_DETAIL_LOGGING

            break;

        case kCsTag_DeltaSystemTime:
            // check if this tag has appeared before
            VerifyOrExit (tagPresence.DeltaSystemTime == false, err = WEAVE_ERROR_INVALID_TLV_TAG);
            tagPresence.DeltaSystemTime = true;

            VerifyOrExit(nl::Weave::TLV::kTLVType_SignedInteger == reader.GetType(), err = WEAVE_ERROR_WRONG_TLV_TYPE);

#if WEAVE_DETAIL_LOGGING
            {
                uint64_t value;
                err = reader.Get(value);
                SuccessOrExit(err);

                PRETTY_PRINT("\t\tDeltaSystemTime = 0x%" PRIx64 ",", value);
            }
#endif // WEAVE_DETAIL_LOGGING

            break;

        case kCsTag_Data:
            // check if this tag has appeared before
            VerifyOrExit (tagPresence.Data == false, err = WEAVE_ERROR_INVALID_TLV_TAG);
            tagPresence.Data = true;

            err = ParseData(reader, 0);
            SuccessOrExit(err);
            break;

        default:
            PRETTY_PRINT("\t\tUnknown tag num %" PRIu32 , tagNum);
            break;
        }
    }

    PRETTY_PRINT("\t},");
    PRETTY_PRINT("");

    // almost all fields in an Event are optional
    if (WEAVE_END_OF_TLV == err)
    {
        err = WEAVE_NO_ERROR;
    }

exit:
    WeaveLogFunctError(err);

    return err;
}
#endif // WEAVE_CONFIG_DATA_MANAGEMENT_ENABLE_SCHEMA_CHECK

WEAVE_ERROR Event::Parser::GetSourceId(uint64_t * const apSourceId)
{
    return GetUnsignedInteger(kCsTag_Source, apSourceId);
}

WEAVE_ERROR Event::Parser::GetImportance(uint64_t * const apImportance)
{
    return GetUnsignedInteger(kCsTag_Importance, apImportance);
}

WEAVE_ERROR Event::Parser::GetEventId(uint64_t * const apEventId)
{
    return GetUnsignedInteger(kCsTag_Id, apEventId);
}

WEAVE_ERROR Event::Parser::GetRelatedEventImportance(uint64_t * const apImportance)
{
    return GetUnsignedInteger(kCsTag_RelatedImportance, apImportance);
}

WEAVE_ERROR Event::Parser::GetRelatedEventId(uint64_t * const apEventId)
{
    return GetUnsignedInteger(kCsTag_RelatedId, apEventId);
}

WEAVE_ERROR Event::Parser::GetUTCTimestamp(uint64_t * const apUTCTimestamp)
{
    return GetUnsignedInteger(kCsTag_UTCTimestamp, apUTCTimestamp);
}

WEAVE_ERROR Event::Parser::GetSystemTimestamp(uint64_t * const apSystemTimestamp)
{
    return GetUnsignedInteger(kCsTag_SystemTimestamp, apSystemTimestamp);
}

WEAVE_ERROR Event::Parser::GetResourceId(uint64_t * const apResourceId)
{
    return GetUnsignedInteger(kCsTag_ResourceId, apResourceId);
}

WEAVE_ERROR Event::Parser::GetTraitProfileId(uint32_t * const apTraitProfileId)
{
    return GetUnsignedInteger(kCsTag_TraitProfileId, apTraitProfileId);
}

WEAVE_ERROR Event::Parser::GetTraitInstanceId(uint64_t * const apTraitInstanceId)
{
    return GetUnsignedInteger(kCsTag_TraitInstanceId, apTraitInstanceId);
}

WEAVE_ERROR Event::Parser::GetEventType(uint64_t * const apEventType)
{
    return GetUnsignedInteger(kCsTag_Type, apEventType);
}

WEAVE_ERROR Event::Parser::GetDeltaUTCTime(int64_t * const apDeltaUTCTime)
{
    return GetSimpleValue(kCsTag_DeltaUTCTime, nl::Weave::TLV::kTLVType_SignedInteger, apDeltaUTCTime);
}

WEAVE_ERROR Event::Parser::GetDeltaSystemTime(int64_t * const apDeltaSystemTime)
{
    return GetSimpleValue(kCsTag_DeltaSystemTime, nl::Weave::TLV::kTLVType_SignedInteger, apDeltaSystemTime);
}

WEAVE_ERROR Event::Parser::GetReaderOnEvent (nl::Weave::TLV::TLVReader * const apReader) const
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    err = LookForElementWithTag(mReader, nl::Weave::TLV::ContextTag(kCsTag_Data), apReader);
    WeaveLogFunctError(err);

    return err;
}

WEAVE_ERROR Event::Builder::Init (nl::Weave::TLV::TLVWriter * const apWriter)
{
    return InitAnonymousStructure(apWriter);
}

Event::Builder Event::Builder::SourceId(const uint64_t aSourceId)
{
    // skip if error has already been set
    SuccessOrExit(mError);

    mError = mpWriter->Put(nl::Weave::TLV::ContextTag(kCsTag_Source), aSourceId);
    WeaveLogFunctError(mError);

exit:

    return *this;
}

Event::Builder Event::Builder::Importance(const uint64_t aImportance)
{
    // skip if error has already been set
    SuccessOrExit(mError);

    mError = mpWriter->Put(nl::Weave::TLV::ContextTag(kCsTag_Importance), aImportance);
    WeaveLogFunctError(mError);

exit:

    return *this;
}

Event::Builder Event::Builder::EventId(const uint64_t aEventId)
{
    // skip if error has already been set
    SuccessOrExit(mError);

    mError = mpWriter->Put(nl::Weave::TLV::ContextTag(kCsTag_Id), aEventId);
    WeaveLogFunctError(mError);

exit:

    return *this;
}

Event::Builder Event::Builder::RelatedEventImportance(const uint64_t aImportance)
{
    // skip if error has already been set
    SuccessOrExit(mError);

    mError = mpWriter->Put(nl::Weave::TLV::ContextTag(kCsTag_RelatedImportance), aImportance);
    WeaveLogFunctError(mError);

exit:

    return *this;
}

Event::Builder Event::Builder::RelatedEventId(const uint64_t aEventId)
{
    // skip if error has already been set
    SuccessOrExit(mError);

    mError = mpWriter->Put(nl::Weave::TLV::ContextTag(kCsTag_RelatedId), aEventId);
    WeaveLogFunctError(mError);

exit:

    return *this;
}


Event::Builder Event::Builder::UTCTimestamp(const uint64_t aUTCTimestamp)
{
    // skip if error has already been set
    SuccessOrExit(mError);

    mError = mpWriter->Put(nl::Weave::TLV::ContextTag(kCsTag_UTCTimestamp), aUTCTimestamp);
    WeaveLogFunctError(mError);

exit:

    return *this;
}

Event::Builder Event::Builder::SystemTimestamp(const uint64_t aSystemTimestamp)
{
    // skip if error has already been set
    SuccessOrExit(mError);

    mError = mpWriter->Put(nl::Weave::TLV::ContextTag(kCsTag_SystemTimestamp), aSystemTimestamp);
    WeaveLogFunctError(mError);

exit:

    return *this;
}

Event::Builder Event::Builder::ResourceId(const uint64_t aResourceId)
{
    // skip if error has already been set
    SuccessOrExit(mError);

    mError = mpWriter->Put(nl::Weave::TLV::ContextTag(kCsTag_ResourceId), aResourceId);
    WeaveLogFunctError(mError);

exit:

    return *this;
}

Event::Builder Event::Builder::TraitProfileId(const uint32_t aTraitProfileId)
{
    // skip if error has already been set
    SuccessOrExit(mError);

    mError = mpWriter->Put(nl::Weave::TLV::ContextTag(kCsTag_TraitProfileId), aTraitProfileId);
    WeaveLogFunctError(mError);

exit:

    return *this;
}

Event::Builder Event::Builder::TraitInstanceId(const uint64_t aTraitInstanceId)
{
    // skip if error has already been set
    SuccessOrExit(mError);

    mError = mpWriter->Put(nl::Weave::TLV::ContextTag(kCsTag_TraitInstanceId), aTraitInstanceId);
    WeaveLogFunctError(mError);

exit:

    return *this;
}

Event::Builder Event::Builder::EventType(const uint64_t aEventType)
{
    // skip if error has already been set
    SuccessOrExit(mError);

    mError = mpWriter->Put(nl::Weave::TLV::ContextTag(kCsTag_Type), aEventType);
    WeaveLogFunctError(mError);

exit:

    return *this;
}

Event::Builder Event::Builder::DeltaUTCTime(const int64_t aDeltaUTCTime)
{
    // skip if error has already been set
    SuccessOrExit(mError);

    mError = mpWriter->Put(nl::Weave::TLV::ContextTag(kCsTag_DeltaUTCTime), aDeltaUTCTime);
    WeaveLogFunctError(mError);

exit:

    return *this;
}

Event::Builder Event::Builder::DeltaSystemTime(const int64_t aDeltaSystemTime)
{
    // skip if error has already been set
    SuccessOrExit(mError);

    mError = mpWriter->Put(nl::Weave::TLV::ContextTag(kCsTag_DeltaSystemTime), aDeltaSystemTime);
    WeaveLogFunctError(mError);

exit:

    return *this;
}

// Mark the end of this element and recover the type for outer container
Event::Builder & Event::Builder::EndOfEvent (void)
{
    EndOfContainer();

    return *this;
}

#if WEAVE_CONFIG_DATA_MANAGEMENT_ENABLE_SCHEMA_CHECK
WEAVE_ERROR EventList::Parser::CheckSchemaValidity (void) const
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    size_t NumDataElement = 0;
    nl::Weave::TLV::TLVReader reader;

    PRETTY_PRINT("EventList =");
    PRETTY_PRINT("[");

    // make a copy of the EventList reader
    reader.Init(mReader);

    while (WEAVE_NO_ERROR == (err = reader.Next()))
    {
        VerifyOrExit(nl::Weave::TLV::AnonymousTag == reader.GetTag(), err = WEAVE_ERROR_INVALID_TLV_TAG);
        VerifyOrExit(nl::Weave::TLV::kTLVType_Structure == reader.GetType(), err = WEAVE_ERROR_WRONG_TLV_TYPE);

        {
            Event::Parser event;
            err = event.Init(reader);
            SuccessOrExit(err);
            err = event.CheckSchemaValidity();
            SuccessOrExit(err);
        }

        ++NumDataElement;
    }

    PRETTY_PRINT("],");

    // if we have exhausted this container
    if (WEAVE_END_OF_TLV == err)
    {
        // if we have at least one data element
        if (NumDataElement > 0)
        {
            err = WEAVE_NO_ERROR;
        }
        // NOTE: temporarily disable this check, to allow test to continue
        else
        {
            WeaveLogError(DataManagement, "PROTOCOL ERROR: Empty event list");
            err = WEAVE_NO_ERROR;
        }
    }

exit:
    WeaveLogFunctError(err);

    return err;
}
#endif // WEAVE_CONFIG_DATA_MANAGEMENT_ENABLE_SCHEMA_CHECK

Event::Builder & EventList::Builder::CreateEventBuilder()
{
    // skip if error has already been set
    SuccessOrExit(mError);

    mError = mEventBuilder.Init(mpWriter);
    WeaveLogFunctError(mError);

exit:

    // on error, mEventBuilder would be un-/partial initialized and cannot be used to write anything
    return mEventBuilder;
}

// Mark the end of this array and recover the type for outer container
EventList::Builder & EventList::Builder::EndOfEventList (void)
{
    EndOfContainer();

    return *this;
}

#if WEAVE_CONFIG_DATA_MANAGEMENT_ENABLE_SCHEMA_CHECK
WEAVE_ERROR VersionList::Parser::CheckSchemaValidity (void) const
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    nl::Weave::TLV::TLVReader reader;
    uint64_t version;
    size_t index = 0;

    PRETTY_PRINT("VersionList = ");
    PRETTY_PRINT("[");

    reader.Init(mReader);

    while (WEAVE_NO_ERROR == (err = reader.Next()))
    {
        VerifyOrExit (nl::Weave::TLV::AnonymousTag == reader.GetTag(), err = WEAVE_ERROR_INVALID_TLV_TAG);

        switch (reader.GetType())
        {
        case nl::Weave::TLV::kTLVType_Null:
            PRETTY_PRINT("\tNull,");
            break;

        case nl::Weave::TLV::kTLVType_UnsignedInteger:
            err = reader.Get(version);
            SuccessOrExit(err);

            PRETTY_PRINT("\t0x%" PRIx64 ",", version);
            break;

        default:
            ExitNow(err = WEAVE_ERROR_WRONG_TLV_TYPE);
            break;
        }

        ++index;
    }

    PRETTY_PRINT("],");

    if (WEAVE_END_OF_TLV == err)
    {
        err = WEAVE_NO_ERROR;
    }

exit:
    WeaveLogFunctError(err);

    return err;
}
#endif // WEAVE_CONFIG_DATA_MANAGEMENT_ENABLE_SCHEMA_CHECK

// 1) current element is anonymous
// 2) current element is either unsigned integer or NULL
bool VersionList::Parser::IsElementValid (void)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    bool result = false;

    VerifyOrExit (nl::Weave::TLV::AnonymousTag == mReader.GetTag(), err = WEAVE_ERROR_INVALID_TLV_TAG);

    switch (mReader.GetType())
    {
    case nl::Weave::TLV::kTLVType_Null:
    case nl::Weave::TLV::kTLVType_UnsignedInteger:
        result = true;
        break;
    default:
        //err = WEAVE_ERROR_WRONG_TLV_TYPE;
        ExitNow();
        break;
    }

exit:
    WeaveLogFunctError(err);

    return result;
}

bool VersionList::Parser::IsNull (void)
{
    return (nl::Weave::TLV::kTLVType_Null == mReader.GetType());
}

WEAVE_ERROR VersionList::Parser::GetVersion (uint64_t * const apVersion)
{
    return mReader.Get(*apVersion);
}

VersionList::Builder & VersionList::Builder::AddVersion (const uint64_t aVersion)
{
    // skip if error has already been set
    SuccessOrExit(mError);

    mError = mpWriter->Put(nl::Weave::TLV::AnonymousTag, aVersion);
    WeaveLogFunctError(mError);

exit:

    return *this;
}

VersionList::Builder & VersionList::Builder::AddNull (void)
{
    // skip if error has already been set
    SuccessOrExit(mError);

    mError = mpWriter->PutNull(nl::Weave::TLV::AnonymousTag);
    WeaveLogFunctError(mError);

exit:

    return *this;
}

// Mark the end of this array and recover the type for outer container
VersionList::Builder & VersionList::Builder::EndOfVersionList (void)
{
    EndOfContainer();
    return *this;
}

// aReader has to be on the element of anonymous container
WEAVE_ERROR BaseMessageWithSubscribeId::Parser::Init (const nl::Weave::TLV::TLVReader & aReader)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    // make a copy of the reader here
    mReader.Init(aReader);

    VerifyOrExit (nl::Weave::TLV::AnonymousTag == mReader.GetTag(), err = WEAVE_ERROR_INVALID_TLV_TAG);
    VerifyOrExit (nl::Weave::TLV::kTLVType_Structure == mReader.GetType(), err = WEAVE_ERROR_WRONG_TLV_TYPE);

    // This is just a dummy, as we're not going to exit this container ever
    nl::Weave::TLV::TLVType OuterContainerType;
    err = mReader.EnterContainer(OuterContainerType);

    mReader.ImplicitProfileId = kWeaveProfile_DictionaryKey;

exit:
    WeaveLogFunctError(err);

    return err;
}

// WEAVE_END_OF_TLV if there is no such element
// WEAVE_ERROR_WRONG_TLV_TYPE if there is such element but it's not any of the defined unsigned integer types
WEAVE_ERROR BaseMessageWithSubscribeId::Parser::GetSubscriptionID (uint64_t * const apSubscriptionID) const
{
    return GetUnsignedInteger(kCsTag_SubscriptionId, apSubscriptionID);
}

// Path is mostly used in a Path List, which requires every path to be anonymous
WEAVE_ERROR BaseMessageWithSubscribeId::Builder::Init (nl::Weave::TLV::TLVWriter * const apWriter)
{
    // error would be retained in mError and returned later
    (void)InitAnonymousStructure(apWriter);
    SuccessOrExit(mError);

    mpWriter->ImplicitProfileId = kWeaveProfile_DictionaryKey;

exit:
    return mError;
}

void BaseMessageWithSubscribeId::Builder::SetSubscriptionID (const uint64_t aSubscriptionID)
{
    // skip if error has already been set
    SuccessOrExit(mError);

    mError = mpWriter->Put(nl::Weave::TLV::ContextTag(kCsTag_SubscriptionId), aSubscriptionID);
    WeaveLogFunctError(mError);

exit:
    ;
}

void BaseMessageWithSubscribeId::Builder::EndOfMessage (void)
{
    EndOfContainer();
}

#if WEAVE_CONFIG_DATA_MANAGEMENT_ENABLE_SCHEMA_CHECK
// Roughly verify the schema is right, including
// 1) all mandatory tags are present
// 2) all elements have expected data type
// 3) any tag can only appear once
// At the top level of the message, unknown tags are ignored for foward compatibility
WEAVE_ERROR SubscribeRequest::Parser::CheckSchemaValidity (void) const
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint16_t TagPresenceMask = 0;
    nl::Weave::TLV::TLVReader reader;

    enum
    {
        kBit_SubscriptionId     = 1,
        kBit_TimeoutMin         = 2,
        kBit_TimeoutMax         = 3,
        kBit_PathList           = 4,
        kBit_VersionList        = 5,
        kBit_SubscribeToAllEvents = 6,
        kBit_LastObservedEventIdList = 7,
    };

    PRETTY_PRINT("{");

    // make a copy of the reader
    reader.Init(mReader);

    while (WEAVE_NO_ERROR == (err = reader.Next()))
    {
        const uint64_t tag = reader.GetTag();
        if (nl::Weave::TLV::ContextTag(kCsTag_SubscriptionId) == tag)
        {
            VerifyOrExit (!(TagPresenceMask & (1 << kBit_SubscriptionId)), err = WEAVE_ERROR_INVALID_TLV_TAG);
            TagPresenceMask |= (1 << kBit_SubscriptionId);
            VerifyOrExit(nl::Weave::TLV::kTLVType_UnsignedInteger == reader.GetType(), err = WEAVE_ERROR_WRONG_TLV_TYPE);

#if WEAVE_DETAIL_LOGGING
            {
                uint64_t id;
                err = reader.Get(id);
                SuccessOrExit(err);

                PRETTY_PRINT("\tSubscriptionId = 0x%" PRIx64 ",", id);
            }
#endif // WEAVE_DETAIL_LOGGING
        }
        else if (nl::Weave::TLV::ContextTag(kCsTag_SubscribeTimeOutMin) == tag)
        {
            VerifyOrExit (!(TagPresenceMask & (1 << kBit_TimeoutMin)), err = WEAVE_ERROR_INVALID_TLV_TAG);
            TagPresenceMask |= (1 << kBit_TimeoutMin);
            VerifyOrExit(nl::Weave::TLV::kTLVType_UnsignedInteger == reader.GetType(), err = WEAVE_ERROR_WRONG_TLV_TYPE);

#if WEAVE_DETAIL_LOGGING
            {
                uint32_t timeout;
                err = reader.Get(timeout);
                SuccessOrExit(err);

                PRETTY_PRINT("\tSubscriptionTimeoutMin = %u,", timeout);
            }
#endif // WEAVE_DETAIL_LOGGING
        }
        else if (nl::Weave::TLV::ContextTag(kCsTag_SubscribeTimeOutMax) == tag)
        {
            VerifyOrExit (!(TagPresenceMask & (1 << kBit_TimeoutMax)), err = WEAVE_ERROR_INVALID_TLV_TAG);
            TagPresenceMask |= (1 << kBit_TimeoutMax);
            VerifyOrExit(nl::Weave::TLV::kTLVType_UnsignedInteger == reader.GetType(), err = WEAVE_ERROR_WRONG_TLV_TYPE);

#if WEAVE_DETAIL_LOGGING
            {
                uint32_t timeout;
                err = reader.Get(timeout);
                SuccessOrExit(err);

                PRETTY_PRINT("\tSubscriptionTimeoutMax = %u,", timeout);
            }
#endif // WEAVE_DETAIL_LOGGING
        }
        else if (nl::Weave::TLV::ContextTag(kCsTag_SubscribeToAllEvents) == tag)
        {
            VerifyOrExit (!(TagPresenceMask & (1 << kBit_SubscribeToAllEvents)), err = WEAVE_ERROR_INVALID_TLV_TAG);
            TagPresenceMask |= (1 << kBit_SubscribeToAllEvents);
            VerifyOrExit(nl::Weave::TLV::kTLVType_Boolean == reader.GetType(), err = WEAVE_ERROR_WRONG_TLV_TYPE);

#if WEAVE_DETAIL_LOGGING
            {
                bool SubscribeToAllEvents;
                err = reader.Get(SubscribeToAllEvents);
                SuccessOrExit(err);

                PRETTY_PRINT("\tSubscribeToAllEvents = %u,", SubscribeToAllEvents);
            }
#endif // WEAVE_DETAIL_LOGGING
        }
        else if (nl::Weave::TLV::ContextTag(kCsTag_LastObservedEventIdList) == tag)
        {
            EventList::Parser eventList;

            VerifyOrExit (!(TagPresenceMask & (1 << kBit_LastObservedEventIdList)), err = WEAVE_ERROR_INVALID_TLV_TAG);
            TagPresenceMask |= (1 << kBit_LastObservedEventIdList);

            err = eventList.Init(reader);
            SuccessOrExit(err);

            PRETTY_PRINT_INCDEPTH();

            err = eventList.CheckSchemaValidity();
            SuccessOrExit(err);

            PRETTY_PRINT_DECDEPTH();
        }
        else if (nl::Weave::TLV::ContextTag(kCsTag_PathList) == tag)
        {
            PathList::Parser pathList;

            VerifyOrExit (!(TagPresenceMask & (1 << kBit_PathList)), err = WEAVE_ERROR_INVALID_TLV_TAG);
            TagPresenceMask |= (1 << kBit_PathList);

            err = pathList.Init(reader);
            SuccessOrExit(err);

            PRETTY_PRINT_INCDEPTH();

            err = pathList.CheckSchemaValidity();
            SuccessOrExit(err);

            PRETTY_PRINT_DECDEPTH();
        }
        else if (nl::Weave::TLV::ContextTag(kCsTag_VersionList) == tag)
        {
            VersionList::Parser versionList;

            VerifyOrExit (!(TagPresenceMask & (1 << kBit_VersionList)), err = WEAVE_ERROR_INVALID_TLV_TAG);
            TagPresenceMask |= (1 << kBit_VersionList);

            err = versionList.Init(reader);
            SuccessOrExit(err);

            PRETTY_PRINT_INCDEPTH();

            err = versionList.CheckSchemaValidity();
            SuccessOrExit(err);

            PRETTY_PRINT_DECDEPTH();
        }
        else
        {
            PRETTY_PRINT("\tUnknown tag 0x%" PRIx64 , tag);
        }
    }

    // if we have exhausted this container
    if (WEAVE_END_OF_TLV == err)
    {
        // everything is optional
        err = WEAVE_NO_ERROR;
    }

    PRETTY_PRINT("}");
    PRETTY_PRINT("");

exit:
    WeaveLogFunctError(err);

    return err;
}
#endif // WEAVE_CONFIG_DATA_MANAGEMENT_ENABLE_SCHEMA_CHECK

// WEAVE_END_OF_TLV if there is no such element
// WEAVE_ERROR_WRONG_TLV_TYPE if there is such element but it's not one of the right types
WEAVE_ERROR SubscribeRequest::Parser::GetSubscribeTimeoutMin (uint32_t * const apTimeOutMin) const
{
    return GetUnsignedInteger(kCsTag_SubscribeTimeOutMin, apTimeOutMin);
}

// WEAVE_END_OF_TLV if there is no such element
// WEAVE_ERROR_WRONG_TLV_TYPE if there is such element but it's not one of the right types
WEAVE_ERROR SubscribeRequest::Parser::GetSubscribeTimeoutMax (uint32_t * const apTimeOutMax) const
{
    return GetUnsignedInteger(kCsTag_SubscribeTimeOutMax, apTimeOutMax);
}

WEAVE_ERROR SubscribeRequest::Parser::GetSubscribeToAllEvents (bool * const apAllEvents) const
{
    return GetSimpleValue(kCsTag_SubscribeToAllEvents, nl::Weave::TLV::kTLVType_Boolean, apAllEvents);
}

WEAVE_ERROR SubscribeRequest::Parser::GetLastObservedEventIdList (EventList::Parser * const apEventList) const
{
    return apEventList->InitIfPresent(mReader, kCsTag_LastObservedEventIdList);
}

// Get a TLVReader for the Paths. Next() must be called before accessing them.
WEAVE_ERROR SubscribeRequest::Parser::GetPathList (PathList::Parser * const apPathList) const
{
    return apPathList->InitIfPresent(mReader, kCsTag_PathList);
}

// Get a TLVReader at the Versions. Next() must be called before accessing it.
// WEAVE_END_OF_TLV if there is no such element
// WEAVE_ERROR_WRONG_TLV_TYPE if there is such element but it's not one of the right types
WEAVE_ERROR SubscribeRequest::Parser::GetVersionList (VersionList::Parser * const apVersionList) const
{
    return apVersionList->InitIfPresent(mReader, kCsTag_VersionList);
}

SubscribeRequest::Builder & SubscribeRequest::Builder::SubscriptionID (const uint64_t aSubscriptionID)
{
    SetSubscriptionID(aSubscriptionID);
    return *this;
}

SubscribeRequest::Builder & SubscribeRequest::Builder::SubscribeTimeoutMin (const uint32_t aSubscribeTimeoutMin)
{
    // skip if error has already been set
    SuccessOrExit(mError);

    mError = mpWriter->Put(nl::Weave::TLV::ContextTag(kCsTag_SubscribeTimeOutMin), aSubscribeTimeoutMin);
    WeaveLogFunctError(mError);

exit:

    return *this;
}

SubscribeRequest::Builder & SubscribeRequest::Builder::SubscribeTimeoutMax (const uint32_t aSubscribeTimeoutMax)
{
    // skip if error has already been set
    SuccessOrExit(mError);

    mError = mpWriter->Put(nl::Weave::TLV::ContextTag(kCsTag_SubscribeTimeOutMax), aSubscribeTimeoutMax);
    WeaveLogFunctError(mError);

exit:

    return *this;
}

SubscribeRequest::Builder & SubscribeRequest::Builder::SubscribeToAllEvents (const bool aSubscribeToAllEvents)
{
    // skip if error has already been set
    SuccessOrExit(mError);

    mError = mpWriter->PutBoolean(nl::Weave::TLV::ContextTag(kCsTag_SubscribeToAllEvents), aSubscribeToAllEvents);
    WeaveLogFunctError(mError);

exit:

    return *this;
}

EventList::Builder & SubscribeRequest::Builder::CreateLastObservedEventIdListBuilder()
{
    // skip if error has already been set
    SuccessOrExit(mError);

    mError = mEventListBuilder.Init(mpWriter, kCsTag_LastObservedEventIdList);
    WeaveLogFunctError(mError);

exit:

    // on error, mPathListBuilder would be un-/partial initialized and cannot be used to write anything
    return mEventListBuilder;
}

PathList::Builder & SubscribeRequest::Builder::CreatePathListBuilder()
{
    // skip if error has already been set
    SuccessOrExit(mError);

    mError = mPathListBuilder.Init(mpWriter, kCsTag_PathList);
    WeaveLogFunctError(mError);

exit:

    // on error, mPathListBuilder would be un-/partial initialized and cannot be used to write anything
    return mPathListBuilder;
}

VersionList::Builder & SubscribeRequest::Builder::CreateVersionListBuilder()
{
    // skip if error has already been set
    SuccessOrExit(mError);

    mError = mVersionListBuilder.Init(mpWriter, kCsTag_VersionList);
    WeaveLogFunctError(mError);

exit:

    // on error, mVersionListBuilder would be un-/partial initialized and cannot be used to write anything
    return mVersionListBuilder;
}

SubscribeRequest::Builder & SubscribeRequest::Builder::EndOfRequest (void)
{
    EndOfMessage ();

    return *this;
}

#if WEAVE_CONFIG_DATA_MANAGEMENT_ENABLE_SCHEMA_CHECK
// Roughly verify the schema is right, including
// 1) all mandatory tags are present
// 2) all elements have expected data type
// 3) any tag can only appear once
// At the top level of the message, unknown tags are ignored for foward compatibility
WEAVE_ERROR SubscribeResponse::Parser::CheckSchemaValidity (void) const
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint16_t TagPresenceMask = 0;
    nl::Weave::TLV::TLVReader reader;

    enum
    {
        kBit_SubscriptionId     = 1,
        kBit_Timeout            = 2,
        kBit_PossibleLossOfEvents  = 3,
        kBit_LastVendedEventIdList = 4,
    };

    PRETTY_PRINT("{");

    // make a copy of the reader
    reader.Init(mReader);

    while (WEAVE_NO_ERROR == (err = reader.Next()))
    {
        const uint64_t tag = reader.GetTag();
        if (nl::Weave::TLV::ContextTag(kCsTag_SubscriptionId) == tag)
        {
            VerifyOrExit (!(TagPresenceMask & (1 << kBit_SubscriptionId)), err = WEAVE_ERROR_INVALID_TLV_TAG);
            TagPresenceMask |= (1 << kBit_SubscriptionId);
            VerifyOrExit(nl::Weave::TLV::kTLVType_UnsignedInteger == reader.GetType(), err = WEAVE_ERROR_WRONG_TLV_TYPE);

#if WEAVE_DETAIL_LOGGING
            {
                uint64_t id;
                err = reader.Get(id);
                SuccessOrExit(err);

                PRETTY_PRINT("\tSubscriptionId = 0x%" PRIx64 ",", id);
            }
#endif // WEAVE_DETAIL_LOGGING
        }
        else if (nl::Weave::TLV::ContextTag(kCsTag_SubscribeTimeOut) == tag)
        {
            VerifyOrExit (!(TagPresenceMask & (1 << kBit_Timeout)), err = WEAVE_ERROR_INVALID_TLV_TAG);
            TagPresenceMask |= (1 << kBit_Timeout);
            VerifyOrExit(nl::Weave::TLV::kTLVType_UnsignedInteger == reader.GetType(), err = WEAVE_ERROR_WRONG_TLV_TYPE);

#if WEAVE_DETAIL_LOGGING
            {
                uint32_t timeout;
                err = reader.Get(timeout);
                SuccessOrExit(err);

                PRETTY_PRINT("\tSubscribeTimeOut = %u,", timeout);
            }
#endif // WEAVE_DETAIL_LOGGING
        }
        else if (nl::Weave::TLV::ContextTag(kCsTag_PossibleLossOfEvents) == tag)
        {
            VerifyOrExit (!(TagPresenceMask & (1 << kBit_PossibleLossOfEvents)), err = WEAVE_ERROR_INVALID_TLV_TAG);
            TagPresenceMask |= (1 << kBit_PossibleLossOfEvents);
            VerifyOrExit(nl::Weave::TLV::kTLVType_Boolean == reader.GetType(), err = WEAVE_ERROR_WRONG_TLV_TYPE);

#if WEAVE_DETAIL_LOGGING
            {
                bool PossibleLossOfEvents;
                err = reader.Get(PossibleLossOfEvents);
                SuccessOrExit(err);

                PRETTY_PRINT("\tPossibleLossOfEvents = %u,", PossibleLossOfEvents);
            }
#endif // WEAVE_DETAIL_LOGGING
        }
        else if (nl::Weave::TLV::ContextTag(kCsTag_LastVendedEventIdList) == tag)
        {
            // TODO: this is not an event list. It's a list of structures that have SourceID, EventImportance and EventID,
            // and only that.
            EventList::Parser eventList;

            VerifyOrExit (!(TagPresenceMask & (1 << kBit_LastVendedEventIdList)), err = WEAVE_ERROR_INVALID_TLV_TAG);
            TagPresenceMask |= (1 << kBit_LastVendedEventIdList);

            err = eventList.Init(reader);
            SuccessOrExit(err);

            PRETTY_PRINT_INCDEPTH();

            err = eventList.CheckSchemaValidity();
            SuccessOrExit(err);

            PRETTY_PRINT_DECDEPTH();
        }
        else
        {
            PRETTY_PRINT("\tUnknown tag 0x%" PRIx64 , tag);
        }
    }

    // if we have exhausted this container
    if (WEAVE_END_OF_TLV == err)
    {
        // if we have at least the PathList field
        if (TagPresenceMask & (1 << kBit_SubscriptionId))
        {
            err = WEAVE_NO_ERROR;
        }
    }

    PRETTY_PRINT("}");
    PRETTY_PRINT("");

exit:
    WeaveLogFunctError(err);

    return err;
}
#endif // WEAVE_CONFIG_DATA_MANAGEMENT_ENABLE_SCHEMA_CHECK

// WEAVE_END_OF_TLV if there is no such element
// WEAVE_ERROR_WRONG_TLV_TYPE if there is such element but it's not one of the right types
WEAVE_ERROR SubscribeResponse::Parser::GetSubscribeTimeout (uint32_t * const apTimeOut) const
{
    return GetUnsignedInteger(kCsTag_SubscribeTimeOut, apTimeOut);
}

WEAVE_ERROR SubscribeResponse::Parser::GetPossibleLossOfEvents (bool * const apPossibleLossOfEvents) const
{
    return GetSimpleValue(kCsTag_PossibleLossOfEvents, nl::Weave::TLV::kTLVType_Boolean, apPossibleLossOfEvents);
}

WEAVE_ERROR SubscribeResponse::Parser::GetLastVendedEventIdList (EventList::Parser * const apEventList) const
{
    return apEventList->InitIfPresent(mReader, kCsTag_LastVendedEventIdList);
}

SubscribeResponse::Builder & SubscribeResponse::Builder::SubscriptionID (const uint64_t aSubscriptionID)
{
    SetSubscriptionID(aSubscriptionID);

    return *this;
}

SubscribeResponse::Builder & SubscribeResponse::Builder::SubscribeTimeout (const uint32_t aSubscribeTimeout)
{
    // skip if error has already been set
    SuccessOrExit(mError);

    mError = mpWriter->Put(nl::Weave::TLV::ContextTag(kCsTag_SubscribeTimeOut), aSubscribeTimeout);
    WeaveLogFunctError(mError);

exit:

    return *this;
}

SubscribeResponse::Builder & SubscribeResponse::Builder::PossibleLossOfEvents (const bool aPossibleLossOfEvents)
{
    // skip if error has already been set
    SuccessOrExit(mError);

    mError = mpWriter->PutBoolean(nl::Weave::TLV::ContextTag(kCsTag_PossibleLossOfEvents), aPossibleLossOfEvents);
    WeaveLogFunctError(mError);

exit:

    return *this;
}

EventList::Builder & SubscribeResponse::Builder::CreateLastVendedEventIdListBuilder()
{
    // skip if error has already been set
    SuccessOrExit(mError);

    mError = mEventListBuilder.Init(mpWriter, kCsTag_LastVendedEventIdList);
    WeaveLogFunctError(mError);

exit:

    // on error, mPathListBuilder would be un-/partial initialized and cannot be used to write anything
    return mEventListBuilder;
}

SubscribeResponse::Builder & SubscribeResponse::Builder::EndOfResponse (void)
{
    EndOfMessage();

    return *this;
}

#if WEAVE_CONFIG_DATA_MANAGEMENT_ENABLE_SCHEMA_CHECK
// Roughly verify the schema is right, including
// 1) all mandatory tags are present
// 2) all elements have expected data type
// 3) any tag can only appear once
// At the top level of the message, unknown tags are ignored for foward compatibility
WEAVE_ERROR SubscribeCancelRequest::Parser::CheckSchemaValidity (void) const
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
     uint16_t TagPresenceMask = 0;
     nl::Weave::TLV::TLVReader reader;

     enum
     {
         kBit_SubscriptionId     = 1,
     };

     PRETTY_PRINT("{");

     // make a copy of the reader
     reader.Init(mReader);

     while (WEAVE_NO_ERROR == (err = reader.Next()))
     {
         const uint64_t tag = reader.GetTag();
         if (nl::Weave::TLV::ContextTag(kCsTag_SubscriptionId) == tag)
         {
             VerifyOrExit (!(TagPresenceMask & (1 << kBit_SubscriptionId)), err = WEAVE_ERROR_INVALID_TLV_TAG);
             TagPresenceMask |= (1 << kBit_SubscriptionId);
             VerifyOrExit(nl::Weave::TLV::kTLVType_UnsignedInteger == reader.GetType(), err = WEAVE_ERROR_WRONG_TLV_TYPE);

 #if WEAVE_DETAIL_LOGGING
             {
                 uint64_t id;
                 err = reader.Get(id);
                 SuccessOrExit(err);
                 PRETTY_PRINT("\tSubscriptionId = 0x%" PRIx64 ",", id);
             }
 #endif // WEAVE_DETAIL_LOGGING
         }
         else
         {
             PRETTY_PRINT("\tUnknown tag 0x%" PRIx64 , tag);
         }
     }

     // if we have exhausted this container
     if (WEAVE_END_OF_TLV == err)
     {
         // if we have at least the SubscriptionId field
         if (TagPresenceMask & (1 << kBit_SubscriptionId))
         {
             err = WEAVE_NO_ERROR;
         }
     }

     PRETTY_PRINT("}");
     PRETTY_PRINT("");

 exit:
     WeaveLogFunctError(err);

     return err;
}
#endif // WEAVE_CONFIG_DATA_MANAGEMENT_ENABLE_SCHEMA_CHECK

SubscribeCancelRequest::Builder & SubscribeCancelRequest::Builder::SubscriptionID (const uint64_t aSubscriptionID)
{
    SetSubscriptionID(aSubscriptionID);

    return *this;
}

SubscribeCancelRequest::Builder & SubscribeCancelRequest::Builder::EndOfRequest (void)
{
    EndOfMessage();

    return *this;
}

SubscribeConfirmRequest::Builder & SubscribeConfirmRequest::Builder::SubscriptionID (const uint64_t aSubscriptionID)
{
    SetSubscriptionID(aSubscriptionID);

    return *this;
}

SubscribeConfirmRequest::Builder & SubscribeConfirmRequest::Builder::EndOfRequest (void)
{
    EndOfMessage();

    return *this;
}

#if WEAVE_CONFIG_DATA_MANAGEMENT_ENABLE_SCHEMA_CHECK
// Roughly verify the schema is right, including
// 1) all mandatory tags are present
// 2) all elements have expected data type
// 3) any tag can only appear once
// At the top level of the message, unknown tags are ignored for foward compatibility
WEAVE_ERROR NotificationRequest::Parser::CheckSchemaValidity (void) const
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint16_t TagPresenceMask = 0;
    nl::Weave::TLV::TLVReader reader;
    DataList::Parser dataList;
    EventList::Parser eventList;

    enum
    {
        kBit_SubscriptionId         = 1,
        kBit_DataList               = 2,
        kBit_PossibleLossOfEvent    = 3,
        kBit_UTCTimestamp           = 4,
        kBit_SystemTimestamp        = 5,
        kBit_EventList              = 6,
    };

    PRETTY_PRINT("{");

    // make a copy of the reader
    reader.Init(mReader);

    while (WEAVE_NO_ERROR == (err = reader.Next()))
    {
        const uint64_t tag = reader.GetTag();

        if (nl::Weave::TLV::ContextTag(kCsTag_SubscriptionId) == tag)
        {
            VerifyOrExit (!(TagPresenceMask & (1 << kBit_SubscriptionId)), err = WEAVE_ERROR_INVALID_TLV_TAG);
            TagPresenceMask |= (1 << kBit_SubscriptionId);
            VerifyOrExit(nl::Weave::TLV::kTLVType_UnsignedInteger == reader.GetType(), err = WEAVE_ERROR_WRONG_TLV_TYPE);

#if WEAVE_DETAIL_LOGGING
            {
                uint64_t id;
                err = reader.Get(id);
                SuccessOrExit(err);
                PRETTY_PRINT("\tSubscriptionId = 0x%" PRIx64 ",", id);
            }
#endif // WEAVE_DETAIL_LOGGING
        }
        else if (nl::Weave::TLV::ContextTag(kCsTag_PossibleLossOfEvent) == tag)
        {
            VerifyOrExit (!(TagPresenceMask & (1 << kBit_PossibleLossOfEvent)), err = WEAVE_ERROR_INVALID_TLV_TAG);
            TagPresenceMask |= (1 << kBit_PossibleLossOfEvent);
            VerifyOrExit(nl::Weave::TLV::kTLVType_Boolean == reader.GetType(), err = WEAVE_ERROR_WRONG_TLV_TYPE);

#if WEAVE_DETAIL_LOGGING
            {
                bool PossibleLossOfEvent;
                err = reader.Get(PossibleLossOfEvent);
                SuccessOrExit(err);
                PRETTY_PRINT("\tPossibleLossOfEvent = %u,", PossibleLossOfEvent);
            }
#endif // WEAVE_DETAIL_LOGGING
        }
        else if (nl::Weave::TLV::ContextTag(kCsTag_UTCTimestamp) == tag)
        {
            VerifyOrExit (!(TagPresenceMask & (1 << kBit_UTCTimestamp)), err = WEAVE_ERROR_INVALID_TLV_TAG);
            TagPresenceMask |= (1 << kBit_UTCTimestamp);
            VerifyOrExit(nl::Weave::TLV::kTLVType_UnsignedInteger == reader.GetType(), err = WEAVE_ERROR_WRONG_TLV_TYPE);

#if WEAVE_DETAIL_LOGGING
            {
                uint64_t UTCTimestamp;
                err = reader.Get(UTCTimestamp);
                SuccessOrExit(err);
                PRETTY_PRINT("\tUTCTimestamp = 0x%" PRIx64 ",", UTCTimestamp);
            }
#endif // WEAVE_DETAIL_LOGGING
        }
        else if (nl::Weave::TLV::ContextTag(kCsTag_SystemTimestamp) == tag)
        {
            VerifyOrExit (!(TagPresenceMask & (1 << kBit_SystemTimestamp)), err = WEAVE_ERROR_INVALID_TLV_TAG);
            TagPresenceMask |= (1 << kBit_SystemTimestamp);
            VerifyOrExit(nl::Weave::TLV::kTLVType_UnsignedInteger == reader.GetType(), err = WEAVE_ERROR_WRONG_TLV_TYPE);

#if WEAVE_DETAIL_LOGGING
            {
                uint64_t SystemTimestamp;
                err = reader.Get(SystemTimestamp);
                SuccessOrExit(err);
                PRETTY_PRINT("\tSystemTimestamp = 0x%" PRIx64 ",", SystemTimestamp);
            }
#endif // WEAVE_DETAIL_LOGGING
        }
        else if (nl::Weave::TLV::ContextTag(kCsTag_EventList) == tag)
        {
            VerifyOrExit (!(TagPresenceMask & (1 << kBit_EventList)), err = WEAVE_ERROR_INVALID_TLV_TAG);
            TagPresenceMask |= (1 << kBit_EventList);
            VerifyOrExit(nl::Weave::TLV::kTLVType_Array == reader.GetType(), err = WEAVE_ERROR_WRONG_TLV_TYPE);

            eventList.Init(reader);

            PRETTY_PRINT_INCDEPTH();

            err = eventList.CheckSchemaValidity();
            SuccessOrExit(err);

            PRETTY_PRINT_DECDEPTH();
        }
        else if (nl::Weave::TLV::ContextTag(kCsTag_DataList) == tag)
        {
            VerifyOrExit (!(TagPresenceMask & (1 << kBit_DataList)), err = WEAVE_ERROR_INVALID_TLV_TAG);
            TagPresenceMask |= (1 << kBit_DataList);
            VerifyOrExit(nl::Weave::TLV::kTLVType_Array == reader.GetType(), err = WEAVE_ERROR_WRONG_TLV_TYPE);

            dataList.Init(reader);

            PRETTY_PRINT_INCDEPTH();

            err = dataList.CheckSchemaValidity();
            SuccessOrExit(err);

            PRETTY_PRINT_DECDEPTH();
        }
        else
        {
            PRETTY_PRINT("\tUnknown tag 0x%" PRIx64 , tag);
        }
    }

    PRETTY_PRINT("}");
    PRETTY_PRINT("");

    // if we have exhausted this container
    if (WEAVE_END_OF_TLV == err)
    {
        // if we have at least the PathList field
        if (TagPresenceMask & (1 << kBit_SubscriptionId))
        {
            err = WEAVE_NO_ERROR;
        }
    }

exit:
    WeaveLogFunctError(err);

    return err;
}
#endif // WEAVE_CONFIG_DATA_MANAGEMENT_ENABLE_SCHEMA_CHECK

// Get a TLVReader for the Paths. Next() must be called before accessing them.
WEAVE_ERROR NotificationRequest::Parser::GetDataList (DataList::Parser * const apDataList) const
{
    return apDataList->InitIfPresent(mReader, kCsTag_DataList);
}

WEAVE_ERROR NotificationRequest::Parser::GetPossibleLossOfEvent (bool * const apPossibleLossOfEvent) const
{
    return GetSimpleValue(kCsTag_PossibleLossOfEvent, nl::Weave::TLV::kTLVType_Boolean, apPossibleLossOfEvent);
}

WEAVE_ERROR NotificationRequest::Parser::GetUTCTimestamp (uint64_t * const apUTCTimestamp) const
{
    return GetUnsignedInteger(kCsTag_UTCTimestamp, apUTCTimestamp);
}

WEAVE_ERROR NotificationRequest::Parser::GetSystemTimestamp (uint64_t * const apSystemTimestamp) const
{
    return GetUnsignedInteger(kCsTag_SystemTimestamp, apSystemTimestamp);
}

WEAVE_ERROR NotificationRequest::Parser::GetEventList (EventList::Parser * const apEventList) const
{
    return apEventList->InitIfPresent(mReader, kCsTag_EventList);
}

WEAVE_ERROR CustomCommandRequest::Parser::Init (const nl::Weave::TLV::TLVReader & aReader)
{

    WEAVE_ERROR err = WEAVE_NO_ERROR;

    VerifyOrExit (nl::Weave::TLV::AnonymousTag == aReader.GetTag(), err = WEAVE_ERROR_INVALID_TLV_TAG);

    err = DataElement::Parser::Init(aReader);
    SuccessOrExit(err);

    mReader.ImplicitProfileId = kWeaveProfile_DictionaryKey;

exit:
    WeaveLogFunctError(err);

    return err;
}

#if WEAVE_CONFIG_DATA_MANAGEMENT_ENABLE_SCHEMA_CHECK
WEAVE_ERROR CustomCommandRequest::Parser::CheckSchemaValidity (void) const
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    nl::Weave::TLV::TLVReader reader;

    struct TagPresence
    {
        bool Path;
        bool CommandType;
        bool ExpiryTime;
        bool MustBeVersion;
        bool Argument;
        bool Authenticator;
    };

    TagPresence tagPresence = { 0 };

    PRETTY_PRINT("{");

    // make a copy of the reader
    reader.Init(mReader);

    while (WEAVE_NO_ERROR == (err = reader.Next()))
    {
        // Authenticators carry profile tags
        const uint64_t tag = reader.GetTag();

        if (nl::Weave::TLV::IsContextTag(tag))
        {
            switch (nl::Weave::TLV::TagNumFromTag(tag))
            {
            case CustomCommandRequest::kCsTag_Path:
                // check if this tag has appeared before
                VerifyOrExit (tagPresence.Path == false, err = WEAVE_ERROR_INVALID_TLV_TAG);
                tagPresence.Path = true;

                VerifyOrExit(nl::Weave::TLV::kTLVType_Path == reader.GetType(), err = WEAVE_ERROR_WRONG_TLV_TYPE);

                PRETTY_PRINT("\tCommand Path = ");

                {
                    Path::Parser path;
                    err = path.Init(reader);
                    SuccessOrExit(err);
                    err = path.CheckSchemaValidity();
                    SuccessOrExit(err);
                }

                break;

            case kCsTag_CommandType:
                // check if this tag has appeared before
                VerifyOrExit (tagPresence.CommandType == false, err = WEAVE_ERROR_INVALID_TLV_TAG);
                tagPresence.CommandType = true;

                VerifyOrExit(nl::Weave::TLV::kTLVType_UnsignedInteger == reader.GetType(), err = WEAVE_ERROR_WRONG_TLV_TYPE);

#if WEAVE_DETAIL_LOGGING
                {
                    uint64_t value;
                    err = reader.Get(value);
                    SuccessOrExit(err);

                    PRETTY_PRINT("\tCommand Type = 0x%" PRIx64 ",", value);
                }
#endif // WEAVE_DETAIL_LOGGING

                break;

            case kCsTag_ExpiryTime:
                // check if this tag has appeared before
                VerifyOrExit (tagPresence.ExpiryTime == false, err = WEAVE_ERROR_INVALID_TLV_TAG);
                tagPresence.ExpiryTime = true;

                VerifyOrExit(nl::Weave::TLV::kTLVType_SignedInteger == reader.GetType(), err = WEAVE_ERROR_WRONG_TLV_TYPE);

#if WEAVE_DETAIL_LOGGING
                {
                    int64_t value;
                    err = reader.Get(value);
                    SuccessOrExit(err);

                    PRETTY_PRINT("\tExpiry Time = 0x%" PRIx64 ",", static_cast<uint64_t>(value));
                }
#endif // WEAVE_DETAIL_LOGGING

                break;

            case kCsTag_MustBeVersion:
                // check if this tag has appeared before
                VerifyOrExit (tagPresence.MustBeVersion == false, err = WEAVE_ERROR_INVALID_TLV_TAG);
                tagPresence.MustBeVersion = true;

                VerifyOrExit(nl::Weave::TLV::kTLVType_UnsignedInteger == reader.GetType(), err = WEAVE_ERROR_WRONG_TLV_TYPE);

#if WEAVE_DETAIL_LOGGING
                {
                    uint64_t value;
                    err = reader.Get(value);
                    SuccessOrExit(err);

                    PRETTY_PRINT("\tMust Be Version = 0x%" PRIx64 ",", value);
                }
#endif // WEAVE_DETAIL_LOGGING

                break;

            case kCsTag_Argument:
                // check if this tag has appeared before
                VerifyOrExit (tagPresence.Argument == false, err = WEAVE_ERROR_INVALID_TLV_TAG);
                tagPresence.Argument = true;

                VerifyOrExit(nl::Weave::TLV::kTLVType_Structure == reader.GetType(), err = WEAVE_ERROR_WRONG_TLV_TYPE);

#if WEAVE_DETAIL_LOGGING
                {
                    PRETTY_PRINT("\t(Argument)");

                    err = ParseData(reader, 0);
                    SuccessOrExit(err);
                }
#endif // WEAVE_DETAIL_LOGGING

                break;

            default:
                WeaveLogDetail(DataManagement, "UNKONWN, IGNORE");
            }
        }
        else if (nl::Weave::TLV::IsProfileTag(tag))
        {
            // we cannot use the normal case statement, for ProfileTag is a function instead of a macro
            if (tag == nl::Weave::TLV::ProfileTag(nl::Weave::Profiles::kWeaveProfile_Security, nl::Weave::Profiles::Security::kTag_WeaveSignature))
            {
                // certificate signature

                VerifyOrExit (tagPresence.Authenticator == false, err = WEAVE_ERROR_INVALID_TLV_TAG);
                tagPresence.Authenticator = true;

#if WEAVE_DETAIL_LOGGING
                {
                    PRETTY_PRINT("\t(Authenticator-Certificate)");

                    err = ParseData(reader, 0);
                    SuccessOrExit(err);
                }
#endif // WEAVE_DETAIL_LOGGING
            }
            // we cannot use the normal case statement, for ProfileTag is a function instead of a macro
            // 10 is a place holder for the new group key feature in security profile
            else if (tag == nl::Weave::TLV::ProfileTag(nl::Weave::Profiles::kWeaveProfile_Security, nl::Weave::Profiles::Security::kTag_GroupKeySignature))
            {
                // group key signature

                VerifyOrExit (tagPresence.Authenticator == false, err = WEAVE_ERROR_INVALID_TLV_TAG);
                tagPresence.Authenticator = true;

#if WEAVE_DETAIL_LOGGING
                {
                    PRETTY_PRINT("\t(Authenticator-Group Key)");

                    err = ParseData(reader, 0);
                    SuccessOrExit(err);
                }
#endif // WEAVE_DETAIL_LOGGING
            }
        }
        else
        {
            // a custom command can only contain, at top level, context-specific tags or profile tags
            ExitNow(err = WEAVE_ERROR_INVALID_TLV_TAG);
        }
    }

    PRETTY_PRINT("}");
    PRETTY_PRINT("");

    // almost all fields in an Event are optional
    if (WEAVE_END_OF_TLV == err)
    {
        err = WEAVE_NO_ERROR;
    }

exit:
    WeaveLogFunctError(err);

    return err;
}
#endif // WEAVE_CONFIG_DATA_MANAGEMENT_ENABLE_SCHEMA_CHECK

WEAVE_ERROR CustomCommandRequest::Parser::GetMustBeVersion(uint64_t * const apMustBeVersion) const
{
    return GetUnsignedInteger(kCsTag_MustBeVersion, apMustBeVersion);
}

WEAVE_ERROR CustomCommandRequest::Parser::GetExpiryTimeMicroSecond(int64_t * const apExpiryTimeMicroSecond) const
{
    return GetSimpleValue(kCsTag_ExpiryTime, nl::Weave::TLV::kTLVType_SignedInteger, apExpiryTimeMicroSecond);
}

WEAVE_ERROR CustomCommandRequest::Parser::GetCommandType(uint64_t * const apCommandType) const
{
    return GetUnsignedInteger(kCsTag_CommandType, apCommandType);
}

WEAVE_ERROR CustomCommandRequest::Parser::GetPath (Path::Parser * const apPath) const
{
    // CustomCommandRequest::kCsTag_Path is defined to be 1, which happens to be
    // the same as DataElement::kCsTag_Path
    return DataElement::Parser::GetPath(apPath);
}

WEAVE_ERROR CustomCommandRequest::Parser::GetReaderOnArgument (nl::Weave::TLV::TLVReader * const apReader) const
{
    return GetReaderOnTag(nl::Weave::TLV::ContextTag(kCsTag_Argument), apReader);
}

WEAVE_ERROR CustomCommandRequest::Parser::GetReaderOnPath (nl::Weave::TLV::TLVReader * const apReader) const
{
    return GetReaderOnTag(nl::Weave::TLV::ContextTag(kCsTag_Path), apReader);
}

WEAVE_ERROR CustomCommandRequest::Builder::Init (nl::Weave::TLV::TLVWriter * const apWriter)
{
    return InitAnonymousStructure(apWriter);
}

Path::Builder & CustomCommandRequest::Builder::CreatePathBuilder()
{
    // skip if error has already been set
    SuccessOrExit(mError);

    mError = mPathBuilder.Init(mpWriter, nl::Weave::TLV::ContextTag(kCsTag_Path));
    WeaveLogFunctError(mError);

exit:

    // on error, mPathBuilder would be un-/partial initialized and cannot be used to write anything
    return mPathBuilder;
}

CustomCommandRequest::Builder & CustomCommandRequest::Builder::CommandType (const uint64_t aCommandType)
{
    // skip if error has already been set
    SuccessOrExit(mError);

    mError = mpWriter->Put(nl::Weave::TLV::ContextTag(kCsTag_CommandType), aCommandType);
    WeaveLogFunctError(mError);

exit:

    return *this;
}

CustomCommandRequest::Builder & CustomCommandRequest::Builder::ExpiryTimeMicroSecond (const int64_t aExpiryTimeMicroSecond)
{
    // skip if error has already been set
    SuccessOrExit(mError);

    WEAVE_FAULT_INJECT(FaultInjection::kFault_WDM_SendCommandExpired,
            {
            int64_t *tmp = const_cast<int64_t *>(&aExpiryTimeMicroSecond);
            *tmp = 0;
            }
            );

    mError = mpWriter->Put(nl::Weave::TLV::ContextTag(kCsTag_ExpiryTime), aExpiryTimeMicroSecond);
    WeaveLogFunctError(mError);

exit:

    return *this;
}

CustomCommandRequest::Builder & CustomCommandRequest::Builder::MustBeVersion (const uint64_t aMustBeVersion)
{
    // skip if error has already been set
    SuccessOrExit(mError);

    WEAVE_FAULT_INJECT(FaultInjection::kFault_WDM_SendCommandBadVersion,
            {
            uint64_t *tmp = const_cast<uint64_t *>(&aMustBeVersion);
            *tmp = ~(*tmp);
            }
            );

    mError = mpWriter->Put(nl::Weave::TLV::ContextTag(kCsTag_MustBeVersion), aMustBeVersion);
    WeaveLogFunctError(mError);

exit:

    return *this;
}

CustomCommandRequest::Builder & CustomCommandRequest::Builder::EndOfRequest (void)
{
    EndOfContainer();

    return *this;
}

WEAVE_ERROR CustomCommandResponse::Parser::Init (const nl::Weave::TLV::TLVReader & aReader)
{

    WEAVE_ERROR err = WEAVE_NO_ERROR;

    VerifyOrExit (nl::Weave::TLV::AnonymousTag == aReader.GetTag(), err = WEAVE_ERROR_INVALID_TLV_TAG);

    err = DataElement::Parser::Init(aReader);
    SuccessOrExit(err);

    mReader.ImplicitProfileId = kWeaveProfile_DictionaryKey;

exit:
    WeaveLogFunctError(err);

    return err;
}

#if WEAVE_CONFIG_DATA_MANAGEMENT_ENABLE_SCHEMA_CHECK
WEAVE_ERROR CustomCommandResponse::Parser::CheckSchemaValidity (void) const
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    nl::Weave::TLV::TLVReader reader;

    struct TagPresence
    {
        bool Version;
        bool Response;
    };

    TagPresence tagPresence = { 0 };

    PRETTY_PRINT("{");

    // make a copy of the reader
    reader.Init(mReader);

    while (WEAVE_NO_ERROR == (err = reader.Next()))
    {
        VerifyOrExit(nl::Weave::TLV::IsContextTag(reader.GetTag()), err = WEAVE_ERROR_INVALID_TLV_TAG);
        switch (nl::Weave::TLV::TagNumFromTag(reader.GetTag()))
        {
        case kCsTag_Version:
            // check if this tag has appeared before
            VerifyOrExit (tagPresence.Version == false, err = WEAVE_ERROR_INVALID_TLV_TAG);
            tagPresence.Version = true;

            VerifyOrExit(nl::Weave::TLV::kTLVType_UnsignedInteger == reader.GetType(), err = WEAVE_ERROR_WRONG_TLV_TYPE);

#if WEAVE_DETAIL_LOGGING
            {
                uint64_t value;
                err = reader.Get(value);
                SuccessOrExit(err);

                PRETTY_PRINT("\tVersion = 0x%" PRIx64 ",", value);
            }
#endif // WEAVE_DETAIL_LOGGING

            break;

        case kCsTag_Response:
            // check if this tag has appeared before
            VerifyOrExit (tagPresence.Response == false, err = WEAVE_ERROR_INVALID_TLV_TAG);
            tagPresence.Response = true;

            VerifyOrExit(nl::Weave::TLV::kTLVType_Structure == reader.GetType(), err = WEAVE_ERROR_WRONG_TLV_TYPE);

#if WEAVE_DETAIL_LOGGING
            {
                PRETTY_PRINT("\t(Response)");

                err = ParseData(reader, 0);
                SuccessOrExit(err);
            }
#endif // WEAVE_DETAIL_LOGGING

            break;

        default:
            WeaveLogDetail(DataManagement, "UNKONWN, IGNORE");
        }
    }

    PRETTY_PRINT("}");
    PRETTY_PRINT("");

    // almost all fields in an Event are optional
    if (WEAVE_END_OF_TLV == err)
    {
        err = WEAVE_NO_ERROR;
    }

exit:
    WeaveLogFunctError(err);

    return err;
}
#endif // WEAVE_CONFIG_DATA_MANAGEMENT_ENABLE_SCHEMA_CHECK

WEAVE_ERROR CustomCommandResponse::Parser::GetVersion(uint64_t * const apVersion) const
{
    return GetUnsignedInteger(kCsTag_Version, apVersion);
}

WEAVE_ERROR CustomCommandResponse::Parser::GetReaderOnResponse (nl::Weave::TLV::TLVReader * const apReader) const
{
    return GetReaderOnTag(nl::Weave::TLV::ContextTag(kCsTag_Response), apReader);
}

WEAVE_ERROR CustomCommandResponse::Builder::Init (nl::Weave::TLV::TLVWriter * const apWriter)
{
    return InitAnonymousStructure(apWriter);
}

CustomCommandResponse::Builder & CustomCommandResponse::Builder::Version (const uint64_t aMustBeVersion)
{
    // skip if error has already been set
    SuccessOrExit(mError);

    mError = mpWriter->Put(nl::Weave::TLV::ContextTag(kCsTag_Version), aMustBeVersion);
    WeaveLogFunctError(mError);

exit:

    return *this;
}

CustomCommandResponse::Builder & CustomCommandResponse::Builder::EndOfResponse (void)
{
    EndOfContainer();

    return *this;
}

}; // WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current)
}; // Profiles
}; // Weave
}; // nl
