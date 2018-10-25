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
 *   Definitions for the abstract ProfileDatabase auxiliary class.
 *
 *  This file contains definitions for the abstract ProfileDatabase
 *  auxiliary class, which is intended to make the construction of
 *  WDM-based applications more straightforward by providing a template
 *  and filling common infrastructure. Also provided is a set of helper
 *  functions for wrangling WDM-related TLV.
 */

#ifndef _WEAVE_DATA_MANAGEMENT_PROFILE_DATABASE_LEGACY_H
#define _WEAVE_DATA_MANAGEMENT_PROFILE_DATABASE_LEGACY_H

#include <Weave/Profiles/data-management/Legacy/WdmManagedNamespace.h>

#include <Weave/Profiles/data-management/DataManagement.h>

namespace nl {
namespace Weave {
namespace Profiles {
namespace WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Legacy) {

    /**
     *  @brief
     *    Start writing a path list.
     *
     *  Write the TLV for the beginning of a WDM path list, e.g. as
     *  the primary argument of a ViewRequest().
     *
     *  @param [in]     aWriter         A reference to a TLV writer
     *                                  with which to start writing.
     *
     *  @return #WEAVE_NO_ERROR On success. Otherwise return a
     *  #WEAVE_ERROR reflecting an inability to start a container.
     */

    static inline WEAVE_ERROR StartPathList(nl::Weave::TLV::TLVWriter &aWriter)
    {
        nl::Weave::TLV::TLVType pathListContainer;

        return aWriter.StartContainer(nl::Weave::TLV::ProfileTag(kWeaveProfile_WDM, kTag_WDMPathList),
                                      nl::Weave::TLV::kTLVType_Array, pathListContainer);
    };


    /**
     *  @brief
     *    Start writing a data list.
     *
     *  Write the TLV for the beginning of a WDM data list, e.g. as
     *  the primary argument of an UpdateRequest().
     *
     *  @param [in]     aWriter         A reference to a TLV writer
     *                                  with which to start writing.
     *
     *  @return #WEAVE_NO_ERROR On success. Otherwise return a
     *  #WEAVE_ERROR reflecting an inability to start a container.
     */

    static inline WEAVE_ERROR StartDataList(nl::Weave::TLV::TLVWriter &aWriter)
    {
        nl::Weave::TLV::TLVType dataListContainer;

        return aWriter.StartContainer(nl::Weave::TLV::ProfileTag(kWeaveProfile_WDM, kTag_WDMDataList),
                                      nl::Weave::TLV::kTLVType_Array, dataListContainer);
    };

    /**
     *  @brief
     *    Start writing a data list to a given ReferencedTLVData object.
     *
     *  Write the TLV for the beginning of a WDM data list. In this
     *  case, we assume that we're writing out the data list to a
     *  referenced TLV data structure. The writer is assumed to be
     *  uninitialized - or, in any case, will be initialized to point
     *  to the given object.
     *
     *  @param [out]    aDataList       A reference to a
     *                                  ReferencedTLVData object that
     *                                  is to be the target of the
     *                                  writer and eventually contain
     *                                  the data of interest.
     *
     *  @param [in]     aWriter         A reference to a TLV writer
     *                                  with which to start writing.
     *
     *  @return #WEAVE_NO_ERROR On success. Otherwise return a
     *  #WEAVE_ERROR reflecting an inability to start a container.
     */

    static inline WEAVE_ERROR StartDataList(ReferencedTLVData &aDataList, nl::Weave::TLV::TLVWriter &aWriter)
    {
        aWriter.Init(aDataList.theData, aDataList.theMaxLength);

        return StartDataList(aWriter);
    };

    /**
     *  @brief
     *    Finish writing a path list or data list.
     *
     *  Write the TLV for the end of a WDM path or data list. Also,
     *  finalize the writer.
     *
     *  @param [in,out] aWriter         A reference to a TLV writer
     *                                  with which to write the end of
     *                                  the list.
     *
     *  @return #WEAVE_NO_ERROR On success. Otherwise return a
     *  #WEAVE_ERROR reflecting an inability to end a container.
     */

