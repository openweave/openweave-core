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
 *    Various defined values used by Weave Data Management.
 *
 *  This file defines enumerations of message types, status codes, tags
 *  and other miscellaneous values required for the operation of the
 *  weave Data Management (WDM) profile.
 *
 *  See the document "Nest Weave-Data Management Protocol" document for
 *  a complete(ish) description.
 *
 */

#ifndef _WEAVE_DATA_MANAGEMENT_DM_CONSTANTS_LEGACY_H
#define _WEAVE_DATA_MANAGEMENT_DM_CONSTANTS_LEGACY_H

#include <Weave/Profiles/data-management/Legacy/WdmManagedNamespace.h>

namespace nl {
namespace Weave {
namespace Profiles {
namespace WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Legacy) {

    /**
     *  @brief
     *    WDM pool and table sizes.
     *
     *  WDM defines various pools and tables that are needed for its
     *  operation, as follows.
     *
     *  Client transaction pools:
     *
     *  - kViewPoolSize
     *  - kSubscribePoolSize
     *  - kCancelSubscriptionPoolSize
     *  - kUpdatePoolSize
     *
     *  Publisher transaction pools:
     *
     *  - kNotifyPoolSize
     *
     *  Protocol engine tables:
     *
     *  - kTransactionTableSize
     *  - kBindingTableSize
     *
     *  Subscription-related tables:
     *
     *  - kNotifierTableSize (client only)
     *  - kSubscriptionMgrTableSize (publisher only)
     *
     *  Note that these are configurable quantities and that the
     *  per-platform values appear in the associated WeaveConfig.h.
     */

    enum
    {
        /**
         *
         *   This is the default size of the view request transaction
         *   pool for a WDM client. This value may be configured via
         *   #WEAVE_CONFIG_WDM_VIEW_POOL_SIZE.
         *
         */
        kViewPoolSize                                = WEAVE_CONFIG_WDM_VIEW_POOL_SIZE,

        /**
         *   This is the default size of the subscribe request
         *   transaction pool for a WDM client. This value may be
         *   configured via #WEAVE_CONFIG_WDM_SUBSCRIBE_POOL_SIZE.
         *
         */
        kSubscribePoolSize                           = WEAVE_CONFIG_WDM_SUBSCRIBE_POOL_SIZE,

        /**
         *   This is the default size of the cancel subscription
         *   request transaction pool for a WDM client. This value may
         *   be configured via
         *   #WEAVE_CONFIG_WDM_CANCEL_SUBSCRIPTION_POOL_SIZE.
         *
         */
        kCancelSubscriptionPoolSize                  = WEAVE_CONFIG_WDM_CANCEL_SUBSCRIPTION_POOL_SIZE,

        /**
         *   This is the default size of the update request
         *   transaction pool for a WDM client. This value may be
         *   configured via #WEAVE_CONFIG_WDM_UPDATE_POOL_SIZE.
         *
         */
        kUpdatePoolSize                              = WEAVE_CONFIG_WDM_UPDATE_POOL_SIZE,

        /**
         *   This is the default size of the notify request
         *   transaction pool for a WDM publisher. This value may be
         *   configured via #WEAVE_CONFIG_WDM_NOTIFY_POOL_SIZE.
         *
         */
        kNotifyPoolSize                              = WEAVE_CONFIG_WDM_NOTIFY_POOL_SIZE,

        /**
         *   This is the default size of the transaction table in the
         *   WDM protocol engine. This value may be configured via
         *   #WEAVE_CONFIG_WDM_TRANSACTION_TABLE_SIZE.
         *
         */
        kTransactionTableSize                        = WEAVE_CONFIG_WDM_TRANSACTION_TABLE_SIZE,

        /**
         *   This is the default size of the binding table in the WDM
         *   protocol engine. This value may be configured via
         *   #WEAVE_CONFIG_WDM_BINDING_TABLE_SIZE.
         *
         */
        kBindingTableSize                            = WEAVE_CONFIG_WDM_BINDING_TABLE_SIZE,

        /**
         *   This is the default size of the notification table for
         *   WDM clients. This value may be configured via
         *   #WEAVE_CONFIG_WDM_NOTIFIER_TABLE_SIZE.
         *
         */
        kNotifierTableSize                           = WEAVE_CONFIG_WDM_NOTIFIER_TABLE_SIZE,

        /**
         *   This is the default size of the subscription table for
         *   WDM publishers. This value may be configured via
         *   #WEAVE_CONFIG_WDM_SUBSCRIPTION_MGR_TABLE_SIZE.
         *
         */
        kSubscriptionMgrTableSize                    = WEAVE_CONFIG_WDM_SUBSCRIPTION_MGR_TABLE_SIZE
    };

