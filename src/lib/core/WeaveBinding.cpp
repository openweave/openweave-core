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
 * @return                         The binding state.
 */

/**
 * @fn bool Binding::IsPreparing() const
 *
 * Returns true if the Binding is currently being prepared.
 */

/**
 * @fn bool Binding::IsReady() const
 *
 * Returns true if the Binding is in the Ready state.
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
 * @fn const nl::Weave::WRMPConfig& Binding::GetDefaultWRMPConfig(void) const
 *
 * Get the default WRMP configuration to be used when communicating with the peer.
 *
 * @return                          A reference to a WRMPConfig structure containing
 *                                  the default configuration values.
 */

/**
 * @fn void Binding::SetDefaultWRMPConfig(const nl::Weave::WRMPConfig& aWRMPConfig)
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
 *  If there are no more references to the binding object, the binding is closed and the associated
 *  resources are freed.
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
 * Close the binding object and release the reference.
 *
 * When closed, the state of the binding is reset and no further API callbacks will be made to the application.
 */
void Binding::Close(void)
{
    VerifyOrDie(mState != kState_NotAllocated);
    VerifyOrDie(mRefCount > 0);

    DoClose();
    Release();
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
 * Transition the Binding to the Closed state.
 */
void Binding::DoClose()
{
    VerifyOrDie(mState != kState_NotAllocated);

    // If not already closed...
    if (mState != kState_Closed)
    {
        // TODO FUTURE: Abort any in-progress async prepare actions associated with the binding,
        // such as pending connection / session establishments.

        // Clear pointers to application state/code to prevent any further use.
        AppState = NULL;
        SetEventCallback(NULL);
        SetProtocolLayerCallback(NULL, NULL);

        // Reset the configuration fields.
        ResetConfig();

        // Mark the binding as closed.
        mState = kState_Closed;

        WeaveLogDetail(ExchangeManager, "Binding[%" PRIu8 "] (%" PRIu16 "): Closed", GetLogId(), mRefCount);
    }
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
 * Reset the configuration parameters to their default values.
 */
void Binding::ResetConfig()
{
    mPeerNodeId = kNodeIdNotSpecified;

    mAddressingOption = kAddressing_NotSpecified;
    mPeerPort = WEAVE_PORT;
    mInterfaceId = INET_NULL_INTERFACEID;

    mTransportOption = kTransport_NotSpecified;
    mDefaultResponseTimeoutMsec = 0;
#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
    mDefaultWRMPConfig = gDefaultWRMPConfig;
#endif

    mSecurityOption = kSecurityOption_NotSpecified;
    mKeyId = WeaveKeyId::kNone;
    mEncType = kWeaveEncryptionType_None;
    mAuthMode = kWeaveAuthMode_Unauthenticated;
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
 * Reset the binding back to an unconfigured state.
 *
 * Note that this method has no effect on a Binding that is already in the Closed state.
 */
void Binding::Reset()
{
    if (mState != kState_NotAllocated && mState != kState_Closed)
    {
        ResetConfig();
        mState = kState_NotConfigured;
    }
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

    mState = kState_Preparing;

    WeaveLogDetail(ExchangeManager, "Binding[%" PRIu8 "] (%" PRIu16 "): Preparing", GetLogId(), mRefCount);

    // Start by preparing the peer address
    PrepareAddress();

exit:
    if (WEAVE_NO_ERROR != err)
    {
        HandleBindingFailed(err, false);
    }
    WeaveLogFunctError(err);
    return err;
}

/**
 * Do any work necessary to determine the address of the peer in preparation for communication.
 */
void Binding::PrepareAddress()
{
    mState = kState_PreparingAddress;

    // TODO FUTURE: Add support for hostname resolution

    // Default to using a Weave fabric address in the default subnet if an address was not specified.
    if (kAddressing_NotSpecified == mAddressingOption)
    {
        mPeerAddress = mExchangeManager->FabricState->SelectNodeAddress(mPeerNodeId);
    }

    // If requested, form a Weave fabric address for the peer in the configured subnet.
    else if (kAddressing_WeaveFabric == mAddressingOption)
    {
        mPeerAddress = mExchangeManager->FabricState->SelectNodeAddress(mPeerNodeId, mPeerAddress.Subnet());
    }

    PrepareTransport();
}

/**
 * Do any work necessary to determine to establish transport-level communication with the peer.
 */
void Binding::PrepareTransport()
{
    mState = kState_PreparingTransport;

    // TODO FUTURE: Add support for TCP and existing connection

    PrepareSecurity();
}

/**
 * Do any work necessary to establish communication security with the peer.
 */
void Binding::PrepareSecurity()
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    mState = kState_PreparingSecurity;

    // Default encryption type, if not specified.
    if (kSecurityOption_None != mSecurityOption && kWeaveEncryptionType_None == mEncType)
    {
        mEncType = kWeaveEncryptionType_AES128CTRSHA1;
    }

    switch (mSecurityOption)
    {
    case kSecurityOption_SharedCASESession:
        {
            // This is also defined in Weave/Profiles/ServiceDirectory.h, but this is in Weave Core
            // TODO: move this to a common location.
            static const uint64_t kServiceEndpoint_CoreRouter = 0x18B4300200000012ull;

            const uint64_t fabricGlobalId = WeaveFabricIdToIPv6GlobalId(mExchangeManager->FabricState->FabricId);
            const IPAddress coreRouterAddress = IPAddress::MakeULA(fabricGlobalId, nl::Weave::kWeaveSubnetId_Service,
                                       nl::Weave::WeaveNodeIdToIPv6InterfaceId(kServiceEndpoint_CoreRouter));

            WeaveLogDetail(ExchangeManager, "Binding[%" PRIu8 "] (%" PRIu16 "): Initiating shared CASE session", GetLogId(), mRefCount);

            mState = kState_PreparingSecurity_EstablishSession;

            // Note that security manager could drive the state of this binding to kState_Ready in this function
            // if the session is already available. It could also call failure handler synchronously.
            err = mExchangeManager->MessageLayer->SecurityMgr->StartCASESession(
                    NULL, mPeerNodeId, coreRouterAddress, WEAVE_PORT, mAuthMode, NULL,
                    NULL, NULL, NULL, kServiceEndpoint_CoreRouter);
            SuccessOrExit(err);
        }
        break;

    case kSecurityOption_SpecificKey:
    case kSecurityOption_None:
        // No further preparation needed.
        HandleBindingReady();
        break;

    default:
        ExitNow(err = WEAVE_ERROR_INVALID_ARGUMENT);
    }

exit:
    if (WEAVE_NO_ERROR != err)
    {
        HandleBindingFailed(err, true);
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

    {
        char ipAddrStr[64];
        char intfStr[64];
        const char *transportStr;
        mPeerAddress.ToString(ipAddrStr, sizeof(ipAddrStr));
        nl::Inet::GetInterfaceName(mInterfaceId, intfStr, sizeof(intfStr));
        switch (mTransportOption)
        {
        case kTransport_UDP:
            transportStr = "UDP";
            break;
        case kTransport_UDP_WRM:
            transportStr = "WRM";
            break;
        case kTransport_TCP:
            transportStr = "TCP"; // TODO FUTURE: Add id of connection
            break;
        case kTransport_ExistingConnection:
            transportStr = "ExistingCon"; // TODO FUTURE: Add id of connection
            break;
        default:
            transportStr = "Unknown";
            break;
        }
        WeaveLogDetail(ExchangeManager, "Binding[%" PRIu8 "] (%" PRIu16 "): Ready, peer %016" PRIX64 " @ [%s]:%" PRId16 " (%s) via %s",
                GetLogId(), mRefCount, mPeerNodeId, ipAddrStr, mPeerPort,
                (mInterfaceId != INET_NULL_INTERFACEID) ? intfStr : "default",
                transportStr);
    }

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
void Binding::HandleBindingFailed(WEAVE_ERROR err, bool raiseEvents)
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
        eventType = kEvent_PrepareFailed;
    }
    else
    {
        inParam.BindingFailed.Reason = err;
        eventType = kEvent_BindingFailed;
    }

    mState = kState_Failed;

    WeaveLogDetail(ExchangeManager, "Binding[%" PRIu8 "] (%" PRIu16 "): %s: peer %" PRIX64 ", %s",
            GetLogId(), mRefCount,
            (eventType == kEvent_BindingFailed) ? "Binding FAILED" : "Prepare FAILED",
            mPeerNodeId, ErrorStr(err));

    ResetConfig();

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

/**
 * Invoked when a security session establishment has completed successfully.
 */
void Binding::OnSecureSessionReady(uint64_t peerNodeId, uint8_t encType, WeaveAuthMode authMode, uint16_t keyId)
{
    // NOTE: This method is called whenever a new session is established.  Thus the code
    // must filter for the specific key applies to the current binding.

    // Ignore the key if the binding is not in the Preparing_EstablishingSecureSession state.
    VerifyOrExit(mState == kState_PreparingSecurity_EstablishSession, /* no-op */);

    // Ignore the key if it is not for the specified peer node.
    VerifyOrExit(peerNodeId == mPeerNodeId, /* no-op */);

    // Ignore the key if its not a session key.
    VerifyOrExit(WeaveKeyId::IsSessionKey(keyId), /* no-op */);

    // Save the session key id and encryption type.
    mKeyId = keyId;
    mEncType = encType;

    // Tell the application that the binding is ready.
    HandleBindingReady();

exit:
    return;
}

/**
 * Invoked security session establishment has failed or a key error has occurred.
 */
void Binding::OnKeyError(uint32_t aKeyId, uint64_t aPeerNodeId, WEAVE_ERROR aKeyErr)
{
    // NOTE: This method is called for any and all key errors that occur system-wide.  Thus this code
    // must filter for errors that apply to the current binding.

    // Ignore the key error if the binding is not in the Ready state or one of the preparing states.
    VerifyOrExit(IsPreparing() || mState == kState_Ready, /* no-op */);

    // Ignore the key error if it is not in relation to the specified peer node.
    VerifyOrExit(aPeerNodeId == mPeerNodeId, /* no-op */);

    // Ignore the key error if the binding is in the Ready state and the failed key id does
    // not match the key id associated with the binding.
    VerifyOrExit(mState != kState_Ready || aKeyId == mKeyId, /* no-op */);

    // Fail the binding.
    HandleBindingFailed(aKeyErr, true);

exit:
    return;
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
 * This method confirms that the message in question originated from the peer node of the binding and
 * that the encryption key and type used to encrypt the message matches those configured in the binding.
 * For bindings configured without the use of security, the method confirms that the incoming message is
 * NOT encrypted.
 *
 * This method is intended to be used in protocols such as WDM where peers can spontaneously initiate
 * exchanges back to the local node after an initial exchange from the node to the peer.  In such cases,
 * the method allows the local node to confirm that the incoming unsolicited message was sent by the
 * associated peer.  (Of course, for Bindings configured without the use of message encryption, this
 * assertion provides no value from a security perspective.  It merely confirms that the sender node
 * id in the received message matches the peer's node id.)
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
 *  @return the max Weave payload size.
 */
uint32_t Binding::GetMaxWeavePayloadSize(const System::PacketBuffer *msgBuf)
{
    // Constrain the max Weave payload size by the UDP MTU if we are using UDP.
    // TODO: Eventually, we may configure a custom UDP MTU size on the binding
    //       instead of using the default value directly.
    bool isUDP = (mTransportOption == kTransport_UDP || mTransportOption == kTransport_UDP_WRM);
    return WeaveMessageLayer::GetMaxWeavePayloadSize(msgBuf, isUDP, WEAVE_CONFIG_DEFAULT_UDP_MTU_SIZE);
}

/**
 * Allocate a new Exchange Context for communicating with the peer that is the target of the binding.
 *
 * @param[out] ec                       A reference to a pointer that will receive the newly allocated
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
WEAVE_ERROR Binding::NewExchangeContext(nl::Weave::ExchangeContext *& ec)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    ec = NULL;

    // Fail if the binding is not in the Ready state.
    VerifyOrExit(kState_Ready == mState, err = WEAVE_ERROR_INCORRECT_STATE);

    // Attempt to allocate a new exchange context.
    ec = mExchangeManager->NewContext(mPeerNodeId, mPeerAddress, mPeerPort, mInterfaceId, NULL);
    VerifyOrExit(NULL != ec, err = WEAVE_ERROR_NO_MEMORY);

    // TODO FUTURE: Add support for connection-based exchanges

#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING

    // Set the default WRMP configuration in the new exchange.
    ec->mWRMPConfig = mDefaultWRMPConfig;

    // If Weave reliable messaging was expressly requested as a transport...
    if (mTransportOption == kTransport_UDP_WRM)
    {
        // Enable the auto-request ACK feature in the exchange so that all outgoing messages
        // include a request for acknowledgment.
        ec->SetAutoRequestAck(true);
    }

#endif // WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING

    // If message encryption is enabled...
    if (mSecurityOption != kSecurityOption_None)
    {
        uint32_t keyId;

        // If the key id specifies a logical group key (e.g. the "current" rotating group key), resolve it to
        // the id for a specific key.
        err = mExchangeManager->FabricState->GroupKeyStore->GetCurrentAppKeyId(mKeyId, keyId);
        SuccessOrExit(err);

        // Configure the binding with the selected key id and encryption type.
        ec->KeyId = keyId;
        ec->EncryptionType = mEncType;
    }

    err = AdjustResponseTimeout(ec);
    SuccessOrExit(err);

exit:
    if (err != WEAVE_NO_ERROR && ec != NULL)
    {
        ec->Close();
        ec = NULL;
    }
    WeaveLogFunctError(err);
    return err;
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
    mError = WEAVE_ERROR_NOT_IMPLEMENTED;
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
    mError = WEAVE_ERROR_NOT_IMPLEMENTED;
#endif // WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
    return *this;
}

/**
 * Use an existing Weave connection to communicate with the peer.
 */
Binding::Configuration& Binding::Configuration::Transport_ExistingConnection(WeaveConnection *)
{
    mError = WEAVE_ERROR_NOT_IMPLEMENTED;
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
    mBinding.mSecurityOption = kSecurityOption_SharedCASESession;
    mBinding.mKeyId = WeaveKeyId::kNone;
    mBinding.mAuthMode = kWeaveAuthMode_CASE_ServiceEndPoint;
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
    // This is also defined in Weave/Profiles/ServiceDirectory.h, but this is in Weave Core
    // TODO: move this elsewhere.
    static const uint64_t kServiceEndpoint_CoreRouter = 0x18B4300200000012ull;

    // TODO: generalize this
    // Only support the router to be Core Router in Nest service
    VerifyOrExit(kServiceEndpoint_CoreRouter == aRouterNodeId, mError = WEAVE_ERROR_NOT_IMPLEMENTED);

    Security_SharedCASESession();

exit:
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

/**
 *  Configure the binding to allow communication with the sender of a received message.
 *
 *  @param[in]  apMsgHeader     Message information structure associated with the received message.
 *  @param[in]  apConnection    The connection over which the message was received; or NULL if the message
 *                              was not received via a connection.
 *  @param[in]  apPktInfo       Packet information for the received message.
 *
 */
Binding::Configuration& Binding::Configuration::ConfigureFromMessage(
        const nl::Weave::WeaveMessageHeader *apMsgHeader,
        const nl::Inet::IPPacketInfo *apPktInfo,
        WeaveConnection *apConnection)
{
    mBinding.mPeerNodeId = apMsgHeader->SourceNodeId;

    TargetAddress_IP(apPktInfo->SrcAddress, apPktInfo->SrcPort, apPktInfo->Interface);

    if (apConnection != NULL)
    {
        Transport_ExistingConnection(apConnection);
    }
    else if (apMsgHeader->Flags & kWeaveMessageFlag_PeerRequestedAck)
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

    if (apMsgHeader->KeyId == WeaveKeyId::kNone)
    {
        Security_None();
    }
    else
    {
        Security_Key(apMsgHeader->KeyId);
        Security_EncryptionType(apMsgHeader->EncryptionType);
    }

    return *this;
}

}; // Weave
}; // nl
