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
 *      This file defines Weave message parsers and encoders for Weave
 *      Data Management (WDM) profile.
 *
 */

#ifndef _WEAVE_DATA_MANAGEMENT_MESSAGE_DEF_CURRENT_H
#define _WEAVE_DATA_MANAGEMENT_MESSAGE_DEF_CURRENT_H

#include <Weave/Profiles/data-management/Current/WdmManagedNamespace.h>
#include <Weave/Core/WeaveTLV.h>
#include <Weave/Profiles/data-management/Current/ResourceIdentifier.h>

namespace nl {
namespace Weave {
namespace Profiles {
namespace DataManagement_Legacy {
enum
{
        /**
         *  This legacy status code means a subscription was successfully canceled.
         */
        kStatus_CancelSuccess                         = 0x0001,

        /**
         *  This legacy status code means a path from the path list of a view or
         *  update request frame did not match the node-resident schema
         *  of the responder.
         */
        kStatus_InvalidPath                           = 0x0013,

        /**
         *  This legacy status code means the topic identifier given in a cancel
         *  request or notification did not match any subscription extant
         *  on the receiving node.
         */
        kStatus_UnknownTopic                          = 0x0014,

        /**
         *  This legacy status code means the node making a request to read
         *  a particular data item does not have permission to do so.
         */
        kStatus_IllegalReadRequest                    = 0x0015,

        /**
         *  This legacy status code means the node making a request to
         *  write a particular data item does not have permission to do so.
         */
        kStatus_IllegalWriteRequest                   = 0x0016,

        /**
         *  This legacy status code means the version for data included in an
         *  update request did not match with the most recent version on the
         *  publisher and so the update could not be applied.
         */
        kStatus_InvalidVersion                        = 0x0017,

        /**
         *  This legacy status code means the requested mode of
         *  subscription is not supported by the receiving device.
         */
        kStatus_UnsupportedSubscriptionMode           = 0x0018,

};
}

namespace WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current) {

/**
 *  @brief
 *    The WDM profile message types.
 *
 *  These values are called out in the data management specification.
 *
 */
enum
{
    kMsgType_ViewRequest                   = 0x20,
    kMsgType_ViewResponse                  = 0x21,
    kMsgType_UpdateRequest                 = 0x22,
    kMsgType_InProgress                    = 0x23,
    kMsgType_SubscribeRequest              = 0x24,
    kMsgType_SubscribeResponse             = 0x25,
    kMsgType_SubscribeCancelRequest        = 0x26,
    kMsgType_SubscribeConfirmRequest       = 0x27,
    kMsgType_NotificationRequest           = 0x28,
    kMsgType_CustomCommandRequest          = 0x29,
    kMsgType_CustomCommandResponse         = 0x2A,
    kMsgType_OneWayCommand                 = 0x2B,
    kMsgType_PartialUpdateRequest          = 0x2C,
    kMsgType_UpdateContinue                = 0x2D,
    kMsgType_SubscriptionlessNotification  = 0x2E,
};

/**
 *  @brief
 *    WDM-specific status codes.
 *
 */
 enum {
    kStatus_InvalidValueInNotification    = 0x20,
    kStatus_InvalidPath                   = 0x21,
    kStatus_ExpiryTimeNotSupported        = 0x22,
    kStatus_NotTimeSyncedYet              = 0x23,
    kStatus_RequestExpiredInTime          = 0x24,
    kStatus_VersionMismatch               = 0x25,
    kStatus_GeneralProtocolError          = 0x26,
    kStatus_SecurityError                 = 0x27,
    kStatus_InvalidSubscriptionID         = 0x28,
    kStatus_GeneralSchemaViolation        = 0x29,
    kStatus_UnpairedDeviceRejected        = 0x2A,
    kStatus_MultipleFailures              = 0x2B,
    kStatus_UpdateOutOfSequence           = 0x2C,
    kStatus_IncompatibleDataSchemaVersion = 0x2D,
};

// TODO: The type is only used in a few places. We should use it everywhere.
// TODO: A typedef like this should come with the relative PRIDataVersion define
typedef uint64_t DataVersion;

/**
 * This is an optimized implementation of the algorithm to compare versions.
 * On the client side, a version received from the service is always
 * the latest one.
 */
static inline bool IsVersionNewer(const DataVersion &aVersion, const DataVersion &aReference)
{
    return (aVersion != aReference);
}

static inline bool IsVersionNewerOrEqual(const DataVersion &aVersion, const DataVersion &aReference)
{
    return true;
}

typedef uint16_t SchemaVersion;

struct ConstSchemaVersionRange
{
    SchemaVersion mMinVersion;
    SchemaVersion mMaxVersion;
};

struct SchemaVersionRange
{
    SchemaVersionRange()
    {
        mMinVersion = 1;
        mMaxVersion = 1;
    }
    SchemaVersionRange(SchemaVersion aMaxVersion, SchemaVersion aMinVersion)
    {
        mMaxVersion = aMaxVersion;
        mMinVersion = aMinVersion;
    }
    bool operator ==(const SchemaVersionRange & rhs) const
    {
        return ((rhs.mMinVersion == mMinVersion) && (rhs.mMaxVersion == mMaxVersion));
    }
    bool IsValid() const { return mMinVersion <= mMaxVersion; }

    SchemaVersion mMinVersion;
    SchemaVersion mMaxVersion;
};

/**
 *  @class ParserBase
 *
 *  @brief
 *    Base class for WDM message parsers
 */
class ParserBase
{
public:
    /**
     *  @brief Initialize a TLVReader to point to the beginning of any tagged element in this request
     *
     *  @param [in]  aTagToFind Tag to find in the request
     *  @param [out] apReader   A pointer to TLVReader, which will be initialized at the specified TLV element
     *                          on success
     *
     *  @retval #WEAVE_NO_ERROR on success
     */
    WEAVE_ERROR GetReaderOnTag(const uint64_t aTagToFind, nl::Weave::TLV::TLVReader * const apReader) const;

protected:
    nl::Weave::TLV::TLVReader mReader;

    ParserBase(void);

    template <typename T>
    WEAVE_ERROR GetUnsignedInteger(const uint8_t aContextTag, T * const apLValue) const;

    template <typename T>
    WEAVE_ERROR GetSimpleValue(const uint8_t aContextTag, const nl::Weave::TLV::TLVType aTLVType, T * const apLValue) const;
};

/**
 *  @class ListParserBase
 *
 *  @brief
 *    Base class for WDM message parsers, specialized in TLV array elements like Data Lists and Version Lists
 */
class ListParserBase : public ParserBase
{
protected:
    ListParserBase(void);

public:
    // aReader has to be on the element of the array element
    WEAVE_ERROR Init(const nl::Weave::TLV::TLVReader & aReader);

    // aReader has to be at the beginning of some container
    WEAVE_ERROR InitIfPresent(const nl::Weave::TLV::TLVReader & aReader, const uint8_t aContextTagToFind);

