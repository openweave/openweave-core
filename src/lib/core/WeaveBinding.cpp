/*
 *
 *    Copyright (c) 2016-2018 Nest Labs, Inc.
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
 *      Implementation of Weave Binding and related classes.
 *
 */

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif // __STDC_FORMAT_MACROS

#include <Weave/Core/WeaveCore.h>
#include <Weave/Support/CodeUtils.h>
#include <Weave/Support/WeaveFaultInjection.h>
#include <SystemLayer/SystemStats.h>

namespace nl {
namespace Weave {

/**
 * @fn Binding::State Binding::GetState(void) const
 *
 * Retrieve the current state of the binding.
 *
 * @return                          The binding state.
 */

/**
 * @fn bool Binding::IsPreparing() const
 *
 * @return                          True if the Binding is currently being prepared.
 */

/**
 * @fn bool Binding::IsReady() const
 *
 * @return                          True if the Binding is in the Ready state.
 */

/**
 * @fn uint64_t Binding::GetPeerNodeId() const
 *
 * Retrieve the node id of the binding peer.
 *
 * Only valid once the binding object has been prepared.
 *
 * @return                          Weave node ID of the peer
 */

/**
 * @fn void Binding::GetPeerIPAddress(nl::Inet::IPAddress & address, uint16_t & port, InterfaceId & interfaceId) const
 *
 * Retrieve the IP address information for the peer, if available.
 *
 * The availability of the peer's IP address information depends on the state and configuration of the binding.
 * IP address information is only available when using an IP-based transport (TCP, UDP, or UDP with WRMP).  Prior to
 * the start of preparation, address information is only available if it has been set expressly by the application
 * during configuration.  During the preparation phase, address information is available when address preparation
 * completes (e.g. after DNS resolution has completed).  After the Binding is ready, address information remains
 * available until the Binding is reset.
 *
 * @param[out]   address            A reference to an IPAddress object that will receive the peer's IP address.
 *                                  If the peer's IP address information is unavailable, this value will be set
 *                                  to IPAddress::Any.
 * @param[out]   port               A reference to an integer that will receive the peer's port number.
 *                                  If the peer's IP address information is unavailable, this value is undefined.
 * @param[out]   interfaceId        A reference to an integer that will receive the id of the network interface
 *                                  via which the peer can be reached.  If the peer's IP address information is
 *                                  unavailable, this value is undefined.
 */
/**
 * @fn uint32_t Binding::GetKeyId() const
 *
 * Retrieve the id of the message encryption key to be used when encrypting messages to/from to the peer.
 */

/**
 * @fn uint8_t Binding::GetEncryptionType() const
 *
 * Retrieve the message encryption type to be used when encrypting messages to/from the peer.
 */

/**
 * @fn uint32_t Binding::GetDefaultResponseTimeout() const
 *
 * Get the default exchange response timeout to be used when communicating with the peer.
 *
 * @return                          Response timeout in ms.
 */

/**
 * @fn void Binding::SetDefaultResponseTimeout(uint32_t timeout)
 *
 * Set the default exchange response timeout to be used when communicating with the peer.
 *
 * @param[in] timeout               The new response timeout in ms.
 */

/**
 * @fn const WRMPConfig& Binding::GetDefaultWRMPConfig(void) const
 *
 * Get the default WRMP configuration to be used when communicating with the peer.
 *
 * @return                          A reference to a WRMPConfig structure containing
 *                                  the default configuration values.
 */

/**
 * @fn void Binding::SetDefaultWRMPConfig(const WRMPConfig& aWRMPConfig)
 *
 * Set the default WRMP configuration to be used when communicating with the peer.
 *
 * @param[in] aWRMPConfig           A reference to a WRMPConfig structure containing
 *                                  the new default configuration.
 */

/**
 * @fn Binding::EventCallback Binding::GetEventCallback() const
 *
 * Get the function that will be called when an API event occurs for the Binding.
 *
 * @return                          A pointer to the callback function.
 */

/**
 * @fn Binding::SetEventCallback(EventCallback aEventCallback)
 *
 * Set the application-defined function to be called when an API event occurs for the Binding.
 *
 * @param[in] aEventCallback        A pointer to the callback function.
 */

/**
 * @fn void Binding::SetProtocolLayerCallback(EventCallback callback, void *state)
 *
 * Set an event callback function for protocol layer code using the Binding on behalf of an
 * application. This function will be called in addition to the application-defined callback
 * function when API events occur for the Binding.
 *
 * @param[in] callback              A pointer to the callback function.
 * @param[in] state                 A pointer to a state object that will be supplied to the
 *                                  protocol layer code when a protocol layer callback occurs.
 */

/**
 * @fn WeaveConnection *Binding::GetConnection() const
 *
 * Get the Weave connection object associated with the binding.
 *
 * @return                          A pointer to a WeaveConnection object, or NULL if there is
 *                                  no connection associated with the binding.
 */

/**
 * @fn Binding::Configuration Binding::BeginConfiguration()
 *
 * Being the process of configuring the Binding.  Applications must call BeginConfiguration() to
 * configure the Binding prior to preparing it for communicating with the peer.
 *
 * @return                          A Binding::Configuration object that can be used to configure
 *                                  the binding.
 */

/**
 * @fn WEAVE_ERROR Binding::Configuration::PrepareBinding(void)
 *
 * Being the process of preparing the Binding for communication with the peer.
 */

/**
 * @fn WEAVE_ERROR Binding::Configuration::GetError(void) const
 *
 * Return any error that has occurred while configuring the Binding.
 */

/**
 * Reserve a reference to the binding object.
 */
void Binding::AddRef()
{
    VerifyOrDie(mState != kState_NotAllocated);
    VerifyOrDie(mRefCount > 0);

    ++mRefCount;
}

/**
 *  Release a reference to the binding object.
 *
 *  If there are no more references to the binding object, the binding is closed and freed.
 */
void Binding::Release()
{
    VerifyOrDie(mState != kState_NotAllocated);
    VerifyOrDie(mRefCount > 0);

    if (mRefCount > 1)
    {
        --mRefCount;
    }
    else
    {
        DoClose();
        mRefCount = 0;
        WeaveLogDetail(ExchangeManager, "Binding[%" PRIu8 "] (%" PRIu16 "): Freed", GetLogId(), mRefCount);
        mExchangeManager->FreeBinding(this);
    }
}

/**
 * Close the binding object and release a reference.
 *
 * When called, this method causes the binding to enter the Closed state.  Any in-progress prepare actions
 * for the binding are canceled and all external communications resources held by the binding are released.
 *
 * Calling Close() decrements the reference count associated with the binding, freeing the object if the
 * reference count becomes zero.
 */
void Binding::Close(void)
{
    VerifyOrDie(mState != kState_NotAllocated);
    VerifyOrDie(mRefCount > 0);

    DoClose();
    Release();
}

/**
 * Reset the binding back to an unconfigured state.
 *
 * When Reset() is called, any in-progress prepare actions for the binding are canceled and all external
 * communications resources held by the binding are released.  Reset() places the binding in the
 * Unconfigured state, after which it may be configured and prepared again.
 *
 * Reset() does not alter the reference count of the binding.
 */
void Binding::Reset()
{
    VerifyOrDie(mState != kState_NotAllocated);
    VerifyOrDie(mRefCount > 0);

    DoReset(kState_NotConfigured);

    WeaveLogDetail(ExchangeManager, "Binding[%" PRIu8 "] (%" PRIu16 "): Reset", GetLogId(), mRefCount);
}

/**
 * Get a unique id for the binding.
 */
uint16_t Binding::GetLogId(void) const
{
    return mExchangeManager->GetBindingLogId(this);
}

/**
 *  Default handler for binding API events.
 *
 *  Applications are required to call this method for any API events that they don't recognize or handle.
 *  Supplied parameters must be the same as those passed by the binding to the application's event handler
 *  function.
 *
 *  @param[in]  apAppState  A pointer to application-defined state information associated with the binding.
 *  @param[in]  aEvent      Event ID passed by the event callback
 *  @param[in]  aInParam    Reference of input event parameters passed by the event callback
 *  @param[in]  aOutParam   Reference of output event parameters passed by the event callback
 *
 */
void Binding::DefaultEventHandler(void *apAppState, EventType aEvent, const InEventParam& aInParam, OutEventParam& aOutParam)
{
    // No actions required for current implementation
    aOutParam.DefaultHandlerCalled = true;
}

/**
 *  Initialize this Binding object
 *
 *  @param[in]  apAppState      A pointer to some context which would be carried in event callback later
 *  @param[in]  aEventCallback  A function pointer to be used for event callback
 *
 */
WEAVE_ERROR Binding::Init(void *apAppState, EventCallback aEventCallback)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    VerifyOrExit(aEventCallback != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);

    mState = kState_NotConfigured;
    mRefCount = 1;
    AppState = apAppState;
    SetEventCallback(aEventCallback);
    mProtocolLayerCallback = NULL;
    mProtocolLayerState = NULL;

