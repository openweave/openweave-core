/*
 *
 *    Copyright (c) 2019 Google LLC.
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
 *      This file defines the Certificate Provisioning Protocol, used to
 *      get new Weave operational device certificate from the CA service.
 *
 */

#ifndef WEAVECERTPROVISIONING_H_
#define WEAVECERTPROVISIONING_H_

#include <Weave/Core/WeaveCore.h>

/**
 *   @namespace nl::Weave::Profiles::Security::CertProvisioning
 *
 *   @brief
 *     This namespace includes all interfaces within Weave for the
 *     Weave Certificate Provisioning protocol within the Weave
 *     security profile.
 */

namespace nl {
namespace Weave {
namespace Profiles {
namespace Security {
namespace CertProvisioning {

class WeaveCertProvClient;

enum EventType
{
    kEvent_PrepareAuthorizeInfo                 = 1,    /**< The application is requested to prepare the payload for the GetCertificateRequest. */
    kEvent_RequestSent                          = 2,    /**< A GetCertificateRequest message was sent to the peer. */
    kEvent_ResponseReceived                     = 3,    /**< A GetCertificateResponse message was received from the peer. */
    kEvent_ResponseProcessError                 = 4,    /**< An error accurred while processing GetCertificateResponse message. */
    kEvent_CommuncationError                    = 5,    /**< A communication error occurred while sending a GetCertificateRequest or waiting for a response. */
    kEvent_ResponseTimeout                      = 6,    /**< An GetCertificateResponse was not received in the alloted time. */
};

enum
{
    kReqType_NotSpecified                       = 0,
    kReqType_GetInitialOpDeviceCert             = 1,
    kReqType_RotateCert                         = 2,
};

/**
 * Input parameters to Weave Certificate Provisioning API event.
 */
struct InEventParam
{
    WeaveCertProvClient * Source;                       /**< The WeaveCertProvClient from which the API event originated. */

    union
    {
        struct
        {
            TLVWriter * Writer;                         /**< A pointer to the TLV Writer object, where get certificate authorization
                                                             information should be encoded. */
        } PrepareAuthorizeInfo;

        struct
        {
            WEAVE_ERROR Reason;                         /**< The error code associated with the communication failure. */
        } CommuncationError;

        struct
        {
            const uint8_t * Cert;                       /**< A pointer to the TLV encoded Weave operational certificate assigned by CA Service. */
            uint16_t CertLen;                           /**< Length of the certificate received in the GetCertificateResponse message. */
            const uint8_t * RelatedCerts;               /**< A pointer to the TLV encoded list of certificate related to the operational certificate. */
            uint16_t RelatedCertsLen;                   /**< Length of the related certificate list received in the GetCertificateResponse message. */
        } ResponseReceived;

        struct
        {
            WEAVE_ERROR Reason;                         /**< The error code associated with the response message processing failure. */
        } ResponseProcessError;
    };

    void Clear() { memset(this, 0, sizeof(*this)); }
};

/**
 * Output parameters to Weave Certificate Provisioning API event.
 */
struct OutEventParam
{
    union
    {
        struct
        {
            WEAVE_ERROR Error;                          /**< An error set by the application indicating that
                                                             an authorization info couldn't be prepared. */
        } PrepareAuthorizeInfo;
    };

    void Clear() { memset(this, 0, sizeof(*this)); }
};

/**
 *  This function is the application callback that is invoked on Certificate Provisioning Client API events.
 *
 *  @param[in]  appState    A pointer to application-defined state information associated with the client object.
 *  @param[in]  eventType   Event ID passed by the event callback.
 *  @param[in]  inParam     Reference of input event parameters passed by the event callback.
 *  @param[in]  outParam    Reference of output event parameters passed by the event callback.
 */
typedef void (* EventCallback)(void * appState, EventType eventType, const InEventParam & inParam, OutEventParam & outParam);

/**
 * Abstract interface to which platform specific actions are delegated during
 * certificate provisioned protocol.
 */
class WeaveCertProvAuthDelegate
{
public:

    // ===== Abstract Interface methods

    /**
     * Encode authentication certificate for the local node.
     *
     * When invoked, the implementation should write a structure containing:
     *   -- local node operational certificate for operational authentication.
     *   -- local node factory provisioned certificate for manufacturer attestation.
     */
    virtual WEAVE_ERROR EncodeNodeCert(TLVWriter & writer) = 0;

    /**
     * Generate and encode authentication signature using one of:
     *   -- local node operational private key for operational authentication.
     *   -- local node factory provisioned private key for manufacturer attestation.
     *
     * When invoked, implementations must compute a signature on the given hash value
     * using the node's private key. The generated signature should then be written in the
     * form of a ECDSASignature or RSASignature structure to the supplied TLV writer.
     *
     * Note: in cases where the node's corresponding Elliptic Curve private key is held
     * in a local buffer, the GenerateAndEncodeWeaveECDSASignature() utility function
     * can be useful for implementing this method.
     */
    virtual WEAVE_ERROR GenerateNodeSig(const uint8_t * hash, uint8_t hashLen, TLVWriter & writer) = 0;
};

extern WEAVE_ERROR GenerateGetCertificateRequest(PacketBuffer * msgBuf, uint8_t reqType,
                                                 WeaveCertProvAuthDelegate * opAuthDelegate,
                                                 WeaveCertProvAuthDelegate * manufAttestDelegate,
                                                 EventCallback eventCallback, void * appState = NULL);

extern WEAVE_ERROR ProcessGetCertificateResponse(PacketBuffer * msgBuf,
                                                 EventCallback eventCallback, void * appState = NULL);


/**
 * Implements the core logic of the Weave Certificate Provisioning Client object.
 */
class NL_DLL_EXPORT WeaveCertProvClient
{
public:

