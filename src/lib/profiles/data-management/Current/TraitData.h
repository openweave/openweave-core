/*
 *
 *    Copyright (c) 2018 Google LLC.
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
 *      This file forms the crux of the TDM layer (trait data management), providing
 *      various classes that manage and process data as it applies to traits and their
 *      associated schemas.
 *
 */

#ifndef _WEAVE_DATA_MANAGEMENT_TRAIT_DATA_CURRENT_H
#define _WEAVE_DATA_MANAGEMENT_TRAIT_DATA_CURRENT_H

#include <Weave/Profiles/data-management/Current/WdmManagedNamespace.h>

#include <Weave/Core/WeaveCore.h>
#include <Weave/Core/WeaveMessageLayer.h>
#include <Weave/Profiles/ProfileCommon.h>
#include <Weave/Support/CodeUtils.h>
#include <Weave/Profiles/data-management/MessageDef.h>

#if WEAVE_CONFIG_DATA_MANAGEMENT_CLIENT_EXPERIMENTAL
#include <string>
#endif // WEAVE_CONFIG_DATA_MANAGEMENT_CLIENT_EXPERIMENTAL

namespace nl {
namespace Weave {
namespace Profiles {
namespace WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current) {

class SubscriptionClient;
class UpdateEncoder;
struct TraitPath;
class TraitSchemaEngine;
class UpdateDirtyPathFilter;
/**
 * A PropertyPathHandle is a unique 32-bit numerical hash of a WDM path relative to the root of a trait instance. It has two parts
 * to it:
 *
 *  - A lower 16-bit number that maps to the static portion of the schema.
 *  - Where the lower 16-bits refer to a path within a dictionary element, an upper 16-bit number is present that represents the
 *    dictionary key associated with that element. If the lower 16-bits refer to a non dictionary element, then the upper 16-bits
 *    should be 0.
 *
 * Some characteristics:
 *  - Every trait has its own property path handle space.
 *  - Every unique WDM sub-path path will have a similarly unique PropertyPathHandle.
 *  - PropertyPathHandles are auto-generated (done by hand for now) by a trait compiler from IDL and is represented as an enumerant
 *    list in the corresponding trait's header file.
 *  - With this construct, application logic never has to deal with WDM paths directly. Rather, their interactions with WDM are
 *    conducted exclusively through these handles.
 *  - There are two reserved values for path handles that have specific meaning:
 *      - 0 indicates a 'NULL' handle
 *      - 1 indicates a handle that points to the root of the trait instance.
 *
 */
typedef uint32_t PropertyPathHandle;
typedef uint16_t PropertySchemaHandle;
typedef uint16_t PropertyDictionaryKey;
typedef uint16_t TraitDataHandle;

/* Reserved property path handles that have special meaning */
enum
{
    kNullPropertyPathHandle = 0,
    kRootPropertyPathHandle = 1
};

inline PropertyPathHandle CreatePropertyPathHandle(PropertySchemaHandle aPropertyPathSchemaId,
                                                   PropertyDictionaryKey aPropertyPathDictionaryKey = 0)
{
    return (((uint32_t) aPropertyPathDictionaryKey << 16) | aPropertyPathSchemaId);
}

inline PropertySchemaHandle GetPropertySchemaHandle(PropertyPathHandle aHandle)
{
    return (aHandle & 0xffff);
}

inline PropertyDictionaryKey GetPropertyDictionaryKey(PropertyPathHandle aHandle)
{
    return (aHandle >> 16);
}

inline bool IsRootPropertyPathHandle(PropertyPathHandle aHandle)
{
    return (aHandle == kRootPropertyPathHandle);
}

inline bool IsNullPropertyPathHandle(PropertyPathHandle aHandle)
{
    return (aHandle == kNullPropertyPathHandle);
}

class IPathFilter
{
public:
    virtual bool FilterPath(PropertyPathHandle aPathhandle) = 0;
};

/**
 *  @class UpdateDirtyPathFilter
 *
 *  @brief  Utility class to filter path when handling notification.
 */
class UpdateDirtyPathFilter : public IPathFilter
{
public:
    UpdateDirtyPathFilter(SubscriptionClient * apSubClient, TraitDataHandle traitDataHandle,
                          const TraitSchemaEngine * aSchemaEngine);
    virtual bool FilterPath(PropertyPathHandle aPathhandle);

private:
    SubscriptionClient * mpSubClient;
    TraitDataHandle mTraitDataHandle;
    const TraitSchemaEngine * mSchemaEngine;
};

class IDirtyPathCut
{
public:
    virtual WEAVE_ERROR CutPath(PropertyPathHandle aPathhandle, const TraitSchemaEngine * apEngine) = 0;
};

/**
 *  @class UpdateDictionaryDirtyPathCut
 *
 *  @brief  Utility class to put the dictionary back to pending queue when process the property path that has dictionary child.
 */
class UpdateDictionaryDirtyPathCut : public IDirtyPathCut
{
public:
    UpdateDictionaryDirtyPathCut(TraitDataHandle aTraitDataHandle, UpdateEncoder * pEncoder);
    virtual WEAVE_ERROR CutPath(PropertyPathHandle aPathhandle, const TraitSchemaEngine * apEngine);

private:
    UpdateEncoder * mpUpdateEncoder;
    TraitDataHandle mTraitDataHandle;
};

/**
 *  @class TraitSchemaEngine
 *
 *  @brief The schema engine takes schema information associated with a particular trait and provides facilities to parse and
 *         translate that into a form usable by the WDM machinery. This includes converting from PathHandles to WDM paths (and vice
 *         versa), methods to interpret/query the schema itself and methods to help read/write out data to/from TLV given a handle.
 *
 *         The schema itself is stored in tabular form, sufficiently described to allow for generic parsing/composition of WDM
 *         paths/data for any given trait. These tables are what will be the eventual output of 'code-gen' (The term itself being
 *         somewhat misleading given the absence of any generated code :P)
 */
class TraitSchemaEngine
{
public:
    /* Provides information about a particular path handle including its parent property schema handle,
     * its context tag and its name.
     */
    struct PropertyInfo
    {
        PropertySchemaHandle mParentHandle;
        uint8_t mContextTag;
    };