    ResetConfig();

    WeaveLogDetail(ExchangeManager, "Binding[%" PRIu8 "] (%" PRIu16 "): Allocated", GetLogId(), mRefCount);

#if DEBUG
    // Verify that the application's event callback function correctly calls the default handler.
    //
    // NOTE: If your code receives WEAVE_ERROR_DEFAULT_EVENT_HANDLER_NOT_CALLED it means that the event hander
    // function you supplied for a binding does not properly call Binding::DefaultEventHandler for unrecognized/
    // unhandled events.
    //
    {
        InEventParam inParam;
        OutEventParam outParam;
        inParam.Clear();
        inParam.Source = this;
        outParam.Clear();
        aEventCallback(apAppState, kEvent_DefaultCheck, inParam, outParam);
        VerifyOrExit(outParam.DefaultHandlerCalled, err = WEAVE_ERROR_DEFAULT_EVENT_HANDLER_NOT_CALLED);
    }
#endif

exit:
    if (err != WEAVE_NO_ERROR)
    {
        mState = kState_NotAllocated;
        mRefCount = 0;
        WeaveLogDetail(ExchangeManager, "Binding[%" PRIu8 "] (%" PRIu16 "): Freed", GetLogId(), mRefCount);
    }
    WeaveLogFunctError(err);
    return err;
}

/**
 * Reset the state of the binding, canceling any outstanding activities and releasing all external resources.
 */
void Binding::DoReset(State newState)
{
    VerifyOrDie(mState != kState_NotAllocated);

    WeaveSecurityManager *sm = mExchangeManager->MessageLayer->SecurityMgr;
    State origState = mState;

    // Temporarily enter the resetting state.  This has the effect of suppressing any callbacks
    // from lower layers that might result from the effort of resetting the binding.
    mState = kState_Resetting;

    // Release any reservation held on the message encryption key.  In the case of
    // locally-initiated, non-shared session keys, this will result in the session
    // being removed.
    if (GetFlag(kFlag_KeyReserved))
    {
        sm->ReleaseKey(mPeerNodeId, mKeyId);
        ClearFlag(kFlag_KeyReserved);
    }

#if WEAVE_CONFIG_ENABLE_DNS_RESOLVER

    // If host name resolution is in progress, cancel it.
    if (origState == kState_PreparingAddress_ResolveHostName)
    {
        mExchangeManager->MessageLayer->Inet->CancelResolveHostAddress(OnResolveComplete, this);
    }

#endif

    // Release the reference to the connection object, if held.  Block any callback to our
    // connection complete handler that may result from releasing the connection.
    if (GetFlag(kFlag_ConnectionReferenced))
    {
        mCon->OnConnectionComplete = NULL;
        mCon->Release();
        ClearFlag(kFlag_ConnectionReferenced);
    }
    mCon = NULL;

    // If a session establishment was in progress, cancel it.
    if (origState == kState_PreparingSecurity_EstablishSession)
    {
        sm->CancelSessionEstablishment(this);
    }

    // Reset the configuration state of the binding, except when entering the Failed state.
    //
    // We leave the configuration state of the binding intact in the Failed state so that
    // applications can inspected it during failure handling.  If the application decides
    // to re-prepare the bind, the configuration state will be reset when binding enters
    // the Configuring state.
    if (newState != kState_Failed)
    {
        ResetConfig();
    }

    // Advance to the new state.
    mState = newState;
}

/**
 * Transition the binding to the Closed state if not already closed.
 */
void Binding::DoClose(void)
{
    // If not already closed...
    if (mState != kState_Closed)
    {
        // Clear pointers to application state/code to prevent any further use.
        AppState = NULL;
        SetEventCallback(NULL);
        SetProtocolLayerCallback(NULL, NULL);

        // Reset the binding and enter the Closed state.
        DoReset(kState_Closed);

        WeaveLogDetail(ExchangeManager, "Binding[%" PRIu8 "] (%" PRIu16 "): Closed", GetLogId(), mRefCount);
    }
}

/**
 * Reset the configuration parameters to their default values.
 */
void Binding::ResetConfig()
{
    mPeerNodeId = kNodeIdNotSpecified;

    mAddressingOption = kAddressing_NotSpecified;
    mPeerAddress = nl::Inet::IPAddress::Any;
    mPeerPort = WEAVE_PORT;
    mInterfaceId = INET_NULL_INTERFACEID;
    mHostName = NULL;

    mCon = NULL;

    mTransportOption = kTransport_NotSpecified;
    mDefaultResponseTimeoutMsec = 0;
#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
    mDefaultWRMPConfig = gDefaultWRMPConfig;
#endif
    mUDPPathMTU = WEAVE_CONFIG_DEFAULT_UDP_MTU_SIZE;

    mSecurityOption = kSecurityOption_NotSpecified;
    mKeyId = WeaveKeyId::kNone;
    mEncType = kWeaveEncryptionType_None;
    mAuthMode = kWeaveAuthMode_Unauthenticated;

    mFlags = 0;

#if WEAVE_CONFIG_ENABLE_DNS_RESOLVER
    mDNSOptions = ::nl::Inet::kDNSOption_Default;
#endif
}

/**
 * Request the application to configure and prepare the Binding.
 *
 * Protocol layer code can use this method on a Binding that has not been configured, or
 * has failed, to trigger an event to the application (kEvent_PrepareRequested) requesting
 * that it configure and prepare the binding for use.
 *
 * This method can only be called on Bindings in the NotConfigured or Failed states.
 *
 * If the application does not support on-demand configuration/preparation of Bindings, the
 * method will fail with WEAVE_ERROR_NOT_IMPLEMENTED.
 *
 */
WEAVE_ERROR Binding::RequestPrepare()
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    InEventParam inParam;
    OutEventParam outParam;

    // Ensure the binding doesn't get freed while we make calls to the application.
    AddRef();

    // Make sure the binding is in a state where preparing is possible.
    VerifyOrExit(CanBePrepared(), err = WEAVE_ERROR_INCORRECT_STATE);

    inParam.Clear();
    inParam.Source = this;
    outParam.Clear();
    outParam.PrepareRequested.PrepareError = WEAVE_NO_ERROR;

    // Invoke the application to configure and prepare the binding.  Note that this event
    // is only ever delivered to the application, not the protocol layer.
    mAppEventCallback(AppState, kEvent_PrepareRequested, inParam, outParam);

    // If the application didn't handle the PrepareRequested event then it doesn't support
    // on-demand configuration/preparation so fail with an error.
    VerifyOrExit(!outParam.DefaultHandlerCalled, err = WEAVE_ERROR_NOT_IMPLEMENTED);

    // Check for a preparation error returned by the app's event handler.  Note that the application
    // is not required to set an error value, since if preparation fails, and the error value is not
    // set, then the code below will catch this and substitute WEAVE_ERROR_INCORRECT_STATE.
    err = outParam.PrepareRequested.PrepareError;
    SuccessOrExit(err);

    // If the application failed to fully configure the binding, fail with an error.
    VerifyOrExit(mState != kState_NotConfigured && mState != kState_Configuring, err = WEAVE_ERROR_INCORRECT_STATE);

exit:
    Release();
    WeaveLogFunctError(err);
    return err;
}

/**
 *  Conduct preparation for this Binding based on configurations supplied before this call.
 *
 *  @return #WEAVE_NO_ERROR on success and an event callback will happen. Otherwise no event callback will happen.
 */
WEAVE_ERROR Binding::DoPrepare(WEAVE_ERROR configErr)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    // Immediately return an error, without changing the state of the Binding, if the Binding is not
    // in the correct state.
    if (kState_Configuring != mState)
    {
        return WEAVE_ERROR_INCORRECT_STATE;
    }

    // Fail if an error occurred during configuration.
    VerifyOrExit(WEAVE_NO_ERROR == configErr, err = configErr);

    // App must set peer node id
    VerifyOrExit(kNodeIdNotSpecified != mPeerNodeId, err = WEAVE_ERROR_INVALID_ARGUMENT);

    // App must pick a transport option
    VerifyOrExit(kTransport_NotSpecified != mTransportOption, err = WEAVE_ERROR_INVALID_ARGUMENT);

    // App must pick a security option
    VerifyOrExit(kSecurityOption_NotSpecified != mSecurityOption, err = WEAVE_ERROR_INVALID_ARGUMENT);

#if WEAVE_CONFIG_ENABLE_CASE_INITIATOR
    // Shared CASE session not supported over connection-oriented transports.
    VerifyOrExit(mSecurityOption != kSecurityOption_SharedCASESession || mTransportOption == kTransport_UDP || mTransportOption == kTransport_UDP_WRM,
                 err = WEAVE_ERROR_NOT_IMPLEMENTED);
#endif

