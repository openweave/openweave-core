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
 *      This file defines a derived unsolicited responder
 *      (i.e., server) for the Certificate Provisioned protocol of the
 *      Weave Security profile used for the Weave mock
 *      device command line functional testing tool.
 *      This server is also known as Weave Operational Certificate Authority (WOCA).
 *
 */

#ifndef MOCKCASERVICE_H_
#define MOCKCASERVICE_H_

//#include "CertProvOptions.h"
#include <Weave/Core/WeaveCore.h>
#include <Weave/Profiles/security/WeaveCertProvisioning.h>
#include <Weave/Support/crypto/RSA.h>

using ::nl::Weave::System::PacketBuffer;

enum {
    kMaxCertCount = 4,
};

enum
{
    kMfrAttestType_Undefined                    = 0,
    kMfrAttestType_WeaveCert                    = 1,
    kMfrAttestType_X509Cert                     = 2,
    kMfrAttestType_HMAC                         = 3,
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

    WeaveCertificateSet MfrAttestWeaveCertSet;
    X509Cert MfrAttestX509CertSet[kMaxCertCount];
    uint8_t MfrAttestX509CertCount;
    uint32_t MfrAttestHMACKeyId;
    const uint8_t * MfrAttestHMACMetaData;
    uint16_t MfrAttestHMACMetaDataLen;

    OID OperationalSigAlgo;
    EncodedECDSASignature OperationalSig;

    OID MfrAttestSigAlgo;
    union
    {
        EncodedECDSASignature EC;
        EncodedRSASignature RSA;
        EncodedHMACSignature HMAC;
    } MfrAttestSig;

    uint8_t RequestType() const { return mReqType; }
    GetCertificateRequestMessage& RequestType (uint8_t val) { mReqType = val; return *this; }

    uint8_t MfrAttestType() const { return mMfrAttestType; }
    GetCertificateRequestMessage& MfrAttestType (uint8_t val) { mMfrAttestType = val; return *this; }

    bool AuthorizeInfoPresent() const { return (AuthorizeInfoPairingToken != NULL); }
    bool MfrAttestPresent() const { return (mMfrAttestType != kMfrAttestType_Undefined); }
    bool MfrAttestRequired() const { return (mReqType == nl::Weave::Profiles::Security::CertProvisioning::WeaveCertProvEngine::kReqType_GetInitialOpDeviceCert); }

    WEAVE_ERROR Decode(PacketBuffer *msgBuf);
    WEAVE_ERROR GenerateTBSHash(uint8_t *tbsHash);

private:
    uint8_t mReqType;
    uint8_t mMfrAttestType;

    bool mOperationalCertSetInitialized;
    bool mMfrAttestCertSetInitialized;
};

class MockWeaveOperationalCAServer
{
public:
    MockWeaveOperationalCAServer();

    WEAVE_ERROR Init(nl::Weave::WeaveExchangeManager *exchangeMgr);
    WEAVE_ERROR Shutdown();

    WEAVE_ERROR ProcessGetCertificateRequest(PacketBuffer *msgBuf, GetCertificateRequestMessage & msg);
    WEAVE_ERROR GenerateGetCertificateResponse(PacketBuffer * msgBuf, WeaveCertificateData & receivedDeviceCertData);

    bool LogMessageData() const { return mLogMessageData; }
    MockWeaveOperationalCAServer& LogMessageData(bool val) { mLogMessageData = val; return *this; }

    bool IncludeRelatedCerts() const { return mIncludeRelatedCerts; }
    MockWeaveOperationalCAServer& IncludeRelatedCerts(bool val) { mIncludeRelatedCerts = val; return *this; }

    bool DoNotRotateCert() const { return mDoNotRotateCert; }
    MockWeaveOperationalCAServer& DoNotRotateCert(bool val) { mDoNotRotateCert = val; return *this; }

    void SetCACert(const uint8_t * cert, uint16_t certLen) { mCACert = cert; mCACertLen = certLen; }
    void SetCAPrivateKey(const uint8_t * privateKey, uint16_t privateKeyLen) { mCAPrivateKey = privateKey; mCAPrivateKeyLen = privateKeyLen; }

private:
    nl::Weave::WeaveExchangeManager *mExchangeMgr;
    bool mLogMessageData;
    bool mIncludeRelatedCerts;
    bool mDoNotRotateCert;

    const uint8_t * mCACert;
    uint16_t mCACertLen;

    const uint8_t * mCAPrivateKey;
    uint16_t mCAPrivateKeyLen;

    WEAVE_ERROR SendStatusReport(nl::Weave::ExchangeContext *ec, uint16_t statusCode);

    static void HandleClientRequest(nl::Weave::ExchangeContext *ec, const nl::Inet::IPPacketInfo *addrInfo,
                                    const nl::Weave::WeaveMessageInfo *msgInfo, uint32_t profileId,
                                    uint8_t msgType, PacketBuffer *payload);
};

#endif /* MOCKCASERVICE_H_ */
