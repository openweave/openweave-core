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
 *    Impementations for the ProtocolEngine class.
 *
 *  This file implements common methods and callbacks for the WDM
 *  ProtocoleEngine, most applicable to both client and publisher.
 *
 *  ProtocolEngine is not, in itself, part of the publish interface to
 *  WDM but it provides the basis of that interface.
 */

#include <Weave/Profiles/data-management/Legacy/WdmManagedNamespace.h>

#include <Weave/Support/CodeUtils.h>

#include <Weave/Profiles/data-management/DataManagement.h>
#include <SystemLayer/SystemStats.h>

using namespace ::nl::Inet;

using namespace ::nl::Weave::TLV;
using namespace ::nl::Weave::Profiles;
using namespace ::nl::Weave::Profiles::Common;
using namespace ::nl::Weave::Profiles::DataManagement;

#if WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
using namespace ::nl::Weave::Profiles::ServiceDirectory;
#endif

using namespace ::nl::Weave::Profiles::StatusReporting;

using nl::Weave::WeaveConnection;
using nl::Weave::ExchangeContext;
using nl::Weave::ExchangeMgr;
using nl::Weave::WeaveMessageInfo;

static void TxnResponseHandler(ExchangeContext *anExchangeCtx,
                               const IPPacketInfo *anAddrInfo,
                               const WeaveMessageInfo *aMsgInfo,
                               uint32_t aProfileId,
                               uint8_t aMsgType,
                               PacketBuffer *aMsg)
{
    ProtocolEngine::DMTransaction *txn = static_cast<ProtocolEngine::DMTransaction *>(anExchangeCtx->AppState);

    txn->OnMsgReceived(aMsgInfo->SourceNodeId, aProfileId, aMsgType, aMsg);
}

static void TxnTimeoutHandler(ExchangeContext *anExchangeCtx)
{
    ProtocolEngine::DMTransaction *txn = static_cast<ProtocolEngine::DMTransaction *>(anExchangeCtx->AppState);

    txn->OnResponseTimeout(anExchangeCtx->PeerNodeId);
}

WEAVE_ERROR DataManagement::SendStatusReport(ExchangeContext *aExchangeCtx, StatusReport &aStatus)
{
    WEAVE_ERROR err;
    PacketBuffer *buf = PacketBuffer::New();
    uint16_t sendFlags = 0;

    VerifyOrExit(buf != NULL, err = WEAVE_ERROR_NO_MEMORY);

    err = aStatus.pack(buf);
    SuccessOrExit(err);

#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
    if (aExchangeCtx->HasPeerRequestedAck())
    {
        sendFlags = ExchangeContext::kSendFlag_RequestAck;
    }
#endif

    err = aExchangeCtx->SendMessage(kWeaveProfile_Common, kMsgType_StatusReport,
                                    buf, sendFlags);
    buf = NULL;
    SuccessOrExit(err);

exit:

    if (buf)
    {
        PacketBuffer::Free(buf);
    }

    return err;
}

void ProtocolEngine::DMTransaction::OnMsgReceived(const uint64_t &aResponderId,
                                                  uint32_t aProfileId,
                                                  uint8_t aMsgType,
                                                  PacketBuffer *aMsg)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    StatusReport report;

    if (aProfileId == kWeaveProfile_StatusReport_Deprecated || (aProfileId == kWeaveProfile_Common && aMsgType == kMsgType_StatusReport))
    {
        err = StatusReport::parse(aMsg, report);
        SuccessOrExit(err);

        if (report.mProfileId == kWeaveProfile_Common && report.mStatusCode == kStatus_Relocated)
        {
#if WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
            /*
             * here, the message is, specifically, a status report
             * from the service explaining that the client needs
             * to go find another service tier. so, we unresolve
             * the service directory in preparation for whatever
             * comes next.
             */

            Binding *svcBinding = mEngine->GetBinding(kServiceEndpoint_Data_Management);

            VerifyOrExit(svcBinding && svcBinding->mServiceMgr, err = WEAVE_ERROR_INCORRECT_STATE);

            svcBinding->mServiceMgr->unresolve();
#endif
        }

        OnStatusReceived(aResponderId, report);
    }

    else if (aProfileId == kWeaveProfile_WDM || aProfileId == kWeaveProfile_Common)
    {
        OnResponseReceived(aResponderId, aMsgType, aMsg);
    }

    else
    {
        err = WEAVE_ERROR_INVALID_PROFILE_ID;
    }