#if WEAVE_CONFIG_ENABLE_PASE_INITIATOR
    // PASE sessions not supported over UDP transports.
    VerifyOrExit(mSecurityOption != kSecurityOption_PASESession ||
                 (mTransportOption != kTransport_UDP && mTransportOption != kTransport_UDP_WRM),
                 err = WEAVE_ERROR_NOT_IMPLEMENTED);
#endif

#if WEAVE_CONFIG_ENABLE_TAKE_INITIATOR
    // TAKE sessions not supported over UDP transports.
    VerifyOrExit(mSecurityOption != kSecurityOption_TAKESession ||
                 (mTransportOption != kTransport_UDP && mTransportOption != kTransport_UDP_WRM),
                 err = WEAVE_ERROR_NOT_IMPLEMENTED);
#endif

    mState = kState_Preparing;

    WeaveLogDetail(ExchangeManager, "Binding[%" PRIu8 "] (%" PRIu16 "): Preparing", GetLogId(), mRefCount);

    // Start by preparing the peer address
    PrepareAddress();

exit:
    if (WEAVE_NO_ERROR != err)
    {
        HandleBindingFailed(err, NULL, false);
    }
    WeaveLogFunctError(err);
    return err;
}

/**
 * Do any work necessary to determine the address of the peer in preparation for communication.
 */
void Binding::PrepareAddress()
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    mState = kState_PreparingAddress;

    // If configured to use an existing connection, extract the peer IP addressing information from the
    // connection if available.  Although this won't be used in contacting the peer (since the connection
    // already exists) this makes the information available via the Binding API.
    if ((mTransportOption == kTransport_TCP || mTransportOption == kTransport_ExistingConnection) && mCon != NULL)
    {
        if (mCon->NetworkType == WeaveConnection::kNetworkType_IP)
        {
            mPeerAddress = mCon->PeerAddr;
            mPeerPort = mCon->PeerPort;
        }
    }

    // Default to using a Weave fabric address in the default subnet if an address was not specified.
    else if (kAddressing_NotSpecified == mAddressingOption)
    {
        mPeerAddress = mExchangeManager->FabricState->SelectNodeAddress(mPeerNodeId);
    }

    // If requested, form a Weave fabric address for the peer in the configured subnet.
    else if (kAddressing_WeaveFabric == mAddressingOption)
    {
        mPeerAddress = mExchangeManager->FabricState->SelectNodeAddress(mPeerNodeId, mPeerAddress.Subnet());
    }

    // If requested, resolve a supplied host name or string-form IP address...
    else if (kAddressing_HostName == mAddressingOption)
    {
#if WEAVE_CONFIG_ENABLE_DNS_RESOLVER

        mState = kState_PreparingAddress_ResolveHostName;

        // Initiate a DNS query for the specified host name.
        err = mExchangeManager->MessageLayer
                ->Inet->ResolveHostAddress(mHostName, mHostNameLen, mDNSOptions, 1, &mPeerAddress, OnResolveComplete, this);

        ExitNow();

#elif WEAVE_CONFIG_RESOLVE_IPADDR_LITERAL

        if (!IPAddress::FromString(mHostName, mHostNameLen, mPeerAddress))
        {
            ExitNow(err = WEAVE_ERROR_INVALID_ARGUMENT);
        }

#else // !WEAVE_CONFIG_RESOLVE_IPADDR_LITERAL

        ExitNow(err = WEAVE_ERROR_UNSUPPORTED_WEAVE_FEATURE);

#endif // !WEAVE_CONFIG_RESOLVE_IPADDR_LITERAL
    }

    PrepareTransport();

exit:
    if (WEAVE_NO_ERROR != err)
    {
        HandleBindingFailed(err, NULL, false);
    }
}

/**
 * Do any work necessary to establish transport-level communication with the peer.
 */
void Binding::PrepareTransport()
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    mState = kState_PreparingTransport;

    // If the application has requested TCP, and no existing connection has been supplied...
    if (mTransportOption == kTransport_TCP && mCon == NULL)
    {
        // Construct a new WeaveConnection object.  This method implicitly establishes a reference
        // to the connection object, which will be owned by the Binding until it is closed or fails.
        mCon = mExchangeManager->MessageLayer->NewConnection();
        VerifyOrExit(mCon != NULL, err = WEAVE_ERROR_NO_MEMORY);

        // Remember that we have to release the connection later when the binding closes.
        SetFlag(kFlag_ConnectionReferenced);

        // Setup a callback function to be called when the connection attempt completes
        // and store a back-reference to the binding in the connection's AppState member.
        mCon->OnConnectionComplete = OnConnectionComplete;
        mCon->AppState = this;

        // Clear the default connection closed handler that is automatically configured on the
        // connection by the message layer.  Bindings receive a callback directly from the exchange
        // manager every time a connection closes, which allows them to automatically release their
        // reference to the connection without using a callback function.  Because of this, leaving
        // in place the default connection closed handler, with its automatic close feature, the
        // would result in a double release.  Thus we suppress that here.
        mCon->OnConnectionClosed = NULL;

        mState = kState_PreparingTransport_TCPConnect;

        // Initiate a connection to the peer.
        err = mCon->Connect(mPeerNodeId, kWeaveAuthMode_None, mPeerAddress, mPeerPort, mInterfaceId);
        SuccessOrExit(err);
    }

    else
    {
        // If using a connection supplied by the application, take a reference to the object.
        if (mTransportOption == kTransport_TCP || mTransportOption == kTransport_ExistingConnection)
        {
            mCon->AddRef();
            SetFlag(kFlag_ConnectionReferenced);
        }

        // No further work to do in preparing the transport, so proceed to preparing security.
        PrepareSecurity();
    }

exit:
    if (WEAVE_NO_ERROR != err)
    {
        HandleBindingFailed(err, NULL, true);
    }
}

/**
 * Do any work necessary to establish communication security with the peer.
 */
void Binding::PrepareSecurity()
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    WeaveSecurityManager *sm = mExchangeManager->MessageLayer->SecurityMgr;

    mState = kState_PreparingSecurity;

    // Default encryption type, if not specified.
    if (kSecurityOption_None != mSecurityOption && kWeaveEncryptionType_None == mEncType)
    {
        mEncType = kWeaveEncryptionType_AES128CTRSHA1;
    }

    switch (mSecurityOption)
    {
#if WEAVE_CONFIG_ENABLE_CASE_INITIATOR
    case kSecurityOption_CASESession:
    case kSecurityOption_SharedCASESession:
        {
            IPAddress peerAddress;
            uint16_t peerPort;
            uint64_t terminatingNodeId;
            const bool isSharedSession = (mSecurityOption == kSecurityOption_SharedCASESession);

            if (isSharedSession)
            {
                // This is also defined in Weave/Profiles/ServiceDirectory.h, but this is in Weave Core
                // TODO: move this to a common location.
                static const uint64_t kServiceEndpoint_CoreRouter = 0x18B4300200000012ull;

                const uint64_t fabricGlobalId = WeaveFabricIdToIPv6GlobalId(mExchangeManager->FabricState->FabricId);
                peerAddress = IPAddress::MakeULA(fabricGlobalId, nl::Weave::kWeaveSubnetId_Service,
                                   nl::Weave::WeaveNodeIdToIPv6InterfaceId(kServiceEndpoint_CoreRouter));
                peerPort = WEAVE_PORT;
                terminatingNodeId = kServiceEndpoint_CoreRouter;
            }
            else
            {
                peerAddress = mPeerAddress;
                peerPort = mPeerPort;
                terminatingNodeId = kNodeIdNotSpecified;
            }

            WeaveLogDetail(ExchangeManager, "Binding[%" PRIu8 "] (%" PRIu16 "): Initiating %sCASE session",
                    GetLogId(), mRefCount, isSharedSession ? "shared " : "");

            mState = kState_PreparingSecurity_EstablishSession;

            // Call the security manager to initiate the CASE session.  Note that security manager will call the
            // OnSecureSessionReady function during this call if a shared session is requested and the session is
            // already available.
            err = sm->StartCASESession(mCon, mPeerNodeId, peerAddress, peerPort, mAuthMode, this,
                    OnSecureSessionReady, OnSecureSessionFailed, NULL, terminatingNodeId);
            SuccessOrExit(err);
        }
        break;
#endif // WEAVE_CONFIG_ENABLE_CASE_INITIATOR

#if WEAVE_CONFIG_ENABLE_PASE_INITIATOR
    case kSecurityOption_PASESession:
        {
            InEventParam inParam;
            OutEventParam outParam;

            WeaveLogDetail(ExchangeManager, "Binding[%" PRIu8 "] (%" PRIu16 "): Initiating PASE session",
                    GetLogId(), mRefCount);

            mState = kState_PreparingSecurity_EstablishSession;

            // Call up to the application to get PASE parameters--essentially, the password.  Note that
            // the application is free to ignore this event, resulting in this code passing NULL to the
            // security manager which will then automatically choose the pairing code from the fabric
            // state object.
            // The application may NOT alter the state of the Binding during this callback.
            inParam.Clear();
            inParam.Source = this;
            inParam.PASEParametersRequested.PasswordSource = PasswordSourceFromAuthMode(mAuthMode);
            outParam.Clear();
            mAppEventCallback(AppState, kEvent_PASEParametersRequested, inParam, outParam);

            // Call the security manager to initiate the PASE session.
            err = sm->StartPASESession(mCon, mAuthMode, this, OnSecureSessionReady, OnSecureSessionFailed,
                    outParam.PASEParametersRequested.Password, outParam.PASEParametersRequested.PasswordLength);
            SuccessOrExit(err);
        }
        break;
#endif // WEAVE_CONFIG_ENABLE_PASE_INITIATOR

#if WEAVE_CONFIG_ENABLE_TAKE_INITIATOR
    case kSecurityOption_TAKESession:
        {
            InEventParam inParam;
            OutEventParam outParam;

            WeaveLogDetail(ExchangeManager, "Binding[%" PRIu8 "] (%" PRIu16 "): Initiating TAKE session",
                    GetLogId(), mRefCount);

            mState = kState_PreparingSecurity_EstablishSession;

            // Call up to the application to get TAKE parameters.
            // NOTE: The application may NOT alter the state of the Binding during this callback.
            inParam.Clear();
            inParam.Source = this;
            outParam.Clear();
            mAppEventCallback(AppState, kEvent_TAKEParametersRequested, inParam, outParam);

            // Verify the application handled the event.
            VerifyOrExit(!outParam.DefaultHandlerCalled, err = WEAVE_ERROR_INVALID_TAKE_PARAMETER);

            // Call the security manager to initiate the TAKE session.
            err = sm->StartTAKESession(mCon, mAuthMode, this, OnSecureSessionReady, OnSecureSessionFailed,
                    outParam.TAKEParametersRequested.EncryptAuthPhase,
                    outParam.TAKEParametersRequested.EncryptCommPhase,
                    outParam.TAKEParametersRequested.TimeLimitedIK,
                    outParam.TAKEParametersRequested.SendChallengerId,
                    outParam.TAKEParametersRequested.AuthDelegate);
            SuccessOrExit(err);
        }
        break;
#endif // WEAVE_CONFIG_ENABLE_TAKE_INITIATOR

    case kSecurityOption_SpecificKey:

        // Add a reservation on the specified key.  This reservation will be owned by the binding
        // until it closes.
        sm->ReserveKey(mPeerNodeId, mKeyId);
        SetFlag(kFlag_KeyReserved);

        HandleBindingReady();
        break;

    case kSecurityOption_None:
        // No further preparation needed.
        HandleBindingReady();
        break;

    default:
        ExitNow(err = WEAVE_ERROR_INVALID_ARGUMENT);
    }

