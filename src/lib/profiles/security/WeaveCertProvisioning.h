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

/**
 * Abstract interface to which platform specific actions are delegated during
 * Weave node operational authentication.
 */
class WeaveNodeOpAuthDelegate
{
public:

    // ===== Abstract Interface methods

    /**
     * Encode Weave operational certificate for the local node.
     *
     * When invoked, the implementation should write a local node operational certificate.
     * The operational certificate should then be written in the form of a WeaveCertificate
     * structure to the supplied TLV writer using the specified tag.
     */
    virtual WEAVE_ERROR EncodeOpCert(TLVWriter & writer, uint64_t tag) = 0;

    /**
     * Encode array of certificates related to the node operational certificate.
     *
     * When invoked, the implementation should write certificates related to local node
     * operational certificate. The related certificates should then be written in the form
     * of an array of WeaveCertificate structures to the supplied TLV writer using the specified tag.
     */
    virtual WEAVE_ERROR EncodeOpRelatedCerts(TLVWriter & writer, uint64_t tag) = 0;

    /**
     * Generate and encode operational signature using local node's operational private key.
     *
     * When invoked, implementations must compute a signature on the given hash value
     * using the node's operational private key. The generated signature should then
     * be written in the form of a ECDSASignature structure to the supplied TLV writer
     * using the specified tag.
     *
     * Note: in cases where the node's corresponding Elliptic Curve private key is held
     * in a local buffer, the GenerateAndEncodeWeaveECDSASignature() utility function
     * can be useful for implementing this method.
     */
    virtual WEAVE_ERROR GenerateAndEncodeOpSig(const uint8_t * hash, uint8_t hashLen, TLVWriter & writer, uint64_t tag) = 0;
};

/**
 * Abstract interface to which platform specific actions are delegated during
 * Weave node manufacturer attestation.
 */
class WeaveNodeManufAttestDelegate
{
public:

    // ===== Abstract Interface methods

    /**
     * Encode Weave manufacturer attestation information for the local node.
     *
     * When invoked, the implementation should write a structure containing information
     * used for node's manufacturer attestation. The manufacturer attestation information
     * should be written in the form of a TLV structure to the supplied TLV writer using
     * the Security Profile specific tag.
     */
    virtual WEAVE_ERROR EncodeMAInfo(TLVWriter & writer) = 0;

    /**
     * Generate and encode manufacturer attestation signature using local node's
     * manufacturer attestation private key.
     *
     * When invoked, implementations must compute a signature on the given hash value
     * using the node's manufacturer attestation private key.
     *
     * First, the enumerated value identifying the manufacturer attestation signature
     * algorithm should be written in the form of unsiged integer to the supplied TLV
     * writer using the following tag:
     *     -- kTag_GetCertReqMsg_ManufAttestSigAlgo
     * Legal enumerated values are taken from the kOID_SigAlgo_* constant namespace.
     *
     * The generated signature should then be written in the form of a ECDSASignature,
     * RSASignature, HMACSignature, or custom structure to the supplied TLV writer
     * using one of the following tags:
     *     -- kTag_GetCertReqMsg_ManufAttestSig_ECDSA
     *     -- kTag_GetCertReqMsg_ManufAttestSig_RSA
     *     -- kTag_GetCertReqMsg_ManufAttestSig_HMAC
     *     -- custom security profile specific tag
     *
     * Note: in cases where the node's corresponding Elliptic Curve private key is held
     * in a local buffer, the GenerateAndEncodeWeaveECDSASignature() utility function
     * can be useful for implementing this method.
     */
    virtual WEAVE_ERROR GenerateAndEncodeMASig(const uint8_t * data, uint16_t dataLen, TLVWriter & writer) = 0;
};

/**
 * Implements the core logic of the Weave Certificate Provisioning protocol object.
 */
class NL_DLL_EXPORT WeaveCertProvEngine
{
public:

    struct InEventParam;
    struct OutEventParam;

    enum State
    {
        kState_NotInitialized                       = 0,    /**< The engine object is not initialized. */
        kState_Idle                                 = 1,    /**< The engine object is idle. */
        kState_PreparingBinding                     = 2,    /**< The engine object is waiting for the binding to become ready. */
        kState_RequestInProgress                    = 3,    /**< A GetCertificateRequest message has been sent and the engine object is awaiting a response. */
    };

    enum EventType
    {
        kEvent_PrepareAuthorizeInfo                 = 1,    /**< The application is requested to prepare the payload for the GetCertificateRequest. */
        kEvent_ResponseReceived                     = 2,    /**< A GetCertificateResponse message was received from the peer. */
        kEvent_CommunicationError                   = 3,    /**< A communication error occurred while sending a GetCertificateRequest or waiting for a response. */
    };

    enum
    {
        kReqType_NotSpecified                       = 0,    /**< The Get Certificate request type is not specified. */
        kReqType_GetInitialOpDeviceCert             = 1,    /**< The Get Certificate request type is to obtain initial operational certificatete. */
        kReqType_RotateOpDeviceCert                 = 2,    /**< The Get Certificate request type is to rotate the current operational certificatete. */
    };

    /**
     *  This function is the application callback that is invoked on Certificate Provisioning Engine API events.
     *
     *  @param[in]  appState    A pointer to application-defined state information associated with the engine object.
     *  @param[in]  eventType   Event ID passed by the event callback.
     *  @param[in]  inParam     Reference of input event parameters passed by the event callback.
     *  @param[in]  outParam    Reference of output event parameters passed by the event callback.
     */
    typedef void (* EventCallback)(void * appState, EventType eventType, const InEventParam & inParam, OutEventParam & outParam);