    WEAVE_ERROR Next(void);
    void GetReader(nl::Weave::TLV::TLVReader * const apReader);
};

/**
 *  @class BuilderBase
 *
 *  @brief
 *    Base class for WDM message encoders
 */
class BuilderBase
{
public:
    void ResetError(void);
    void ResetError(WEAVE_ERROR aErr);
    WEAVE_ERROR GetError(void) const { return mError; };
    nl::Weave::TLV::TLVWriter * GetWriter(void) { return mpWriter; };

protected:
    WEAVE_ERROR mError;
    nl::Weave::TLV::TLVWriter * mpWriter;
    nl::Weave::TLV::TLVType mOuterContainerType;

    BuilderBase(void);
    void EndOfContainer(void);

    WEAVE_ERROR InitAnonymousStructure(nl::Weave::TLV::TLVWriter * const apWriter);
};

/**
 *  @class ListBuilderBase
 *
 *  @brief
 *    Base class for WDM message encoders, specialized in TLV array elements like Data Lists and Version Lists
 */
class ListBuilderBase : public BuilderBase
{
protected:
    ListBuilderBase(void);

public:
    WEAVE_ERROR Init(nl::Weave::TLV::TLVWriter * const apWriter, const uint8_t aContextTagToUse);
    WEAVE_ERROR Init(nl::Weave::TLV::TLVWriter * const apWriter);
};

/**
 *  @brief
 *    WDM Path definition
 *
 */
namespace Path {
enum
{
    kCsTag_InstanceLocator = 1,

    kCsTag_ResourceID      = 1,
    kCsTag_TraitProfileID  = 2,
    kCsTag_TraitInstanceID = 3,
    kCsTag_RequestedVersion = 4,
};

class Parser;
class Builder;
}; // namespace Path

/**
 *  @class Parser
 *
 *  @brief
 *    Parses a WDM Path container
 */
class Path::Parser : public ParserBase
{
public:
    // aReader has to be on the element of Path container
    WEAVE_ERROR Init(const nl::Weave::TLV::TLVReader & aReader);

    // Roughly verify the schema is right, including
    // 1) all mandatory tags are present
    // 2) no unknown tags
    // 3) all elements have expected data type
    // 4) any tag can only appear once
    WEAVE_ERROR CheckSchemaValidity(void) const;

    // Resource ID could be of any type, so we can only position the reader so the caller has
    // full information of tag, element type, length, and value
    // WEAVE_END_OF_TLV if there is no such element
    WEAVE_ERROR GetResourceID(nl::Weave::TLV::TLVReader * const apReader) const;

    // Instance ID could be of any type, so we can only position the reader so the caller has
    // full information of tag, element type, length, and value
    WEAVE_ERROR GetInstanceID(nl::Weave::TLV::TLVReader * const apReader) const;

    // WEAVE_END_OF_TLV if there is no such element
    // WEAVE_ERROR_WRONG_TLV_TYPE if there is such element but it's not any of the defined unsigned integer types
    WEAVE_ERROR GetInstanceID(uint64_t * const apInstanceID) const;

    // Profile ID can only be uint32_t and not any other type
    // WEAVE_END_OF_TLV if there is no such element
    // WEAVE_ERROR_WRONG_TLV_TYPE if there is such element but it's not any of the defined unsigned integer types
    WEAVE_ERROR GetProfileID(uint32_t * const apProfileID, SchemaVersionRange * const apSchemaVersionRange);

    // Get a TLVReader at the additional tags section. Next() must be called before accessing it.
    WEAVE_ERROR GetTags(nl::Weave::TLV::TLVReader * const apReader) const;
};

class Path::Builder : public BuilderBase
{
public:
    WEAVE_ERROR Init(nl::Weave::TLV::TLVWriter * const apWriter);

    WEAVE_ERROR Init(nl::Weave::TLV::TLVWriter * const apWriter, const uint8_t aContextTagToUse);

    Path::Builder & ResourceID(const uint64_t aResourceID);
    Path::Builder & ResourceID(const ResourceIdentifier& aResourceID);
    Path::Builder & InstanceID(const uint64_t aInstanceID);
    Path::Builder & ProfileID(const uint32_t aProfileID);
    Path::Builder & ProfileID(const uint32_t aProfileID, const SchemaVersionRange & aSchemaVersionRange);

    Path::Builder & TagSection(void);
    Path::Builder & AdditionalTag(const uint64_t aTagInApiForm);

    Path::Builder & EndOfPath(void);

private:
    bool mInTagSection;

    WEAVE_ERROR _Init(nl::Weave::TLV::TLVWriter * const apWriter, const uint64_t aTagInApiForm);
};

/**
 *  @brief
 *    WDM Status Element definition
 *
 */
namespace StatusElement {
    enum
    {
        kCsTag_ProfileID         = 1,
        kCsTag_Status          = 2,
    };

    class Parser;
    class Builder;
}; // namespace DataElement

/**
 *  @brief
 *    WDM Status Element parser definition
 */
class StatusElement::Parser : public ParserBase
{
public:
    // aReader has to be on the element of DataElement
    WEAVE_ERROR Init(const nl::Weave::TLV::TLVReader & aReader);

    // Roughly verify the schema is right, including
    // 1) all mandatory tags are present
    // 2) all elements have expected data type
    // 3) any tag can only appear once
    // At the top level of the structure, unknown tags are ignored for foward compatibility
    WEAVE_ERROR CheckSchemaValidity(void) const;
    WEAVE_ERROR CheckSchemaValidityDeprecated(void) const;
    WEAVE_ERROR CheckSchemaValidityCurrent(void) const;

    // WEAVE_END_OF_TLV if there is no such element
    // WEAVE_ERROR_WRONG_TLV_TYPE if there is such element but it's not any of the defined unsigned integer types
    WEAVE_ERROR GetProfileIDAndStatusCode(uint32_t * apProfileID, uint16_t * aStatusCode) const;

private:
    bool mDeprecatedFormat;
};

/**
 *  @brief
 *    WDM Status Element encoder definition
 */
class StatusElement::Builder : public ListBuilderBase
{
public:
    WEAVE_ERROR Init(nl::Weave::TLV::TLVWriter * const apWriter);
    WEAVE_ERROR InitDeprecated(nl::Weave::TLV::TLVWriter * const apWriter);

    StatusElement::Builder & ProfileIDAndStatus(const uint32_t aProfileID, const uint16_t aStatusCode);

    StatusElement::Builder & EndOfStatusElement(void);

private:
    bool mDeprecatedFormat;
};


/**
 *  @brief
 *    WDM Data Element definition
 *
 */
namespace DataElement {
enum
{
    kCsTag_Path            = 1,
    kCsTag_Version         = 2,
    kCsTag_IsPartialChange = 3,