exit:

    // If the security manager is currently busy, wait for it to finish.  When this happens,
    // Binding::OnSecurityManagerAvailable() will be called, which will give the binding an opportunity
    // to try again.
    if (err == WEAVE_ERROR_SECURITY_MANAGER_BUSY)
    {
        WeaveLogDetail(ExchangeManager, "Binding[%" PRIu8 "] (%" PRIu16 "): Security manager busy; waiting.",
                GetLogId(), mRefCount);

        mState = kState_PreparingSecurity_WaitSecurityMgr;
        err = WEAVE_NO_ERROR;
    }

    if (WEAVE_NO_ERROR != err)
    {
        HandleBindingFailed(err, NULL, true);
    }
}

/**
 * Transition the Binding to the Ready state.
 */
void Binding::HandleBindingReady()
{
    InEventParam inParam;
    OutEventParam outParam;

    // Should never be called in anything other than a preparing state.
    VerifyOrDie(IsPreparing());

    // Transition to the Ready state.
    mState = kState_Ready;

#if WEAVE_DETAIL_LOGGING
    {
        char peerDesc[kGetPeerDescription_MaxLength];
        const char *transport;
        GetPeerDescription(peerDesc, sizeof(peerDesc));
        switch (mTransportOption)
        {
        case kTransport_UDP:
            transport = "UDP";
            break;
        case kTransport_UDP_WRM:
            transport = "WRM";
            break;
        case kTransport_TCP:
        case kTransport_ExistingConnection:
            switch (mCon->NetworkType)
            {
            case WeaveConnection::kNetworkType_IP:
                transport = "TCP";
                break;
            case WeaveConnection::kNetworkType_BLE:
                transport = "WoBLE";
                break;
            default:
                transport = "Unknown";
                break;
            }
            break;
        default:
            transport = "Unknown";
            break;
        }
        WeaveLogDetail(ExchangeManager, "Binding[%" PRIu8 "] (%" PRIu16 "): Ready, peer %s via %s",
                GetLogId(), mRefCount, peerDesc, transport);
    }
#endif // WEAVE_DETAIL_LOGGING

    inParam.Clear();
    inParam.Source = this;
    outParam.Clear();

    // Prevent the application from freeing the Binding until we're done using it.
    AddRef();

    // Tell the application that the prepare operation succeeded and the binding is ready for use.
    mAppEventCallback(AppState, kEvent_BindingReady, inParam, outParam);

    // If the Binding is still in the Ready state, and a protocol layer callback has been registered,
    // tell the protocol layer that the Binding is ready for use.
    if (mState == kState_Ready && mProtocolLayerCallback != NULL)
    {
        mProtocolLayerCallback(mProtocolLayerState, kEvent_BindingReady, inParam, outParam);
    }

    Release();
}

/**
 * Transition the Binding to the Failed state.
 */
void Binding::HandleBindingFailed(WEAVE_ERROR err, Profiles::StatusReporting::StatusReport *statusReport, bool raiseEvents)
{
    InEventParam inParam;
    OutEventParam outParam;
    EventType eventType;

    inParam.Clear();
    inParam.Source = this;
    outParam.Clear();

    if (IsPreparing())
    {
        inParam.PrepareFailed.Reason = err;
        inParam.PrepareFailed.StatusReport = statusReport;
        eventType = kEvent_PrepareFailed;
    }
    else
    {
        inParam.BindingFailed.Reason = err;
        eventType = kEvent_BindingFailed;
    }

    WeaveLogDetail(ExchangeManager, "Binding[%" PRIu8 "] (%" PRIu16 "): %s FAILED: peer %" PRIX64 ", %s%s",
            GetLogId(), mRefCount,
            (eventType == kEvent_BindingFailed) ? "Binding" : "Prepare",
            mPeerNodeId,
            (err == WEAVE_ERROR_STATUS_REPORT_RECEIVED && statusReport != NULL)
                ? "Status Report received: "
                : "",
            (err == WEAVE_ERROR_STATUS_REPORT_RECEIVED && statusReport != NULL)
                ? nl::StatusReportStr(statusReport->mProfileId, statusReport->mStatusCode)
                : ErrorStr(err));

    // Reset the binding and enter the Failed state.
    DoReset(kState_Failed);

    // Prevent the application from freeing the Binding until we're done using it.
    AddRef();

    // If requested, deliver the failure events to the application and protocol layer.
    if (raiseEvents)
    {
        mAppEventCallback(AppState, eventType, inParam, outParam);
        if (mProtocolLayerCallback != NULL)
        {
            mProtocolLayerCallback(mProtocolLayerState, eventType, inParam, outParam);
        }
    }

    Release();
}

#if WEAVE_CONFIG_ENABLE_DNS_RESOLVER

/**
 * Invoked when DNS host name resolution completes (successfully or otherwise).
 */
void Binding::OnResolveComplete(void *appState, INET_ERROR err, uint8_t addrCount, IPAddress *addrArray)
{
    Binding *_this = (Binding *)appState;

    // It is legal for a DNS entry to exist but contain no A/AAAA records. If this happens, return a reasonable error
    // to the user.
    if (err == INET_NO_ERROR && addrCount == 0)
        err = INET_ERROR_HOST_NOT_FOUND;

    WeaveLogDetail(ExchangeManager, "Binding[%" PRIu8 "] (%" PRIu16 "): DNS resolution %s%s",
            _this->GetLogId(), _this->mRefCount,
            (err == INET_NO_ERROR) ? "succeeded" : "failed: ",
            (err == INET_NO_ERROR) ? "" : ErrorStr(err));

    // If the resolution succeeded, proceed to preparing the transport, otherwise fail the binding.
    if (err == INET_NO_ERROR)
    {
        _this->PrepareTransport();
    }
    else
    {
        _this->HandleBindingFailed(err, NULL, true);
    }
}

#endif // WEAVE_CONFIG_ENABLE_DNS_RESOLVER

/**
 * Invoked when TCP connection establishment completes (successfully or otherwise).
 */