    /**
     *  @brief
     *    The main schema structure that houses the schema information.
     */
    struct Schema
    {
        uint32_t mProfileId;                   ///< The ID of the trait profile.
        const PropertyInfo * mSchemaHandleTbl; ///< A pointer to the schema handle table, which provides parent info and context
                                               ///< tags for each schema handle.
        uint32_t mNumSchemaHandleEntries;      ///< The number of schema handles in this trait.
        uint32_t mTreeDepth;                   ///< The max depth of this schema.
#if (TDM_EXTENSION_SUPPORT) || (TDM_VERSIONING_SUPPORT)
        PropertyPathHandle mMaxParentPathHandle; ///< Max handle supported by the parent schema
#endif
        uint8_t * mIsDictionaryBitfield;  ///< A bitfield indicating whether each schema handle is a dictionary or not.
        uint8_t * mIsOptionalBitfield;    ///< A bitfield indicating whether each schema handle is optional or not.
        uint8_t * mIsImplementedBitfield; ///< A bitfield indicating whether each optional schema handle is implemented or not.
        uint8_t * mIsNullableBitfield;    ///< A bitfield indicating whether each schema handle is nullable or not.
        uint8_t * mIsEphemeralBitfield;   ///< A bitfield indicating whether each schema handle is ephemeral or not.
#if (TDM_EXTENSION_SUPPORT)
        Schema * mParentSchema; ///< A pointer to the parent schema
#endif
#if (TDM_VERSIONING_SUPPORT)
        const ConstSchemaVersionRange * mVersionRange; ///< Range of versions supported by this trait
#endif
    };

    /* While traits can have deep nested structures (which can include dictionaries), application logic is only expected to provide
     * getters/setters for 'leaf' nodes in the schema. If one can visualize a schema as a tree (a directed graph where you can have
     * at most one parent for any given node) where branches indicate the presence of a nested structure, then this analogy fits in
     * quite nicely.
     */
    class ISetDataDelegate
    {
    public:
        enum SetDataEventType
        {
            /* Start of replacement of an entire dictionary */
            kSetDataEvent_DictionaryReplaceBegin,

            /* End of replacement of an entire dictionary */
            kSetDataEvent_DictionaryReplaceEnd,

            /* Start of modification or addition of a dictionary item */
            kSetDataEvent_DictionaryItemModifyBegin,

            /* End of modification or addition of a dictionary item */
            kSetDataEvent_DictionaryItemModifyEnd,
        };

        /**
         * Given a path handle to a leaf node and a TLV reader, set the leaf data in the callee.
         *
         * @retval #WEAVE_NO_ERROR On success.
         * @retval other           Was unable to read out data from the reader.
         */
        virtual WEAVE_ERROR SetLeafData(PropertyPathHandle aLeafHandle, nl::Weave::TLV::TLVReader & aReader) = 0;

        /**
         * Given a path handle to a node, a TLV reader, and an indication
         * of whether a null type was received, set the data in the callee.
         * TDM will only call this function for handles that are nullable,
         * optional, ephemeral, or leafs. If aHandle is a non-leaf node
         * and is nullified, TDM will not call SetData for its children.
         *
         * @param[in] aHandle       The PropertyPathHandle in question.
         *
         * @param[inout] aReader    The TLV reader to read from.
         *
         * @param[out] aIsNull      Is aHandle nullified?
         *
         * @retval #WEAVE_NO_ERROR On success.
         * @retval other           Was unable to read out data from the reader.
         */
        virtual WEAVE_ERROR SetData(PropertyPathHandle aHandle, nl::Weave::TLV::TLVReader & aReader, bool aIsNull) = 0;

        /**
         * Signals to delegates when notable events occur while parsing dictionaries. In all cases, a property path handle is
         * provided that provides more context on what this event applies to.
         *
         * For dictionary replace begin/ends, these handles are purely schema handles.
         * For dictionary item added/modififed events, these handles are property path handles as they contain the dictionary key as
         * well.
         */
        virtual void OnSetDataEvent(SetDataEventType aType, PropertyPathHandle aHandle) = 0;
    };

    class IGetDataDelegate
    {
    public:
        /**
         * Given a path handle to a leaf node and a TLV writer, get the data from the callee.
         *
         * @retval #WEAVE_NO_ERROR On success.
         * @retval other           Was unable to retrieve data and write it into the writer.
         */
        virtual WEAVE_ERROR GetLeafData(PropertyPathHandle aLeafHandle, uint64_t aTagToWrite,
                                        nl::Weave::TLV::TLVWriter & aWriter) = 0;