    static inline WEAVE_ERROR EndList(nl::Weave::TLV::TLVWriter &aWriter)
    {
        WEAVE_ERROR err = WEAVE_NO_ERROR;

        /*
         * in the normal case, which is what these convenience methods
         * are attempting to address, both the path list and data list
         * appear as the only elements of a top-level TLV
         * structure. thus, the outer container type is always
         * "structure".
         */

        err = aWriter.EndContainer(nl::Weave::TLV::kTLVType_Structure);
        if (err == WEAVE_NO_ERROR)
        {
            err = aWriter.Finalize();
        }

        return err;
    };

    /**
     *  @brief
     *    Finish writing a path list or data list.
     *
     *  Write the TLV for the end of a WDM path or data list. Also,
     *  finalize the writer. In this case, the ReferencedTLVData object
     *  to which the TLV is being written is passed in and modified to
     *  reflect the amount of data written.
     *
     *  @param [in,out] aList           A reference to the
     *                                  ReferenceTLVData object to
     *                                  which the list was being
     *                                  written.
     *
     *  @param [in]     aWriter         A reference to a TLV writer
     *                                  with which to write the end of
     *                                  the list.
     *
     *  @return #WEAVE_NO_ERROR On success. Otherwise return a
     *  #WEAVE_ERROR reflecting an inability to end a container.
     */

    static inline WEAVE_ERROR EndList(ReferencedTLVData &aList, nl::Weave::TLV::TLVWriter &aWriter)
    {
        WEAVE_ERROR err = WEAVE_NO_ERROR;

        err = EndList(aWriter);

        aList.theLength = aWriter.GetLengthWritten();

        return err;
    };

    /**
     *  @brief
     *    Start writing a data list element.
     *
     *  Write the TLV for the beginning of a WDM data list element.
     *
     *  @param [in]     aWriter         A reference to a TLV writer
     *                                  with which to start writing.
     *
     *  @return #WEAVE_NO_ERROR On success. Otherwise return a
     *  #WEAVE_ERROR reflecting an inability to start a container.
     */

    static inline WEAVE_ERROR StartDataListElement(nl::Weave::TLV::TLVWriter &aWriter)
    {
        nl::Weave::TLV::TLVType itemContainer;

        return aWriter.StartContainer(nl::Weave::TLV::AnonymousTag, nl::Weave::TLV::kTLVType_Structure, itemContainer);
    };

    /**
     *  @brief
     *    Finish writing a data list element.
     *
     *  Write the TLV for the end of a WDM data list element. Note this
     *  this automatically passes in a type of kTLVType_Array to the
     *  EndContainer() call assuming that we are always closing a list
     *  item.
     *
     *  @param [in]     aWriter         A reference to a TLV writer
     *                                  with which to write the end of
     *                                  the item.
     *
     *  @return #WEAVE_NO_ERROR On success. Otherwise return a
     *  #WEAVE_ERROR reflecting an inability to end a container.
     */

    static inline WEAVE_ERROR EndDataListElement(nl::Weave::TLV::TLVWriter &aWriter)
    {
        /*
         * obviously, the outer container of a list element is a
         * "list", i.e. an array.
         */

        return aWriter.EndContainer(nl::Weave::TLV::kTLVType_Array);
    };

    bool CheckWDMTag(uint32_t aTagNum, nl::Weave::TLV::TLVReader &aReader);

    /**
     *  @brief
     *    Validate that a TLV element being read has the expected WDM
     *    tag.
     *
     *  @param [in]     aTagNum         The 32-bit tag number of the
     *                                  expected WDM tag.
     *
     *  @param [in]     aReader         A TLV reader positioned at the
     *                                  element to be validated.
     *
     *  @return #WEAVE_NO_ERROR On success. Otherwise
     *  #WEAVE_ERROR_INVALID_TLV_TAG if the tag does not match the
     *  given tag number when interpreted as a WDM tag.
     */

