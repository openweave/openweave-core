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
 *          Defines the Device Layer CertificateProvisioningClient object.
 */

#ifndef CERTIFICATE_PROVISIONING_CLIENT_H
#define CERTIFICATE_PROVISIONING_CLIENT_H

#include <Weave/DeviceLayer/internal/WeaveDeviceLayerInternal.h>
#include <Weave/Profiles/security/WeaveCertProvisioning.h>

namespace nl {
namespace Weave {
namespace DeviceLayer {
namespace Internal {

using nl::Weave::Profiles::Security::CertProvisioning::WeaveCertProvEngine;

extern WEAVE_ERROR GenerateAndStoreOperationalDeviceCredentials(uint64_t deviceId = kNodeIdNotSpecified);

/**
 * Implements the Weave Certificate Provisioning profile for a Weave device.
 */
class CertificateProvisioningClient final
    : public ::nl::Weave::Profiles::Security::CertProvisioning::WeaveNodeOpAuthDelegate,
      public ::nl::Weave::Profiles::Security::CertProvisioning::WeaveNodeManufAttestDelegate
{

public:

    /**
     *  This function is the callback that is invoked by Certificate Provisioning Client to add
     *  authorization info to the GetCertificateRequest message in a TLV form to the supplied TLV writer.
     *
     *  @param[in]  writer      A reference to a TLV writer to write request authorization data.
     */
    typedef WEAVE_ERROR (* EncodeReqAuthInfoFunct)(TLVWriter & writer);

    // ===== Members for internal use by other Device Layer components.

    WEAVE_ERROR Init(uint8_t reqType);
    WEAVE_ERROR Init(uint8_t reqType, EncodeReqAuthInfoFunct encodeReqAuthInfo);
    void OnPlatformEvent(const WeaveDeviceEvent * event);

    // ===== Methods that implement the WeaveNodeOpAuthDelegate interface

    WEAVE_ERROR EncodeOpCert(TLVWriter & writer, uint64_t tag) __OVERRIDE;
    WEAVE_ERROR EncodeOpRelatedCerts(TLVWriter & writer, uint64_t tag) __OVERRIDE;
    WEAVE_ERROR GenerateAndEncodeOpSig(const uint8_t * hash, uint8_t hashLen, TLVWriter & writer, uint64_t tag) __OVERRIDE;

    // ===== Methods that implement the WeaveNodeManufAttestDelegate interface

    WEAVE_ERROR EncodeMAInfo(TLVWriter & writer) __OVERRIDE;
    WEAVE_ERROR GenerateAndEncodeMASig(const uint8_t * data, uint16_t dataLen, TLVWriter & writer) __OVERRIDE;

    // ===== Members that override virtual methods on ServiceProvisioningDelegate

    void StartCertificateProvisioning(void);
    void HandleCertificateProvisioningResult(WEAVE_ERROR localErr, uint32_t serverStatusProfileId, uint16_t serverStatusCode);

private:

    // ===== Members for internal use by this class only.

    WeaveCertProvEngine mCertProvEngine;
    ::nl::Weave::Binding * mBinding;
    uint8_t mReqType;
    bool mDoManufAttest;
    EncodeReqAuthInfoFunct mEncodeReqAuthInfo;
    bool mWaitingForServiceConnectivity;

    void SendGetCertificateRequest(void);

    static void HandleServiceConnectivityTimeout(::nl::Weave::System::Layer * layer, void * appState, ::nl::Weave::System::Error err);
    static void HandleCertProvBindingEvent(void * appState, nl::Weave::Binding::EventType eventType,
            const nl::Weave::Binding::InEventParam & inParam, nl::Weave::Binding::OutEventParam & outParam);
    static void CertProvClientEventHandler(void * appState, WeaveCertProvEngine::EventType eventType, const WeaveCertProvEngine::InEventParam & inParam, WeaveCertProvEngine::OutEventParam & outParam);

protected:

    // Construction/destruction limited to subclasses.
    CertificateProvisioningClient() = default;
    ~CertificateProvisioningClient() = default;

    // No copy, move or assignment.
    CertificateProvisioningClient(const CertificateProvisioningClient &) = delete;
    CertificateProvisioningClient(const CertificateProvisioningClient &&) = delete;
    CertificateProvisioningClient & operator=(const CertificateProvisioningClient &) = delete;
};

} // namespace Internal
} // namespace DeviceLayer
} // namespace Weave
} // namespace nl

#endif // CERTIFICATE_PROVISIONING_CLIENT_H