        /**
         * Given a path handle to a node, a TLV writer, and booleans indicating whether the
         * value is null or not present, get the data from the trait source that will build a
         * notify. If the path handle is not a leaf node, TDM will handle writing values to
         * the writer (like opening containers, nullifying the struct, etc). If a non-leaf
         * node is null or not present, TDM will not call GetData for its children.
         *
         * This function will only be called for handles that are nullable, optional,
         * ephemeral, or leafs. The expectation is that any traits with handles that
         * have those options enabled will implement appropriate logic to populate
         * aIsNull and aIsPresent.
         *
         * @param[in] aHandle       The PropertyPathHandle in question.
         *
         * @param[in] aTagToWrite   The tag to write for the aHandle.
         *
         * @param[inout] aWriter    The writer to write TLV elements to.
         *
         * @param[out] aIsNull      Is aHandle nullified? If yes, TDM will write
         *                          a null element. If aHandle is not a leaf,
         *                          TDM will skip over its children.
         *
         * @param[out] aIsPresent   Is aHandle present? If no and if aHandle
         *                          is not a leaf, TDM will skip over the
         *                          path and its children.
         *
         *
         * @retval #WEAVE_NO_ERROR On success.
         * @retval other           Was unable to retrieve data and write it into the writer.
         */
        virtual WEAVE_ERROR GetData(PropertyPathHandle aHandle, uint64_t aTagToWrite, nl::Weave::TLV::TLVWriter & aWriter,
                                    bool & aIsNull, bool & aIsPresent) = 0;

#if TDM_ENABLE_PUBLISHER_DICTIONARY_SUPPORT
        /**
         * Given a handle to a particular dictionary and some context (that is usable by the delegate to track state between
         * invocations), return the next dictionary item key.
         *
         * The context is initially set to 0 on the first call. There-after, the application is allowed to store any data
         * (up to width of a pointer) within that variable and it will be passed in un-modified on successive invocations.
         *
         * @retval #WEAVE_NO_ERROR On success.
         * @retval #WEAVE_END_OF_INPUT if there are no more keys to iterate over in the dictionary.
         *
         */
        virtual WEAVE_ERROR GetNextDictionaryItemKey(PropertyPathHandle aDictionaryHandle, uintptr_t & aContext,
                                                     PropertyDictionaryKey & aKey) = 0;
#endif
    };

    /**
     * Given a reader positioned at the root of a WDM path element, read out the relevant tags and provide
     * the equivalent path handle.
     *
     *
     * @retval #WEAVE_NO_ERROR                  On success.
     * @retval #WEAVE_ERROR_TLV_TAG_NOT_FOUND   If a matching handle could not be found due to a
     *                                          malformed/incorrectly specified path.
     */
    WEAVE_ERROR MapPathToHandle(nl::Weave::TLV::TLVReader & aPathReader, PropertyPathHandle & aHandle) const;

    /**
     * Given the a string representation of a WDM path read out the relevant tags and provide
     * the equivalent path handle.  The WDM path is represented as a string using the following rules:
     * - tags are separated with `/`
     * - the path MUST begin with a leading `/` and MUST NOT contain a trailing slash
     * - numerical tags in the WDM path MUST be encoded using the standard C library for integer to string encoding,
     * i.e. decimal encoding (default) MUST NOT contain a leading 0, a hexadecimal  encoding MUST begin with `0x`,
     * and octal encoding MUST contain a leading `0`.
     *
     * @retval #WEAVE_NO_ERROR                  On success.
     * @retval #WEAVE_ERROR_TLV_TAG_NOT_FOUND   If a matching handle could not be found.
     * @retval #WEAVE_ERROR_INVALID_ARGUMENT    If a path string is malformed
     */
    WEAVE_ERROR MapPathToHandle(const char * aPathString, PropertyPathHandle & aHandle) const;

    /**
     * Convert the path handle to a TLV path.
     *
     * @retval #WEAVE_NO_ERROR On success.
     * @retval other           Was unable to convert the handle to a TLV path
     */
    WEAVE_ERROR MapHandleToPath(PropertyPathHandle aHandle, nl::Weave::TLV::TLVWriter & aPathWriter) const;

#if WEAVE_CONFIG_DATA_MANAGEMENT_CLIENT_EXPERIMENTAL
    /**
     * Convert the path handle to a std string path
     *
     * @retval #WEAVE_NO_ERROR On success.
     * @retval other           Was unable to convert the handle to a std string path
     */
    WEAVE_ERROR MapHandleToPath(PropertyPathHandle aHandle, std::string & aPathString) const;
#endif //WEAVE_CONFIG_DATA_MANAGEMENT_CLIENT_EXPERIMENTAL

    /**
     * Given a path handle and a reader positioned on the corresponding data element, process the data buffer pointed to by the
     * reader and store it into the sink by invoking the SetLeafData call whenever a leaf data item is encountered.
     *
     * @retval #WEAVE_NO_ERROR On success.
     * @retval other           Encountered errors parsing/processing the data.
     */
    WEAVE_ERROR StoreData(PropertyPathHandle aHandle, nl::Weave::TLV::TLVReader & aReader, ISetDataDelegate * aDelegate,
                          IPathFilter * aPathFilter) const;

    /**
     * Given a path handle and a writer position on the corresponding data element, retrieve leaf data from the source and write it
     * into the buffer pointed to by the writer in a schema compliant manner.
     *
     * @retval #WEAVE_NO_ERROR On success.
     * @retval other           Encountered errors writing out the data.
     */
    WEAVE_ERROR RetrieveData(PropertyPathHandle aHandle, uint64_t aTagToWrite, nl::Weave::TLV::TLVWriter & aWriter,
                             IGetDataDelegate * aDelegate, IDirtyPathCut * apDirtyPathCut = NULL) const;

    WEAVE_ERROR RetrieveUpdatableDictionaryData(PropertyPathHandle aHandle, uint64_t aTagToWrite,
                                                nl::Weave::TLV::TLVWriter & aWriter, IGetDataDelegate * aDelegate,
                                                PropertyPathHandle & aPropertyPathHandleOfDictItemToStartFrom) const;
    /**********
     *
     * Schema Query Functions
     *
     *********/

    /* Offset from the value of a path that provides the index into the table for that handle
     * It is 2 since the 0 and 1 are special handles.
     */
    static const uint32_t kHandleTableOffset = 2;