exit:

    PacketBuffer::Free(aMsg);

    if (err != WEAVE_NO_ERROR)
        OnError(aResponderId, err);
}

/*
 * the default here is that we don't receive a response.  for
 * transactions in which a response is expected, this method is
 * overridden elsewhere.
 */

WEAVE_ERROR ProtocolEngine::DMTransaction::OnResponseReceived(const uint64_t &aResponderId, uint8_t aMsgType, PacketBuffer *aMsg)
{
    OnError(aResponderId, WEAVE_ERROR_INVALID_MESSAGE_TYPE);

    return WEAVE_ERROR_INVALID_MESSAGE_TYPE;
}

void ProtocolEngine::DMTransaction::OnResponseTimeout(const uint64_t &aResponderId)
{
   OnError(aResponderId, WEAVE_ERROR_TIMEOUT);
}


WEAVE_ERROR ProtocolEngine::DMTransaction::Init(ProtocolEngine *aEngine, uint16_t aTxnId, uint32_t aTimeout)
{
    mEngine = aEngine;
    mTxnId = aTxnId;
    mTimeout = aTimeout;

    mExchangeCtx = NULL;

#if WEAVE_CONFIG_WDM_ALLOW_CLIENT_LEGACY_MESSAGE_TYPES

    mUseLegacyMsgType = false;

#endif // WEAVE_CONFIG_WDM_ALLOW_CLIENT_LEGACY_MESSAGE_TYPES

    return WEAVE_NO_ERROR;
}

void ProtocolEngine::DMTransaction::Free(void)
{
    mEngine = NULL;
    mTxnId = kTransactionIdNotSpecified;
    mTimeout = kResponseTimeoutNotSpecified;
    mExchangeCtx = NULL;

#if WEAVE_CONFIG_WDM_ALLOW_CLIENT_LEGACY_MESSAGE_TYPES

    mUseLegacyMsgType = false;

#endif // WEAVE_CONFIG_WDM_ALLOW_CLIENT_LEGACY_MESSAGE_TYPES
}

bool ProtocolEngine::DMTransaction::IsFree(void)
{
    return mEngine == NULL;
}

WEAVE_ERROR ProtocolEngine::DMTransaction::Start(uint8_t aTransport)
{
    WEAVE_ERROR     err = WEAVE_NO_ERROR;
    PacketBuffer*   buf = PacketBuffer::New();

    uint16_t flags = ExchangeContext::kSendFlag_ExpectResponse;

    VerifyOrExit(buf, err = WEAVE_ERROR_NO_MEMORY);

    mExchangeCtx->ResponseTimeout = mTimeout;
    mExchangeCtx->OnMessageReceived = TxnResponseHandler;
    mExchangeCtx->OnResponseTimeout = TxnTimeoutHandler;

#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
    if (aTransport == kTransport_WRMP)
        flags |= ExchangeContext::kSendFlag_RequestAck;
#endif

    err = SendRequest(buf, flags);
    buf = NULL;

exit:

    if (buf)
    {
        PacketBuffer::Free(buf);
    }

    return err;
}

WEAVE_ERROR ProtocolEngine::DMTransaction::Finalize(void)
{
    if (!IsFree())
    {
        if (mEngine)
        {
            mEngine->DequeueTransaction(this);
        }

        if (mExchangeCtx)
            mExchangeCtx->Close();

        Free();
    }

    return WEAVE_NO_ERROR;
}

void ProtocolEngine::DMTransaction::OnError(const uint64_t &aResponderId, WEAVE_ERROR aError)
{
    StatusReport report;
    report.init(aError);

    OnStatusReceived(aResponderId, report);
};

ProtocolEngine::ProtocolEngine(void)
{
    Clear();
}

ProtocolEngine::~ProtocolEngine(void)
{
    Finalize();
}