    static inline WEAVE_ERROR ValidateWDMTag(uint32_t aTagNum, nl::Weave::TLV::TLVReader &aReader)
    {
        WEAVE_ERROR err;

        if (CheckWDMTag(aTagNum, aReader))
            err = WEAVE_NO_ERROR;

        else
            err = WEAVE_ERROR_INVALID_TLV_TAG;

        return err;
    };

    /**
     *  @brief
     *    Check that a TLV element being read has the expected TLV
     *    type.
     *
     *  Check a given TLV type against
     *  the element type at the head of a TLV reader.
     *
     *  @sa WeaveTLVTypes.h
     *
     *  @param [in]     aType           The TLVType to be checked
     *                                  against a specific element
     *
     *  @param [in]     aReader         A reference to a TLV reader
     *                                  positioned at the element to
     *                                  be checked
     *
     *  @return true iff the TLVType of the element and aType match.
     */

    static inline bool CheckTLVType(nl::Weave::TLV::TLVType aType, nl::Weave::TLV::TLVReader &aReader)
    {
        nl::Weave::TLV::TLVType type = aReader.GetType();

        return type == aType;
    };

    /**
     *  @brief
     *    Validate that a TLV element being read has the expected TLV
     *    type.
     *
     *  Check a given TLV type against
     *  the element type at the head of a TLV reader and return an
     *  error if there is no match.
     *
     *  @sa WeaveTLVTypes.h
     *
     *  @param [in]     aType           The TLVType to be validated
     *                                  against a specific element.
     *
     *  @param [in]     aReader         A reference to a TLV reader
     *                                  positioned at the element to
     *                                  be validated.
     *
     *  @return #WEAVE_NO_ERROR if there is a match or
     *  #WEAVE_ERROR_WRONG_TLV_TYPE if not.
     */

    inline WEAVE_ERROR ValidateTLVType(nl::Weave::TLV::TLVType aType, nl::Weave::TLV::TLVReader &aReader)
    {
        WEAVE_ERROR err;

        if (CheckTLVType(aType, aReader))
            err = WEAVE_NO_ERROR;

        else
            err = WEAVE_ERROR_WRONG_TLV_TYPE;

        return err;
    };

    WEAVE_ERROR OpenPathList(ReferencedTLVData &aPathList, nl::Weave::TLV::TLVReader &aReader);

    WEAVE_ERROR OpenDataList(ReferencedTLVData &aDataList, nl::Weave::TLV::TLVReader &aReader);

    WEAVE_ERROR OpenDataListElement(nl::Weave::TLV::TLVReader &aReader, nl::Weave::TLV::TLVReader &aPathReader, uint64_t &aVersion);

    /**
     *  @brief
     *    Stop reading a WDM path or data list.
     *
     *  This method assumes that the list in question is the topmost
     *  TLV element and so passes kTLVType_Sructure to ExitContainer().
     *
     *  @param [in,out] aReader         A TLV reader positioned in
     *                                  a WDM path or data list.
     *
     *  @return #WEAVE_NO_ERROR On success. Otherwise return a
     *  #WEAVE_ERROR reflecting an inability to exit a container.
     */

    inline WEAVE_ERROR CloseList(nl::Weave::TLV::TLVReader &aReader)
    {
        return aReader.ExitContainer(nl::Weave::TLV::kTLVType_Structure);
    };

    /**
     *  @brief
     *    Stop reading a WDM data list element.
     *
     *  This method assumes that the element in question is part of a
     *  WDM data list and so passes kTLVType_Array to ExitContainer().
     *
     *  @param [in,out] aReader         A TLV reader positioned in
     *                                  a WDM data list element.
     *
     *  @return #WEAVE_NO_ERROR On success. Otherwise return a
     *  #WEAVE_ERROR reflecting an inability to exit a container.
     */

    inline WEAVE_ERROR CloseDataListElement(nl::Weave::TLV::TLVReader &aReader)
    {
        return aReader.ExitContainer(nl::Weave::TLV::kTLVType_Array);
    };