    /* Returns the property path handle of the child of a parent given the parent handle and the child's context tag.
     * If the parent happens to be in a dictionary, the key is preserved in the child.
     */
    PropertyPathHandle GetChildHandle(PropertyPathHandle aParentHandle, uint8_t aContextTag) const;

    /* Returns the property path handle of the dictionary item given its parent dictionary handle and an item
     * key.
     */
    PropertyPathHandle GetDictionaryItemHandle(PropertyPathHandle aParentHandle, uint16_t aDictionaryKey) const;

    /**
     * Returns true if the handle refers to a leaf node in the schema tree.
     *
     * @retval bool
     */
    bool IsLeaf(PropertyPathHandle aPropertyHandle) const;

    /**
     * Returns a pointer to the PropertyInfo structure describing a particular path handle.
     *
     * @retval PropertyInfo*
     */
    const PropertyInfo * GetMap(PropertyPathHandle aHandle) const;

    /**
     * Returns the tag associated with a path handle. If it's a dictionary element, this function returns the ProfileTag. Otherwise,
     * it returns context tags.
     *
     * @retval uint64_t
     */
    uint64_t GetTag(PropertyPathHandle aHandle) const;

    /**
     * Returns the parent handle of a given child path handle. Dictionary keys in the handle are preserved in the case where the
     * parent handle is also a dictionary element.
     *
     * @retval PropertyPathHandle   Handle of the parent.
     */
    PropertyPathHandle GetParent(PropertyPathHandle aHandle) const;

    /**
     * Returns the first child handle associated with a particular parent.
     *
     * @retval PropertyPathHandle   Handle of the first child.
     */
    PropertyPathHandle GetFirstChild(PropertyPathHandle aParentHandle) const;

    /**
     * Given a handle to an existing child, returns the next child handle associated with a particular parent.
     *
     * @retval PropertyPathHandle   Handle of the next child.
     */
    PropertyPathHandle GetNextChild(PropertyPathHandle aParentId, PropertyPathHandle aChildHandle) const;

    /**
     * Calculate the depth in the schema tree for a given handle.
     *
     * @retval int32_t              The depth in the tree
     */
    int32_t GetDepth(PropertyPathHandle aHandle) const;

    /**
     * Given two property handles, calculate the lowest handle that serves as a parent to both of these handles. Additionally,
     * return the two child branches that contain each of the two handles (even if they are the same).
     *
     * @retval PropertyPathHandle   Handle to the lowest parent.
     */
    PropertyPathHandle FindLowestCommonAncestor(PropertyPathHandle aHandle1, PropertyPathHandle aHandle2,
                                                PropertyPathHandle * aHandle1BranchChild,
                                                PropertyPathHandle * aHandle2BranchChild) const;

    /**
     * Converts a PropertyPathHandle to an array of context tags.
     *
     * @param[in]  aCandidateHandle  The PropertyPathHandle to be converted.
     * @param[in]  aTags             Pointer to the output array.
     * @param[in]  aTagsSize         Size of the aTags array, in number of elements.
     * @param[out] aNumTags          The number of tags written to aTags
     *
     * @return     WEAVE_NO_ERROR in case of success; WEAVE_ERROR_NO_MEMORY if aTags is too small
     *             to store the full path.
     */
    WEAVE_ERROR GetRelativePathTags(const PropertyPathHandle aCandidateHandle, uint64_t * aTags, const uint32_t aTagsSize,
                                    uint32_t & aNumTags) const;
    /**
     * Returns true if the passed in profileId matches that stored in the schema.
     *
     * @retval bool
     */
    bool MatchesProfileId(uint32_t aProfileId) const;

    /**
     * Returns the profile id of the associated trait.
     *
     * @retval Trait profile id
     */
    uint32_t GetProfileId(void) const;

    /**
     * Returns true if the handle is a dictionary (and not in a dictionary - see method below).
     *
     * @retval bool
     */
    bool IsDictionary(PropertyPathHandle aHandle) const;

    /**
     * Returns true if the handle is *inside* a dictionary (a dictionary element). A user passed in handle (aDictionaryItemHandle)
     * is updated to point to the top-most dictionary element handle within the dictionary.
     *
     * @retval bool
     */
    bool IsInDictionary(PropertyPathHandle aHandle, PropertyPathHandle & aDictionaryItemHandle) const;

    bool IsNullable(PropertyPathHandle aHandle) const;
    bool IsEphemeral(PropertyPathHandle aHandle) const;
    bool IsOptional(PropertyPathHandle aHandle) const;

    /**
     * Checks if a given handle is a child of another handle. This can be an in-direct parent.
     *
     * @retval bool
     */
    bool IsParent(PropertyPathHandle aChildHandle, PropertyPathHandle aParentHandle) const;

    /**
     * Given a version range, this function checks to see if there is a compatibility intersection between that
     * and what is supported by schema that is backing this schema engine. If there is an intersection, the function will
     * return true and update the aIntersection argument passed in to reflect that results of that intersection test.
     */
    bool GetVersionIntersection(SchemaVersionRange & aVersion, SchemaVersionRange & aIntersection) const;

    /**
     * Given a provided data schema version, this will return the highest forward compatible schema version.
     */
    SchemaVersion GetHighestForwardVersion(SchemaVersion aVersion) const;

    /**
     * Given a provided data schema version, this will return the minimum compatible schema version
     */
    SchemaVersion GetLowestCompatibleVersion(SchemaVersion aVersion) const;

    SchemaVersion GetMinVersion() const;
    SchemaVersion GetMaxVersion() const;

private:
    PropertyPathHandle _GetChildHandle(PropertyPathHandle aParentHandle, uint8_t aContextTag) const;
    bool GetBitFromPathHandleBitfield(uint8_t * aBitfield, PropertyPathHandle aPathHandle) const;