WEAVE_ERROR ProtocolEngine::Init(WeaveExchangeManager *aExchangeMgr)
{
    return Init(aExchangeMgr, kResponseTimeoutNotSpecified);
}

WEAVE_ERROR ProtocolEngine::Init(WeaveExchangeManager *aExchangeMgr, uint32_t aResponseTimeout)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    if (mExchangeMgr != NULL)
    {
        err =  WEAVE_ERROR_INCORRECT_STATE;
    }

    else if (aExchangeMgr == NULL)
    {
        err = WEAVE_ERROR_INVALID_ARGUMENT;
    }

    else
    {
        mExchangeMgr = aExchangeMgr;
        mResponseTimeout = aResponseTimeout;
    }

    return err;
}

void ProtocolEngine::Finalize(void)
{
    FinalizeBindingTable();

    FinalizeTransactionTable();

    Clear();
}

void ProtocolEngine::Clear(void)
{
    mExchangeMgr = NULL;
    mResponseTimeout = kResponseTimeoutNotSpecified;

    ClearBindingTable();

    ClearTransactionTable();
}

/**
 *  @brief
 *    Request a binding using a known peer node ID and transport
 *    specifier.
 *
 *  Given a peer node ID and a transport specification this request sets up
 *  a binding to that peer. The biding will require additional completion
 *  ONLY if the transport is TCP. If a binding to the peer is already present,
 *  it is re-used.
 *
 *  @sa #WeaveTransportOption
 *
 *  @param [in]     aPeerNodeId     A reference to the 64-bit node ID
 *                                  of the peer entity that is the
 *                                  binding target.
 *
 *  @param [in]      aTransport      The transport to use.
 *
 *  @return #WEAVE_NO_ERROR on success or #WEAVE_ERROR_NO_MEMORY if
 *  the binding table is full. Otherwise return a #WEAVE_ERROR
 *  reflecting a failure to initialize the binding.
 */

WEAVE_ERROR ProtocolEngine::BindRequest(const uint64_t &aPeerNodeId, uint8_t aTransport)
{
    WEAVE_ERROR err = WEAVE_ERROR_NO_MEMORY;
    Binding *b;

    /*
     * if a binding is already there then re-use it. it is, i think,
     * way too complicated to maintain bindings with different
     * protocols to the sam enode so we're just not going to sdupport
     * it unless someone can think of a real use case for it.
     */

    b = GetBinding(aPeerNodeId);

    if (b)
    {
        err = WEAVE_NO_ERROR;
    }

    else
    {
        b = NewBinding();

        if (b)
            err = b->Init(aPeerNodeId, aTransport);
    }

    return err;
}

#if WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
/**
 *  @brief
 *    Request a binding to the Weave service's WDM endpoint.
 *
 *  Often devices will want to engage in WDM exchanges with the Weave
 *  service. Bindings established in this way must be completed using
 *  the service manager.
 *
 *  @param [in]     aServiceMgr     A pointer to a service manager
 *                                  instance to be used in subsequent
 *                                  communications with the Weave
 *                                  service.
 *
 *  @param [in]     aAuthMode       The authentication mode to employ
 *                                   in making requests to the service.
 *
 *  @return #WEAVE_NO_ERROR on success or #WEAVE_ERROR_NO_MEMORY if
 *  the binding table is full. Otherwise return a #WEAVE_ERROR
 *  reflecting a failure to initialize the binding.
 */

WEAVE_ERROR ProtocolEngine::BindRequest(WeaveServiceManager *aServiceMgr, WeaveAuthMode aAuthMode)
{
    WEAVE_ERROR err = WEAVE_ERROR_NO_MEMORY;
    Binding *b;

    /*
     * if a binding is already there then re-use it.
     */

    b = GetBinding(kServiceEndpoint_Data_Management);

    if (b)
        err = WEAVE_NO_ERROR;

    else
    {
        b = NewBinding();

        if (b)
            err = b->Init(kServiceEndpoint_Data_Management, aServiceMgr, aAuthMode);
    }

    return err;
}
#endif // WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY

