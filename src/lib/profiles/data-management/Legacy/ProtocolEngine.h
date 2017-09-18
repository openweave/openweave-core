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
 *  @file
 *
 *  @brief
 *    Definitions for the ProtocolEngine class.
 *
 *  This file defines common methods and callbacks for the WDM
 *  ProtocoleEngine, most applicable to both client and publisher. See
 *  the document, "Nest Weave-Data Management Protocol" document for a
 *  complete description.
 *
 *  ProtocolEngine is not, in itself, part of the publish interface to
 *  WDM but it provides the basis of that interface.
 */

#ifndef _WEAVE_DATA_MANAGEMENT_PROTOCOL_ENGINE_LEGACY_H
#define _WEAVE_DATA_MANAGEMENT_PROTOCOL_ENGINE_LEGACY_H

#include <Weave/Profiles/data-management/Legacy/WdmManagedNamespace.h>
#include <SystemLayer/SystemStats.h>

namespace nl {
namespace Weave {
namespace Profiles {
namespace WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Legacy) {

    /*
     * and various entities need to be able to send a status report
     */

    WEAVE_ERROR SendStatusReport(ExchangeContext *aExchangeCtx, StatusReport &aStatus);

    /**
     *  @class ProtocolEngine
     *
     *  @brief
     *    The WDM protocol engine class.
     *
     *  A data management entity, client or publisher, has a protocol
     *  engine component and a data manager component. This abstract
     *  class represents the common features of the protocol engine.
     */

    class ProtocolEngine
    {
        friend class Binding;
        friend class ClientNotifier;
        friend class DMClient;
        friend class DMPublisher;

    public:
        /**
         *  @class Transaction
         *
         *  @brief
         *    A protocol transaction containing application state.
         *
         *  Both client and publisher define transactions that may take
         *  a while to complete and require multiple steps,
         *  e.g. connection establishment, data marshaling etc. These
         *  maintain necessary application state for the duration.
         *
         *  Note that transactions, even though they are made public
         *  for access reasons, are really only used in the context of
         *  the protocol engine and are not part of the public
         *  interface to WDM in any significant sense.
         */

        class DMTransaction
        {
            friend class ProtocolEngine;
            friend class DMClient;
            friend class DMPublisher;
        public:

            void OnMsgReceived(const uint64_t &aResponderId, uint32_t aProfileId, uint8_t aMsgType, PacketBuffer *aMsg);

            /*
             * subclass-defined "special sauce"
             */

            virtual WEAVE_ERROR SendRequest(PacketBuffer *aBuffer, uint16_t aSendFlags) = 0;

            virtual WEAVE_ERROR OnResponseReceived(const uint64_t &aResponderId, uint8_t aMsgType, PacketBuffer *aMsg);

            virtual void OnResponseTimeout(const uint64_t &aResponderid);

            virtual WEAVE_ERROR OnStatusReceived(const uint64_t &aResponderId, StatusReport &aStatus) = 0;

        protected:

            WEAVE_ERROR Init(ProtocolEngine *aEngine, uint16_t aTxnId, uint32_t aTimeout);

            virtual void Free(void);

            bool IsFree(void);

            WEAVE_ERROR Start(uint8_t aTransport);

            void Finish(void);

            WEAVE_ERROR Finalize(void);

            void OnError(const uint64_t &aResponderId, WEAVE_ERROR aError);

            uint16_t              mTxnId;
            ProtocolEngine       *mEngine;
            uint32_t              mTimeout;
            ExchangeContext      *mExchangeCtx;

#if WEAVE_CONFIG_WDM_ALLOW_CLIENT_LEGACY_MESSAGE_TYPES

            /*
             * this is declared here and accessible only from friend
             * classes by which it may be used to select the old-style
             * message types.
             */

            bool                    mUseLegacyMsgType;

#endif // WEAVE_CONFIG_WDM_ALLOW_CLIENT_LEGACY_MESSAGE_TYPES
        };

        ProtocolEngine(void);

        virtual ~ProtocolEngine(void);

        virtual WEAVE_ERROR Init(WeaveExchangeManager *aExchangeMgr);

        virtual WEAVE_ERROR Init(WeaveExchangeManager *aExchangeMgr, uint32_t aResponseTimeout);

    protected:

        virtual void Finalize(void);

        void Clear(void);

        /*
         *  the procedure for sending a status response is the same all over
         */

        inline WEAVE_ERROR StatusResponse(ExchangeContext *aExchangeCtx, StatusReport &aStatus)
        {
            return SendStatusReport(aExchangeCtx, aStatus);
        }

    public:
        /*
         * the public portion of the ProtocolEngine API has to do with
         * the care adn feeding of the binding table.
         */

        WEAVE_ERROR BindRequest(const uint64_t &aPeerNodeId, uint8_t aTransport);

        /**
         *  @brief
         *    Bind to a known peer using the default transport.
         *
         *  @param [in]     aPeerNodeId     A reference to the 64-bit
         *                                  node ID of the peer entity
         *                                  that is the binding target.
         *
         *  @return #WEAVE_NO_ERROR On success. Otherwise return a
         *  #WEAVE_ERROR reflecting the failure of the bind operation.
         */