    /* 4-8 are reserved */
    kCsTag_DeletedDictionaryKeys = 9,
    kCsTag_Data                  = 10,
};

class Parser;
class Builder;
}; // namespace DataElement

/**
 *  @brief
 *    WDM Data Element parser definition
 */
class DataElement::Parser : public ParserBase
{
public:
    // aReader has to be on the element of DataElement
    WEAVE_ERROR Init(const nl::Weave::TLV::TLVReader & aReader);

    // Roughly verify the schema is right, including
    // 1) all mandatory tags are present
    // 2) all elements have expected data type
    // 3) any tag can only appear once
    // At the top level of the structure, unknown tags are ignored for foward compatibility
    WEAVE_ERROR CheckSchemaValidity(void) const;

    // WEAVE_END_OF_TLV if there is no such element
    // WEAVE_ERROR_WRONG_TLV_TYPE if there is such element but it's not a Path
    WEAVE_ERROR GetPath(Path::Parser * const apPath) const;

    // WEAVE_END_OF_TLV if there is no such element
    // WEAVE_ERROR_WRONG_TLV_TYPE if there is such element but it's not any of the defined unsigned integer types
    WEAVE_ERROR GetVersion(uint64_t * const apVersion) const;

    // on successful return, apDataPresentFlag will be TRUE if the
    // DataElement contains data, and GetData method may be called
    // without an error.  apDeletePresentFlag will be set to TRUE if this
    // DataElement contains a DictionaryDeletedKeyList.
    // WEAVE_NO_ERROR if the element is properly formatted,
    // WEAVE_ERROR_WDM_MALFORMED_DATA_ELEMENT if the element contains
    // neither the data merge element nor the deleted dictionary key
    // list
    WEAVE_ERROR CheckPresence(bool * const apDataPresentFlag, bool * const apDeletePresentFlag) const;

    // Data could be of any type, so we can only position the reader so the caller has
    // full information of tag, element type, length, and value
    WEAVE_ERROR GetData(nl::Weave::TLV::TLVReader * const apReader) const;

    // On success, the apReader is initialized to point to the first
    // element in the deleted keys list.
    // WEAVE_NO_ERROR on success.  WEAVE_ERROR_INVALID_TLV_TAG if the
    // DeletedDictionaryKeyList is not present.
    // WEAVE_ERROR_WRONG_TLV_TYPE if the element type is not a TLV
    // Array
    WEAVE_ERROR GetDeletedDictionaryKeys(nl::Weave::TLV::TLVReader * const apReader) const;

    // Default is false if there is no such element
    // WEAVE_ERROR_WRONG_TLV_TYPE if there is such element but it's not a boolean
    WEAVE_ERROR GetPartialChangeFlag(bool * const apPartialChangeFlag) const;

    WEAVE_ERROR GetReaderOnPath(nl::Weave::TLV::TLVReader * const apReader) const;

protected:
    // A recursively callable function to parse a data element and pretty-print it.
    WEAVE_ERROR ParseData(nl::Weave::TLV::TLVReader & aReader, int aDepth) const;
};

/**
 *  @brief
 *    WDM Data Element encoder definition
 */
class DataElement::Builder : public BuilderBase
{
public:
    // DataElement is only used in a Data List, which requires every path to be anonymous
    WEAVE_ERROR Init(nl::Weave::TLV::TLVWriter * const apWriter);

    Path::Builder & CreatePathBuilder(void);

    DataElement::Builder & Version(const uint64_t aVersion);

    // Nothing would be written if aIsPartialChange == false, as that's the default value
    DataElement::Builder & PartialChange(const bool aIsPartialChange);

    DataElement::Builder & EndOfDataElement(void);

private:
    Path::Builder mPathBuilder;
};

/**
 *  @brief
 *    WDM Path List definition
 */
namespace PathList {
class Parser;
class Builder;
}; // namespace PathList

class PathList::Parser : public ListParserBase
{
public:
    // Roughly verify the schema is right, including
    // 1) at least one element is there
    // 2) all elements are anonymous and of Path type
    // 3) every path is also valid in schema
    WEAVE_ERROR CheckSchemaValidity(void) const;
};

class PathList::Builder : public ListBuilderBase
{
public:
    // Re-initialize the shared PathBuilder with anonymous tag
    Path::Builder & CreatePathBuilder(void);

    // Mark the end of this array and recover the type for outer container
    PathList::Builder & EndOfPathList(void);

private:
    Path::Builder mPathBuilder;
};

namespace DataList {
class Parser;
class Builder;
}; // namespace DataList

class DataList::Parser : public ListParserBase
{
public:
    // Roughly verify the schema is right, including
    // 1) at least one element is there
    // 2) all elements are anonymous and of Structure type
    // 3) every Data Element is also valid in schema
    WEAVE_ERROR CheckSchemaValidity(void) const;
};

class DataList::Builder : public ListBuilderBase
{
public:
    // Re-initialize the shared PathBuilder with anonymous tag
    DataElement::Builder & CreateDataElementBuilder(void);

    // Mark the end of this array and recover the type for outer container
    DataList::Builder & EndOfDataList(void);

private:
    DataElement::Builder mDataElementBuilder;
};

namespace Event {
enum
{
    kCsTag_Source     = 1,
    kCsTag_Importance = 2,
    kCsTag_Id         = 3,

    /* 4-9 are reserved */

    kCsTag_RelatedImportance = 10,
    kCsTag_RelatedId         = 11,
    kCsTag_UTCTimestamp      = 12,
    kCsTag_SystemTimestamp   = 13,
    kCsTag_ResourceId        = 14,
    kCsTag_TraitProfileId    = 15,
    kCsTag_TraitInstanceId   = 16,
    kCsTag_Type              = 17,

    /* 18-29 are reserved */

    kCsTag_DeltaUTCTime    = 30,
    kCsTag_DeltaSystemTime = 31,

    /* 32-49 are reserved */

    kCsTag_Data = 50,
};

class Parser;
class Builder;
}; // namespace Event

class Event::Parser : protected DataElement::Parser
{
public:
    WEAVE_ERROR Init(const nl::Weave::TLV::TLVReader & aReader);

    // Roughly verify the schema is right
    WEAVE_ERROR CheckSchemaValidity(void) const;

    WEAVE_ERROR GetSourceId(uint64_t * const apSourceId);
    WEAVE_ERROR GetImportance(uint64_t * const apImportance);
    WEAVE_ERROR GetEventId(uint64_t * const apEventId);

    WEAVE_ERROR GetRelatedEventImportance(uint64_t * const apImportance);
    WEAVE_ERROR GetRelatedEventId(uint64_t * const apEventId);