/**
 *  @brief
 *    Request a binding using an active Weave connection.
 *
 *  A binding may also be established using an existing, and open,
 *  connection. Note that bindings that are established in this way
 *  require no additional completion.
 *
 *  @param [in]     aConnection     A Pointer to a Weave connection to
 *                                  be used by the binding.
 *
 *  @return #WEAVE_NO_ERROR on success or #WEAVE_ERROR_NO_MEMORY if
 *  the binding table is full. Otherwise return an error reflecting a
 *  failure to inititialize the binding.
 */

WEAVE_ERROR ProtocolEngine::BindRequest(WeaveConnection *aConnection)
{
    WEAVE_ERROR err = WEAVE_ERROR_NO_MEMORY;
    Binding *b;

    /*
     * if a binding is already there then re-use it.
     */

    b = GetBinding(aConnection->PeerNodeId);

    if (b)
        err = WEAVE_NO_ERROR;

    else
    {
        b = NewBinding();

        if (b)
            err = b->Init(aConnection);
    }

    return err;
}

/**
 *  @brief
 *    Handle confirmation that a bind request has been successfully
 *    completed.
 *
 *  Once a binding has been completed, the protocol engine goes through
 *  the transaction table and starts any transactions that are dependent
 *  on that binding.
 *
 *  @param [in]     aBinding        A pointer to the completed binding.
 *
 *  @return #WEAVE_NO_ERROR On success. Otherwise return a
 *  #WEAVE_ERROR reflecting an inability to start a transaction.
 */

WEAVE_ERROR ProtocolEngine::BindConfirm(Binding *aBinding)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    TransactionTableEntry *item;
    DMTransaction *txn;
    ExchangeContext *ctx = NULL;

    for (int i = 0; i < kTransactionTableSize; i++)
    {
        item = &(mTransactionTable[i]);
        txn = item->mTransaction;

        if (txn && item->mBinding == aBinding)
        {

            ctx = aBinding->GetExchangeCtx(mExchangeMgr, txn);
            VerifyOrExit(ctx, err = WEAVE_ERROR_NO_MEMORY);

            /*
             * assign the exchange context here ONLY if you actually
             * managed to make one.
             */

            txn->mExchangeCtx = ctx;

            err = txn->Start(aBinding->mTransport);
            SuccessOrExit(err);
        }
    }

exit:

    if (err != WEAVE_NO_ERROR)
    {
        if (!item->IsFree())
            item->Free();

        if (ctx && !txn->IsFree())
            txn->OnError(aBinding->mPeerNodeId, err);
    }

    return err;
}

/**
 *  @brief
 *    Handle confirmation that a bind request has failed.
 *
 *  When a bind request fails, the protocol engine must go through the
 *  transaction table and fail any transactions depending on the
 *  binding.
 *
 *  @param [in]     aBinding        A pointer to the failed binding.
 *
 *  @param [in]     aReport         A reference to a StatusReport
 *                                  object detailing the reason for
 *                                  failure.
 *
 *  @return #WEAVE_NO_ERROR.
 */

WEAVE_ERROR ProtocolEngine::BindConfirm(Binding *aBinding, StatusReport &aReport)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    FailTransactions(aBinding, aReport);

    return err;
}

/**
 *  @brief
 *    Request that a binding be undone and removed from the binding
 *    table.
 *
 *  When a binding is "unbound" any transactions that currently depend
 *  on it should be removed as well. This method finalizes
 *  all transactions with this binding automatically.
 *
 *  @param[in] aPeerNodeId A reference to the 64-bit node ID or
 *    service endpoint that identifies the binding.
 *
 *  @param[in] aErr WEAVE_NO_ERROR if there is no specific reason for this
 *    unbind request, otherwise the cause of error would be passed down.
 *
 *  @sa UnbindRequest(const uint64_t)
 */

void ProtocolEngine::UnbindRequest(const uint64_t &aPeerNodeId, WEAVE_ERROR aErr)
{
    for (int i = 0; i < kBindingTableSize; i++)
    {
        Binding &b = mBindingTable[i];

        if (b.mPeerNodeId == aPeerNodeId)
        {
            FinalizeTransactions(&b);

            b.Finalize(aErr);

            break;
        }
    }
}