    /*
     * it also turns out that we have to support a range of instance
     * identifiers.  we make the assumption that, at least in an
     * embedded context, they are all binary encodings. so, that means
     * that for all the numeric ones we can probably get away with
     * using a uint64 - since that will be the normal case anyway -
     * and for longer values we can use the TLV byte array.
     *
     * what this means, sadly, is that all the "convenience" methods
     * below that take or extract an instance ID have to account for
     * both types.
     */

    WEAVE_ERROR EncodePath(nl::Weave::TLV::TLVWriter &aWriter,
                           const uint64_t &aTag,
                           uint32_t aProfileId,
                           const uint64_t &aInstanceId,
                           uint32_t aPathLen,
                           ...);

    WEAVE_ERROR EncodePath(nl::Weave::TLV::TLVWriter &aWriter,
                           const uint64_t &aTag,
                           uint32_t aProfileId,
                           const uint32_t aInstanceIdLen,
                           const uint8_t *aInstanceId,
                           uint32_t aPathLen,
                           ...);

    WEAVE_ERROR EncodePath(nl::Weave::TLV::TLVWriter &aWriter,
                           const uint64_t &aTag,
                           uint32_t aProfileId,
                           const char *aInstanceId,
                           uint32_t aPathLen,
                           ...);

    /*
     * and for compatibility with Topaz, actually with older versions
     * of the Service that were in use under Topaz 1.0, we need these
     * versions.
     */

    WEAVE_ERROR EncodeDeprecatedPath(nl::Weave::TLV::TLVWriter &aWriter,
                                     const uint64_t &aTag,
                                     uint32_t aProfileId,
                                     const uint64_t &aInstanceId,
                                     uint32_t aPathLen,
                                     ...);

    WEAVE_ERROR EncodeDeprecatedPath(nl::Weave::TLV::TLVWriter &aWriter,
                                     const uint64_t &aTag,
                                     uint32_t aProfileId,
                                     const char *aInstanceId,
                                     uint32_t aPathLen,
                                     ...);

    /**
     *  @class ProfileDatabase
     *
     *  @brief
     *    The abstract ProfileDatabase auxiliary class.
     *
     *  WDM separates the protocol implementation from the data
     *  management implementation and, at least in principle, leaves
     *  most of the latter to the application developer. All of the
     *  interesting calls in WDM and all of the abstract methods that
     *  the profile developer is required to implement take TLV-encoded
     *  path lists or data lists. This puts a burden on profile
     *  developers and, in practice, will cause a lot of code
     *  duplication as developer after developer writes the same code
     *  for packing and unpacking TLV and so on. To ease things a bit
     *  we provide a kind of "data management toolkit".
     *
     *  This auxiliary class provides support for storing and retrieving data
     *  provided that the necessary concrete ProfileData sub-classes have been
     *  supplied and added to the LookupProfileData() method below.
     */

    class ProfileDatabase
    {
    public:

        /**
         *  @class ProfileData
         *
         *  @brief
         *    The abstract ProfileData auxiliary inner class.
         *
         *  ProfileDatabase sub-class implementers should implement
         *  sub-classes of this auxiliary inner class as well. The
         *  function of ProfileData objects is to provide hooks
         *  whereby concrete data may be stored, given a its
         *  representation as TLV and retrieved as a TLV
         *  representation given a TLV-encoded list of paths.
         */

        class ProfileData
        {
        public:
            ProfileData(void);

            virtual ~ProfileData(void);

            virtual WEAVE_ERROR Store(nl::Weave::TLV::TLVReader &aPathReader, uint64_t aVersion, nl::Weave::TLV::TLVReader &aDataReader);

