/*
 *
 *    Copyright (c) 2019-2020 Google LLC.
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
 *      Definition of CertProvOptions class, which handles Certificate Provisioning specific
 *      command line options and provides an implementation of the WeaveNodeOpAuthDelegate and
 *      WeaveNodeMfrAttestDelegate interfaces for use in test applications.
 *
 */

#ifndef CERTPROVOPTIONS_H_
#define CERTPROVOPTIONS_H_

#include <Weave/Core/WeaveCore.h>
#include <Weave/Profiles/security/WeaveSecurity.h>
#include <Weave/Profiles/security/WeaveCertProvisioning.h>
#include "ToolCommonOptions.h"

using namespace nl::Weave::Profiles::Security::CertProvisioning;


extern WEAVE_ERROR GenerateTestDeviceCert(uint64_t deviceId, EncodedECPublicKey& devicePubKey,
                                          uint8_t *cert, uint16_t certBufSize, uint16_t& certLen);

extern WEAVE_ERROR GenerateTestDeviceCert(uint64_t deviceId, EncodedECPublicKey& devicePubKey,
                                          const uint8_t *caCert, uint16_t caCertLen,
                                          const uint8_t *caKey, uint16_t caKeyLen,
                                          uint8_t *cert, uint16_t certBufSize, uint16_t& certLen);

class DeviceCredentialsStore
{
public:

    enum
    {
        kWeaveDeviceCertBufSize           = 300,    // Size of buffer needed to hold Weave device certificate.
        kWeaveDevicePrivateKeyBufSize     = 128,    // Size of buffer needed to hold Weave device private key.
    };

    DeviceCredentialsStore();

    WEAVE_ERROR StoreDeviceId(uint64_t deviceId) { mDeviceId = deviceId; return WEAVE_NO_ERROR; };
    WEAVE_ERROR StoreDeviceCert(const uint8_t * cert, uint16_t certLen);
    WEAVE_ERROR StoreDeviceICACerts(const uint8_t * certs, uint16_t certsLen);
    WEAVE_ERROR StoreDevicePrivateKey(const uint8_t * key, uint16_t keyLen);

    WEAVE_ERROR GetDeviceId(uint64_t & deviceId);
    WEAVE_ERROR GetDeviceCert(const uint8_t *& cert, uint16_t & certLen);
    WEAVE_ERROR GetDeviceICACerts(const uint8_t *& cert, uint16_t & certLen);
    WEAVE_ERROR GetDevicePrivateKey(const uint8_t *& key, uint16_t & keyLen);

    void ClearDeviceId(void);
    void ClearDeviceCert(void);
    void ClearDeviceICACerts(void);
    void ClearDevicePrivateKey(void);
    void ClearDeviceCredentials(void);

    bool DeviceIdExists(void);
    bool DeviceCertExists(void);
    bool DeviceICACertsExists(void);
    bool DevicePrivateKeyExists(void);
    bool DeviceCredentialsExist(void);

    WEAVE_ERROR GenerateAndStoreDeviceCredentials(uint64_t deviceId = kNodeIdNotSpecified);
    WEAVE_ERROR GenerateAndReplaceCurrentDeviceCert(void);

private:

    uint64_t mDeviceId;
    uint8_t mDevicePrivateKey[kWeaveDevicePrivateKeyBufSize];
    uint16_t mDevicePrivateKeyLen;
    uint8_t mDeviceCert[kWeaveDeviceCertBufSize];
    uint16_t mDeviceCertLen;
    uint8_t mDeviceICACerts[kWeaveDeviceCertBufSize];
    uint16_t mDeviceICACertsLen;
};

enum
{
    kMfrAttestType_Undefined                    = 0,
    kMfrAttestType_WeaveCert                    = 1,
    kMfrAttestType_X509Cert                     = 2,
    kMfrAttestType_HMAC                         = 3,
};

extern void CertProvClientEventHandler(void * appState, WeaveCertProvEngine::EventType eventType, const WeaveCertProvEngine::InEventParam & inParam, WeaveCertProvEngine::OutEventParam & outParam);

class CertProvOptions : public WeaveNodeOpAuthDelegate, public WeaveNodeMfrAttestDelegate, public OptionSetBase
{
public:
    uint64_t DeviceId;

    uint8_t RequestType;

    bool IncludeAuthorizeInfo;
    const uint8_t *PairingToken;
    uint16_t PairingTokenLen;
    const uint8_t *PairingInitData;
    uint16_t PairingInitDataLen;

    const uint8_t *OperationalCert;
    uint16_t OperationalCertLen;

    const uint8_t *OperationalPrivateKey;
    uint16_t OperationalPrivateKeyLen;

    bool IncludeOperationalICACerts;
    const uint8_t *OperationalICACerts;
    uint16_t OperationalICACertsLen;

    uint64_t MfrAttestDeviceId;

    uint8_t MfrAttestType;

    const uint8_t *MfrAttestCert;
    uint16_t MfrAttestCertLen;

    const uint8_t *MfrAttestPrivateKey;
    uint16_t MfrAttestPrivateKeyLen;

    bool IncludeMfrAttestICACerts;
    const uint8_t *MfrAttestICACert1;
    uint16_t MfrAttestICACert1Len;
    const uint8_t *MfrAttestICACert2;
    uint16_t MfrAttestICACert2Len;

    CertProvOptions();

    static void CertProvClientEventHandler(void * appState, WeaveCertProvEngine::EventType eventType,
                                           const WeaveCertProvEngine::InEventParam & inParam, WeaveCertProvEngine::OutEventParam & outParam);

private:

    virtual bool HandleOption(const char *progName, OptionSet *optSet, int id, const char *name, const char *arg) __OVERRIDE;

    // ===== Methods that implement the WeaveNodeOpAuthDelegate interface

    WEAVE_ERROR EncodeOpCert(TLVWriter & writer, uint64_t tag) __OVERRIDE;
    WEAVE_ERROR EncodeOpRelatedCerts(TLVWriter & writer, uint64_t tag) __OVERRIDE;
    WEAVE_ERROR GenerateAndEncodeOpSig(const uint8_t * hash, uint8_t hashLen, TLVWriter & writer, uint64_t tag) __OVERRIDE;

    // ===== Methods that implement the WeaveNodeMfrAttestDelegate interface

    WEAVE_ERROR EncodeMAInfo(TLVWriter & writer) __OVERRIDE;
    WEAVE_ERROR GenerateAndEncodeMASig(const uint8_t * data, uint16_t dataLen, TLVWriter & writer) __OVERRIDE;
};

extern DeviceCredentialsStore gDeviceCredsStore;

extern CertProvOptions gCertProvOptions;

#endif /* CERTPROVOPTIONS_H_ */
