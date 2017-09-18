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
 *    Implementations for the Binding class.
 *
 *  This file implements common methods and callbacks for the WDM
 *  Binding class, which is responsible for keeping track of state
 *  required to communicate with a particular device.
 *
 *  Binding is not, in itself, part of the published interface to WDM
 *  but it provides the basis for portions of that interface.
 */

#include <Weave/Profiles/data-management/Legacy/WdmManagedNamespace.h>
#include <Weave/Profiles/data-management/DataManagement.h>
#include <SystemLayer/SystemStats.h>

using namespace ::nl::Inet;
using namespace ::nl::Weave::TLV;
using namespace ::nl::Weave::Profiles;
using namespace ::nl::Weave::Profiles::Common;

#if WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
using namespace ::nl::Weave::Profiles::ServiceDirectory;
#endif

using namespace ::nl::Weave::Profiles::StatusReporting;
using namespace ::nl::Weave::Profiles::DataManagement;

using nl::Weave::WeaveConnection;
using nl::Weave::ExchangeContext;
using nl::Weave::ExchangeMgr;

/**
 *  @brief
 *    Handle the closure of a Weave connection.
 *
 *  When using TCP, either via the service manager or the ungarnished
 *  Weave message layer, the binding object is hung in the connection
 *  object pending completion and thereafter in case the connection is
 *  closed unexpectedly. When the connection is closed, the binding
 *  needs to be "incompleted" and the necessary cleanup carried
 *  out. This method shouldn't be installed in the connection until
 *  AFTER the connection is completed for the first time.
 *
 *  @param [in]     aConnection     A pointer to the WeaveConnection
 *                                  that has been closed.
 *
 *  @param [in]     aError          A #WEAVE_ERROR associated with the
 *                                  closure.
 */

void ConnectionClosedHandler(WeaveConnection *aConnection, WEAVE_ERROR aError)
{
    Binding *binding = static_cast<Binding *>(aConnection->AppState);
    StatusReport report;

    /*
     * we want to label an unexpected closure as such even if the Inet
     * layer thinks its OK.
     */

    if (aError == WEAVE_NO_ERROR)
        aError = WEAVE_ERROR_CONNECTION_CLOSED_UNEXPECTEDLY;

    report.init(aError);
    binding->IncompleteIndication(report);
}

/**
 *  @brief
 *    Handle the completion of a requested connection.
 *
 *  When a requested connection is completed on behalf of a TCP
 *  binding, whether successfully or unsuccessfully, this is handler
 *  that is called. As before, at this stage, the Binding object
 *  resides in the WeaveConnection's AppState member variable.
 *
 *  @param [in]     aConnection     A pointer to the WeaveConnection
 *                                  that has been completed.
 *
 *  @param [in]     aError          A #WEAVE_ERROR associated with the
 *                                  status of the connection. If this
 *                                  has a value of #WEAVE_NO_ERROR
 *                                  then the connection was completed
 *                                  successfully. Otherwise the
 *                                  connection failed.
 */

void ConnectionCompleteHandler(WeaveConnection *aConnection, WEAVE_ERROR aError)
{
    Binding *binding = static_cast<Binding *>(aConnection->AppState);

    /*
     * someone may have made an incomplete or unbind request while we
     * were waiting. in this case the state will be incomplete and we
     * want to dispose of the connection but NOT call any of the usual
     * handlers.
     */

    if (binding->mState == Binding::kState_Incomplete)
    {
        aConnection->Close();
    }

    /*
     * otherwise we want to call the appropriate handlers.
     */

    else
    {
        if (aError == WEAVE_NO_ERROR)
        {
            binding->CompleteConfirm(aConnection);
        }

        else
        {
            StatusReport report;
            report.init(aError);

            aConnection->Close();

            binding->CompleteConfirm(report);
        }
    }
}

#if WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
/**
 *  @brief
 *    Handle a service manager failure.
 *
 *  Service manager bindings are completed using a
 *  ServiceManager::connect() call. In case this fails, this is the
 *  handler that is called.
 *
 *  @param [in]     aAppState           A void pointer to the
 *                                      application state object
 *                                      supplied to the service
 *                                      manager, which in this case
 *                                      should be a Binding.
 *
 *  @param [in]     aError              If the failure was the result
 *                                      of an error on the client
 *                                      side, it will be provided
 *                                      here. Otherwise, this will
 *                                      have a value of
 *                                      #WEAVE_NO_ERROR.
 *
 *  @param [in]     aReport             If the error occurred during
 *                                      the service directory lookup,
 *                                      a pointer to a StatusReport
 *                                      object will be provided
 *                                      here. Otherwise, the value
 *                                      will be NULL.
 */