    /*
     * the path MUST begin with a leading `/` and MUST NOT contain a trailing slash
     * numerical tags in the WDM path MUST be encoded using the standard C library for integer to string encoding,
     * aParseRes must be less than kContextTagMaxNum. If apEndptr is not NULL,
     * it stores the address of the first "/" in *apEndptr.
     */
    WEAVE_ERROR ParseTagString(const char * apTagString, char ** apEndptr, uint8_t & aParseRes) const;

public:
    const Schema mSchema;
};

/*
 * @class  TraitDataSink
 *
 * @brief  Base abstract class that represents a particular instance of a trait on a specific external resource (client).
 * Application developers are expected to subclass this to make a concrete sink that ingests data received from publishers.
 *
 *         It takes in a pointer to a schema that it then uses to help decipher incoming TLV data from a publisher and invoke the
 *         relevant data setter calls to pass the data up to subclasses.
 */
class TraitDataSink : protected TraitSchemaEngine::ISetDataDelegate
{
public:
    TraitDataSink(const TraitSchemaEngine * aEngine);
    virtual ~TraitDataSink() { }
    const TraitSchemaEngine * GetSchemaEngine(void) const { return mSchemaEngine; }

    typedef WEAVE_ERROR (*OnChangeRejection)(uint16_t aRejectionStatusCode, uint64_t aVersion, void * aContext);

    enum ChangeFlags
    {
        kFirstElementInChange = (1 << 0),
        kLastElementInChange  = (1 << 1)
    };

    /**
     * Given a reader that points to a data element conformant to a schema bound to this object, this method processes that data and
     * invokes the relevant SetLeafData call below for all leaf items in the buffer.
     *
     * A change rejection function can be passed in as well that will be invoked if the sink chooses to reject this data for any
     * reason.
     *
     * @retval #WEAVE_NO_ERROR On success.
     * @retval other           Encountered errors writing out the data.
     */
    WEAVE_ERROR StoreDataElement(PropertyPathHandle aHandle, TLV::TLVReader & aReader, uint8_t aFlags, OnChangeRejection aFunc,
                                 void * aContext, TraitDataHandle aDatahandle = 0);

    /**
     * Retrieves the current version of the data that resides in this sink.
     */
    uint64_t GetVersion(void) const { return mVersion; }
    /**
     * Returns a boolean value that determines whether the version is valid.
     */
    bool IsVersionValid(void) const { return mHasValidVersion; }

    virtual bool IsVersionNewer(DataVersion & aVersion) { return aVersion != mVersion || false == mHasValidVersion; }

#if WDM_ENABLE_SUBSCRIPTIONLESS_NOTIFICATION
    /**
     * Returns a boolean value that indicates if this sink accepts
     * subscriptionless notifications.
     */
    bool AcceptsSubscriptionlessNotifications(void) const { return mAcceptsSubscriptionlessNotifications; }
#endif // WDM_ENABLE_SUBSCRIPTIONLESS_NOTIFICATION

    /**
     * Convenience function for data sinks to handle unknown leaf handles with
     * a system level tolerance for mismatched schema as defined by
     * TDM_DISABLE_STRICT_SCHEMA_COMPILANCE.
     */
    WEAVE_ERROR HandleUnknownLeafHandle(void)
    {
#if TDM_DISABLE_STRICT_SCHEMA_COMPLIANCE
        return WEAVE_NO_ERROR;
#else
        return WEAVE_ERROR_TLV_TAG_NOT_FOUND;
#endif
    }

    enum EventType
    {
        /* Signals the beginning of a change record which in certain scenarios can span multiple data elements over multiple
         * notifies (the latter only a possibility if the data being transmitted is unable to fit within a single packet)
         */
        kEventChangeBegin,

        /* Start of a data element */
        kEventDataElementBegin,

        /* End of a data element */
        kEventDataElementEnd,

        /* End of a change record */
        kEventChangeEnd,

        /* Start of replacement of an entire dictionary */
        kEventDictionaryReplaceBegin,

        /* End of replacement of an entire dictionary */
        kEventDictionaryReplaceEnd,

        /* Start of modification or addition of a dictionary item */
        kEventDictionaryItemModifyBegin,

        /* End of modification or addition of a dictionary item */
        kEventDictionaryItemModifyEnd,

        /* Deletion of a dictionary item */
        kEventDictionaryItemDelete,

        /* Signals the start of the processing of a notify packet
         */
        kEventNotifyRequestBegin,

        /* Signals the end of the processing of a notify packet
         */
        kEventNotifyRequestEnd,

        /* Signals the start of the processing of a view response
         * TODO: I'm not entirely convinced of the need to have this event.
         */
        kEventViewResponseBegin,

        /* Signals the end of the processing of a view response
         * TODO: I'm not entirely convinced of the need to have this event.
         */
        kEventViewResponseEnd,

        /* Signals the termination of a subscription either due to an error, or the subscription was cancelled */
        kEventSubscriptionTerminated
    };

    union InEventParam
    {
        struct
        {
            PropertyPathHandle mTargetHandle;
        } mDictionaryReplaceBegin;

        struct
        {
            PropertyPathHandle mTargetHandle;
        } mDictionaryReplaceEnd;

        struct
        {
            PropertyPathHandle mTargetHandle;
        } mDictionaryItemModifyBegin;

        struct
        {
            PropertyPathHandle mTargetHandle;
        } mDictionaryItemModifyEnd;

        struct
        {
            PropertyPathHandle mTargetHandle;
        } mDictionaryItemDelete;
    };

