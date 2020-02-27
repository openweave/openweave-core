/*
 *
 *    Copyright (c) 2020 Google LLC.
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
 *          Defines the Device Layer MockCertificateProvisioningClient object.
 */

#ifndef MOCKCPCLIENT_H_
#define MOCKCPCLIENT_H_

#include <Weave/Core/WeaveCore.h>
#include <Weave/Profiles/security/WeaveCertProvisioning.h>

#include <Weave/Core/WeaveExchangeMgr.h>

using nl::Weave::Profiles::Security::CertProvisioning::WeaveCertProvEngine;

/**
 * Implements the Weave Certificate Provisioning profile for a Weave device.
 */
class MockCertificateProvisioningClient final
    : public ::nl::Weave::Profiles::Security::CertProvisioning::WeaveNodeOpAuthDelegate,
      public ::nl::Weave::Profiles::Security::CertProvisioning::WeaveNodeMfrAttestDelegate
{

public:

    /**
     *  This function is the callback that is invoked by Certificate Provisioning Client to add
     *  authorization info to the GetCertificateRequest message in a TLV form to the supplied TLV writer.
     *
     *  @param[in]  appState         A pointer to a higher layer object.
     *  @param[in]  writer           A reference to a TLV writer to write request authorization data.
     */
    typedef WEAVE_ERROR (* EncodeReqAuthInfoFunct)(void *const appState, TLVWriter & writer);

    /**
     *  This function is the callback that is invoked by Certificate Provisioning Client upon
     *  completion (successful or erroneous) of the Certificate Provisioning protocol.
     *
     *  @param[in]  appState         A pointer to a higher layer object.
     *  @param[in]  localErr         The WEAVE_ERROR encountered during key export protocol.
     *  @param[in]  statusProfileId  The profile for the specified status code.
     *  @param[in]  statusCode       The status code to send.
     */
    typedef void (*HandleCertificateProvisioningResultFunct)(void *const appState, WEAVE_ERROR localErr, uint32_t statusProfileId, uint16_t statusCode);

    MockCertificateProvisioningClient(void);

    // ===== Members for internal use by other Device Layer components.

    WEAVE_ERROR Init(WeaveExchangeManager *exchangeMgr);
    WEAVE_ERROR Shutdown(void);

    void Reset(void);
    void Preconfig(void);

    uint64_t WOCAServerEndPointId;
    const char *WOCAServerAddr;
    int WOCAServerTransport;
    bool WOCAServerUseCASE;

    // ===== Methods that implement the WeaveNodeOpAuthDelegate interface

    WEAVE_ERROR EncodeOpCert(TLVWriter & writer, uint64_t tag) __OVERRIDE;
    WEAVE_ERROR EncodeOpRelatedCerts(TLVWriter & writer, uint64_t tag) __OVERRIDE;
    WEAVE_ERROR GenerateAndEncodeOpSig(const uint8_t * hash, uint8_t hashLen, TLVWriter & writer, uint64_t tag) __OVERRIDE;

    // ===== Methods that implement the WeaveNodeMfrAttestDelegate interface

    WEAVE_ERROR EncodeMAInfo(TLVWriter & writer) __OVERRIDE;
    WEAVE_ERROR GenerateAndEncodeMASig(const uint8_t * data, uint16_t dataLen, TLVWriter & writer) __OVERRIDE;

    // ===== Other Methods

    WEAVE_ERROR StartCertificateProvisioning(uint8_t reqType, EncodeReqAuthInfoFunct encodeReqAuthInfo,
                                             void * requesterState, HandleCertificateProvisioningResultFunct onCertProvDone);
    void HandleCertificateProvisioningResult(WEAVE_ERROR localErr, uint32_t statusProfileId, uint16_t statusCode);

    WEAVE_ERROR GetDevicePrivateKey(uint8_t *& key, uint16_t & keyLen);

    WEAVE_ERROR GenerateAndStoreOperationalDeviceCredentials(uint64_t deviceId = nl::Weave::kNodeIdNotSpecified);

private:

    // ===== Members for internal use by this class only.

    uint8_t mReqType;
    bool mDoMfrAttest;
    EncodeReqAuthInfoFunct mEncodeReqAuthInfo;
    HandleCertificateProvisioningResultFunct mOnCertProvDone;
    void * mRequesterState;

    WeaveCertProvEngine mCertProvEngine;
    WeaveExchangeManager * mExchangeMgr;
    ::nl::Weave::Binding * mBinding;
    nl::Weave::ExchangeContext * mEC; // TODO: Remove

    void SendGetCertificateRequest(void);

    static void CertProvClientEventHandler(void * appState, WeaveCertProvEngine::EventType eventType,
                                           const WeaveCertProvEngine::InEventParam & inParam, WeaveCertProvEngine::OutEventParam & outParam);
    static void HandleCertificateProvisioningBindingEvent(void *const appState,
                                                          const nl::Weave::Binding::EventType event,
                                                          const nl::Weave::Binding::InEventParam &inParam,
                                                          nl::Weave::Binding::OutEventParam &outParam);

    // Persisted Operational Device Credentials.
    uint64_t mDeviceId;
    uint8_t * mDeviceCert;
    uint16_t mDeviceCertLen;
    uint8_t * mDeviceIntermediateCACerts;
    uint16_t mDeviceIntermediateCACertsLen;
    uint8_t * mDevicePrivateKey;
    uint16_t mDevicePrivateKeyLen;

    WEAVE_ERROR GetDeviceId(uint64_t & deviceId);
    WEAVE_ERROR GetDeviceCertificate(uint8_t *& cert, uint16_t & certLen);
    WEAVE_ERROR GetDeviceIntermediateCACerts(uint8_t *& certs, uint16_t & certsLen);
    WEAVE_ERROR StoreDeviceId(uint64_t deviceId);
    WEAVE_ERROR StoreDeviceCertificate(const uint8_t * cert, uint16_t certLen);
    WEAVE_ERROR StoreDeviceIntermediateCACerts(const uint8_t * certs, uint16_t certsLen);
    WEAVE_ERROR StoreDevicePrivateKey(const uint8_t * key, uint16_t keyLen);
    void ClearOperationalDeviceCredentials(void);

    WEAVE_ERROR GetManufacturerDeviceCertificate(uint8_t *& cert, uint16_t & certLen);
    WEAVE_ERROR GetManufacturerDeviceIntermediateCACerts(uint8_t *& certs, uint16_t & certsLen);
    WEAVE_ERROR GetManufacturerDevicePrivateKey(uint8_t *& key, uint16_t & keyLen);
};

#endif // MOCKCPCLIENT_H_
