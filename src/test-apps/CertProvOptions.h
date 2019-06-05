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
 *      Definition of CertProvOptions class, which handles Certificate Provisioning specific
 *      command line options and provides an implementation of the WeaveNodeOpAuthDelegate and
 *      WeaveNodeManufAttestDelegate interfaces for use in test applications.
 *
 */

#ifndef CERTPROVOPTIONS_H_
#define CERTPROVOPTIONS_H_

#include <Weave/Core/WeaveCore.h>
#include <Weave/Profiles/security/WeaveSecurity.h>
#include <Weave/Profiles/security/WeaveCertProvisioning.h>
#include "ToolCommonOptions.h"

using namespace nl::Weave::Profiles::Security::CertProvisioning;

enum
{
    kManufAttestType_Undefined                    = 0,
    kManufAttestType_WeaveCert                    = 1,
    kManufAttestType_X509Cert                     = 2,
    kManufAttestType_HMAC                         = 3,
};

extern void CertProvClientEventHandler(void * appState, WeaveCertProvEngine::EventType eventType, const WeaveCertProvEngine::InEventParam & inParam, WeaveCertProvEngine::OutEventParam & outParam);

class CertProvOptions : public WeaveNodeOpAuthDelegate, public WeaveNodeManufAttestDelegate, public OptionSetBase
{
public:
    uint64_t EphemeralNodeId;

    uint8_t RequestType;

    bool IncludeAuthorizeInfo;
    const uint8_t *PairingToken;
    uint16_t PairingTokenLength;
    const uint8_t *PairingInitData;
    uint16_t PairingInitDataLength;

    const uint8_t *OperationalCert;
    uint16_t OperationalCertLength;

    const uint8_t *OperationalPrivateKey;
    uint16_t OperationalPrivateKeyLength;

    bool IncludeOperationalCACerts;
    const uint8_t *OperationalCACert;
    uint16_t OperationalCACertLength;

    uint8_t ManufAttestType;

    const uint8_t *ManufAttestCert;
    uint16_t ManufAttestCertLength;

    const uint8_t *ManufAttestPrivateKey;
    uint16_t ManufAttestPrivateKeyLength;

    bool IncludeManufAttestCACerts;
    const uint8_t *ManufAttestCACert;
    uint16_t ManufAttestCACertLength;
    const uint8_t *ManufAttestCACert2;
    uint16_t ManufAttestCACert2Length;

    CertProvOptions();

private:

    virtual bool HandleOption(const char *progName, OptionSet *optSet, int id, const char *name, const char *arg) __OVERRIDE;

    // ===== Methods that implement the WeaveNodeOpAuthDelegate interface

    WEAVE_ERROR EncodeOpCert(TLVWriter & writer, uint64_t tag) __OVERRIDE;
    WEAVE_ERROR EncodeOpRelatedCerts(TLVWriter & writer, uint64_t tag) __OVERRIDE;
    WEAVE_ERROR GenerateAndEncodeOpSig(const uint8_t * hash, uint8_t hashLen, TLVWriter & writer, uint64_t tag) __OVERRIDE;

    // ===== Methods that implement the WeaveNodeManufAttestDelegate interface

    WEAVE_ERROR EncodeMAInfo(TLVWriter & writer) __OVERRIDE;
    WEAVE_ERROR GenerateAndEncodeMASig(const uint8_t * data, uint16_t dataLen, TLVWriter & writer) __OVERRIDE;
};

extern CertProvOptions gCertProvOptions;

#endif /* CERTPROVOPTIONS_H_ */
