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
 *      This file defines a derived unsolicited responder
 *      (i.e., server) for the Certificate Provisioned protocol of the
 *      Weave Security profile used for the Weave mock
 *      device command line functional testing tool.
 *
 */

#ifndef MOCKCASERVICE_H_
#define MOCKCASERVICE_H_

#include "CertProvOptions.h"
#include <Weave/Core/WeaveCore.h>
#include <Weave/Profiles/security/WeaveCertProvisioning.h>

using ::nl::Weave::System::PacketBuffer;

enum {
    kMaxCertCount = 4,
};

struct X509Cert
{
    const uint8_t *Cert;
    uint16_t Len;
};

class GetCertificateRequestMessage
{
public:
    GetCertificateRequestMessage();

    WeaveCertificateSet OperationalCertSet;

    const uint8_t *mTBSDataStart;
    uint16_t mTBSDataLen;

    const uint8_t *AuthorizeInfoPairingToken;
    uint16_t AuthorizeInfoPairingTokenLen;
    const uint8_t *AuthorizeInfoPairingInitData;
    uint16_t AuthorizeInfoPairingInitDataLen;

    WeaveCertificateSet ManufAttestWeaveCertSet;
    X509Cert ManufAttestX509CertSet[kMaxCertCount];
    uint8_t ManufAttestX509CertCount;
    uint32_t ManufAttestHMACKeyId;
    const uint8_t * ManufAttestHMACMetaData;
    uint16_t ManufAttestHMACMetaDataLen;

    OID OperationalSigAlgo;
    EncodedECDSASignature OperationalSig;

    OID ManufAttestSigAlgo;
    union
    {
        EncodedECDSASignature EC;
        EncodedRSASignature RSA;
        EncodedHMACSignature HMAC;
    } ManufAttestSig;

    uint8_t RequestType() const { return mReqType; }
    GetCertificateRequestMessage& RequestType (uint8_t val) { mReqType = val; return *this; }

    uint8_t ManufAttestType() const { return mManufAttestType; }
    GetCertificateRequestMessage& ManufAttestType (uint8_t val) { mManufAttestType = val; return *this; }

    bool AuthorizeInfoPresent() const { return (AuthorizeInfoPairingToken != NULL); }
    bool ManufAttestPresent() const { return (mManufAttestType != kManufAttestType_Undefined); }
    bool ManufAttestRequired() const { return (mReqType == nl::Weave::Profiles::Security::CertProvisioning::WeaveCertProvEngine::kReqType_GetInitialOpDeviceCert); }

    WEAVE_ERROR Decode(PacketBuffer *msgBuf);
    WEAVE_ERROR GenerateTBSHash(uint8_t *tbsHash);

private:
    uint8_t mReqType;
    uint8_t mManufAttestType;

    bool mOperationalCertSetInitialized;
    bool mManufAttestCertSetInitialized;
};

class MockCAService
{
public:
    MockCAService();

    WEAVE_ERROR Init(nl::Weave::WeaveExchangeManager *exchangeMgr);
    WEAVE_ERROR Shutdown();

    WEAVE_ERROR ProcessGetCertificateRequest(PacketBuffer *msgBuf, GetCertificateRequestMessage & msg);
    WEAVE_ERROR GenerateGetCertificateResponse(PacketBuffer *msgBuf, WeaveCertificateData& currentOpDeviceCert);

    bool LogMessageData() const { return mLogMessageData; }
    MockCAService& LogMessageData(bool val) { mLogMessageData = val; return *this; }

    bool IncludeRelatedCerts() const { return mIncludeRelatedCerts; }
    MockCAService& IncludeRelatedCerts(bool val) { mIncludeRelatedCerts = val; return *this; }

    bool DoNotRotateCert() const { return mDoNotRotateCert; }
    MockCAService& DoNotRotateCert(bool val) { mDoNotRotateCert = val; return *this; }

    void SetCACert(const uint8_t * cert, uint16_t certLen) { mCACert = cert; mCACertLength = certLen; }
    void SetCAPrivateKey(const uint8_t * privateKey, uint16_t privateKeyLen) { mCAPrivateKey = privateKey; mCAPrivateKeyLength = privateKeyLen; }

    WEAVE_ERROR GenerateServiceAssignedDeviceCert(WeaveCertificateData& certData, uint8_t *cert, uint16_t certBufSize, uint16_t& certLen);

private:
    nl::Weave::WeaveExchangeManager *mExchangeMgr;
    bool mLogMessageData;
    bool mIncludeRelatedCerts;
    bool mDoNotRotateCert;

    const uint8_t * mCACert;
    uint16_t mCACertLength;

    const uint8_t * mCAPrivateKey;
    uint16_t mCAPrivateKeyLength;

    WEAVE_ERROR SendStatusReport(nl::Weave::ExchangeContext *ec, uint16_t statusCode);

    static void HandleClientRequest(nl::Weave::ExchangeContext *ec, const nl::Inet::IPPacketInfo *addrInfo,
                                    const nl::Weave::WeaveMessageInfo *msgInfo, uint32_t profileId,
                                    uint8_t msgType, PacketBuffer *payload);
};

#endif /* MOCKCASERVICE_H_ */