void ServiceMgrStatusHandler(void* aAppState, WEAVE_ERROR aError, StatusReport *aReport)
{
    Binding *binding = static_cast<Binding *>(aAppState);

    if (aReport)
    {
        binding->CompleteConfirm(*aReport);
    }

    else
    {
        StatusReport status;

        status.init(aError);
        binding->CompleteConfirm(status);
    }
}
#endif // WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY

/**
 *  @brief
 *    The default constructor for Binding objects.
 *
 *  Clears all internal state.
 */

Binding::Binding(void)
{
    Free();
}

/**
 *  @brief
 *    The destructor for Binding objects.
 *
 *  Clears all internal state AND, if necessary, closes open
 *  connections.
 */

Binding::~Binding(void)
{
    Finalize();
}

/**
 *  @brief
 *    Initialize a Binding object based on peer ID and transport.
 *
 *  @note
 *    Bindings initialized in this way, where the transport is UDP or
 *    WRMP, are "self-completing" in the sense that they may be used
 *    immediately. TCP bindings, specifially the Weave connection that
 *    underlies them, must be completed before use.
 *
 *  @param [in]     aPeerNodeId     A reference to the 64-bit node
 *                                  identifier of the binding target.
 *
 *  @param [in]     aTransport      The transport specification, from
 *                                  WeaveTransportOption.
 *
 *  @retval #WEAVE_NO_ERROR On success.
 *
 *  @retval #WEAVE_ERROR_INVALID_ARGUMENT If the binding is
 *  under-specified.
 */

WEAVE_ERROR Binding::Init(const uint64_t &aPeerNodeId, uint8_t aTransport)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    /*
     * Bindings must specify a node ID, and (if they specify any
     * protocol other than UDP) they must specify an actual,
     * unicast-able node as opposed to a broadcast address.
     */

    if (aPeerNodeId == kNodeIdNotSpecified || (aPeerNodeId == kAnyNodeId && (
#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
                                                   aTransport == kTransport_WRMP ||
#endif
                                                   aTransport == kTransport_TCP)))
    {

        err = WEAVE_ERROR_INVALID_ARGUMENT;
    }

    else
    {
        Finalize();

        mPeerNodeId = aPeerNodeId;
        mTransport = aTransport;

        if (aTransport == kTransport_TCP)
            mState = kState_Incomplete;

        else
            mState = kState_Complete;
    }

    return err;
}

#if WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
/**
 *  @brief
 *    Initialize a Binding object to a service endpoint.
 *
 *  This is how you bind to a particular endpoint on the Nest
 *  service. A binding of this kind requires a multi-stage completion
 *  process, which may include populating or updating the local service
 *  directory cache. For the most part, this process is hidden from
 *  the application but it means that errors arising later in the
 *  process may be delivered, normally via the relevant "confirm"
 *  callback, after - sometimes long after - the original request to
 *  use (and complete) the binding.
 *
 *  @param [in]     aServiceEpt     A reference to the 64-bit
 *                                  identifier for the the Weave
 *                                  Service endpoint of interest.
 *
 *  @param [in]     aServiceMgr     A pointer to the service manager
 *                                  instance to use in looking up a
 *                                  service tier and connecting to it.
 *
 *  @param [in]     aAuthMode       The authentication mode to use
 *                                  in connecting.
 *
 *  @retval #WEAVE_NO_ERROR On success.
 *
 *  @retval #WEAVE_ERROR_INVALID_ARGUMENT If the binding is
 *  under-specified.
 */

WEAVE_ERROR Binding::Init(const uint64_t &aServiceEpt, WeaveServiceManager *aServiceMgr, WeaveAuthMode aAuthMode)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    /*
     * in this case, again, you actually have to specify a real
     * service endpoint AND a non-null service manager.
     */

    if (!aServiceMgr || aServiceEpt == kNodeIdNotSpecified || aServiceEpt == kAnyNodeId)
    {
        err = WEAVE_ERROR_INVALID_ARGUMENT;
    }

    else
    {
        Finalize();

        mPeerNodeId = aServiceEpt;
        mServiceMgr = aServiceMgr;
        mAuthMode = aAuthMode;

        mTransport = kTransport_TCP;
        mState = kState_Incomplete;
    }

    return err;
}
#endif // WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY

/**
 *  @brief
 *    Initialize a Binding object with a WeaveConnection.
 *
 *  @note
 *    Like UDP-based peer node bindings, these are "self-completing"
 *    in the sense that they may be used immediately. This is because
 *    the Weave connection on which they are based is already
 *    complete.
 *
 *  @param [in]     aConnection         A pointer to a
 *                                      WeaveConnection to use as a
 *                                      basis for the binding..
 *
 *  @retval #WEAVE_NO_ERROR On success.
 *
 *  @retval #WEAVE_ERROR_INVALID_ARGUMENT If the binding is
 *  under-specified.
 */

WEAVE_ERROR Binding::Init(WeaveConnection *aConnection)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    if (!aConnection)
    {
        err = WEAVE_ERROR_INVALID_ARGUMENT;
    }

    else
    {
        Finalize();

        mTransport = kTransport_TCP;
        mConnection = aConnection;
        mPeerNodeId = aConnection->PeerNodeId;
        mState = kState_Complete;
    }

    return err;
}

/**
 *  @brief
 *    Complete a TCP binding by providing a completed connection.
 *
 *  A newly initialized TCP binding cannot be used until it has been
 *  completed. Normally this is done on demand when the application
 *  attempts to make use of the binding to send messages but it can
 *  also be explicitly completed by providing a Weave connection.
 *
 *  @param [in]     aConnection         A pointer to a
 *                                      WeaveConnection used to
 *                                      complete the binding.
 *
 *  @retval #WEAVE_NO_ERROR On success.
 *
 *  @retval #WEAVE_ERROR_INCORRECT_STATE If the binding already has a
 *  connection.
 *
 *  @retval #WEAVE_ERROR_INVALID_ARGUMENT If the connection is NULL.
 */

WEAVE_ERROR Binding::Connect(WeaveConnection *aConnection)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    if (mConnection || mTransport != kTransport_TCP)
    {
        err = WEAVE_ERROR_INCORRECT_STATE;
    }

    else if (aConnection)
    {
        aConnection->OnConnectionClosed = ConnectionClosedHandler;
        mConnection = aConnection;

        mPeerNodeId = aConnection->PeerNodeId;

        mState = kState_Complete;
    }

    else
    {
        err = WEAVE_ERROR_INVALID_ARGUMENT;
    }

    return err;
}

/**
 *  @brief
 *    "Uncomplete" a binding and free it.
 *
 *  Bindings may have state that requires cleanup, e.g. connection
 *  closure, which is handled by the Uncomplete() method in addition to
 *  state that is simply cleared to its initial state by the Free()
 *  method. This method, largely for the sake of convenience, invokes
 *  both.
 *
 *  @param [in]     aErr    This error code indicates the cause of this
 *    request. If not WEAVE_NO_ERROR, the TCP connection could be aborted.
 *
 *  @sa Finalize(void)
 */

void Binding::Finalize(WEAVE_ERROR aErr)
{
    if (!IsFree())
    {
        UncompleteRequest(aErr);

        Free();
    }
}

/**
 *  @brief
 *    "Uncomplete" a binding and free it.
 *
 *  Bindings may have state that requires cleanup, e.g. connection
 *  closure, which is handled by the Uncomplete() method in addition to
 *  state that is simply cleared to its initial state by the Free()
 *  method. This method, largely for the sake of convenience, invokes
 *  both.
 *
 *  @sa Finalize(WEAVE_ERROR)
 */

void Binding::Finalize(void)
{
    Finalize(WEAVE_NO_ERROR);
}

/**
 *  @brief
 *    Clear the binding state.
 *
 *  Unconditionally return all binding state to its original state.
 */