            /**
             *  @brief
             *    Store a data item based on its tag.
             *
             *  @note
             *    ProfileDatabase subclass implementers must supply a
             *    concrete implementation of this method in order to
             *    store a particular kind of data under a known tag.
             *
             *  @param [in]     aTag            A reference to the
             *                                  fully-qualified 64-bit
             *                                  TLV tag under which
             *                                  the data should be
             *                                  stored.
             *
             *  @param [in]     aDataReader     A TLV reader
             *                                  positioned at the data
             *                                  item to be stored.
             *
             *  @return #WEAVE_NO_ERROR on success, otherwise return
             *  a #WEAVE_ERROR reflecting an inability either to
             *  recognize the tag or to store the data.
             */

            virtual WEAVE_ERROR StoreItem(const uint64_t &aTag, nl::Weave::TLV::TLVReader &aDataReader) = 0;

            /**
             *  @brief
             *    Write out a data item given a residual WDM path.
             *
             *  ProfileDatabase subclass implementers must provide a
             *  concrete implementation for this method in every case
             *  where the individual elements of a ProfileData subclass
             *  object are accessible under particular tags.
             *
             *  @param [in]     aPathReader     A reference to a TLV
             *                                  reader positioned in a
             *                                  WDM path after the
             *                                  profile information,
             *                                  i.e. at the 'residual'
             *                                  path elements if any.
             *
             *  @param [in]     aDataWriter     A reference to a TLV
             *                                  writer used to write
             *                                  out the data indicated
             *                                  by the residual path.
             *
             *  @return #WEAVE_NO_ERROR On success. Otherwise return a
             *  #WEAVE_ERROR reflecting an inability to recognize a
             *  residual tag or else to write the corresponding data.
             */

            virtual WEAVE_ERROR Retrieve(nl::Weave::TLV::TLVReader &aPathReader, nl::Weave::TLV::TLVWriter &aDataWriter) = 0;

            /*
             * data members:
             */

            /**
             *  @brief
             *    Profile data version.
             *
             *  The version given here in the ProfileData object
             *  applies to the whole profile data set for a particular
             *  instance.
             */

            uint64_t mVersion;

        };

        WEAVE_ERROR Store(ReferencedTLVData &aDataList);

        WEAVE_ERROR Retrieve(ReferencedTLVData &aPathList, ReferencedTLVData &aDataList);

        WEAVE_ERROR Retrieve(ReferencedTLVData &aPathList, nl::Weave::TLV::TLVWriter &aWriter);

        WEAVE_ERROR LookupProfileData(nl::Weave::TLV::TLVReader &aPathReader, ProfileData **aProfileData);

        WEAVE_ERROR LookupDataFromProfileDescriptor(nl::Weave::TLV::TLVReader &aDescReader, ProfileData **aProfileData);

        /**
         *  @brief
         *    Look up a ProfileData object.
         *
         *  Look up a specific ProfileData object given a profile ID
         *  and an (optional) instance ID< provided as a TLV reader.
         *
         *  @param [in]     aProfileId      The 32-bit profile number
         *                                  of the profile of interest.
         *
         *  @param [in]     aInstanceIdRdr  A pointer to a TLV reader
         *                                  positioned at the instance
         *                                  identifier data. If not
         *                                  instance identifier was
         *                                  provided then this shall
         *                                  be NULL.
         *
         *  @param [out]    aResult         A pointer, intended to return a
         *                                  pointer to the ProfileData object
         *                                  of interest.
         *
         *  @return #WEAVE_NO_ERROR On success. Otherwise return a
         *  #WEAVE_ERROR reflecting an inability to find a ProfileData
         *  object.
         */

        virtual WEAVE_ERROR LookupProfileData(uint32_t aProfileId, nl::Weave::TLV::TLVReader *aInstanceIdRdr, ProfileData **aResult) = 0;

    protected:

        WEAVE_ERROR StoreInternal(nl::Weave::TLV::TLVReader &aPathReader, uint64_t aVersion, nl::Weave::TLV::TLVReader &aDataReader);
    };

}; // WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Legacy)
}; // Profiles
}; // Weave
}; // nl

#endif // _WEAVE_DATA_MANAGEMENT_PROFILE_DATABASE_LEGACY_H
