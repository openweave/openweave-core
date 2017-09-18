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
 *    Definitions for the Binding class.
 *
 *  This file defines common methods and callbacks for the WDM
 *  Binding class, which is responsible for keeping track of state
 *  required to communicate with a particular device.
 *
 *  Binding is not, in itself, part of the published interface to WDM
 *  but it provides the basis for portions of that interface.
 */


#ifndef _WEAVE_DATA_MANAGEMENT_BINDING_LEGACY_H
#define _WEAVE_DATA_MANAGEMENT_BINDING_LEGACY_H

#include <Weave/Profiles/data-management/Legacy/WdmManagedNamespace.h>
#include <Weave/Profiles/data-management/DMConstants.h>

namespace nl {
namespace Weave {
namespace Profiles {
namespace WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Legacy) {

#if WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
    using ServiceDirectory::WeaveServiceManager;
#endif

    class ProtocolEngine;

    /**
     *  @class Binding
     *
     *  @brief
     *    The Binding class manages communications state on behalf of
     *    an application entity using Weave.
     *
     *  When an application wants to use Weave to communicate with a
     *  remote entity there exists a wide variety of options. The
     *  Binding class corrals these options and arranges them in such
     *  a way that the easy stuff is easy and the more difficult stuff
     *  is at least tractable. Options covered include:
     *
     *  - unicast UDP communication with a known peer node.
     *
     *  - UDP broadcast with "any" node.
     *
     *  - unicast WRMP communication with a known peer node.
     *
     *  - TCP communications with a known peer node.
     *
     *  - TCP communications with a known service endpoint using a
     *    service manager instance to set things up.
     *
     *  - TCP communications based on a pre-established connection.
     */

    class Binding
    {

        friend class ProtocolEngine;

    public:

        Binding(void);

        virtual ~Binding(void);

        /**
         *  @brief
         *    Initialize a Binding with just a node ID.
         *
         *  This results in a binding with the configured default
         *  transport.
         *
         *  @param [in]     aPeerNodeId     A reference to the 64-bit
         *                                  ID of the binding target.
         *
         *  @retval #WEAVE_NO_ERROR On success.
         *
         *  @retval #WEAVE_ERROR_INVALID_ARGUMENT If the binding is
         *  under-specified.
         */

        inline WEAVE_ERROR Init(const uint64_t &aPeerNodeId)
        {
#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
            return Init(aPeerNodeId, kTransport_WRMP);
#else
            return Init(aPeerNodeId, kTransport_TCP);
#endif
        }

        WEAVE_ERROR Init(const uint64_t &aPeerNodeId, uint8_t aTransport);

#if WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
        WEAVE_ERROR Init(const uint64_t &aServiceEpt,
                         WeaveServiceManager *aServiceMgr,
                         WeaveAuthMode aAuthMode);
#endif

        WEAVE_ERROR Init(WeaveConnection *aConnection);

        WEAVE_ERROR Connect(WeaveConnection *aConnection);

        void Finalize(void);

        void Finalize(WEAVE_ERROR aErr);

        void Free(void);

        bool IsFree(void);

        WEAVE_ERROR CompleteRequest(ProtocolEngine *aEngine);

        void CompleteConfirm(WeaveConnection *aConnection);

        void CompleteConfirm(StatusReport &aReport);

        void CompleteConfirm(void);

        void UncompleteRequest(void);

        void UncompleteRequest(WEAVE_ERROR aErr);

        void IncompleteIndication(StatusReport &aReport);

        /**
         *  @brief
         *    Check if a binding is complete.
         *
         *  @note
         *    "Complete", in the case of a binding is tantamount to
         *    "ready for use". Thus, at this point, UDP-based bindings
         *    are always complete at initialization time as are
         *    TCP-based bindings that are initialized with a
         *    previously completed Weave connection. Bindings that
         *    depend on TCP and, especially, that bind to service
         *    endpoints are not, generally speaking, complete at
         *    initialization time and must go through a multi-stage
         *    process, as described elsewhere, before they are
         *    complete.
         *
         *  @sa CompleteRequest(ProtocolEngine *aEngine)
         *
         *  @return true if it's complete, false otherwise.
         */

        inline bool IsComplete(void)
        {
            return mState == kState_Complete;
        }

        ExchangeContext *GetExchangeCtx(WeaveExchangeManager *aExchangeMgr, void *aAppState);

        /*
         * There are the various attributes governing communication
         * via the exchange layer. For TCP-based communication, all
         * you really need is a connection but there are various ways
         * to get at that. For UDP/WRMP even less is required -
         * really, just a node ID or other address.
         *
         * NOTE!! These member should be treated as READ-ONLY.
         */

        /**
         *  @brief
         *    The 64-bit node ID of the binding target. (READ-ONLY)
         *
         *  Every Binding has a target entity, which is named here. In
         *  addition to a Weave node ID this may name a service
         *  endpoint.
         */

        uint64_t                                                    mPeerNodeId;

        /**
         *  @brief
         *    The transport to use in completing this Binding.
         *    (READ-ONLY)
         *
         *  Possible values for mTransport are defined in DMConstants.h.
         */

        uint8_t                                                     mTransport;

#if WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
        /**
         *  @brief
         *    A pointer to the (optional) ServiceManager object to use
         *    in completing this binding. (READ-ONLY)
         *
         *  When binding to the Weave service, a 64-bit service
         *  endpoint ID may be supplied at initialization time in
         *  place of a Weave node ID. In this case, a ServiceManager
         *  object is also required to complete the binding. Normal
         *  TCP or WRMP bindings do not require a ServiceManager
         *  object.
         */

        nl::Weave::Profiles::ServiceDirectory::WeaveServiceManager *mServiceMgr;
#endif

        /**
         *  @brief
         *    The Weave authentication mode to be used. (READ_ONLY)
         *
         *  This is the authentication mode used in all communications
         *  governed by this binding.
         */

        WeaveAuthMode                                               mAuthMode;

        /**
         *  @brief
         *    A pointer to the Weave connection currently in use in
         *    this binding. (READ-ONLY)
         *
         *  TCP bindings may be initialized with a connection right off
         *  the bat or they may allocate one at completion time.
         *
         *  @note
         *    Although it is permissible to read, it is an error and
         *    could cause unexpected results to modify the value of
         *    mConnection directly.
         */

        WeaveConnection                                            *mConnection;

        /**
         *  @brief
         *    A pointer to the ProtocolEngine object related to
         *    this Binding.
         *
         *  A binding is generally completed with respect to a
         *  particular protocol engine, which is mostly used as a way
         *  of accessing the MessageLayer. This is where we keep track
         *  of it.
         */

        ProtocolEngine                                             *mEngine;

        /**
         *  @brief
         *    The set of Binding object states.
         */

        enum
        {
            kState_Incomplete                                     = 0, /**< The initial (and final) state of a Binding. */
            kState_Completing                                     = 1, /**< The state of a Binding that is in the process of being completed */
            kState_Complete                                       = 2  /**< The state of a Binding that is complete and ready for use. */
        };

        /**
         *  @brief
         *    The current Binding object state.
         *
         *  Only one "complete" operation can run at a time and, in any
         *  case, if you ask to complete a completed binding it just
         *  calls the confirm function immediately. The state variable
         *  below tracks the current state and acts as a lockout.
         */

        uint8_t                                                     mState;
    };

}; // WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Legacy)
}; // Profiles
}; // Weave
}; // nl

#endif // _WEAVE_DATA_MANAGEMENT_BINDING_LEGACY_H