void Binding::OnConnectionComplete(WeaveConnection *con, WEAVE_ERROR conErr)
{
    Binding *_this = (Binding *)con->AppState;

    VerifyOrDie(_this->mState == kState_PreparingTransport_TCPConnect);
    VerifyOrDie(_this->mCon == con);

    // If the connection was successfully established...
    if (conErr == WEAVE_NO_ERROR)
    {
        WeaveLogDetail(ExchangeManager, "Binding[%" PRIu8 "] (%" PRIu16 "): TCP con established (%04" PRIX16 ")",
                _this->GetLogId(), _this->mRefCount, con->LogId());

        // Deliver a ConnectionEstablished API event to the application.  This gives the application an opportunity
        // to adjust the configuration of the connection, e.g. to enable TCP keep-alive.
        {
            InEventParam inParam;
            OutEventParam outParam;
            inParam.Clear();
            inParam.Source = _this;
            outParam.Clear();
            _this->mAppEventCallback(_this->AppState, kEvent_ConnectionEstablished, inParam, outParam);
        }

        // If the binding is still in the TCPConnect state, proceed to preparing security.
        if (_this->mState == kState_PreparingTransport_TCPConnect)
        {
            _this->PrepareSecurity();
        }
    }

    // Otherwise the connection failed...
    else
    {
        WeaveLogDetail(ExchangeManager, "Binding[%" PRIu8 "] (%" PRIu16 "): TCP con failed (%04" PRIX16 "): %s",
                _this->GetLogId(), _this->mRefCount, con->LogId(), ErrorStr(conErr));
        _this->HandleBindingFailed(conErr, NULL, true);
    }
}

/**
 * Invoked when a Weave connection (of any type) closes.
 */
void Binding::OnConnectionClosed(WeaveConnection *con, WEAVE_ERROR err)
{
    // NOTE: This method is called whenever a connection is closed anywhere in the system.  Thus
    // this code must filter for events that apply to the current binding only.

    // Ignore the key error if the binding is not in the Ready state or one of the preparing states.
    VerifyOrExit(IsPreparing() || mState == kState_Ready, /* no-op */);

    // Ignore the close if it is associated with a different connection.
    VerifyOrExit(mCon == con, /* no-op */);

    // If the other side closed the connection gracefully, signal this to the user by indicating
    // that the connection closed unexpectedly.
    if (err == WEAVE_NO_ERROR)
    {
        err = WEAVE_ERROR_CONNECTION_CLOSED_UNEXPECTEDLY;
    }

    // Transition the binding to a failed state.
    HandleBindingFailed(err, NULL, true);

exit:
    return;
}

#if WEAVE_CONFIG_ENABLE_CASE_INITIATOR || WEAVE_CONFIG_ENABLE_PASE_INITIATOR || WEAVE_CONFIG_ENABLE_TAKE_INITIATOR

/**
 * Invoked when a security session establishment has completed successfully.
 */
void Binding::OnSecureSessionReady(WeaveSecurityManager *sm, WeaveConnection *con, void *reqState, uint16_t keyId, uint64_t peerNodeId, uint8_t encType)
{
    Binding *_this = (Binding *)reqState;

    // Verify the state of the binding.
    VerifyOrDie(_this->mState == kState_PreparingSecurity_EstablishSession);

    // Save the session key id and encryption type.
    _this->mKeyId = keyId;
    _this->mEncType = encType;

    // Remember that the key must be released when the binding closes.
    _this->SetFlag(kFlag_KeyReserved);

    // Tell the application that the binding is ready.
    _this->HandleBindingReady();
}

/**
 * Invoked when security session establishment fails.
 */
void Binding::OnSecureSessionFailed(WeaveSecurityManager *sm, WeaveConnection *con, void *reqState,
        WEAVE_ERROR localErr, uint64_t peerNodeId, Profiles::StatusReporting::StatusReport *statusReport)
{
    Binding *_this = (Binding *)reqState;

    // Verify the state of the binding.
    VerifyOrDie(_this->mState == kState_PreparingSecurity_EstablishSession);

    // Tell the application that the binding has failed.
    _this->HandleBindingFailed(localErr, statusReport, true);
}

#endif // WEAVE_CONFIG_ENABLE_CASE_INITIATOR || WEAVE_CONFIG_ENABLE_PASE_INITIATOR || WEAVE_CONFIG_ENABLE_TAKE_INITIATOR

/**
 * Invoked when a message encryption key has been rejected by a peer (via a KeyError), or a key has
 * otherwise become invalid (e.g. by ending a session).
 */
void Binding::OnKeyFailed(uint64_t peerNodeId, uint32_t keyId, WEAVE_ERROR keyErr)
{
    // NOTE: This method is called for any and all key errors that occur system-wide.  Thus this code
    // must filter for errors that apply to the current binding.

    // Ignore the key error if the binding is not in the Ready state or one of the preparing states.
    VerifyOrExit(IsPreparing() || mState == kState_Ready, /* no-op */);

    // Ignore the key error if it is not in relation to the specified peer node.
    VerifyOrExit(peerNodeId == mPeerNodeId, /* no-op */);

    // Ignore the key error if the binding is in the Ready state and the failed key id does
    // not match the key id associated with the binding.
    VerifyOrExit(mState != kState_Ready || keyId == mKeyId, /* no-op */);

    // Fail the binding.
    HandleBindingFailed(keyErr, NULL, true);

exit:
    return;
}

/**
 *  Invoked when the security manager becomes available for initiating new sessions.
 */
void Binding::OnSecurityManagerAvailable()
{
    // NOTE: This method is called for all binding objects any time the security manager becomes
    // available.  Thus this method must filter the notification based on the state of the binding.

    // If the binding is waiting for the security manager, retry preparing security.
    if (mState == kState_PreparingSecurity_WaitSecurityMgr)
    {
        PrepareSecurity();
    }
}

/**
 * Re-configure an existing Exchange Context to adjust the response timeout.
 *
 * @param[in]  apExchangeContext        A pointer to an Exchange Context object to be re-configured
 *
 */
WEAVE_ERROR Binding::AdjustResponseTimeout(nl::Weave::ExchangeContext *apExchangeContext) const
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    // Binding must be in the Ready state.
    VerifyOrExit(kState_Ready == mState, err = WEAVE_ERROR_INCORRECT_STATE);

    // If a default response timeout has been configured, adjust the response timeout value in
    // the exchange to match.
    if (mDefaultResponseTimeoutMsec)
    {
        apExchangeContext->ResponseTimeout = mDefaultResponseTimeoutMsec;
    }

exit:
    WeaveLogFunctError(err);
    return err;
}

/**
 * Determine if a particular incoming message is from the configured peer and is suitably authenticated.
 *
 * This method confirms the following details about the given message:
 *
 * - The message originated from the peer node of the binding
 *
 * - The message was received over the same transport type as the binding. If the message was
 * received over a connection, the method also confirms that the message was received over the
 * exact connection associated with the binding.
 *
 * - The encryption key and type used to encrypt the message matches those configured in the binding.
 * For bindings configured without the use of security, the method confirms that the incoming message is
 * NOT encrypted.
 *
 * This method is intended to be used in protocols such as WDM where peers can spontaneously initiate
 * exchanges back to the local node after an initial exchange from the node to the peer.  In such cases,
 * the method allows the local node to confirm that the incoming unsolicited message was sent by the
 * associated peer.  (Of course, for Bindings configured without the use of message encryption, this
 * assertion provides no value from a security perspective.  It merely confirms that the sender node
 * id and transport types match.)
 *
 * Note that if the binding is not in the Ready state, this method will always return false.
 *
 * @param[in] msgInfo                   The Weave message information for the incoming message.
 *
 * @return                              True if the message is authentically from the peer.
 */
bool Binding::IsAuthenticMessageFromPeer(const nl::Weave::WeaveMessageHeader *msgInfo)
{
    if (mState != kState_Ready)
        return false;

    if (msgInfo->SourceNodeId != mPeerNodeId)
        return false;

    if (msgInfo->InCon != NULL)
    {
        if ((mTransportOption != kTransport_TCP && mTransportOption != kTransport_ExistingConnection) || msgInfo->InCon != mCon)
            return false;
    }
    else
    {
        if (mTransportOption != kTransport_UDP && mTransportOption != kTransport_UDP_WRM)
            return false;
    }

    if (msgInfo->EncryptionType != mEncType)
        return false;

    if (mEncType != kWeaveEncryptionType_None && !WeaveKeyId::IsSameKeyOrGroup(msgInfo->KeyId, mKeyId))
        return false;

    return true;
}

/**
 *  Get the max Weave payload size that can fit inside the supplied PacketBuffer.
 *
 *  For UDP, including UDP with WRM, the maximum payload size returned will
 *  ensure the resulting Weave message will not overflow the configured UDP MTU.
 *
 *  Additionally, this method will ensure the Weave payload will not overflow
 *  the supplied PacketBuffer.
 *
 *  @param[in]    msgBuf        A pointer to the PacketBuffer to which the message
 *                              payload will be written.
 *
 *  @return                     The max Weave payload size.
 */