    WEAVE_ERROR GetUTCTimestamp(uint64_t * const apUTCTimestamp);
    WEAVE_ERROR GetSystemTimestamp(uint64_t * const apSystemTimestamp);
    WEAVE_ERROR GetResourceId(uint64_t * const apResourceId);
    WEAVE_ERROR GetTraitProfileId(uint32_t * const apTraitProfileId);
    WEAVE_ERROR GetTraitInstanceId(uint64_t * const apTraitInstanceId);
    WEAVE_ERROR GetEventType(uint64_t * const apEventType);

    WEAVE_ERROR GetDeltaUTCTime(int64_t * const apDeltaUTCTime);
    WEAVE_ERROR GetDeltaSystemTime(int64_t * const apDeltaSystemTime);

    WEAVE_ERROR GetReaderOnEvent(nl::Weave::TLV::TLVReader * const apReader) const;
};

class Event::Builder : public BuilderBase
{
public:
    WEAVE_ERROR Init(nl::Weave::TLV::TLVWriter * const apWriter);

    Event::Builder SourceId(const uint64_t aSourceId);
    Event::Builder Importance(const uint64_t aImportance);
    Event::Builder EventId(const uint64_t aEventId);

    Event::Builder RelatedEventImportance(const uint64_t aImportance);
    Event::Builder RelatedEventId(const uint64_t aEventId);

    Event::Builder UTCTimestamp(const uint64_t aUTCTimestamp);
    Event::Builder SystemTimestamp(const uint64_t aSystemTimestamp);
    Event::Builder ResourceId(const uint64_t aResourceId);
    Event::Builder TraitProfileId(const uint32_t aTraitProfileId);
    Event::Builder TraitInstanceId(const uint64_t aTraitInstanceId);
    Event::Builder EventType(const uint64_t aEventType);

    Event::Builder DeltaUTCTime(const int64_t aDeltaUTCTime);
    Event::Builder DeltaSystemTime(const int64_t aDeltaSystemTime);

    // Mark the end of this array and recover the type for outer container
    Event::Builder & EndOfEvent(void);
};

namespace EventList {
class Parser;
class Builder;
}; // namespace EventList

class EventList::Parser : public ListParserBase
{
public:
    // Roughly verify the schema is right, including
    // 1) at least one element is there
    // 2) all elements are anonymous and of Structure type
    // 3) every Event is also valid in schema
    WEAVE_ERROR CheckSchemaValidity(void) const;
};

class EventList::Builder : public ListBuilderBase
{
public:
    // Re-initialize the shared EventBuilder with anonymous tag
    Event::Builder & CreateEventBuilder(void);

    // Mark the end of this array and recover the type for outer container
    EventList::Builder & EndOfEventList(void);

private:
    Event::Builder mEventBuilder;
};

namespace VersionList {
class Parser;
class Builder;
}; // namespace VersionList

class VersionList::Parser : public ListParserBase
{
public:
    // verify the schema is right, including
    // 1) all elements are anonymous
    // 2) all elements are either null or unsigned integer
    WEAVE_ERROR CheckSchemaValidity(void) const;

    // 1) current element is anonymous
    // 2) current element is either unsigned integer or NULL
    bool IsElementValid(void);

    bool IsNull(void);
    WEAVE_ERROR GetVersion(uint64_t * const apVersion);
};

class VersionList::Builder : public ListBuilderBase
{
public:
    VersionList::Builder & AddVersion(const uint64_t aVersion);
    VersionList::Builder & AddNull(void);

    // Mark the end of this array and recover the type for outer container
    VersionList::Builder & EndOfVersionList(void);
};

namespace StatusList {
    class Parser;
    class Builder;
}; // namespace StatusList

/**
 * StatusList builder.
 * Supports both the current and the deprecated StatusList format.
 */
class StatusList::Builder : public ListBuilderBase
{
public:

    /**
     * Write the list as an array of structures, instead of an array of arrays.
     */
    void UseDeprecatedFormat() { this->mDeprecatedFormat = true; }

    StatusList::Builder & AddStatus(uint32_t aProfileID, uint16_t aStatusCode);

    StatusList::Builder & EndOfStatusList(void);
private:
    bool mDeprecatedFormat;

};

class StatusList::Parser : public ListParserBase
{
public:
    WEAVE_ERROR CheckSchemaValidity(void) const;
    WEAVE_ERROR GetProfileIDAndStatusCode(uint32_t * const apProfileID, uint16_t * const apStatusCode);
};

namespace ViewRequest {
enum
{
    kCsTag_PathList = 1,
};
};

namespace ViewResponse {
enum
{
    kCsTag_DataList = 2,
};
};

namespace BaseMessageWithSubscribeId {
enum
{
    kCsTag_SubscriptionId = 1,
};

class Parser;
class Builder;
}; // namespace BaseMessageWithSubscribeId

class BaseMessageWithSubscribeId::Parser : public ParserBase
{
public:
    // aReader has to be on the element of anonymous container
    WEAVE_ERROR Init(const nl::Weave::TLV::TLVReader & aReader);

    // WEAVE_END_OF_TLV if there is no such element
    // WEAVE_ERROR_WRONG_TLV_TYPE if there is such element but it's not any of the defined unsigned integer types
    WEAVE_ERROR GetSubscriptionID(uint64_t * const apSubscriptionID) const;
};

class BaseMessageWithSubscribeId::Builder : public BuilderBase
{
public:
    enum
    {
        kBaseMessageSubscribeId_PayloadLen = 12, // StartContainer (1) + kCsTag_SubscriptionId TLV (10) + EndContainer (1)
    };

    // Path is mostly used in a Path List, which requires every path to be anonymous
    WEAVE_ERROR Init(nl::Weave::TLV::TLVWriter * const apWriter);

protected:
    void SetSubscriptionID(const uint64_t aSubscriptionID);
    void EndOfMessage(void);
};

namespace SubscribeRequest {
enum
{
    kCsTag_SubscriptionId          = 1,
    kCsTag_SubscribeTimeOutMin     = 2,
    kCsTag_SubscribeTimeOutMax     = 3,
    kCsTag_SubscribeToAllEvents    = 4,
    kCsTag_LastObservedEventIdList = 5,

    /* 5-19 are reserved */

    kCsTag_PathList    = 20,
    kCsTag_VersionList = 21,
};

class Parser;
class Builder;
}; // namespace SubscribeRequest

/**
 *  @brief
 *    WDM Path parser definition
 *
 */
// Note that in theory this class can be derived from SubscribeCancelRequest, but we are anticipating the tags to be changed
class SubscribeRequest::Parser : public BaseMessageWithSubscribeId::Parser
{
public:
    // aReader has to be on the element of anonymous container
    // WEAVE_ERROR Init(const nl::Weave::TLV::TLVReader & aReader);

    // Roughly verify the schema is right, including
    // 1) all mandatory tags are present
    // 2) no unknown tags
    // 3) all elements have expected data type
    // 4) any tag can only appear once
    WEAVE_ERROR CheckSchemaValidity(void) const;

