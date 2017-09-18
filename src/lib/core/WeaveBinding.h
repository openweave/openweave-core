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
 *      Defines the Weave Binding class and its supporting types.
 *
 */

#ifndef WEAVE_BINDING_H_
#define WEAVE_BINDING_H_

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif // __STDC_FORMAT_MACROS

#include <Weave/Core/WeaveWRMPConfig.h>
#include <Weave/Core/WeaveCore.h>

namespace nl {
namespace Weave {

class WeaveExchangeManager;
class ExchangeContext;

/**
 * @class Binding
 *
 * @brief
 *     Captures the intended target of a Weave communication and associated configuration
 *     information.
 *
 * A Binding object identifies the intended target of a Weave communication (also known as the
 * "peer"), along with a set of configuration parameters describing how communication with the
 * peer should take place. Bindings are independent of the application protocol being spoken
 * between the two parties. As such, they capture the "who" and the "how" of a communication,
 * but not the "what."
 *
 * ## Configuration
 *
 * Applications must configure a Binding with parameters specific to the type of communication
 * channel desired. Bindings provide support for a range of network transports, including
 * TCP, UDP, UDP with Weave Reliable Messaging, and Weave over BLE (WoBLE). Applications can
 * also request the use of specific security mechanisms to protect messages sent between the
 * parties. These include CASE and PASE sessions, and application group keys. The interface for
 * configuring a Binding uses a declarative API style that allows applications to state their
 * requirements for the communication in simple terms.
 *
 * See the documentation for Binding::Configuration for further details.
 *
 * ## Preparation
 *
 * Prior to communication taking place, a Binding must be "prepared." The act of preparing a
 * Binding involves establishing the necessary state for communication to take place. This can
 * include things such as: resolving the network address of the peer, establishing a network
 * connection, and negotiating security keys. Once configured by the application, the Binding
 * takes care of all the steps necessary to prepare for communication, calling back to the
 * application when the process is complete. In this way, Bindings hide the mechanics of
 * communication, allowing applications to concentrate on the high-level interactions.
 *
 * ## Communication
 *
 * Once a Binding has been prepared it becomes ready for use. In this state, applications (or
 * more commonly, protocol layer code working on behalf of an application) request the Binding
 * to allocate a Weave exchange context. The resultant exchange context comes pre-configured
 * for communication, allowing the application to immediately initiate a Weave exchange with
 * the peer. The application can continue to request exchange contexts from the Binding until
 * such time as the Binding is closed, or some event, e.g., a network failure, terminates the
 * underlying communication channel.
 *
 * ## Binding State Changes
 *
 * Over the course of its use, a Binding will deliver API events to the application informing it
 * of changes in the Binding's state. For example, when preparation succeeds, the application
 * will receive an event informing it that the Binding is ready for use. Similarly, if the
 * underlying communication channel fails, an event is delivered to the application informing
 * it that the Binding is no longer in the ready state.
 *
 * API events are delivered to the application via an event callback function supplied when
 * the Binding is allocated.
 *
 * ## Binding Lifetime
 *
 * Binding are reference counted to allow shared use across multiple software components.
 * When a Binding is allocated, a single reference to the binding is created. The application
 * is responsible for releasing this reference at some point in the future such that the
 * Binding is free for subsequent reuse.
 *
 * When an application is done with a Binding it may call Close() on the binding. This
 * releases the application's reference to the Binding and blocks all further delivery of API
 * events. When the last reference to a Binding is released, it is automatically closed.
 *
 */
class Binding
{
public:

    class Configuration;
    struct InEventParam;
    struct OutEventParam;

    enum State
    {
        kState_NotAllocated                         = 0,
        kState_NotConfigured                        = 1,
        kState_Configuring                          = 2,
        kState_Preparing                            = 3,
        kState_PreparingAddress                     = 4,
        kState_PreparingTransport                   = 5,
        kState_PreparingSecurity                    = 6,
        kState_PreparingSecurity_EstablishSession   = 7,
        kState_Ready                                = 8,
        kState_Closed                               = 9,
        kState_Failed                               = 10,
    };

    enum EventType
    {
        kEvent_BindingReady                         = 1,    //< The prepare action on the binding succeeded and the binding may now be used to communicate with the peer.
        kEvent_PrepareFailed                        = 2,    //< The prepare action on the binding failed.
        kEvent_BindingFailed                        = 3,    //< The binding failed and can no longer be used to communicate with the peer.
        kEvent_PrepareRequested                     = 4,    //< The application is requested to configure and prepare the binding for use by the network stack.

        kEvent_DefaultCheck                         = 100,  //< Used to verify correct default event handling in the application.
    };

    typedef void (*EventCallback)(void *apAppState, EventType aEvent, const InEventParam& aInParam, OutEventParam& aOutParam);

    void *AppState;

    void AddRef(void);
    void Release(void);
    void Close(void);

    Configuration BeginConfiguration();