uint32_t Binding::GetMaxWeavePayloadSize(const System::PacketBuffer *msgBuf)
{
    // Constrain the max Weave payload size by the UDP MTU if we are using UDP.
    // TODO: Eventually, we may configure a custom UDP MTU size on the binding
    //       instead of using the default value directly.
    bool isUDP = (mTransportOption == kTransport_UDP || mTransportOption == kTransport_UDP_WRM);
    return WeaveMessageLayer::GetMaxWeavePayloadSize(msgBuf, isUDP, mUDPPathMTU);
}

/**
 * Constructs a string describing the peer node and its associated address / connection information.
 *
 * @param[in] buf                       A pointer to a buffer into which the string should be written. The supplied
 *                                      buffer should be at least as big as kGetPeerDescription_MaxLength. If a
 *                                      smaller buffer is given the string will be truncated to fit. The output
 *                                      will include a NUL termination character in all cases.
 * @param[in] bufSize                   The size of the buffer pointed at by buf.
 */
void Binding::GetPeerDescription(char *buf, uint32_t bufSize) const
{
    WeaveMessageLayer::GetPeerDescription(buf, bufSize, mPeerNodeId,
            (mPeerAddress != nl::Inet::IPAddress::Any) ? &mPeerAddress : NULL,
            mPeerPort, mInterfaceId, mCon);
}

/**
 * Allocate a new Exchange Context for communicating with the peer that is the target of the binding.
 *
 * @param[out] appExchangeContext       A reference to a pointer that will receive the newly allocated
 *                                      Exchange Context object.  The pointer will be set to NULL in
 *                                      the event that the method fails.
 *
 * @retval #WEAVE_NO_ERROR              If the exchange context was successfully allocated.
 *
 * @retval #WEAVE_ERROR_NO_MEMORY       If no memory was available to allocate the exchange context.
 *
 * @retval #WEAVE_ERROR_INCORRECT_STATE If the binding is not in the Ready state.
 *
 * @retval other                        Other errors related to configuring the exchange context based
 *                                      on the configuration of the binding.
 */
WEAVE_ERROR Binding::NewExchangeContext(nl::Weave::ExchangeContext *& appExchangeContext)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    appExchangeContext = NULL;

    // Fail if the binding is not in the Ready state.
    VerifyOrExit(kState_Ready == mState, err = WEAVE_ERROR_INCORRECT_STATE);

    // Attempt to allocate a new exchange context.
    appExchangeContext = mExchangeManager->NewContext(mPeerNodeId, mPeerAddress, mPeerPort, mInterfaceId, NULL);
    VerifyOrExit(NULL != appExchangeContext, err = WEAVE_ERROR_NO_MEMORY);

#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING

    // Set the default WRMP configuration in the new exchange.
    appExchangeContext->mWRMPConfig = mDefaultWRMPConfig;

    // If Weave reliable messaging was expressly requested as a transport...
    if (mTransportOption == kTransport_UDP_WRM)
    {
        // Enable the auto-request ACK feature in the exchange so that all outgoing messages
        // include a request for acknowledgment.
        appExchangeContext->SetAutoRequestAck(true);
    }

#endif // WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING

    // If using a connection-oriented transport...
    if (mTransportOption == kTransport_TCP || mTransportOption == kTransport_ExistingConnection)
    {
        // Add a reference to the connection object.
        mCon->AddRef();

        // Configure the exchange context to use the connection and release it when it's done.
        appExchangeContext->Con = mCon;
        appExchangeContext->SetShouldAutoReleaseConnection(true);
    }

    // If message encryption is enabled...
    if (mSecurityOption != kSecurityOption_None)
    {
        uint32_t keyId;

        // If the key id specifies a logical group key (e.g. the "current" rotating group key), resolve it to
        // the id for a specific key.
        err = mExchangeManager->FabricState->GroupKeyStore->GetCurrentAppKeyId(mKeyId, keyId);
        SuccessOrExit(err);

        // Configure the exchange context with the selected key id and encryption type.
        appExchangeContext->KeyId = keyId;
        appExchangeContext->EncryptionType = mEncType;

        // Add a reservation for the key.
        mExchangeManager->MessageLayer->SecurityMgr->ReserveKey(mPeerNodeId, keyId);

        // Arrange for the exchange context to automatically release the key when it is freed.
        appExchangeContext->SetAutoReleaseKey(true);
    }

#if WEAVE_CONFIG_ENABLE_MESSAGE_CAPTURE
    // If message is marked for capture set flag in Exchange Context
    if (GetFlag(kFlag_CaptureTxMessage))
    {
        appExchangeContext->SetCaptureSentMessage(true);
    }
#endif // WEAVE_CONFIG_ENABLE_MESSAGE_CAPTURE

    err = AdjustResponseTimeout(appExchangeContext);
    SuccessOrExit(err);

exit:
    if (err != WEAVE_NO_ERROR && appExchangeContext != NULL)
    {
        appExchangeContext->Close();
        appExchangeContext = NULL;
    }
    WeaveLogFunctError(err);
    return err;
}

/* Utility function to allocate an appropriately sized buffer.
 *
 * This function takes in a supplied desired size of the payload and a
 * minimum size that the caller is willing to tolerate from the system.
 *
 * The system would accept these parameters and output a maximum
 * payload size in the buffer it managed to allocate.
 * It would try to honor the desired size based on system
 * resources and constraints, but return an appropriate error if
 * the minimum size cannot be met.
 *
 */
WEAVE_ERROR Binding::AllocateRightSizedBuffer(PacketBuffer *& buf,
                                              const uint32_t desiredSize,
                                              const uint32_t minSize,
                                              uint32_t & outMaxPayloadSize)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint32_t bufferAllocSize;
    uint32_t maxWeavePayloadSize;
    uint32_t weaveTrailerSize = GetWeaveTrailerSize();
    uint32_t weaveHeaderSize = GetWeaveHeaderSize();

    bufferAllocSize = nl::Weave::min(desiredSize,
                                     static_cast<uint32_t>(WEAVE_SYSTEM_CONFIG_PACKETBUFFER_CAPACITY_MAX -
                                      weaveHeaderSize -
                                      weaveTrailerSize));

    // Add the Weave Trailer size as NewWithAvailableSize() includes that in
    // availableSize.
    bufferAllocSize += weaveTrailerSize;

    buf = PacketBuffer::NewWithAvailableSize(weaveHeaderSize, bufferAllocSize);
    VerifyOrExit(buf != NULL, err = WEAVE_ERROR_NO_MEMORY);

    maxWeavePayloadSize = GetMaxWeavePayloadSize(buf);

    outMaxPayloadSize = nl::Weave::min(maxWeavePayloadSize, bufferAllocSize);

    if (outMaxPayloadSize < minSize)
    {
        err = WEAVE_ERROR_BUFFER_TOO_SMALL;

        PacketBuffer::Free(buf);
        buf = NULL;
    }

exit:
    return err;
}

uint32_t Binding::GetWeaveHeaderSize(void)
{
    return WEAVE_SYSTEM_CONFIG_HEADER_RESERVE_SIZE;
}

uint32_t Binding::GetWeaveTrailerSize(void)
{
    return WEAVE_TRAILER_RESERVE_SIZE;
}

/**
 * Construct a new binding configuration object.
 *
 * @param[in] aBinding                  A reference to the Binding to be configured.
 */
Binding::Configuration::Configuration(Binding& aBinding)
: mBinding(aBinding)
{
    if (mBinding.CanBePrepared())
    {
        if (mBinding.mState != kState_NotConfigured)
        {
            mBinding.ResetConfig();
        }

        mBinding.mState = kState_Configuring;
        mError = WEAVE_NO_ERROR;

        WeaveLogDetail(ExchangeManager, "Binding[%" PRIu8 "] (%" PRIu16 "): Configuring", mBinding.GetLogId(), mBinding.mRefCount);
    }
    else
    {
        mError = WEAVE_ERROR_INCORRECT_STATE;
    }
}

/**
 * Configure the binding to communicate with a specific Weave node id.
 *
 * @param[in] aPeerNodeId               Node id of the peer node.
 *
 * @return                              A reference to the binding object.
 */
Binding::Configuration& Binding::Configuration::Target_NodeId(uint64_t aPeerNodeId)
{
    mBinding.mPeerNodeId = aPeerNodeId;
    return *this;
}

/**
 * Configure the binding to communicate with a specific Weave service endpoint.
 *
 * If not otherwise configured, the peer address is set to the Weave fabric address of the service endpoint.
 *
 * @param[in] serviceEndpointId         The node id of the service endpoint with which communication will take place.
 *
 * @return                              A reference to the binding object.
 */
Binding::Configuration& Binding::Configuration::Target_ServiceEndpoint(uint64_t serviceEndpointId)
{
    Target_NodeId(serviceEndpointId);
    if (mBinding.mAddressingOption == Binding::kAddressing_NotSpecified)
    {
        TargetAddress_WeaveService();
    }
    return *this;
}