    void * AppState;                                        /**< A pointer to application-specific data. */

    WeaveCertProvEngine(void);

    WEAVE_ERROR Init(Binding * binding, WeaveNodeOpAuthDelegate * opAuthDelegate, WeaveNodeManufAttestDelegate * manufAttestDelegate, EventCallback eventCallback, void * appState = NULL);
    void Shutdown(void);

    WEAVE_ERROR StartCertificateProvisioning(uint8_t reqType, bool doManufAttest);
    void AbortCertificateProvisioning(void);

    State GetState(void) const;
    uint8_t GetReqType(void) const;

    Binding * GetBinding(void) const;
    void SetBinding(Binding * binding);

    WeaveNodeOpAuthDelegate * GetOpAuthDelegate(void) const;
    void SetOpAuthDelegate(WeaveNodeOpAuthDelegate * opAuthDelegate);

    WeaveNodeManufAttestDelegate * GetManufAttestDelegate(void) const;
    void SetManufAttestDelegate(WeaveNodeManufAttestDelegate * manufAttestDelegate);

    EventCallback GetEventCallback(void) const;
    void SetEventCallback(EventCallback eventCallback);

    WEAVE_ERROR GenerateGetCertificateRequest(PacketBuffer * msgBuf, uint8_t reqType, bool doManufAttest);
    WEAVE_ERROR ProcessGetCertificateResponse(PacketBuffer * msgBuf);

private:

    uint8_t mReqType;
    bool mDoManufAttest;
    Binding * mBinding;
    WeaveNodeOpAuthDelegate *mOpAuthDelegate;
    WeaveNodeManufAttestDelegate *mManufAttestDelegate;
    EventCallback mEventCallback;
    ExchangeContext * mEC;
    State mState;

    WEAVE_ERROR SendGetCertificateRequest(void);
    void HandleRequestDone(void);
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

/**
 * Input parameters to Weave Certificate Provisioning API event.
 */
struct WeaveCertProvEngine::InEventParam
{
    WeaveCertProvEngine * Source;                       /**< The WeaveCertProvEngine from which the API event originated. */

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
            StatusReport * RcvdStatusReport;            /**< A pointer to the StatusReport object. Relevant if status report message received from the peer. */
        } CommunicationError;

        struct
        {
            bool ReplaceCert;                           /**< Boolean indicator of whether operational device certificate should be replaced. */
            const uint8_t * Cert;                       /**< A pointer to the TLV encoded Weave operational certificate assigned by CA Service. */
            uint16_t CertLen;                           /**< Length of the certificate received in the GetCertificateResponse message. */
            const uint8_t * RelatedCerts;               /**< A pointer to the TLV encoded list of certificate related to the operational certificate. */
            uint16_t RelatedCertsLen;                   /**< Length of the related certificate list received in the GetCertificateResponse message. */
        } ResponseReceived;
    };

    void Clear() { memset(this, 0, sizeof(*this)); }
};

/**
 * Output parameters to Weave Certificate Provisioning API event.
 */
struct WeaveCertProvEngine::OutEventParam
{
    union
    {
        struct
        {
            WEAVE_ERROR Error;                          /**< An error set by the application indicating that
                                                             an authorization info couldn't be prepared. */
        } PrepareAuthorizeInfo;

        struct
        {
            WEAVE_ERROR Error;                          /**< An error set by the application indicating that
                                                             response data couldn't be processed. */
        } ResponseReceived;
    };

    void Clear() { memset(this, 0, sizeof(*this)); }
};

/*
 * Inline Functions
 *
 * Documentation for these functions can be found at the end of the .cpp file.
 */

inline WeaveCertProvEngine::WeaveCertProvEngine(void)
{
}

inline void WeaveCertProvEngine::AbortCertificateProvisioning(void)
{
    return HandleRequestDone();
}

inline WeaveCertProvEngine::State WeaveCertProvEngine::GetState(void) const
{
    return mState;
}

inline uint8_t WeaveCertProvEngine::GetReqType(void) const
{
    return mReqType;
}

inline Binding *WeaveCertProvEngine::GetBinding(void) const
{
    return mBinding;
}

inline void WeaveCertProvEngine::SetBinding(Binding * binding)
{
    mBinding = binding;
}

inline WeaveNodeOpAuthDelegate * WeaveCertProvEngine::GetOpAuthDelegate(void) const
{
    return mOpAuthDelegate;
}

inline void WeaveCertProvEngine::SetOpAuthDelegate(WeaveNodeOpAuthDelegate * opAuthDelegate)
{
    mOpAuthDelegate = opAuthDelegate;
}

inline WeaveNodeManufAttestDelegate * WeaveCertProvEngine::GetManufAttestDelegate(void) const
{
    return mManufAttestDelegate;
}

inline void WeaveCertProvEngine::SetManufAttestDelegate(WeaveNodeManufAttestDelegate * manufAttestDelegate)
{
    mManufAttestDelegate = manufAttestDelegate;
}

inline WeaveCertProvEngine::EventCallback WeaveCertProvEngine::GetEventCallback(void) const
{
    return mEventCallback;
}

inline void WeaveCertProvEngine::SetEventCallback(WeaveCertProvEngine::EventCallback eventCallback)
{
    mEventCallback = eventCallback;
}

} // namespace CertProvisioning
} // namespace Security
} // namespace Profiles
} // namespace Weave
} // namespace nl

#endif /* WEAVECERTPROVISIONING_H_ */