    // WEAVE_END_OF_TLV if there is no such element
    // WEAVE_ERROR_WRONG_TLV_TYPE if there is such element but it's not any of the defined unsigned integer types
    // WEAVE_ERROR GetSubscriptionID(uint64_t * const apSubscriptionID) const;

    // WEAVE_END_OF_TLV if there is no such element
    // WEAVE_ERROR_WRONG_TLV_TYPE if there is such element but it's not one of the right types
    WEAVE_ERROR GetSubscribeTimeoutMin(uint32_t * const apTimeOutMin) const;

    // WEAVE_END_OF_TLV if there is no such element
    // WEAVE_ERROR_WRONG_TLV_TYPE if there is such element but it's not one of the right types
    WEAVE_ERROR GetSubscribeTimeoutMax(uint32_t * const apTimeOutMax) const;

    // WEAVE_END_OF_TLV if there is no such element
    // WEAVE_ERROR_WRONG_TLV_TYPE if there is such element but it's not one of the right types
    WEAVE_ERROR GetSubscribeToAllEvents(bool * const apAllEvents) const;

    // Get a TLVReader for the last observed events. Next() must be called before accessing them.
    WEAVE_ERROR GetLastObservedEventIdList(EventList::Parser * const apEventList) const;

    // Get a TLVReader for the Paths. Next() must be called before accessing them.
    WEAVE_ERROR GetPathList(PathList::Parser * const apPathList) const;

    // Get a TLVReader at the Versions. Next() must be called before accessing it.
    // WEAVE_END_OF_TLV if there is no such element
    // WEAVE_ERROR_WRONG_TLV_TYPE if there is such element but it's not one of the right types
    WEAVE_ERROR GetVersionList(VersionList::Parser * const apVersionList) const;
};

// Note that in theory this class can be derived from SubscribeCancelRequest, but we are anticipating the tags to be changed
class SubscribeRequest::Builder : public BaseMessageWithSubscribeId::Builder
{
public:
    SubscribeRequest::Builder & SubscriptionID(const uint64_t aSubscriptionID);
    SubscribeRequest::Builder & SubscribeTimeoutMin(const uint32_t aSubscribeTimeoutMin);
    SubscribeRequest::Builder & SubscribeTimeoutMax(const uint32_t aSubscribeTimeoutMax);
    SubscribeRequest::Builder & SubscribeToAllEvents(const bool aSubscribeToAllEvents);

    EventList::Builder & CreateLastObservedEventIdListBuilder(void);

    PathList::Builder & CreatePathListBuilder(void);

    VersionList::Builder & CreateVersionListBuilder(void);

    SubscribeRequest::Builder & EndOfRequest(void);

private:
    EventList::Builder mEventListBuilder;
    PathList::Builder mPathListBuilder;
    VersionList::Builder mVersionListBuilder;
};

namespace SubscribeResponse {
enum
{
    kCsTag_SubscriptionId   = 1,
    kCsTag_SubscribeTimeOut = 2,

    /* 3-9 are reserved */

    kCsTag_PossibleLossOfEvents  = 10,
    kCsTag_LastVendedEventIdList = 11,
};

class Parser;
class Builder;
}; // namespace SubscribeResponse

/**
 *  @brief
 *    WDM Path parser definition
 *
 */
// Note that in theory this class can be derived from SubscribeCancelRequest, but we are anticipating the tags to be changed
class SubscribeResponse::Parser : public BaseMessageWithSubscribeId::Parser
{
public:
    // Roughly verify the schema is right, including
    // 1) all mandatory tags are present
    // 2) no unknown tags
    // 3) all elements have expected data type
    // 4) any tag can only appear once
    WEAVE_ERROR CheckSchemaValidity(void) const;

    // WEAVE_END_OF_TLV if there is no such element
    // WEAVE_ERROR_WRONG_TLV_TYPE if there is such element but it's not any of the defined unsigned integer types
    // WEAVE_ERROR GetSubscriptionID(uint64_t * const apSubscriptionID) const;

    // WEAVE_END_OF_TLV if there is no such element
    // WEAVE_ERROR_WRONG_TLV_TYPE if there is such element but it's not one of the right types
    WEAVE_ERROR GetSubscribeTimeout(uint32_t * const apTimeOut) const;

    // WEAVE_END_OF_TLV if there is no such element
    // WEAVE_ERROR_WRONG_TLV_TYPE if there is such element but it's not one of the right types
    WEAVE_ERROR GetPossibleLossOfEvents(bool * const apPossibleLossOfEvents) const;

    // Get a TLVReader for the last observed events. Next() must be called before accessing them.
    WEAVE_ERROR GetLastVendedEventIdList(EventList::Parser * const apEventList) const;
};

// Note that in theory this class can be derived from SubscribeCancelRequest, but we are anticipating the tags to be changed
class SubscribeResponse::Builder : public BaseMessageWithSubscribeId::Builder
{
public:
    // Path is mostly used in a Path List, which requires every path to be anonymous
    // WEAVE_ERROR Init(nl::Weave::TLV::TLVWriter * const apWriter);

    SubscribeResponse::Builder & SubscriptionID(const uint64_t aSubscriptionID);
    SubscribeResponse::Builder & SubscribeTimeout(const uint32_t aSubscribeTimeout);

    SubscribeResponse::Builder & PossibleLossOfEvents(const bool aPossibleLossOfEvents);

    EventList::Builder & CreateLastVendedEventIdListBuilder(void);

    SubscribeResponse::Builder & EndOfResponse(void);

private:
    EventList::Builder mEventListBuilder;
};

namespace SubscribeCancelRequest {
enum
{
    kCsTag_SubscriptionId = 1,
};

class Parser;
class Builder;
}; // namespace SubscribeCancelRequest

class SubscribeCancelRequest::Parser : public BaseMessageWithSubscribeId::Parser
{
public:
    // aReader has to be on the element of anonymous container
    // WEAVE_ERROR Init(const nl::Weave::TLV::TLVReader & aReader);

    // Roughly verify the schema is right, including
    // 1) all mandatory tags are present
    // 2) all elements have expected data type
    // 3) any tag can only appear once
    WEAVE_ERROR CheckSchemaValidity(void) const;

    // WEAVE_END_OF_TLV if there is no such element
    // WEAVE_ERROR_WRONG_TLV_TYPE if there is such element but it's not any of the defined unsigned integer types
    // WEAVE_ERROR GetSubscriptionID(uint64_t * const apSubscriptionID) const;
};

class SubscribeCancelRequest::Builder : public BaseMessageWithSubscribeId::Builder
{
public:
    // Path is mostly used in a Path List, which requires every path to be anonymous
    // WEAVE_ERROR Init(nl::Weave::TLV::TLVWriter * const apWriter);