/**
 * When communicating with the peer, use the specific IP address, port and network interface.
 *
 * @param[in] aPeerAddress              IP address for the peer
 * @param[in] aPeerPort                 Remote port
 * @param[in] aInterfaceId              The ID of local network interface to use for communication
 *
 * @return                              A reference to the binding object.
 */
Binding::Configuration& Binding::Configuration::TargetAddress_IP(const nl::Inet::IPAddress aPeerAddress, const uint16_t aPeerPort, const InterfaceId aInterfaceId)
{
    mBinding.mAddressingOption = Binding::kAddressing_UnicastIP;
    mBinding.mPeerAddress = aPeerAddress;
    mBinding.mPeerPort = (aPeerPort != 0) ? aPeerPort : WEAVE_PORT;
    mBinding.mInterfaceId = aInterfaceId;
    return *this;
}

/**
 * When communicating with the peer, use the specific host name, port and network interface.
 *
 * NOTE: The caller must ensure that the supplied host name string remains valid until the
 * binding preparation phase completes.
 *
 * @param[in] aHostName                 A NULL-terminated string containing the host name of the peer.
 * @param[in] aPeerPort                 Remote port to use when communicating with the peer.
 * @param[in] aInterfaceId              The ID of local network interface to use for communication.
 *
 * @return                              A reference to the binding object.
 */
Binding::Configuration& Binding::Configuration::TargetAddress_IP(const char *aHostName, uint16_t aPeerPort, InterfaceId aInterfaceId)
{
    return TargetAddress_IP(aHostName, strlen(aHostName), aPeerPort, aInterfaceId);
}

/**
 * When communicating with the peer, use the specific host name, port and network interface.
 *
 * NOTE: The caller must ensure that the supplied host name string remains valid until the
 * binding preparation phase completes.
 *
 * @param[in] aHostName                 A string containing the host name of the peer.  This string
 *                                      does not need to be NULL terminated.
 * @param[in] aHostNameLen              The length of the string pointed at by aHostName.
 * @param[in] aPeerPort                 Remote port to use when communicating with the peer.
 * @param[in] aInterfaceId              The ID of local network interface to use for communication.
 *
 * @return                              A reference to the binding object.
 */
Binding::Configuration& Binding::Configuration::TargetAddress_IP(const char *aHostName, size_t aHostNameLen, uint16_t aPeerPort, InterfaceId aInterfaceId)
{
    if (aHostNameLen <= UINT8_MAX)
    {
        mBinding.mAddressingOption = Binding::kAddressing_HostName;
        mBinding.mHostName = aHostName;
        mBinding.mHostNameLen = (uint8_t)aHostNameLen;
        mBinding.mPeerPort = (aPeerPort != 0) ? aPeerPort : WEAVE_PORT;
        mBinding.mInterfaceId = aInterfaceId;
    }
    else
    {
        mError = WEAVE_ERROR_INVALID_ARGUMENT;
    }
    return *this;
}

/**
 * When resolving the host name of the peer, use the specified DNS options.
 *
 * @param[in]  dnsOptions               An integer value controlling how host name resolution is performed.
 *                                      Value should be one of values from the #::nl::Inet::DNSOptions enumeration.
 *
 * @return                              A reference to the binding object.
 */
Binding::Configuration& Binding::Configuration::DNS_Options(uint8_t dnsOptions)
{
#if WEAVE_CONFIG_ENABLE_DNS_RESOLVER
    mBinding.mDNSOptions = dnsOptions;
#else // WEAVE_CONFIG_ENABLE_DNS_RESOLVER
    mError = WEAVE_ERROR_NOT_IMPLEMENTED;
#endif // WEAVE_CONFIG_ENABLE_DNS_RESOLVER
    return *this;
}

/**
 * When communicating with the peer, use a Weave service fabric address derived from the peer's node id.
 *
 * @return                              A reference to the binding object.
 */
Binding::Configuration& Binding::Configuration::TargetAddress_WeaveService()
{
    return TargetAddress_WeaveFabric(nl::Weave::kWeaveSubnetId_Service);
}

/**
 * When communicating with the peer, use a Weave fabric address derived from the peer's node id and a specified subnet.
 *
 * @param[in]  aSubnetId                The subnet id to be used in forming the Weave fabric address of the peer.
 *
 * @return                              A reference to the binding object.
 */
Binding::Configuration& Binding::Configuration::TargetAddress_WeaveFabric(uint16_t aSubnetId)
{
    mBinding.mAddressingOption = kAddressing_WeaveFabric;
    mBinding.mPeerAddress = IPAddress::MakeULA(0, aSubnetId, 0); // Save the subnet in the peer address field.
    return *this;
}

/**
 * Use TCP to communicate with the peer.
 *
 * @return                              A reference to the binding object.
 */
Binding::Configuration& Binding::Configuration::Transport_TCP()
{
    mBinding.mTransportOption = kTransport_TCP;
    return *this;
}

/**
 * Use UDP to communicate with the peer.
 *
 * @return                              A reference to the binding object.
 */
Binding::Configuration& Binding::Configuration::Transport_UDP()
{
    mBinding.mTransportOption = kTransport_UDP;
    return *this;
}

/**
 * Use the Weave Reliable Messaging protocol when communicating with the peer.
 *
 * @return                              A reference to the binding object.
 */
Binding::Configuration& Binding::Configuration::Transport_UDP_WRM()
{
#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
    mBinding.mTransportOption = kTransport_UDP_WRM;
#else // WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
    mError = WEAVE_ERROR_NOT_IMPLEMENTED;
#endif // WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
    return *this;
}

/**
 * Set the expected path MTU for UDP packets travelling to the peer. For some Weave protocols
 * this will be used to dynamically adjust the Weave message payload size.
 *
 * @param[in] aPathMTU                  The expected path MTU for UDP packets travelling to the peer.
 *
 * @return                              A reference to the binding object.
 */
Binding::Configuration& Binding::Configuration::Transport_UDP_PathMTU(uint32_t aPathMTU)
{
    mBinding.mUDPPathMTU = aPathMTU;
    return *this;
}

/**
 * Set the default WRMP configuration for exchange contexts created from this Binding object.
 *
 * @param[in] aWRMPConfig               A reference to the new default WRMP configuration.
 *
 * @return                              A reference to the binding object.
 */
Binding::Configuration& Binding::Configuration::Transport_DefaultWRMPConfig(const nl::Weave::WRMPConfig& aWRMPConfig)
{
#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
    mBinding.mDefaultWRMPConfig = aWRMPConfig;
#else // WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
    IgnoreUnusedVariable(aWRMPConfig);
    mError = WEAVE_ERROR_NOT_IMPLEMENTED;
#endif // WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
    return *this;
}

/**
 * Use an existing Weave connection to communicate with the peer.
 *
 * NOTE: The reference count on the connection object is incremented when binding
 * preparation succeeds. Thus the application is responsible for ensuring the
 * connection object remain alive until that time.
 *
 * @param[in] con		        A pointer to the existing Weave connection.
 *
 * @return                      A reference to the binding object.
 */
Binding::Configuration& Binding::Configuration::Transport_ExistingConnection(WeaveConnection *con)
{
    mBinding.mTransportOption = kTransport_ExistingConnection;
    mBinding.mCon = con;
    return *this;
}

/**
 * Set default response timeout for exchange contexts created from this Binding object
 *
 * @param[in] aResponseTimeoutMsec      The default response time, in ms.
 *
 * @return                              A reference to the binding object.
 */
Binding::Configuration& Binding::Configuration::Exchange_ResponseTimeoutMsec(uint32_t aResponseTimeoutMsec)
{
    mBinding.mDefaultResponseTimeoutMsec = aResponseTimeoutMsec;
    return *this;
}

/**
 * When communicating with the peer, send and receive unencrypted (i.e. unsecured) messages.
 *
 * @return                              A reference to the binding object.
 */
Binding::Configuration& Binding::Configuration::Security_None()
{
    mBinding.mSecurityOption = kSecurityOption_None;
    mBinding.mKeyId = WeaveKeyId::kNone;
    mBinding.mAuthMode = kWeaveAuthMode_Unauthenticated;
    return *this;
}

/**
 * When communicating with the peer, send and receive messages encrypted using a CASE session key
 * established with the peer node.
 *
 * If the necessary session is not available, it will be established automatically as part of
 * preparing the binding.
 *
 * @return                              A reference to the binding object.
 */
Binding::Configuration& Binding::Configuration::Security_CASESession(void)
{
#if WEAVE_CONFIG_ENABLE_CASE_INITIATOR
    mBinding.mSecurityOption = kSecurityOption_CASESession;
    mBinding.mKeyId = WeaveKeyId::kNone;
    mBinding.mAuthMode = kWeaveAuthMode_CASE_AnyCert;
#else
    mError = WEAVE_ERROR_NOT_IMPLEMENTED;
#endif
    return *this;
}