        inline WEAVE_ERROR BindRequest(const uint64_t &aPeerNodeId)
        {
#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
            return BindRequest(aPeerNodeId, kTransport_WRMP);
#else
            return BindRequest(aPeerNodeId, kTransport_TCP);
#endif
        };

#if WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
        WEAVE_ERROR BindRequest(nl::Weave::Profiles::ServiceDirectory::WeaveServiceManager *aServiceMgr, WeaveAuthMode aAuthMode);
#endif

        WEAVE_ERROR BindRequest(WeaveConnection *aConnection);

        virtual WEAVE_ERROR BindConfirm(Binding *aBinding);

        virtual WEAVE_ERROR BindConfirm(Binding *aBinding, StatusReport &aReport);

        void UnbindRequest(const uint64_t &aPeerNodeId);

        void UnbindRequest(const uint64_t &aPeerNodeId, WEAVE_ERROR aErr);

        virtual void IncompleteIndication(Binding *aBinding, StatusReport &aReport);

        /**
         *  @brief
         *    Handle an indication that a binding has become
         *    incomplete.
         *
         *  Higher layers that want to be informed of binding failure
         *  should use this method, which simply passes the peer ID
         *  along with a status report. In fact, since this method is
         *  virtual void, any DMClient or DMPublisher subclass must
         *  provide an implementation.
         *
         *  @param [in]     aPeerNodeId     A reference to the 64-bit
         *                                  ID of the peer node or
         *                                  service endpoint that is
         *                                  that target of the failed
         *                                  binding.
         *
         *  @param [in]     aReport         A reference to a StatusReport
         *                                  object detailing the
         *                                  reason for failure.
         */

        virtual void IncompleteIndication(const uint64_t &aPeerNodeId, StatusReport &aReport) = 0;

    protected:

        Binding *FromExchangeCtx(ExchangeContext *aExchangeCtx);

        Binding *GetBinding(const uint64_t &aPeerNodeId);

        Binding *NewBinding(void);

        void ClearBindingTable(void);

        void FinalizeBindingTable(void);

        /*
         * the protocol engine has to keep a queue of transactions
         * pending the completion of a binding, including its default
         * binding. these methods manage it and transactions generally.
         */

        WEAVE_ERROR StartTransaction(DMTransaction *aTransaction, Binding *aBinding);

        WEAVE_ERROR StartTransaction(DMTransaction *aTransaction)
        {
            return StartTransaction(aTransaction, &mBindingTable[kDefaultBindingTableIndex]);
        };

        WEAVE_ERROR EnqueueTransaction(DMTransaction *aTransaction, Binding *aBinding);

        inline WEAVE_ERROR EnqueueTransaction(DMTransaction *aTransaction)
        {
            return EnqueueTransaction(aTransaction, &mBindingTable[kDefaultBindingTableIndex]);
        };

        void DequeueTransaction(DMTransaction *aTransaction);

        void FinalizeTransactions(Binding *aBinding);

        bool FailTransactions(Binding *aBinding, StatusReport &aReport);

        void ClearTransactionTable(void);

        void FinalizeTransactionTable(void);

        // data members

        WeaveExchangeManager *mExchangeMgr;
        uint32_t              mResponseTimeout;   // milliseconds, 0 == "don't wait"

        /**
         * The ProtocolEngine has a binding table that, if the engine
         * is going to be responsible for anything beyond simply
         * receiving broadcast notifications, probably needs to
         * contain at least one entry. Bindings are generally indexed
         * by node ID. What this means is that each engine can only
         * have a single binding to a given service endpoint.
         *
         * The WDM specification has this notion of "default binding" which is
         * the place messages go if no explicit destination is
         * supplied. This will mostly be used in very simple devices
         * with a single binding or a small number of bindings and,
         * for other purposes, will just be the first binding formed.
         */

        Binding               mBindingTable[kBindingTableSize];

        /*
         * the ProtocolEngine object also keeps track of at least one
         * pending transaction, which may be awaiting the completion
         * of a binding. when the binding completes, all the pending
         * transactions that depend on it are started, and when the
         * binding fails they are all canceled.
         */

        struct TransactionTableEntry
        {
            inline void Init(DMTransaction *aTransaction, Binding *aBinding)
            {
                mTransaction = aTransaction;
                mBinding = aBinding;
            };

            inline void Free(void)
            {
                mTransaction = NULL;
                mBinding = NULL;

                SYSTEM_STATS_DECREMENT(nl::Weave::System::Stats::kWDMClient_NumTransactions);
            };

            inline bool IsFree(void)
            {
                return !mTransaction;
            };

            void Finalize(void);

            void Fail(const uint64_t &aPeerId, StatusReport &aReport);

            DMTransaction    *mTransaction;
            Binding          *mBinding;
        };

        TransactionTableEntry mTransactionTable[kTransactionTableSize];
    };

}; // WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Legacy)
}; // Profiles
}; // Weave
}; // nl

#endif // _WEAVE_DATA_MANAGEMENT_PROTOCOL_ENGINE_LEGACY_H