    /*
     * Invoked either by the base class or by an external agent (like the subscription engine) to signal the occurence of an event
     * (of type EventType). Sub-classes are expected to over-ride this if they desire to be made known of these events.
     */
    virtual WEAVE_ERROR OnEvent(uint16_t aType, void * aInEventParam) { return WEAVE_NO_ERROR; }

#if WEAVE_CONFIG_ENABLE_WDM_UPDATE
    virtual bool IsUpdatableDataSink(void) { return false; }

    virtual WEAVE_ERROR SetSubscriptionClient(SubscriptionClient * apSubClient) { return WEAVE_NO_ERROR; };
    virtual WEAVE_ERROR SetUpdateEncoder(UpdateEncoder * apEncoder) { return WEAVE_NO_ERROR; };

#endif // WEAVE_CONFIG_ENABLE_WDM_UPDATE

    virtual SubscriptionClient * GetSubscriptionClient() { return NULL; }
    virtual UpdateEncoder * GetUpdateEncoder() { return NULL; }

    /* Subclass can invoke this to clear out their version */
    void ClearVersion(void);

protected: // ISetDataDelegate
    virtual WEAVE_ERROR SetLeafData(PropertyPathHandle aLeafHandle, nl::Weave::TLV::TLVReader & aReader) __OVERRIDE = 0;

    /*
     * Defaults to calling SetLeafData if aHandle is a leaf. DataSinks
     * can optionally implement this if they need to support nullable,
     * ephemeral, or optional properties.
     *
     * TODO: make this the defacto API, moving all the logic from
     * SetLeafData into this function.
     */
    virtual WEAVE_ERROR SetData(PropertyPathHandle aHandle, nl::Weave::TLV::TLVReader & aReader, bool aIsNull) __OVERRIDE;

    /* Subclass can invoke this if they desire to reject a particular data change */
    void RejectChange(uint16_t aRejectionStatusCode);

#if WDM_ENABLE_SUBSCRIPTIONLESS_NOTIFICATION
    /**
     * Returns a boolean value that indicates if this sink accepts
     * subscriptionless notifications.
     */
    void SetAcceptsSubscriptionlessNotifications(const bool aAcceptsSublessNotifies)
    {
        mAcceptsSubscriptionlessNotifications = aAcceptsSublessNotifies;
    }
#endif // WDM_ENABLE_SUBSCRIPTIONLESS_NOTIFICATION

    // Set current version of the data in this sink.
    void SetVersion(uint64_t version);
    void SetLastNotifyVersion(uint64_t version);
    uint64_t GetLastNotifyVersion(void) const { return mLastNotifyVersion; }

    const TraitSchemaEngine * mSchemaEngine;

private:
    // Current version of the data in this sink.
    uint64_t mVersion;
    uint64_t mLastNotifyVersion;
    void OnSetDataEvent(SetDataEventType aType, PropertyPathHandle aHandle) __OVERRIDE;
    static OnChangeRejection sChangeRejectionCb;
    static void * sChangeRejectionContext;
    bool mHasValidVersion;
#if WDM_ENABLE_SUBSCRIPTIONLESS_NOTIFICATION
    /* Set to true if sink accepts subscriptionless notifications */
    bool mAcceptsSubscriptionlessNotifications;
#endif // WDM_ENABLE_SUBSCRIPTIONLESS_NOTIFICATION

    friend class TestTdm;
};

#if WEAVE_CONFIG_ENABLE_WDM_UPDATE
/*
 * @class  TraitUpdatableDataSink
 *
 * @brief  Base abstract class that represents a particular instance of a trait on a specific external resource (client).
 * Application developers are expected to subclass this to make a concrete sink that ingests data received from publishers
 * and generate local sink changes to publisher.
 *
 */
class TraitUpdatableDataSink : public TraitDataSink, protected TraitSchemaEngine::IGetDataDelegate
{
public:
    TraitUpdatableDataSink(const TraitSchemaEngine * aEngine);

    WEAVE_ERROR ReadData(TraitDataHandle aTraitDataHandle, PropertyPathHandle aHandle, uint64_t aTagToWrite,
                         TLV::TLVWriter & aWriter, PropertyPathHandle & aPropertyPathHandleOfDictItemToStartFrom);

    virtual WEAVE_ERROR GetData(PropertyPathHandle aHandle, uint64_t aTagToWrite, nl::Weave::TLV::TLVWriter & aWriter,
                                bool & aIsNull, bool & aIsPresent) __OVERRIDE;

    WEAVE_ERROR SetUpdated(SubscriptionClient * apSubClient, PropertyPathHandle aPropertyHandle, bool aIsConditional = false);
    WEAVE_ERROR ClearUpdated(SubscriptionClient * apSubClient, PropertyPathHandle aPropertyHandle);

    void Lock(SubscriptionClient * apSubClient);
    void Unlock(SubscriptionClient * apSubClient);

    virtual bool IsUpdatableDataSink(void) __OVERRIDE { return true; }

    virtual WEAVE_ERROR SetSubscriptionClient(SubscriptionClient * apSubClient) __OVERRIDE
    {
        mpSubClient = apSubClient;
        return WEAVE_NO_ERROR;
    }
    virtual WEAVE_ERROR SetUpdateEncoder(UpdateEncoder * apEncoder) __OVERRIDE
    {
        mpUpdateEncoder = apEncoder;
        return WEAVE_NO_ERROR;
    }

    virtual SubscriptionClient * GetSubscriptionClient() __OVERRIDE { return mpSubClient; }
    virtual UpdateEncoder * GetUpdateEncoder() __OVERRIDE { return mpUpdateEncoder; }

private:
    friend class SubscriptionClient;
    friend class UpdateEncoder;