/**
 * When communicating with the peer, send and receive messages encrypted using a shared CASE
 * session key established with the Nest core router.
 *
 * If the necessary session is not available, it will be established automatically as part of
 * preparing the binding.
 *
 * @return                              A reference to the binding object.
 */
Binding::Configuration& Binding::Configuration::Security_SharedCASESession(void)
{
#if WEAVE_CONFIG_ENABLE_CASE_INITIATOR
    mBinding.mSecurityOption = kSecurityOption_SharedCASESession;
    mBinding.mKeyId = WeaveKeyId::kNone;
    mBinding.mAuthMode = kWeaveAuthMode_CASE_ServiceEndPoint;
#else
    mError = WEAVE_ERROR_NOT_IMPLEMENTED;
#endif
    return *this;
}

/**
 * When communicating with the peer, send and receive messages encrypted using a shared CASE
 * session key established with a specified router node.
 *
 * If the necessary session is not available, it will be established automatically as part of
 * preparing the binding.
 *
 * @param[in] aRouterNodeId             The Weave node ID of the router with which shared CASE
 *                                      session should be established.
 *
 * @return                              A reference to the binding object.
 */
Binding::Configuration& Binding::Configuration::Security_SharedCASESession(uint64_t aRouterNodeId)
{
#if WEAVE_CONFIG_ENABLE_CASE_INITIATOR
    // This is also defined in Weave/Profiles/ServiceDirectory.h, but this is in Weave Core
    // TODO: move this elsewhere.
    static const uint64_t kServiceEndpoint_CoreRouter = 0x18B4300200000012ull;

    // TODO: generalize this
    // Only support the router to be Core Router in Nest service
    VerifyOrExit(kServiceEndpoint_CoreRouter == aRouterNodeId, mError = WEAVE_ERROR_NOT_IMPLEMENTED);

    Security_SharedCASESession();

exit:
#else
    IgnoreUnusedVariable(aRouterNodeId);
    mError = WEAVE_ERROR_NOT_IMPLEMENTED;
#endif
    return *this;
}

/**
 * When communicating with the peer, send and receive messages encrypted using a PASE session key
 * established with the peer node.
 *
 * If the necessary session is not available, it will be established automatically as part of
 * preparing the binding.
 *
 * @param[in] aPasswordSource   The source for the password to be used during PASE session
 *                              establishment.
 *
 * @return                      A reference to the binding object.
 */
Binding::Configuration& Binding::Configuration::Security_PASESession(uint8_t aPasswordSource)
{
#if WEAVE_CONFIG_ENABLE_PASE_INITIATOR
    mBinding.mSecurityOption = kSecurityOption_PASESession;
    mBinding.mKeyId = WeaveKeyId::kNone;
    mBinding.mAuthMode = kWeaveAuthModeCategory_PASE | (kWeaveAuthMode_PASE_PasswordSourceMask & aPasswordSource);
#else
    IgnoreUnusedVariable(aPasswordSource);
    mError = WEAVE_ERROR_NOT_IMPLEMENTED;
#endif
    return *this;
}

/**
 * When communicating with the peer, send and receive messages encrypted using a TAKE session key
 * established with the peer node.
 *
 * If the necessary session is not available, it will be established automatically as part of
 * preparing the binding.
 *
 * @return                      A reference to the binding object.
 */
Binding::Configuration& Binding::Configuration::Security_TAKESession()
{
#if WEAVE_CONFIG_ENABLE_TAKE_INITIATOR
    mBinding.mSecurityOption = kSecurityOption_TAKESession;
    mBinding.mKeyId = WeaveKeyId::kNone;
    mBinding.mAuthMode = kWeaveAuthMode_TAKE_IdentificationKey;
#else
    mError = WEAVE_ERROR_NOT_IMPLEMENTED;
#endif
    return *this;
}

/**
 * When communicating with the peer, send and receive messages encrypted using a specified key.
 *
 * @param[in] aKeyId            The id of the encryption key.  The specified key must be
 *                              suitable for Weave message encryption.
 *
 * @return                      A reference to the Binding object.
 */
Binding::Configuration& Binding::Configuration::Security_Key(uint32_t aKeyId)
{
    if (WeaveKeyId::IsMessageEncryptionKeyId(aKeyId))
    {
        mBinding.mSecurityOption = kSecurityOption_SpecificKey;
        if (!WeaveKeyId::IsAppRotatingKey(aKeyId))
            mBinding.mKeyId = aKeyId;
        else
            mBinding.mKeyId = WeaveKeyId::ConvertToCurrentAppKeyId(aKeyId);
        mBinding.mAuthMode = kWeaveAuthMode_NotSpecified;
    }
    else
    {
        mError = WEAVE_ERROR_INVALID_KEY_ID;
    }
    return *this;
}

/**
 * When communicating with the peer, send and receive messages encrypted for a specified
 * Weave Application Group.
 *
 * @param[in] aAppGroupGlobalId The global id of the application group for which messages should
 *                              be encrypted.
 * @param[in] aRootKeyId        The root key used to derive encryption keys for the specified
 *                              Weave Application Group.
 * @param[in] aUseRotatingKey   True if the Weave Application Group uses rotating message keys.
 *
 * @return                      A reference to the Binding object.
 */
Binding::Configuration& Binding::Configuration::Security_AppGroupKey(uint32_t aAppGroupGlobalId, uint32_t aRootKeyId, bool aUseRotatingKey)
{
    if (mError == WEAVE_NO_ERROR)
    {
#if WEAVE_CONFIG_USE_APP_GROUP_KEYS_FOR_MSG_ENC
        mError = mBinding.mExchangeManager->FabricState->GetMsgEncKeyIdForAppGroup(aAppGroupGlobalId, aRootKeyId, aUseRotatingKey, mBinding.mKeyId);
        if (mError == WEAVE_NO_ERROR)
        {
            mBinding.mSecurityOption = kSecurityOption_SpecificKey;
            mBinding.mAuthMode = GroupKeyAuthMode(mBinding.mKeyId);
        }
#else
        mError = WEAVE_ERROR_UNSUPPORTED_WEAVE_FEATURE;
#endif
    }
    return *this;
}

/**
 * When communicating with the peer, send and receive messages encrypted using the specified message encryption type.
 *
 * @param[in] aEncType          The Weave message encryption type.
 *
 * @return                      A reference to the Binding object.
 */
Binding::Configuration& Binding::Configuration::Security_EncryptionType(uint8_t aEncType)
{
    mBinding.mEncType = aEncType;
    return *this;
}

/**
 *  Set the requested authentication mode to be used to authenticate the peer.
 *
 *  @param[in] aAuthMode        The requested authentication mode.
 *
 *  @return                     A reference to the Binding object.
 */
Binding::Configuration& Binding::Configuration::Security_AuthenticationMode(WeaveAuthMode aAuthMode)
{
    mBinding.mAuthMode = aAuthMode;
    return *this;
}

#if WEAVE_CONFIG_ENABLE_MESSAGE_CAPTURE
/**
 *  Set the flag for capturing sent messages
 *
 *  @return                     A reference to the Binding object.
 */
Binding::Configuration& Binding::Configuration::CaptureTxMessage(void)
{
    mBinding.SetFlag(kFlag_CaptureTxMessage);
    return *this;
}
#endif // WEAVE_CONFIG_ENABLE_MESSAGE_CAPTURE

/**
 *  Configure the binding to allow communication with the sender of a received message.
 *
 *  @param[in]  aMsgInfo        Message information structure associated with the received message.
 *  @param[in]  aPacketInfo     Packet information for the received message.
 *
 */
Binding::Configuration& Binding::Configuration::ConfigureFromMessage(
        const nl::Weave::WeaveMessageInfo *aMsgInfo,
        const nl::Inet::IPPacketInfo *aPacketInfo)
{
    mBinding.mPeerNodeId = aMsgInfo->SourceNodeId;

    if (aMsgInfo->InCon != NULL)
    {
        Transport_ExistingConnection(aMsgInfo->InCon);
    }
    else
    {
        if (aMsgInfo->Flags & kWeaveMessageFlag_PeerRequestedAck)
        {
#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
            Transport_UDP_WRM();
#else
            mError = WEAVE_ERROR_NOT_IMPLEMENTED;
#endif // #if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
        }
        else
        {
            Transport_UDP();
        }

        // Configure the outgoing interface only if the received message is from a
        // link-local address because we need to specify the interface when we are
        // sending to a link local address. Otherwise, defer to the routing logic
        // to choose the outgoing interface.
        TargetAddress_IP(aPacketInfo->SrcAddress, aPacketInfo->SrcPort,
                aPacketInfo->SrcAddress.IsIPv6LinkLocal() ? aPacketInfo->Interface : INET_NULL_INTERFACEID);
    }

    if (aMsgInfo->KeyId == WeaveKeyId::kNone)
    {
        Security_None();
    }
    else
    {
        Security_Key(aMsgInfo->KeyId);
        Security_EncryptionType(aMsgInfo->EncryptionType);
    }

    return *this;
}

}; // Weave
}; // nl