/**
 *  @brief
 *    Request that a binding be undone and removed from the binding
 *    table.
 *
 *  When a binding is "unbound" any transactions that currently depend
 *  on it should be removed as well. This method finalizes
 *  all transactions with this binding automatically.
 *
 *  @param[in] aPeerNodeId A reference to the 64-bit node ID or
 *    service endpoint that identifies the binding.
 *
 *  @sa UnbindRequest(const uint64_t, WEAVE_ERROR)
 */

void ProtocolEngine::UnbindRequest(const uint64_t &aPeerNodeId)
{
    UnbindRequest(aPeerNodeId, WEAVE_NO_ERROR);
}

/**
 * @brief
 *   Handle an indication that a binding has failed.
 *
 *  When a binding becomes incomplete, i.e. when the connection is
 *  closed for a TCP binding, the protocol engine must fail any
 *  transactions that depend on it, which includes calling their status
 *  handlers. Also, the incomplete indication is passed up to any
 *  superclass object implementing the alternate form of this method
 *  that takes a peer ID.
 *
 *  @param [in]     aBinding        A pointer to the failed binding.
 *
 *  @param [in]     aReport         A reference to a StatusReport
 *                                  object detailing the reason for
 *                                  failure.
 */

void ProtocolEngine::IncompleteIndication(Binding *aBinding, StatusReport &aReport)
{
    bool indicated = FailTransactions(aBinding, aReport);

    if (!indicated)
        IncompleteIndication(aBinding->mPeerNodeId, aReport);
}

Binding *ProtocolEngine::FromExchangeCtx(ExchangeContext *aExchangeCtx)
{
    WeaveConnection *con = aExchangeCtx->Con;
    uint64_t peerId = aExchangeCtx->PeerNodeId;
    Binding *binding = GetBinding(peerId);

    if (!binding)
    {
        binding = NewBinding();

        if (binding)
        {
            if (con)
            {
                binding->Init(con);
            }

            else
            {
                /*
                 * the problem is that there's not really a good way to tell
                 * if you're using WRMP. so... for now i'm using the protocol
                 * version.
                 */
#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
                if (aExchangeCtx->mMsgProtocolVersion == kWeaveMessageVersion_V2)
                {
                    binding->Init(peerId, kTransport_WRMP);
                }
                else
#endif
                {
                    binding->Init(peerId, kTransport_UDP);
                }
            }
        }
    }

    return binding;
}

Binding *ProtocolEngine::GetBinding(const uint64_t &aPeerNodeId)
{
    Binding *retVal = NULL;

    if (aPeerNodeId != kNodeIdNotSpecified)
    {
        for (int i = 0; i < kBindingTableSize; i++)
        {
            Binding &b = mBindingTable[i];

            if (b.mPeerNodeId == aPeerNodeId)
            {
                retVal = &b;

                break;
            }
        }
    }

    return retVal;
}

Binding *ProtocolEngine::NewBinding(void)
{
    Binding *retVal = NULL;

    for (int i = 0; i < kBindingTableSize; i++)
    {
        Binding &b = mBindingTable[i];

        if (b.IsFree())
        {
            retVal = &b;

            SYSTEM_STATS_INCREMENT(nl::Weave::System::Stats::kWDMClient_NumBindings);

            break;
        }
    }

    return retVal;
}

void ProtocolEngine::ClearBindingTable(void)
{
    for (int i = 0; i < kBindingTableSize; i++)
        mBindingTable[i].Free();

    SYSTEM_STATS_RESET(nl::Weave::System::Stats::kWDMClient_NumBindings);
}

void ProtocolEngine::FinalizeBindingTable(void)
{
    for (int i = 0; i < kBindingTableSize; i++)
    {
        Binding &binding = mBindingTable[i];

        binding.Finalize();
    }

    SYSTEM_STATS_RESET(nl::Weave::System::Stats::kWDMClient_NumBindings);
}