    enum State
    {
        kState_NotInitialized                       = 0,    /**< The client object is not initialized. */
        kState_Idle                                 = 1,    /**< The client object is idle. */
        kState_PreparingBinding                     = 2,    /**< The client object is waiting for the binding to become ready. */
        kState_RequestInProgress                    = 3,    /**< A GetCertificateRequest message has been sent and the client object is awaiting a response. */
    };

    void * AppState;                                        /**< A pointer to application-specific data. */

    WeaveCertProvClient(void);

    WEAVE_ERROR Init(Binding * binding, WeaveCertProvAuthDelegate * opAuthDelegate, WeaveCertProvAuthDelegate * manufAttestDelegate, EventCallback eventCallback, void * appState = NULL);
    void Shutdown(void);

    WEAVE_ERROR StartCertificateProvisioning(uint8_t reqType);

    State GetState(void) const;

    bool RequestInProgress() const;

    Binding * GetBinding(void) const;

    WeaveCertProvAuthDelegate * GetOpAuthDelegate(void) const;
    void SetOpAuthDelegate(WeaveCertProvAuthDelegate * opAuthDelegate);

    WeaveCertProvAuthDelegate * GetManufAttestDelegate(void) const;
    void SetManufAttestDelegate(WeaveCertProvAuthDelegate * manufAttestDelegate);

    EventCallback GetEventCallback(void) const;
    void SetEventCallback(EventCallback eventCallback);

private:

    uint8_t mReqType;
    Binding * mBinding;
    WeaveCertProvAuthDelegate *mOpAuthDelegate;
    WeaveCertProvAuthDelegate *mManufAttestDelegate;
    EventCallback mEventCallback;
    ExchangeContext * mEC;
    State mState;

    WEAVE_ERROR SendGetCertificateRequest(void);
    void HandleRequestDone(void);
    void ClearRequestState(void);
    void DeliverCommunicationError(WEAVE_ERROR err);

    static void HandleBindingEvent(void * const appState, const Binding::EventType eventType, const Binding::InEventParam & inParam,
                                   Binding::OutEventParam & outParam);
    static void HandleResponse(ExchangeContext * ec, const IPPacketInfo * pktInfo, const WeaveMessageInfo * msgInfo,
                               uint32_t profileId, uint8_t msgType, PacketBuffer * payload);
    static void HandleResponseTimeout(ExchangeContext * ec);
#if WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
    static void HandleAckRcvd(ExchangeContext * ec, void * msgCtxt);
#endif // WEAVE_CONFIG_ENABLE_RELIABLE_MESSAGING
    static void HandleSendError(ExchangeContext * ec, WEAVE_ERROR sendErr, void * msgCtxt);
    static void HandleKeyError(ExchangeContext * ec, WEAVE_ERROR keyErr);
    static void HandleConnectionClosed(ExchangeContext *ec, WeaveConnection *con, WEAVE_ERROR conErr);
};


/*
 * Inline Functions
 *
 * Documentation for these functions can be found at the end of the .cpp file.
 */

inline WeaveCertProvClient::WeaveCertProvClient(void)
{
}

inline WeaveCertProvClient::State WeaveCertProvClient::GetState(void) const
{
    return mState;
}

inline bool WeaveCertProvClient::RequestInProgress() const
{
    return mState == kState_RequestInProgress;
}

inline Binding *WeaveCertProvClient::GetBinding(void) const
{
    return mBinding;
}

inline WeaveCertProvAuthDelegate * WeaveCertProvClient::GetOpAuthDelegate(void) const
{
    return mOpAuthDelegate;
}

inline void WeaveCertProvClient::SetOpAuthDelegate(WeaveCertProvAuthDelegate * opAuthDelegate)
{
    mOpAuthDelegate = opAuthDelegate;
}

inline WeaveCertProvAuthDelegate * WeaveCertProvClient::GetManufAttestDelegate(void) const
{
    return mManufAttestDelegate;
}

inline void WeaveCertProvClient::SetManufAttestDelegate(WeaveCertProvAuthDelegate * manufAttestDelegate)
{
    mManufAttestDelegate = manufAttestDelegate;
}

inline EventCallback WeaveCertProvClient::GetEventCallback(void) const
{
    return mEventCallback;
}

inline void WeaveCertProvClient::SetEventCallback(EventCallback eventCallback)
{
    mEventCallback = eventCallback;
}

} // namespace CertProvisioning
} // namespace Security
} // namespace Profiles
} // namespace Weave
} // namespace nl

#endif /* WEAVECERTPROVISIONING_H_ */