    /**
     * Checks if a DataVersion is more recent than the one currently stored in the Sink.
     * Note: publishers are allowed to reset the version of a trait instance after rebooting.
     * This implies that a notification with the lastest data for a trait instance can have
     * a version number that is lower than the current one. Please refer to the WDM
     * specification for more details.
     */
    bool IsVersionNewer(DataVersion & aVersion) __OVERRIDE
    {
        return false == IsVersionValid() || aVersion > GetVersion() || aVersion < GetLastNotifyVersion();
    }
    uint64_t GetUpdateRequiredVersion(void) const { return mUpdateRequiredVersion; }
    void ClearUpdateRequiredVersion(void) { SetUpdateRequiredVersion(0); }
    void SetUpdateRequiredVersion(const uint64_t & aUpdateRequiredVersion);

    uint64_t GetUpdateStartVersion(void) const { return mUpdateStartVersion; }
    void SetUpdateStartVersion(void);
    void ClearUpdateStartVersion(void) { mUpdateStartVersion = 0; }

    bool IsConditionalUpdate(void) { return mConditionalUpdate; }
    void SetConditionalUpdate(bool aValue) { mConditionalUpdate = aValue; }

    bool IsPotentialDataLoss(void) { return mPotentialDataLoss; }
    void SetPotentialDataLoss(bool aValue) { mPotentialDataLoss = aValue; }

    uint64_t mUpdateRequiredVersion;
    uint64_t mUpdateStartVersion;
    bool mConditionalUpdate;
    bool mPotentialDataLoss;
    SubscriptionClient * mpSubClient;
    UpdateEncoder * mpUpdateEncoder;
};
#endif // WEAVE_CONFIG_ENABLE_WDM_UPDATE

class Command;

class TraitDataSource : private TraitSchemaEngine::IGetDataDelegate
{
public:
    TraitDataSource(const TraitSchemaEngine * aEngine);
    virtual ~TraitDataSource() { }
    const TraitSchemaEngine * GetSchemaEngine(void) const { return mSchemaEngine; }

    uint64_t GetVersion(void);

    WEAVE_ERROR ReadData(PropertyPathHandle aHandle, uint64_t aTagToWrite, TLV::TLVWriter & aWriter);

    /* Interactions with the underlying data has to always be done within a locked context. This applies to both the app logic
     * (e.g., a publisher when modifying its source data) as well as to the core WDM logic (when trying to access that published
     * data).  This is required of both publishers and clients.
     */
    WEAVE_ERROR Lock(void);
    WEAVE_ERROR Unlock(void);

    void SetDirty(PropertyPathHandle aPropertyHandle);

#if WDM_ENABLE_PUBLISHER_UPDATE_SERVER_SUPPORT
    WEAVE_ERROR Unlock(bool aSkipVersionIncrement);

    virtual bool IsUpdatableDataSource(void) { return false; }

    // Check if version is equal to the internal trait version
    bool IsVersionEqual(DataVersion & aVersion) { return aVersion == GetVersion(); }
#endif // WDM_ENABLE_PUBLISHER_UPDATE_SERVER_SUPPORT

#if TDM_ENABLE_PUBLISHER_DICTIONARY_SUPPORT
    void DeleteKey(PropertyPathHandle aPropertyHandle);
#endif

    // This API has been deprecated.
    virtual void OnCustomCommand(Command * aCommand, const nl::Weave::WeaveMessageInfo * aMsgInfo,
                                 nl::Weave::PacketBuffer * aPayload, const uint64_t & aCommandType, const bool aIsExpiryTimeValid,
                                 const int64_t & aExpiryTimeMicroSecond, const bool aIsMustBeVersionValid,
                                 const uint64_t & aMustBeVersion, nl::Weave::TLV::TLVReader & aArgumentReader);

    // API with all meta information housed within the Command
    // object.
    virtual void OnCustomCommand(Command * aCommand, const nl::Weave::WeaveMessageInfo * aMsgInfo,
                                 nl::Weave::PacketBuffer * aPayload, nl::Weave::TLV::TLVReader & aArgumentReader);

    enum EventType
    {
        kEventDataSourceMax
    };
    /*
     * Invoked either by the base class or by an external agent (like the subscription engine) to signal the occurrence of an event
     * (of type EventType). Sub-classes are expected to over-ride this if they desire to be made known of these events.
     */
    virtual WEAVE_ERROR OnEvent(uint16_t aType, void * aInEventParam) { return WEAVE_NO_ERROR; }

#if (WEAVE_CONFIG_WDM_PUBLISHER_GRAPH_SOLVER == IntermediateGraphSolver)
    /* Set of functions to be called by the intermediate graph solver on the notification engine for marking/clearing this entire
     * data source as dirty */
    void SetRootDirty(void) { mRootIsDirty = true; }
    void ClearRootDirty(void) { mRootIsDirty = false; }
    bool IsRootDirty(void) const { return mRootIsDirty; }
    bool mRootIsDirty;
#endif

protected: // IGetDataDelegate
    /*
     * Defaults to calling GetLeafData if aHandle is a leaf. DataSources
     * can optionally implement this if they need to support nullable,
     * ephemeral, or optional properties.
     *
     * TODO: make this the defacto API, moving all the logic from
     * GetLeafData into this function.
     */
    virtual WEAVE_ERROR GetData(PropertyPathHandle aHandle, uint64_t aTagToWrite, nl::Weave::TLV::TLVWriter & aWriter,
                                bool & aIsNull, bool & aIsPresent) __OVERRIDE;

    virtual WEAVE_ERROR GetLeafData(PropertyPathHandle aLeafHandle, uint64_t aTagToWrite,
                                    nl::Weave::TLV::TLVWriter & aWriter) __OVERRIDE = 0;

#if TDM_ENABLE_PUBLISHER_DICTIONARY_SUPPORT
    virtual WEAVE_ERROR GetNextDictionaryItemKey(PropertyPathHandle aDictionaryHandle, uintptr_t & aContext,
                                                 PropertyDictionaryKey & aKey) __OVERRIDE
    {
        return WEAVE_ERROR_INVALID_ARGUMENT;
    }
#endif

