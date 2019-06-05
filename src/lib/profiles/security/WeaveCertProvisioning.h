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
//#include <Weave/Core/WeaveTLV.h>
//#include <Weave/Profiles/security/WeaveSecurity.h>

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
 * certificate provisioned protocol.
 */
class WeaveCertProvDelegate
{
public:

    // ===== Abstract Interface methods

    /**
     * Encode Weave Operational Certificate for the local node.
     *
     * When invoked, the implementation should write a WeaveCertificate structure
     * containing the local node's operational certificate.
     */
    virtual WEAVE_ERROR EncodeNodeOperationalCert(TLVWriter & writer) = 0;

    /**
     * Encode Weave Manufacturer Attestation Information for the local node.
     *
     * When invoked, the implementation should write a structure containing
     * the local node's manufacturer attestation information.
     */
    virtual WEAVE_ERROR EncodeNodeManufAttestInfo(TLVWriter & writer) = 0;

    /**
     * Generate and encode Weave signature using local node's operational private key.
     *
     * When invoked, implementations must compute a signature on the given hash value
     * using the node's operational private key. The generated signature should then
     * be written in the form of a ECDSASignature structure to the supplied TLV writer
     * using the specified tag.
     *
     * In cases where the node's private key is held in a local buffer, the GenerateAndEncodeWeaveECDSASignature()
     * utility function can be useful for implementing this method.
     */
    virtual WEAVE_ERROR GenerateNodeOperationalSig(const uint8_t * hash, uint8_t hashLen, TLVWriter & writer) = 0;

    /**
     * Generate and encode signature using local node's manufacturer attestation private key.
     *
     * When invoked, implementations must compute a signature on the given hash value
     * using the node's manufacturer attestation private key. The generated signature
     * should then be written in the form of a ECDSASignature or RSASignature structure
     * to the supplied TLV writing using the specified tag.
     *
     * In cases where the node's Elliptic Curve private key is held in a local buffer, the GenerateAndEncodeWeaveECDSASignature()
     * utility function can be useful for implementing this method.
     */
    virtual WEAVE_ERROR GenerateNodeManufAttestSig(const uint8_t * hash, uint8_t hashLen, TLVWriter & writer) = 0;

    /**
     * Store Weave operational and related certificates assigned by the CA service for the local node.
     *
     * When invoked, implementations must replace current Weave operational certificate and
     * related certificates assigned by the CA service.
     */
    virtual WEAVE_ERROR StoreServiceAssignedNodeOperationalCert(uint8_t * cert, uint16_t certLen,
                                                                uint8_t * relatedCerts, uint16_t relatedCertsLen) = 0;
};

/**
 * Implements the core logic of the Weave Certificate Provisioning protocol.
 */
class NL_DLL_EXPORT WeaveCertProvClient
{
public:

    enum EngineState
    {
        kState_Idle                             = 0,
        kState_GetCertificateRequestGenerated   = 1,
        kState_Complete                         = 2,
        kState_Failed                           = 3,
    };

    enum
    {
        kReqType_NotSpecified                   = 0,
        kReqType_GetInitialOpDeviceCert         = 1,
        kReqType_RotateCert                     = 2,
    };

    WeaveCertProvDelegate *CertProvDelegate;                // Certificate provisioning delegate object.
    EngineState State;

    void Init(void);
    void Shutdown(void);
    void Reset(void);

    WEAVE_ERROR GenerateGetCertificateRequest(PacketBuffer * msgBuf, uint8_t reqType,
                                              uint8_t * pairingToken, uint16_t pairingTokenLen,
                                              uint8_t * pairingInitData, uint16_t pairingInitDataLen);
    WEAVE_ERROR ProcessGetCertificateResponse(PacketBuffer *msgBuf);
};

} // namespace CertProvisioning
} // namespace Security
} // namespace Profiles
} // namespace Weave
} // namespace nl

#endif /* WEAVECERTPROVISIONING_H_ */