    /**
     *  @brief
     *    The WDM profile message types.
     *
     *  These values are called out in the data management specification.
     *
     *  NOTE!! As of Q1 2015, the message types used in previous
     *  versions of WDM have been deprecated and new message types have
     *  been defined, reflecting a sufficient shift with past packing
     *  and parsing details to justify a clean break.
     */

    enum
    {
        /**
         *  View request message
         */
        kMsgType_ViewRequest                          = 0x10,

        /**
         *  View response message
         */
        kMsgType_ViewResponse                         = 0x11,

        /**
         *  Subscribe request message
         */
        kMsgType_SubscribeRequest                     = 0x12,

        /**
         *  Subscribe response message
         */
        kMsgType_SubscribeResponse                    = 0x13,

        /**
         *  Cancel subscription request message
         */
        kMsgType_CancelSubscriptionRequest            = 0x14,

        /**
         *  Update request message
         */
        kMsgType_UpdateRequest                        = 0x15,

        /**
         *  Notify request message
         */
        kMsgType_NotifyRequest                        = 0x16,

        kMsgType_ViewRequest_Deprecated               = 0x00,   ///< deprecated
        kMsgType_ViewResponse_Deprecated              = 0x01,   ///< deprecated
        kMsgType_SubscribeRequest_Deprecated          = 0x02,   ///< deprecated
        kMsgType_SubscribeResponse_Deprecated         = 0x03,   ///< deprecated
        kMsgType_CancelSubscriptionRequest_Deprecated = 0x04,   ///< deprecated
        kMsgType_UpdateRequest_Deprecated             = 0x05,   ///< deprecated
        kMsgType_NotifyRequest_Deprecated             = 0x06    ///< deprecated
    };

    /**
     *  @brief
     *    WDM transport options
     *
     *  These are  mutually exclusive transport options for WDM.
     *
     */

    enum WeaveTransportOption
    {
        /**
         *  The underlying transport is TCP. The binding may be completed either
         *  using the service manager or directly using the message layer.
         */
        kTransport_TCP                                = 1,

        /**
         *  The underlying transport is exclusively UDP
         *  but with "application support layer" reliability enhancements.
         */
        kTransport_WRMP                               = 2,

        /**
         *  The underlying transport is exclusively UDP.
         */
        kTransport_UDP                                = 3
    };

    /**
     *  @brief
     *    Miscellaneous WDM-specific constants.
     */

    enum
    {
        /**
         *  In methods and data structures that take a transaction ID, indicates "none"
         *  or a wild-card value.
         */
        kTransactionIdNotSpecified                    = 0,

        /**
         *  In methods requesting a transaction, indicates that the caller has
         *  declined to specify a timeout. Generally this means a default should be used.
         */
        kResponseTimeoutNotSpecified                  = 0,

        /**
         *  In calls requiring the specification of a data version, indicates
         *  "no particular version".
         */
        kVersionNotSpecified                          = 0xFFFFFFFFFFFFFFFFULL,

        /**
         *  In calls requiring a profile instance specification, indicates "none'.
         *  Most often this means that only one instance of the profile is
         *  present on the entity in question.
         */
        kInstanceIdNotSpecified                       = 0,

        /**
         *  The index of the default binding in a protocol engine binding table
         *  with more than one entry.
         */
        kDefaultBindingTableIndex                     = 0,

        /**
         *  The standard length in bytes of a full-qualified TLV tag,
         *  used in support methods that encode WDM structures in TLV.
         */
        kWeaveTLVTagLen                               = 8,

        /**
         *  The length of a TLV control byte, used in support methods that encode
         *  WDM structures in TLV.
         */
        kWeaveTLVControlByteLen                       = 1,
    };

    /**
     *  @brief
     *    WDM-specific status codes.
     *
     */

    enum
    {
        /**
         *  This status code means a subscription was successfully canceled.
         */
        kStatus_CancelSuccess                         = 0x0001,

        /**
         *  This status code means a path from the path list of a view or
         *  update request frame did not match the node-resident schema
         *  of the responder.
         */
        kStatus_InvalidPath                           = 0x0013,

        /**
         *  This status code means the topic identifier given in a cancel
         *  request or notification did not match any subscription extant
         *  on the receiving node.
         */
        kStatus_UnknownTopic                          = 0x0014,

        /**
         *  This status code means the node making a request to read
         *  a particular data item does not have permission to do so.
         */
        kStatus_IllegalReadRequest                    = 0x0015,

        /**
         *  This status code means the node making a request to
         *  write a particular data item does not have permission to do so.
         */
        kStatus_IllegalWriteRequest                   = 0x0016,

        /**
         *  This status code means the version for data included in an
         *  update request did not match with the most recent version on the
         *  publisher and so the update could not be applied.
         */
        kStatus_InvalidVersion                        = 0x0017,