    WEAVE_ERROR RequestPrepare();

    void Reset();

    State GetState(void) const;
    bool IsPreparing(void) const;
    bool IsReady(void) const;
    bool CanBePrepared(void) const;

    uint16_t GetLogId(void) const;

    uint64_t GetPeerNodeId(void) const;
    uint32_t GetKeyId(void) const;
    uint8_t GetEncryptionType(void) const;
    uint32_t GetDefaultResponseTimeout() const;
    void SetDefaultResponseTimeout(uint32_t msec);
#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
    const WRMPConfig& GetDefaultWRMPConfig(void) const;
    void SetDefaultWRMPConfig(const WRMPConfig& wrmpConfig);
#endif // #if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
    EventCallback GetEventCallback() const;
    void SetEventCallback(EventCallback aEventCallback);

    void GetProtocolLayerCallback(EventCallback& callback, void *& state) const;
    void SetProtocolLayerCallback(EventCallback callback, void *state);

    WEAVE_ERROR NewExchangeContext(ExchangeContext *& appExchangeContext);

    WEAVE_ERROR AdjustResponseTimeout(ExchangeContext * apExchangeContext) const;

    bool IsAuthenticMessageFromPeer(const WeaveMessageInfo *msgInfo);

    uint32_t GetMaxWeavePayloadSize(const System::PacketBuffer *msgBuf);

    static void DefaultEventHandler(void *apAppState, EventType aEvent, const InEventParam& aInParam, OutEventParam& aOutParam);

private:

    friend class WeaveExchangeManager;

    enum AddressingOption
    {
        kAddressing_NotSpecified                    = 0,
        kAddressing_UnicastIP                       = 1,
        kAddressing_HostName                        = 2,
        kAddressing_WeaveFabric                     = 3,
        kAddressing_ServiceDirectory                = 4,
        kAddressing_MulticastIP                     = 5,
    };

    enum TransportOption
    {
        kTransport_NotSpecified                     = 0,
        kTransport_UDP                              = 1,
        kTransport_UDP_WRM                          = 2,
        kTransport_TCP                              = 3,
        kTransport_ExistingConnection               = 4,
    };

    enum SecurityOption
    {
        kSecurityOption_NotSpecified                = 0,
        kSecurityOption_None                        = 1,
        kSecurityOption_SpecificKey                 = 2,
        kSecurityOption_SharedCASESession           = 3,
    };

    WeaveExchangeManager * mExchangeManager;

    uint8_t mRefCount;
    State mState : 4;
    SecurityOption mSecurityOption : 4;
    AddressingOption mAddressingOption : 4;
    TransportOption mTransportOption : 4;

    EventCallback mAppEventCallback;
    EventCallback mProtocolLayerCallback;
    void *mProtocolLayerState;

    uint64_t mPeerNodeId;

    // Addressing-specific configuration
    InterfaceId mInterfaceId;
    uint16_t mPeerPort;

    // Security-specific configuration
    uint8_t mEncType;
    WeaveAuthMode mAuthMode;
    uint32_t mKeyId;

    uint32_t mDefaultResponseTimeoutMsec;
#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
    WRMPConfig mDefaultWRMPConfig;
#endif

    // Note that IPAddress has a (redundant) copy assignment operator defined, so it cannot be part of any union pre C++11
    // This is not a big deal, but peer address is not really needed in ServiceDirectory scenarios
    nl::Inet::IPAddress mPeerAddress;

    WEAVE_ERROR Init(void * apAppState, EventCallback aEventCallback);
    void ResetConfig();

    WEAVE_ERROR DoPrepare(WEAVE_ERROR configErr);
    void PrepareAddress();
    void PrepareTransport();
    void PrepareSecurity();
    void DoClose(void);
    void HandleBindingReady();
    void HandleBindingFailed(WEAVE_ERROR err, bool raiseEvent);
    void OnSecureSessionReady(uint64_t peerNodeId, uint8_t encType, WeaveAuthMode authMode, uint16_t keyId);
    void OnKeyError(const uint32_t aKeyId, const uint64_t aPeerNodeId, const WEAVE_ERROR aKeyErr);
};

/**
 * Provides a declarative-style interface for configuring and preparing a Binding object.
 *
 * When configuring a Binding, applications must call at least one method from each of the
 * following configuration groups: Target, Transport, and Security. Other methods may be
 * called as needed to override default behavior.
 *
 * If mutually exclusive configurations are invoked (e.g., Transport_TCP() followed by
 * Transport_UDP()), the last one to be called wins.
 */
class Binding::Configuration
{
public:

    // NOTE TO IMPLEMENTERS: Binding::Configuration uses a declarative-style interface.  This means
    // the application is free to call the object's configuration methods IN ANY ORDER.  Any new
    // methods added to the class must follow this pattern.

    Configuration& Target_NodeId(uint64_t aPeerNodeId);
    Configuration& Target_ServiceEndpoint(uint64_t aPeerNodeId);