    SubscribeCancelRequest::Builder & SubscriptionID(const uint64_t aSubscriptionID);
    SubscribeCancelRequest::Builder & EndOfRequest(void);
};

namespace SubscribeConfirmRequest {
enum
{
    kCsTag_SubscriptionId = 1,
};

class Parser;
class Builder;
}; // namespace SubscribeConfirmRequest

class SubscribeConfirmRequest::Parser : public SubscribeCancelRequest::Parser
{
    // exactly the same as cancel, even the member function CheckSchemaValidity
};

class SubscribeConfirmRequest::Builder : public BaseMessageWithSubscribeId::Builder
{
public:
    SubscribeConfirmRequest::Builder & SubscriptionID(const uint64_t aSubscriptionID);
    SubscribeConfirmRequest::Builder & EndOfRequest(void);
};

namespace RejectionRecord {
enum
{
    kCsTag_Path    = 1,
    kCsTag_Version = 2,
    kCsTag_Reason  = 3,
};

class Parser;
class Builder;
}; // namespace RejectionRecord

class RejectionRecord::Parser : public ParserBase
{
public:
    // aReader has to be on the element of Event
    WEAVE_ERROR Init(const nl::Weave::TLV::TLVReader & aReader);

    // Roughly verify the schema is right
    WEAVE_ERROR CheckSchemaValidity(void) const;
};

class RejectionRecord::Builder : public BuilderBase
{
public:
    WEAVE_ERROR Init(nl::Weave::TLV::TLVWriter * const apWriter);

    // Mark the end of this array and recover the type for outer container
    DataList::Builder & EndOfRecord(void);
};

namespace RejectionRecordList {
enum
{
    kCsTag_RejectionRecordList = 1,
};

class Parser;
class Builder;
}; // namespace RejectionRecordList

class RejectionRecordList::Parser : public ListParserBase
{
public:
    // Roughly verify the schema is right, including
    // 1) at least one element is there
    // 2) all elements are anonymous and of Structure type
    // 3) every Event is also valid in schema
    WEAVE_ERROR CheckSchemaValidity(void) const;

private:
    RejectionRecord::Parser mRejectionRecordParser;
};

class RejectionRecordList::Builder : public ListBuilderBase
{
public:
    // Re-initialize the shared RejectionRecord::Builder with anonymous tag
    RejectionRecord::Builder & CreateRejectionRecord(void);

    // Mark the end of this array and recover the type for outer container
    EventList::Builder & EndOfRejectionRecordList(void);

private:
    RejectionRecord::Builder mRejectionRecordBuilder;
};

namespace NotificationRequest {
enum
{
    kCsTag_SubscriptionId = 1,

    /* 2-9 are reserved */

    kCsTag_DataList = 10,

    /* 11-19 are reserved */

    kCsTag_PossibleLossOfEvent = 20,
    kCsTag_UTCTimestamp        = 21,
    kCsTag_SystemTimestamp     = 22,
    kCsTag_EventList           = 23,
};

class Parser;
}; // namespace NotificationRequest

class NotificationRequest::Parser : public BaseMessageWithSubscribeId::Parser
{
public:
    // Roughly verify the schema is right, including
    // 1) all mandatory tags are present
    // 2) no unknown tags
    // 3) all elements have expected data type
    // 4) any tag can only appear once
    WEAVE_ERROR CheckSchemaValidity(void) const;

    // Get a TLVReader for the Paths. Next() must be called before accessing them.
    WEAVE_ERROR GetDataList(DataList::Parser * const apDataList) const;

    // WEAVE_END_OF_TLV if there is no such element
    // WEAVE_ERROR_WRONG_TLV_TYPE if there is such element but it's not one of the right types
    WEAVE_ERROR GetPossibleLossOfEvent(bool * const apPossibleLossOfEvent) const;

    // WEAVE_END_OF_TLV if there is no such element
    // WEAVE_ERROR_WRONG_TLV_TYPE if there is such element but it's not one of the right types
    WEAVE_ERROR GetUTCTimestamp(uint64_t * const apUTCTimestamp) const;

    // WEAVE_END_OF_TLV if there is no such element
    // WEAVE_ERROR_WRONG_TLV_TYPE if there is such element but it's not one of the right types
    WEAVE_ERROR GetSystemTimestamp(uint64_t * const apSystemTimestamp) const;

    // Get a TLVReader for the events. Next() must be called before accessing them.
    WEAVE_ERROR GetEventList(EventList::Parser * const apEventList) const;
};

/**
 *  @brief
 *    WDM Custom Command definition
 *
 */
namespace CustomCommand {

/// @brief Context-Specific tags used in this message
enum
{
    kCsTag_Path            = 1,
    kCsTag_CommandType     = 2,
    kCsTag_ExpiryTime      = 3,
    kCsTag_MustBeVersion   = 4,
    kCsTag_InitiationTime  = 5,
    kCsTag_ActionTime      = 6,

    /* 5-19 are reserved */
    kCsTag_Argument = 20,
};

class Parser;
class Builder;
}; // namespace CustomCommand

/**
 *  @brief
 *    WDM Custom Command Request parser definition
 */
class CustomCommand::Parser : protected DataElement::Parser
{
public:
    /**
     *  @brief Initialize the parser object with TLVReader
     *
     *  @param [in] aReader A pointer to a TLVReader, which should point to the beginning of this request
     *
     *  @retval #WEAVE_NO_ERROR on success
     */
    WEAVE_ERROR Init(const nl::Weave::TLV::TLVReader & aReader);

    /**
     *  @brief Roughly verify the message is correctly formed
     *
     *  @note The main use of this function is to print out what we're
     *    receiving during protocol development and debugging.
     *    The encoding rule has changed in WDM Next so this
     *    check is only "roughly" conformant now.
     *
     *  @retval #WEAVE_NO_ERROR on success
     */
    WEAVE_ERROR CheckSchemaValidity(void) const;

    /**
     *  @brief Initialize a Path::Parser with the path component in this command
     *
     *  @param [out] apPath A pointer to a Path::Parser, which will be
     *                      initialized with embedded path component on success
     *
     *  @retval #WEAVE_NO_ERROR on success
     *  @retval #WEAVE_END_OF_TLV if there is no such element
     *  @retval #WEAVE_ERROR_WRONG_TLV_TYPE if there is such element but it's not a Path
     */
    WEAVE_ERROR GetPath(Path::Parser * const apPath) const;

    /**
     *  @brief Get the command type id for this command
     *
     *  @param [out] apCommandType  A pointer to some variable to receive
     *                              the command type id on success
     *
     *  @retval #WEAVE_NO_ERROR on success
     *  @retval #WEAVE_END_OF_TLV if there is no such element
     *  @retval #WEAVE_ERROR_WRONG_TLV_TYPE if there is such element but it's not an unsigned integer
     */
    WEAVE_ERROR GetCommandType(uint64_t * const apCommandType) const;