        /**
         *  This status code means the requested mode of
         *  subscription is not supported by the the receiving device.
         */
        kStatus_UnsupportedSubscriptionMode           = 0x0018,
    };

    /**
     *  @brief
     *    Data Management Protocol Tags
     *
     *  The data management protocol defines a number of tags to be
     *  used in the TLV representation of profile data.
     *
     *  As usual there are compatibility issues between new WDM and
     *  old. in the bad old days, all of these tags were applied as
     *  profile-specific, which was a waste of space.  now we are
     *  using context tags where possible but we need to keep the
     *  old ones around (and have a mechanism for encoding paths
     *  with them in place) where appropriate.
     *
     *  The kTag_WDMDataListElementData tag was not used in in previous
     *  releases and was completely ignored by the code, so we don't
     *  have to provide a deprecated version.
     *
     */

    enum
    {
        /**
         *  The element is a list of TLV paths.
         *
         *  Tag Type: Profile-specific
         *  Element Type: Array
         *  Disposition: Top-level
         */
        kTag_WDMPathList                              = 100,

        /**
         *  The element is a structure that is used
         *  to start a path and contains the profile information in light
         *  of which the tags in the path are to be interpreted.
         *
         *  Tag Type: Profile-specific
         *  Element Type: Structure
         *  Disposition: Required
         */
        kTag_WDMPathProfile                           = 101,

        /**
         *  The element is a profile ID component
         *  of the path profile element that begins a TLV path.
         *
         *  Tag Type: Context-specific
         *  Element Type: Integer
         *  Disposition: Required
         */
        kTag_WDMPathProfileId                         = 1,

        /**
         *  The element is a profile
         *  instance, which may follow the profile ID in a TLV path. Note
         *  that a node may or may not have multiple instances of a
         *  particular profile and, in the case where there is only one,
         *  this element may be omitted.
         *
         *  Tag Type: Context-specific
         *  Element Type: Any
         *  Disposition: Optional
         */
        kTag_WDMPathProfileInstance                   = 2,

        /**
         *  Deprecated
         *
         *  Tag Type: Profile-specific
         *  Element Type: Integer
         *  Disposition: Required
         */
        kTag_WDMPathProfileId_Deprecated              = 102,

        /**
         *  Deprecated
         *
         *  Tag Type: Profile-specific
         *  Element Type: Any
         *  Disposition: Optional
         */
        kTag_WDMPathProfileInstance_Deprecated        = 103,

        /**
         *  The path element corresponds
         *  to an array in the schema and the contained integer element is
         *  to be used as an index into that array.
         *
         *  Tag Type: Profile-specific
         *  Element Type: Integer
         *  Disposition: Optional
         */
        kTag_WDMPathArrayIndexSelector                = 104,

        /**
         *  The path element corresponds to
         *  an array in the schema and the encapsulated element is to be
         *  used as a record selector.
         *
         *  Tag Type: Profile-specific
         *  Element Type: Structure
         *  Disposition: Optional
         */
        kTag_WDMPathArrayValueSelector                = 105,

        /**
         *  The element is a list of structures
         *  containing path, optional version and data elements.
         *
         *  Tag Type: Profile-specific
         *  Element Type: Array
         *  Disposition: Top-level
         */
        kTag_WDMDataList                              = 200,

        /**
         *  The element is the path component of a data list element.
         *
         *  Tag Type: Context-specific
         *  Element Type: Path
         *  Disposition: Required
         */
        kTag_WDMDataListElementPath                   = 3,

        /**
         *  The element is the version component of a data list element.
         *
         *  Tag Type: Context-specific
         *  Element Type: Integer
         *  Disposition: Required
         */
        kTag_WDMDataListElementVersion                = 4,

        /**
         *  The element represents the data pointed at by given path
         *  and having the given version.
         *
         *  Tag Type: Context-specific
         *  Element Type: Any
         *  Disposition: Required
         */
        kTag_WDMDataListElementData                   = 5,

        /**
         *  Deprecated
         *
         *  Tag Type: Profile-specific
         *  Element Type: Path
         *  Disposition: Required
         */
        kTag_WDMDataListElementPath_Deprecated        = 201,

        /**
         *  Deprecated
         *
         *  Tag Type: Profile-specific
         *  Element Type: Integer
         *  Disposition: Required
         */
        kTag_WDMDataListElementVersion_Deprecated     = 202,

        /**
         *  Deprecated
         *
         *  Tag Type: Context-specific
         *  Element Type: Any
         *  Disposition: Required
         */
        kTag_WDMDataListElementData_Deprecated        = 203
    };

}; // WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Legacy)
}; // Profiles
}; // Weave
}; // nl

#endif // _WEAVE_DATA_MANAGEMENT_DM_CONSTANTS_LEGACY_H