void Binding::Free(void)
{
    mPeerNodeId = kNodeIdNotSpecified;
#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
    mTransport = kTransport_WRMP;
#else
    mTransport = kTransport_TCP;
#endif

#if WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
    mServiceMgr = NULL;
#endif

    mAuthMode = kWeaveAuthMode_Unauthenticated;
    mConnection = NULL;

    mEngine = NULL;

    mState = kState_Incomplete;

    SYSTEM_STATS_DECREMENT(nl::Weave::System::Stats::kWDMClient_NumBindings);
}

/**
 *  @brief
 *    Check is a binding is free.
 *
 *  "Free" in this context simply means, "has a defined peer node
 *  ID". IsFree() should be thought of as meaning "has had Free()
 *  called on it and has not been used since".
 *
 *  @return true if the binding is free, false otherwise.
 */

bool Binding::IsFree(void)
{
    return (mPeerNodeId == kNodeIdNotSpecified);
}

/**
 *  @brief
 *    Request completion of a binding.
 *
 *  Completion of a binding is, at least for bindings requiring TCP,
 *  carried out with respect to a particular ProtocolEngine object,
 *  which provides access to an ExchangeManager instance.
 *
 *  @note
 *    Normally applications are not required to explicitly call
 *    CompleteRequest() since the it is invoked on demand by the
 *    underlying WDM code when the application tries to make use of
 *    the binding by sending a message. However, if the application
 *    wants to control when, for example, a connection is made, it may
 *    use this method.
 *
 *  @param [in]     aEngine             A pointer to a ProtocolEngine
 *                                      object on behalf of which the
 *                                      completion is being performed.
 *
 *  @retval #WEAVE_NO_ERROR On success.
 *
 *  @retval #WEAVE_ERROR_INCORRECT_STATE if the binding is already being
 *  completed.
 *
 *  @retval #WEAVE_ERROR_NO_MEMORY If a connection is required and
 *  none are available
 *
 *  @return Otherwise, any #WEAVE_ERROR returned while trying to
 *  connect.
 */

WEAVE_ERROR Binding::CompleteRequest(ProtocolEngine *aEngine)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    WeaveConnection *con;

    mEngine = aEngine;

    if (mState == kState_Complete)
    {
        WeaveLogProgress(DataManagement, "Binding::CompleteRequest() - mState == kState_Complete");

        CompleteConfirm();
    }

    else if (mState == kState_Completing)
    {
        WeaveLogProgress(DataManagement, "Binding::CompleteRequest() - mState == kState_Completing");
    }

    else
    {
        WeaveLogProgress(DataManagement, "Binding::CompleteRequest() - mState == kState_Incomplete");

#if WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
        if (mServiceMgr)
        {
            mState = kState_Completing;

            err = mServiceMgr->connect(mPeerNodeId, mAuthMode, this, ServiceMgrStatusHandler, ConnectionCompleteHandler);
        }

        else
#endif
        if (mTransport == kTransport_TCP)
        {
            con = aEngine->mExchangeMgr->MessageLayer->NewConnection();

            if (con == NULL)
            {
                err = WEAVE_ERROR_NO_MEMORY;
            }

            else
            {
                mState = kState_Completing;

                con->OnConnectionComplete = ConnectionCompleteHandler;
                con->AppState = this;

                err = con->Connect(mPeerNodeId);
            }
        }

        else
        {
            /*
             * for now, these are "auto-completing" this may
             * change when we add more security.
             */

            if (mPeerNodeId == kNodeIdNotSpecified)
                err = WEAVE_ERROR_INCORRECT_STATE;

            else
                mState = kState_Complete;
        }
    }

    return err;
}

/**
 *  @brief
 *    Handle confirmation of a bind request.
 *
 *  @param [in]     aConnection         A pointer to an active
 *                                      WeaveConnection to the binding
 *                                      target.
 */

void Binding::CompleteConfirm(WeaveConnection *aConnection)
{
    Connect(aConnection);

    CompleteConfirm();
}

/**
 *  @brief
 *    Handle the failure of a bind request.
 *
 *  @param [in]     aReport             A reference to a StatusReport
 *                                      object describing the failure.
 */

void Binding::CompleteConfirm(StatusReport &aReport)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    WeaveLogProgress(DataManagement, "Binding::CompleteConfirm() - failure %s, %s",
                     StatusReportStr(aReport.mProfileId, aReport.mStatusCode), ErrorStr(aReport.mError));

    /*
     * probably safest to put this here in case the higher layer code
     * clears and re-uses the binding.
     */

    UncompleteRequest(aReport.mError);

    if (mEngine)
        err = mEngine->BindConfirm(this, aReport);

    if (err != WEAVE_NO_ERROR)
        WeaveLogProgress(DataManagement, "ProtocolEngine::BindConfirm() => failure %s", ErrorStr(err));
}