    Configuration& TargetAddress_WeaveService(void);
    Configuration& TargetAddress_WeaveFabric(uint16_t aSubnetId);
    Configuration& TargetAddress_IP(nl::Inet::IPAddress aPeerAddress, uint16_t aPeerPort = WEAVE_PORT, InterfaceId aInterfaceId = INET_NULL_INTERFACEID);

    Configuration& Transport_TCP(void);
    Configuration& Transport_UDP(void);
    Configuration& Transport_UDP_WRM(void);
    Configuration& Transport_DefaultWRMPConfig(const WRMPConfig& aWRMPConfig);
    Configuration& Transport_ExistingConnection(WeaveConnection *apConnection);

    Configuration& Exchange_ResponseTimeoutMsec(uint32_t aResponseTimeoutMsec);

    Configuration& Security_None(void);
    Configuration& Security_SharedCASESession(void);
    Configuration& Security_SharedCASESession(uint64_t aRouterNodeId);
    Configuration& Security_Key(uint32_t aKeyId);
    Configuration& Security_AppGroupKey(uint32_t aAppGroupGlobalId, uint32_t aRootKeyId, bool aUseRotatingKey);
    Configuration& Security_EncryptionType(uint8_t aEncType);
    Configuration& Security_AuthenticationMode(WeaveAuthMode aAuthMode);

    Configuration& ConfigureFromMessage(
            const WeaveMessageInfo *apMsgHeader,
            const nl::Inet::IPPacketInfo *apPktInfo,
            WeaveConnection *apConnection = NULL);

    WEAVE_ERROR PrepareBinding(void);

    WEAVE_ERROR GetError(void) const;

private:
    friend class Binding;

    Binding& mBinding;
    WEAVE_ERROR mError;

    Configuration(Binding& aBinding);
};

/**
 * Input parameters to a Binding API event.
 */
struct Binding::InEventParam
{
    Binding *Source;
    union
    {
        struct
        {
            WEAVE_ERROR Reason;
        } PrepareFailed;

        struct
        {
            WEAVE_ERROR Reason;
        } BindingFailed;
    };

    void Clear() { memset(this, 0, sizeof(*this)); }
};

/**
 * Output parameters to a Binding API event.
 */
struct Binding::OutEventParam
{
    bool DefaultHandlerCalled;
    union
    {
        struct
        {
            WEAVE_ERROR PrepareError;
        } PrepareRequested;
    };

    void Clear() { memset(this, 0, sizeof(*this)); }
};


/*
 * Inline Functions
 *
 * Documentation for these functions can be found in the .cpp file.
 */

inline Binding::State Binding::GetState(void) const
{
    return mState;
}

inline bool Binding::IsPreparing() const
{
    return mState >= kState_Preparing && mState < kState_Ready;
}

inline bool Binding::IsReady() const
{
    return mState == kState_Ready;
}

inline bool Binding::CanBePrepared(void) const
{
    return mState == kState_NotConfigured || mState == kState_Failed;
}

inline uint64_t Binding::GetPeerNodeId() const
{
    return mPeerNodeId;
}

inline uint32_t Binding::GetKeyId(void) const
{
    return mKeyId;
}

inline uint8_t Binding::GetEncryptionType(void) const
{
    return mEncType;
}

inline uint32_t Binding::GetDefaultResponseTimeout() const
{
    return mDefaultResponseTimeoutMsec;
}

inline void Binding::SetDefaultResponseTimeout(uint32_t timeout)
{
    mDefaultResponseTimeoutMsec = timeout;
}

#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING

inline const WRMPConfig& Binding::GetDefaultWRMPConfig(void) const
{
    return mDefaultWRMPConfig;
}

inline void Binding::SetDefaultWRMPConfig(const WRMPConfig& aWRMPConfig)
{
    mDefaultWRMPConfig = aWRMPConfig;
}

#endif // #if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING

inline Binding::EventCallback Binding::GetEventCallback() const
{
    return mAppEventCallback;
}

inline void Binding::SetEventCallback(EventCallback aEventCallback)
{
    mAppEventCallback = aEventCallback;
}

inline void Binding::GetProtocolLayerCallback(EventCallback& callback, void *& state) const
{
    callback = mProtocolLayerCallback;
    state = mProtocolLayerState;
}

inline void Binding::SetProtocolLayerCallback(EventCallback callback, void *state)
{
    mProtocolLayerCallback = callback;
    mProtocolLayerState = state;
}

inline Binding::Configuration Binding::BeginConfiguration()
{
    return Configuration(*this);
}

inline WEAVE_ERROR Binding::Configuration::PrepareBinding(void)
{
    return mBinding.DoPrepare(mError);
}

inline WEAVE_ERROR Binding::Configuration::GetError(void) const
{
    return mError;
}

}; // Weave
}; // nl

#endif // WEAVE_BINDING_H_
