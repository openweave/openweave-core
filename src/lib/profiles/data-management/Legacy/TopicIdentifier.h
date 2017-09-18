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
 *    Definitions for the TopicIdentifier type.
 *
 *  This file contains definitions for the TopicIdentifier type used by
 *  Weave Data Management. See the document "Nest Weave-Data Management
 *  Protocol" document for a complete(ish) description.
 */

#ifndef _WEAVE_DATA_MANAGEMENT_TOPIC_IDENTIFIER_LEGACY_H
#define _WEAVE_DATA_MANAGEMENT_TOPIC_IDENTIFIER_LEGACY_H

#include <Weave/Profiles/data-management/Legacy/WdmManagedNamespace.h>

namespace nl {
namespace Weave {
namespace Profiles {
namespace WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Legacy) {

    /**
     *  @brief
     *    The topic identifier.
     *
     *  Topic identifiers are 64-bit quantities with two related
     *  uses/semantics.  First of all, they may be used as a
     *  conventional shorthand for a well-known set of paths, e.g. "the
     *  Nest smoke detector". Topic IDs that are used in this way shall,
     *  like profile identifiers, contain a vendor code that prevents topic
     *  IDs chosen autonomously by disparate vendors from conflicting,
     *  as follows:
     *
     *  | bit 48 - 63 | bit 0 - 47   |
     *  | :----:      | :----:       |
     *  | Vendor ID   | Topic number |
     *
     *  The second use/semantics for topic identifiers arises in that
     *  case of a dynamic subscription between a WDM client and a
     *  publisher. in this case, the publisher shall always supply an
     *  unique topic ID that stands for the specific subscription and
     *  it shall do this whether the subscription was requested using a
     *  well-known topic ID or an arbitrary path list. topic
     *  identifiers of this form are distinguished by having a vendor
     *  code of 0xFFFF.
     *
     *  @sa WeaveVendorIdentifiers.hpp
     */

    typedef uint64_t TopicIdentifier;

    /**
     *  @brief Distinguished topic IDs
     *
     *  There are three distinguished topic IDs of interest, all three
     *  of which are formatted as "publisher-specific".
     */

    enum
    {
        /**
         *  This is used as a mask to create or decompose a topci ID
         */
        kTopicIdPublisherSpecificMask =         0xFFFF000000000000ULL,

        /**
         *  This is a special value reserved to express either an invalid
         *  or a wild-card topic ID
         *
         */
        kTopicIdNotSpecified =                  0x0000000000000000ULL,

        /**
         *  This is reserved as a wild-card topic ID
         */
        kAnyTopicId =                           0xFFFFFFFFFFFFFFFFULL
    };

    /**
     *  @brief Generate a publisher-specific topic ID from scratch.
     *
     *  @return the new topic identifier.
     */

    inline TopicIdentifier PublisherSpecificTopicId(void)
    {
        return kTopicIdPublisherSpecificMask | rand();
    }

    /**
     *  @brief Check if a topic ID is publisher-specific
     *
     *  @param [in] aTopicId a topic identifier ot check
     *
     *  @return true if the topic ID is publisher-specific, false otherwise
     */

    inline bool IsPublisherSpecific(const TopicIdentifier &aTopicId)
    {
        return (aTopicId != kAnyTopicId && (aTopicId & kTopicIdPublisherSpecificMask) == kTopicIdPublisherSpecificMask);
    }

}; // WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Legacy)
}; // Profiles
}; // Weave
}; // nl

#endif // _WEAVE_DATA_MANAGEMENT_TOPIC_IDENTIFIER_LEGACY_H