/**
 *  @brief
 *    Handle confirmation of a bind request.
 */

void Binding::CompleteConfirm()
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    WeaveLogProgress(DataManagement, "Binding::CompleteConfirm() - success");

    if (mEngine)
        err = mEngine->BindConfirm(this);

    if (err != WEAVE_NO_ERROR)
        WeaveLogProgress(DataManagement, "ProtocolEngine::BindConfirm() => failure %s", ErrorStr(err));
}

/**
 *  @brief
 *    Cause a binding to be incomplete.
 *
 *  Fundamentally, a binding shall be in the "incomplete" state after
 *  this method has been called on it but, more subtly, any relevant
 *  state not contained in the binding itself, e.g. TCP connection,
 *  should be cleaned up as well. Applications may consider invoking
 *  UncompleteRequest() as part of the cleanup on error.
 *
 *  @param [in] aErr    If not WEAVE_NO_ERROR, the existing connection,
 *    if any, would be aborted instead of gracefully closed.
 *
 *  @sa UncompleteRequest(void)
 */

void Binding::UncompleteRequest(WEAVE_ERROR aErr)
{
    if (mConnection)
    {
        if (WEAVE_NO_ERROR == aErr)
        {
            mConnection->Close();
        }
        else
        {
            mConnection->Abort();
        }

        mConnection = NULL;
    }

    /*
     * OK. so if this is a service manager binding there's a chance
     * that a service directory transaction is pending. fix it.
     */

#if WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
    if (mServiceMgr)
        mServiceMgr->cancel(mPeerNodeId, this);
#endif

    mState = kState_Incomplete;
}

/**
 *  @brief
 *    Cause a binding to be incomplete.
 *
 *  Fundamentally, a binding shall be in the "incomplete" state after
 *  this method has been called on it but, more subtly, any relevant
 *  state not contained in the binding itself, e.g. TCP connection,
 *  should be cleaned up as well. Applications may consider invoking
 *  UncompleteRequest() as part of the cleanup on error.
 *
 *  @sa UncompleteRequest(WEAVE_ERROR)
 */

void Binding::UncompleteRequest()
{
    UncompleteRequest(WEAVE_NO_ERROR);
}

/**
 *  @brief
 *    Handle the failure of a binding.
 *
 *  This method is invoked and, in turn, invokes higher-layer handlers
 *  when the binding fails AFTER completion, i.e. after
 *  CompleteConfirm() has been invoked with a status denoting success.
 *
 *  @sa CompleteConfirm(StatusReport &aReport).
 *
 *  @param [in]     aReport             A referemce to a StatusReport
 *                                      describing what went wrong.
 */

void Binding::IncompleteIndication(StatusReport &aReport)
{
    WeaveLogProgress(DataManagement, "Binding::IncompleteIndication() - %s", ErrorStr(aReport.mError));

    /*
     * probably safest to put this here in case the hegher layer code
     * clears and re-uses the binding.
     */

    UncompleteRequest(aReport.mError);

    if (mEngine)
        mEngine->IncompleteIndication(this, aReport);
}

/**
 *  @brief
 *    Produce an ExchangeContext object from a Binding.
 *
 *  @param [in]     aExchangeMgr        A pointer to the the exchange
 *                                      manager from which to request
 *                                      a context.
 *
 *  @param [in]     aAppState           A void pointer to an
 *                                      application state object to be
 *                                      stored in the exchange context
 *                                      for later use.
 *
 *  @return a pointer to an ExchangeContext object, or NULL on
 *  failure.
 */

ExchangeContext *Binding::GetExchangeCtx(WeaveExchangeManager *aExchangeMgr, void *aAppState)
{
    ExchangeContext *retVal = NULL;

    if (mConnection)
        retVal = aExchangeMgr->NewContext(mConnection, aAppState);

    else if (mPeerNodeId && mTransport != kTransport_TCP)
        retVal = aExchangeMgr->NewContext(mPeerNodeId, aAppState);

    return retVal;
}