    // Set current version of the data in this source.
    void SetVersion(uint64_t version) { mVersion = version; }

    // Increment current version of the data in this source.
    void IncrementVersion(void);
    // Controls whether mVersion is incremented automatically or not.
    bool mManagedVersion;

    const TraitSchemaEngine * mSchemaEngine;

private:
    // Current version of the data in this source.
    uint64_t mVersion;
    // Tracks whether SetDirty was called within a Lock/Unlock 'session'
    bool mSetDirtyCalled;
};

#if WDM_ENABLE_PUBLISHER_UPDATE_SERVER_SUPPORT
/*
 * @class  TraitUpdatableDataSource
 *
 * @brief  Base abstract class that represents a particular instance of a trait on a specific resource (publisher).
 * Application developers are expected to subclass this to make a concrete source that apply data received from client
 *
 */
class TraitUpdatableDataSource : public TraitDataSource, protected TraitSchemaEngine::ISetDataDelegate
{
public:
    TraitUpdatableDataSource(const TraitSchemaEngine * aEngine);

    typedef WEAVE_ERROR (*OnChangeRejection)(uint16_t aRejectionStatusCode, uint64_t aVersion, void * aContext);

    /**
     * Convenience function for data sinks to handle unknown leaf handles with
     * a system level tolerance for mismatched schema as defined by
     * TDM_DISABLE_STRICT_SCHEMA_COMPILANCE.
     */
    WEAVE_ERROR HandleUnknownLeafHandle(void)
    {
#if TDM_DISABLE_STRICT_SCHEMA_COMPLIANCE
        return WEAVE_NO_ERROR;
#else
        return WEAVE_ERROR_TLV_TAG_NOT_FOUND;
#endif
    }

    enum EventType
    {
        /* Start of a data element */
        kEventDataElementBegin = kEventDataSourceMax + 1,

        /* End of a data element */
        kEventDataElementEnd,

        /* Signal complete of update processing */
        kEventUpdateProcessingComplete,

        /* Start of replacement of an entire dictionary */
        kEventDictionaryReplaceBegin,

        /* End of replacement of an entire dictionary */
        kEventDictionaryReplaceEnd,

        /* Start of modification or addition of a dictionary item */
        kEventDictionaryItemModifyBegin,

        /* End of modification or addition of a dictionary item */
        kEventDictionaryItemModifyEnd,

        /* Deletion of a dictionary item */
        kEventDictionaryItemDelete,

        /* Signals the start of the processing of a update request packet
         */
        kEventUpdateRequestBegin,

        /* Signals the end of the processing of a update request packet
         */
        kEventUpdateRequestEnd,

        /* Signals the termination of a subscription either due to an error, or the subscription was cancelled */
        kEventSubscriptionTerminated,

        kEventUpdatableDataSourceMax
    };

    union InEventParam
    {
        struct
        {
            PropertyPathHandle mTargetHandle;
        } mDictionaryReplaceBegin;

        struct
        {
            PropertyPathHandle mTargetHandle;
        } mDictionaryReplaceEnd;

        struct
        {
            PropertyPathHandle mTargetHandle;
        } mDictionaryItemModifyBegin;

        struct
        {
            PropertyPathHandle mTargetHandle;
        } mDictionaryItemModifyEnd;

        struct
        {
            PropertyPathHandle mTargetHandle;
        } mDictionaryItemDelete;
    };

    /**
     * Given a reader that points to a data element conformant to a schema bound to this object, this method processes that data and
     * invokes the relevant SetLeafData call below for all leaf items in the buffer.
     *
     * A change rejection function can be passed in as well that will be invoked if the updatable source chooses to reject this data
     * for any reason.
     *
     * @retval #WEAVE_NO_ERROR On success.
     * @retval other           Encountered errors writing out the data.
     */
    WEAVE_ERROR StoreDataElement(PropertyPathHandle aHandle, TLV::TLVReader & aReader, uint8_t aFlags, OnChangeRejection aFunc,
                                 void * aContext);
    virtual bool IsUpdatableDataSource(void) __OVERRIDE { return true; }

protected: // ISetDataDelegate
    virtual WEAVE_ERROR SetLeafData(PropertyPathHandle aLeafHandle, nl::Weave::TLV::TLVReader & aReader) __OVERRIDE = 0;

    /*
     * Defaults to calling SetLeafData if aHandle is a leaf. DataSinks
     * can optionally implement this if they need to support nullable,
     * ephemeral, or optional properties.
     *
     * TODO: make this the defacto API, moving all the logic from
     * SetLeafData into this function.
     */
    virtual WEAVE_ERROR SetData(PropertyPathHandle aHandle, nl::Weave::TLV::TLVReader & aReader, bool aIsNull) __OVERRIDE;

    /* Subclass can invoke this if they desire to reject a particular data change */
    void RejectChange(uint16_t aRejectionStatusCode);

private:
    friend class SubscriptionEngine;

    void OnSetDataEvent(SetDataEventType aType, PropertyPathHandle aHandle) __OVERRIDE;

private:
    static OnChangeRejection sChangeRejectionCb;
    static void * sChangeRejectionContext;
};
#endif // WDM_ENABLE_PUBLISHER_UPDATE_SERVER_SUPPORT

}; // namespace WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current)
}; // namespace Profiles
}; // namespace Weave
}; // namespace nl

#endif // _WEAVE_DATA_MANAGEMENT_TRAIT_DATA_CURRENT_H