    /**
     *  @brief Get the initiation time for this command
     *
     *  @param [out] apInitiationTimeMicroSecond      A pointer to some variable to receive
     *                                                the Command initiation time on success
     *
     *  @retval #WEAVE_NO_ERROR on success
     *  @retval #WEAVE_END_OF_TLV if there is no such element
     *  @retval #WEAVE_ERROR_WRONG_TLV_TYPE if there is such element but it's not a signed integer
     */
    WEAVE_ERROR GetInitiationTimeMicroSecond(int64_t * const apInitiationTimeMicroSecond) const;

    /**
     *  @brief Get the scheduled action time for this command
     *
     *  @param [out] apActionTimeMicroSecond    A pointer to some variable to receive
     *                                          the Command action time on success
     *
     *  @retval #WEAVE_NO_ERROR on success
     *  @retval #WEAVE_END_OF_TLV if there is no such element
     *  @retval #WEAVE_ERROR_WRONG_TLV_TYPE if there is such element but it's not a signed integer
     */
    WEAVE_ERROR GetActionTimeMicroSecond(int64_t * const apActionTimeMicroSecond) const;

    /**
     *  @brief Get the expiry time for this command
     *
     *  @param [out] apExpiryTimeMicroSecond    A pointer to some variable to receive
     *                                          the expiry time on success
     *
     *  @retval #WEAVE_NO_ERROR on success
     *  @retval #WEAVE_END_OF_TLV if there is no such element
     *  @retval #WEAVE_ERROR_WRONG_TLV_TYPE if there is such element but it's not a signed integer
     */
    WEAVE_ERROR GetExpiryTimeMicroSecond(int64_t * const apExpiryTimeMicroSecond) const;

    /**
     *  @brief Get the must-be version for this command
     *
     *  @param [out] apMustBeVersion    A pointer to some variable to receive
     *                                  the must-be version on success
     *
     *  @retval #WEAVE_NO_ERROR on success
     *  @retval #WEAVE_END_OF_TLV if there is no such element
     *  @retval #WEAVE_ERROR_WRONG_TLV_TYPE if there is such element but it's not an unsigned integer
     */
    WEAVE_ERROR GetMustBeVersion(uint64_t * const apMustBeVersion) const;

    /**
     *  @brief Initialize a TLVReader to point to the beginning of the argument component in this command
     *
     *  @param [out] apReader   A pointer to TLVReader, which will be initialized at the argument TLV element
     *                          on success
     *
     *  @retval #WEAVE_NO_ERROR on success
     */
    WEAVE_ERROR GetReaderOnArgument(nl::Weave::TLV::TLVReader * const apReader) const;

    /**
     *  @brief Initialize a TLVReader to point to the beginning of the path component in this command
     *
     *  @param [out] apReader   A pointer to TLVReader, which will be initialized at the argument TLV element
     *                          on success
     *
     *  @retval #WEAVE_NO_ERROR on success
     */
    WEAVE_ERROR GetReaderOnPath(nl::Weave::TLV::TLVReader * const apReader) const;
};

/**
 *  @brief
 *    WDM Custom Command encoder definition
 *
 *  The argument, and the authenticator elements are not directly supported, as they do not have a fixed schema.
 */
class CustomCommand::Builder : public BuilderBase
{
public:
    /**
     *  @brief Initialize a CustomCommand::Builder for writing into a TLV stream
     *
     *  @param [in] apWriter    A pointer to TLVWriter
     *
     *  @retval #WEAVE_NO_ERROR on success
     */
    WEAVE_ERROR Init(nl::Weave::TLV::TLVWriter * const apWriter);

    /**
     *  @brief Initialize a Path::Builder for writing into the TLV stream
     *
     *  @return A reference to Path::Builder
     */
    Path::Builder & CreatePathBuilder(void);

    /**
     *  @brief Inject command type id into the TLV stream
     *
     *  @param [in] aCommandType    Command type ID for this command
     *
     *  @return A reference to *this
     */
    CustomCommand::Builder & CommandType(const uint64_t aCommandType);

    /**
     *  @brief Inject init time into the TLV stream
     *
     *  @param [in] aInitiationTimeMicroSecond  Init time for this command, in microseconds since UNIX epoch
     *
     *  @return A reference to *this
     */
    CustomCommand::Builder & InitiationTimeMicroSecond(const int64_t aInitiationTimeMicroSecond);

    /**
     *  @brief Inject action time into the TLV stream
     *
     *  @param [in] aActionTimeMicroSecond  Action time for this command, in microseconds since UNIX epoch
     *
     *  @return A reference to *this
     */
    CustomCommand::Builder & ActionTimeMicroSecond(const int64_t aActionTimeMicroSecond);

    /**
     *  @brief Inject expiry time into the TLV stream
     *
     *  @param [in] aExpiryTimeMicroSecond  Expiry time for this command, in microseconds since UNIX epoch
     *
     *  @return A reference to *this
     */
    CustomCommand::Builder & ExpiryTimeMicroSecond(const int64_t aExpiryTimeMicroSecond);

    /**
     *  @brief Inject must-be version into the TLV stream
     *
     *  @param [in] aMustBeVersion  Trait instance in the path must be at this version for this command to be accepted
     *
     *  @return A reference to *this
     */
    CustomCommand::Builder & MustBeVersion(const uint64_t aMustBeVersion);

    /**
     *  @brief Mark the end of this command
     *
     *  @return A reference to *this
     */
    CustomCommand::Builder & EndOfCustomCommand(void);

private:
    Path::Builder mPathBuilder;
};

/**
 *  @brief
 *    WDM Custom Command Response definition
 *
 */
namespace CustomCommandResponse {
/// @brief Context-Specific tags used in this message
enum
{
    kCsTag_Version  = 1,
    kCsTag_Response = 2,
};

class Parser;
class Builder;
}; // namespace CustomCommandResponse

/**
 *  @brief
 *    WDM Custom Command Response parser definition
 */
class CustomCommandResponse::Parser : protected DataElement::Parser
{
public:
    /**
     *  @brief Initialize the parser object with TLVReader
     *
     *  @param [in] aReader A pointer to a TLVReader, which should point to the beginning of this response
     *
     *  @retval #WEAVE_NO_ERROR on success
     */
    WEAVE_ERROR Init(const nl::Weave::TLV::TLVReader & aReader);

    /**
     *  @brief Roughly verify the message is correctly formed
     *
     *  @note The main use of this function is to print out what we're
     *    receiving during protocol development and debugging.
     *    The encoding rule has changed in WDM Next so this
     *    check is only "roughly" conformant now.
     *
     *  @retval #WEAVE_NO_ERROR on success
     */
    WEAVE_ERROR CheckSchemaValidity(void) const;