WEAVE_ERROR ProtocolEngine::StartTransaction(DMTransaction *aTransaction, Binding *aBinding)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    ExchangeContext *ctx = NULL;

    VerifyOrExit(!aBinding->IsFree(), err = WEAVE_ERROR_INCORRECT_STATE);

    err = EnqueueTransaction(aTransaction, aBinding);
    SuccessOrExit(err);

    if (aBinding->IsComplete())
    {
        ctx = aBinding->GetExchangeCtx(mExchangeMgr, aTransaction);
        VerifyOrExit(ctx, err = WEAVE_ERROR_NO_MEMORY);

        /*
         * assign the exchange context here ONLY if you actually
         * managed to make one.
         */

        aTransaction->mExchangeCtx = ctx;

        err = aTransaction->Start(aBinding->mTransport);
    }

    else
    {
        err = aBinding->CompleteRequest(this);
    }

exit:

    /*
     * the reason we just call Finalize() here is to avoid, in some
     * cases, double-invocation of the higher-layer error handling
     * code, BUT, it is possible for this to happen anyway so, be
     * prepared for it.
     */

    if (err != WEAVE_NO_ERROR)
        aTransaction->Finalize();

    return err;
}

WEAVE_ERROR ProtocolEngine::EnqueueTransaction(DMTransaction *aTxn, Binding *aBinding)
{
    WEAVE_ERROR err = WEAVE_ERROR_NO_MEMORY;

    for (int i = 0; i < kTransactionTableSize; i++)
    {
        TransactionTableEntry &entry = mTransactionTable[i];

        if (entry.IsFree())
        {
            err = WEAVE_NO_ERROR;

            entry.Init(aTxn, aBinding);

            SYSTEM_STATS_INCREMENT(nl::Weave::System::Stats::kWDMClient_NumTransactions);

            break;
        }
    }

    return err;
}

void ProtocolEngine::DequeueTransaction(DMTransaction *aTransaction)
{
    for (int i = 0; i < kTransactionTableSize; i++)
    {
        TransactionTableEntry &entry = mTransactionTable[i];

        if (entry.mTransaction == aTransaction)
        {
            entry.Free();

            break;
        }
    }
}

void ProtocolEngine::FinalizeTransactions(Binding *aBinding)
{
    for (int i = 0; i < kTransactionTableSize; i++)
    {
        TransactionTableEntry &entry = mTransactionTable[i];

        if (entry.mBinding == aBinding)
            entry.Finalize();
    }
}

bool ProtocolEngine::FailTransactions(Binding *aBinding, StatusReport &aReport)
{
    bool indicated = false;

    for (int i = 0; i < kTransactionTableSize; i++)
    {
        TransactionTableEntry &entry = mTransactionTable[i];

        if (!entry.IsFree() && entry.mBinding == aBinding)
        {
            indicated = true;

            entry.Fail(aBinding->mPeerNodeId, aReport);
        }
    }

    return indicated;
}

void ProtocolEngine::ClearTransactionTable(void)
{
    for (int i = 0; i < kTransactionTableSize; i++)

        mTransactionTable[i].Free();

    SYSTEM_STATS_RESET(nl::Weave::System::Stats::kWDMClient_NumTransactions);
}

void ProtocolEngine::FinalizeTransactionTable(void)
{
    for (int i = 0; i < kTransactionTableSize; i++)
    {
        TransactionTableEntry &entry = mTransactionTable[i];

        if (!entry.IsFree())
        {
            Binding *binding = entry.mBinding;

            entry.Finalize();

            binding->Finalize();
        }
    }
}

/*
 * transaction table entries are pretty simple but the methods below
 * encapsulate specific behaviors. in particular, Fail() fails the
 * transaction only on the theory that transaction handlers have a
 * privileged status. And, finalizing a transaction table entry
 * finalizes the transaction but preserves the binding since these are
 * intended to have a logner life-cycle.
 */

void ProtocolEngine::TransactionTableEntry::Finalize(void)
{
    if (!IsFree())
    {
        if (mTransaction->mExchangeCtx)
            mTransaction->mExchangeCtx->Close();

        mTransaction->Free();

        Free();
    }
}

void ProtocolEngine::TransactionTableEntry::Fail(const uint64_t &aPeerId, StatusReport &aReport)
{
    DMTransaction *txn = mTransaction;

    if (!IsFree())
    {
        Free();

        txn->OnStatusReceived(aPeerId, aReport);
    }
}