    /**
     *  @brief Get the trait instance version in this response
     *
     *  @param [out] apVersion    A pointer to some variable to receive
     *                            the version on success
     *
     *  @retval #WEAVE_NO_ERROR on success
     *  @retval #WEAVE_END_OF_TLV if there is no such element
     *  @retval #WEAVE_ERROR_WRONG_TLV_TYPE if there is such element but it's not an unsigned integer
     */
    WEAVE_ERROR GetVersion(uint64_t * const apVersion) const;

    /**
     *  @brief Initialize a TLVReader to point to the beginning of the response component in this message
     *
     *  @param [out] apReader   A pointer to TLVReader, which will be initialized at the response TLV element
     *                          on success
     *
     *  @retval #WEAVE_NO_ERROR on success
     */
    WEAVE_ERROR GetReaderOnResponse(nl::Weave::TLV::TLVReader * const apReader) const;
};

/**
 *  @brief
 *    WDM Custom Command Response encoder definition
 *
 *  The response TLV element is not directly supported, as it does not have a fixed schema.
 */
class CustomCommandResponse::Builder : public BuilderBase
{
public:
    /**
     *  @brief Initialize a CustomCommandResponse::Builder for writing into a TLV stream
     *
     *  @param [in] apWriter    A pointer to TLVWriter
     *
     *  @retval #WEAVE_NO_ERROR on success
     */
    WEAVE_ERROR Init(nl::Weave::TLV::TLVWriter * const apWriter);

    /**
     *  @brief Inject trait instance version into the TLV stream
     *
     *  @param [in] aVersion  Trait instance version after the command execution
     *
     *  @return A reference to *this
     */
    CustomCommandResponse::Builder & Version(const uint64_t aVersion);

    /**
     *  @brief Mark the end of this message
     *
     *  @return A reference to *this
     */
    CustomCommandResponse::Builder & EndOfResponse(void);
};

/**
 *  @brief
 *    WDM Update Request Request definition
 *
 */
namespace UpdateRequest {
/// @brief Context-Specific tags used in this message
    enum
    {
        kCsTag_ExpiryTime                   = 1,
        /* 2-9 are reserved */
        kCsTag_Argument                     = 10,
        /* 11-19 are reserved */
        kCsTag_DataList                     = 20,
        kCsTag_UpdateRequestIndex           = 21,
    };

    class Parser;
}; // namespace UpdateRequest

/**
 *  @brief
 *    WDM Update Request parser definition
 */
class UpdateRequest::Parser : protected DataElement::Parser
{
public:
    /**
     *  @brief Initialize the parser object with TLVReader
     *
     *  @param [in] aReader A pointer to a TLVReader, which should point to the beginning of this request
     *
     *  @retval #WEAVE_NO_ERROR on success
     */
    WEAVE_ERROR Init(const nl::Weave::TLV::TLVReader & aReader);

    /**
     *  @brief Roughly verify the message is correctly formed
     *
     *  @note The main use of this function is to print out what we're
     *    receiving during protocol development and debugging.
     *    The encoding rule has changed in WDM Next so this
     *    check is only "roughly" conformant now.
     *
     *  @retval #WEAVE_NO_ERROR on success
     */
    WEAVE_ERROR CheckSchemaValidity(void) const;

    /**
     *  @brief Get the expiry time for this request
     *
     *  @param [out] apExpiryTimeMicroSecond    A pointer to some variable to receive
     *                                          the expiry time on success
     *
     *  @retval #WEAVE_NO_ERROR on success
     *  @retval #WEAVE_END_OF_TLV if there is no such element
     *  @retval #WEAVE_ERROR_WRONG_TLV_TYPE if there is such element but it's not a unsigned integer
     */
    WEAVE_ERROR GetExpiryTimeMicroSecond(int64_t * const apExpiryTimeMicroSecond) const;

    /**
     *  @brief Initialize a TLVReader to point to the beginning of the argument component in this request
     *
     *  @param [out] apReader   A pointer to TLVReader, which will be initialized at the argument TLV element
     *                          on success
     *
     *  @retval #WEAVE_NO_ERROR on success
     */
    WEAVE_ERROR GetReaderOnArgument(nl::Weave::TLV::TLVReader * const apReader) const;

    // Get a TLVReader for the Paths. Next() must be called before accessing them.
    WEAVE_ERROR GetDataList (DataList::Parser * const apDataList) const;

    /**
     *  @brief Get the UpdateRequestIndex of this request.
     *
     *  @param [out] apUpdateRequestIndex       A pointer to some variable to receive
     *                                          the index of the payload.
     *
     *  @retval #WEAVE_NO_ERROR on success
     *  @retval #WEAVE_END_OF_TLV if there is no such element
     *  @retval #WEAVE_ERROR_WRONG_TLV_TYPE if there is such element but it's not a unsigned integer
     */
    WEAVE_ERROR GetUpdateRequestIndex(uint32_t * const apUpdateRequestIndex) const;
};

namespace UpdateResponse {
    enum {
        kCsTag_VersionList = 1,
        kCsTag_StatusList = 2,
    };

    class Parser;
    class Builder;
};

/**
 *  @brief
 *    WDM Update Response encoder definition
 */
class UpdateResponse::Builder : public BuilderBase
{
public:
    WEAVE_ERROR Init(nl::Weave::TLV::TLVWriter * const apWriter);
    /**
     *  @brief Create the VersionList::Builder
     *
     *  @return A reference to a VersionList::Builder
     */
    VersionList::Builder & CreateVersionListBuilder(void);

    /**
     *  @brief Create the StatusList::Builder
     *
     *  @return A reference to a StatusList::Builder
     */
    StatusList::Builder & CreateStatusListBuilder(void);

    /**
     *  @brief Mark the end of this message
     *
     *  @return A reference to *this
     */
    UpdateResponse::Builder & EndOfResponse(void);

private:
    VersionList::Builder mVersionListBuilder;
    StatusList::Builder mStatusListBuilder;
};

class UpdateResponse::Parser : public ParserBase
{
public:
    // aReader has to be on the element of anonymous container
    WEAVE_ERROR Init(const nl::Weave::TLV::TLVReader & aReader);

    // Roughly verify the schema is right, including
    // 1) all mandatory tags are present
    // 2) no unknown tags
    // 3) all elements have expected data type
    // 4) any tag can only appear once
    WEAVE_ERROR CheckSchemaValidity(void) const;

    // Get a TLVReader for the Status. Next() must be called before accessing them.
    WEAVE_ERROR GetStatusList(StatusList::Parser * const apStatusList) const;

    // Get a TLVReader at the Versions. Next() must be called before accessing it.
    // WEAVE_END_OF_TLV if there is no such element
    // WEAVE_ERROR_WRONG_TLV_TYPE if there is such element but it's not one of the right types
    WEAVE_ERROR GetVersionList(VersionList::Parser * const apVersionList) const;
};

}; // namespace WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current)
}; // namespace Profiles
}; // namespace Weave
}; // namespace nl

#endif // _WEAVE_DATA_MANAGEMENT_MESSAGE_DEF_CURRENT_H
