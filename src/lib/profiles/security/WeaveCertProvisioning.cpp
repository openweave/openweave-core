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
 *      This file implements the Certificate Provisioning Protocol, used to
 *      get new Weave operational device certificate from the CA service.
 *
 */

#include <Weave/Core/WeaveCore.h>
#include <Weave/Core/WeaveTLV.h>
#include <Weave/Support/CodeUtils.h>
#include <Weave/Core/WeaveEncoding.h>
#include <Weave/Profiles/WeaveProfiles.h>
#include "WeaveCertProvisioning.h"
#include <Weave/Profiles/common/CommonProfile.h>
#include <Weave/Support/crypto/HashAlgos.h>

namespace nl {
namespace Weave {
namespace Profiles {
namespace Security {
namespace CertProvisioning {

using namespace nl::Weave::Crypto;
using namespace nl::Weave::Encoding;
using namespace nl::Weave::TLV;
using namespace nl::Weave::Platform::Security;

void WeaveCertProvClient::Init()
{
    Reset();
}

void WeaveCertProvClient::Shutdown()
{
    Reset();
}

void WeaveCertProvClient::Reset()
{
    CertProvDelegate = NULL;
    State = kState_Idle;
}

WEAVE_ERROR WeaveCertProvClient::GenerateGetCertificateRequest(PacketBuffer * msgBuf, uint8_t reqType,
                                                               uint8_t * pairingToken, uint16_t pairingTokenLen,
                                                               uint8_t * pairingInitData, uint16_t pairingInitDataLen)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    TLVWriter writer;
    TLVType containerType;
    SHA256 sha256;
    uint8_t *tbsDataStart;
    uint16_t tbsDataStartOffset;
    uint16_t tbsDataLen;
    uint8_t tbsHash[SHA256::kHashLength];

    WeaveLogDetail(SecurityManager, "CertProv:GenerateGetCertificateRequest");

    VerifyOrExit(State == kState_Idle, err = WEAVE_ERROR_INCORRECT_STATE);

    VerifyOrExit(reqType == kReqType_GetInitialOpDeviceCert ||
                 reqType == kReqType_RotateCert, err = WEAVE_ERROR_INVALID_ARGUMENT);

    writer.Init(msgBuf, msgBuf->AvailableDataLength());

    err = writer.StartContainer(AnonymousTag, kTLVType_Structure, containerType);
    SuccessOrExit(err);

    // Record the offset and the start of the TBS (To-Be-Signed) portion of the message.
    tbsDataStartOffset = writer.GetLengthWritten();
    tbsDataStart = msgBuf->Start() + tbsDataStartOffset;

    // Request type.
    err = writer.Put(ContextTag(kTag_GetCertReqMsg_ReqType), reqType);
    SuccessOrExit(err);

    // Get Certificate authorization information.
    if (pairingToken != NULL)
    {
        TLVType containerType2;

        err = writer.StartContainer(ProfileTag(kWeaveProfile_Security, kTag_GetCertAuthorizeInfo), kTLVType_Structure, containerType2);
        SuccessOrExit(err);

        // Pairing Token.
        err = writer.PutBytes(ContextTag(kTag_GetCertAuthorizeInfo_PairingToken), pairingToken, pairingTokenLen);
        SuccessOrExit(err);

        // Pairing Initialization Data.
        err = writer.PutBytes(ContextTag(kTag_GetCertAuthorizeInfo_PairingInitData), pairingInitData, pairingInitDataLen);
        SuccessOrExit(err);

        err = writer.EndContainer(containerType2);
        SuccessOrExit(err);
    }

    // Call the delegate to write the local node Weave operational certificate.
    err = CertProvDelegate->EncodeNodeOperationalCert(writer);
    SuccessOrExit(err);

    // Manufacturer attestation information.
    if (reqType == kReqType_GetInitialOpDeviceCert)
    {
        // Call the delegate to write the local node manufacturer attestation information.
        err = CertProvDelegate->EncodeNodeManufAttestInfo(writer);
        SuccessOrExit(err);
    }

    tbsDataLen = writer.GetLengthWritten() - tbsDataStartOffset;

    // Calculate TBS hash.
    sha256.Begin();
    sha256.AddData(tbsDataStart, tbsDataLen);
    sha256.Finish(tbsHash);

    // Generate and encode operational device signature.
    err = CertProvDelegate->GenerateNodeOperationalSig(tbsHash, SHA256::kHashLength, writer);
    SuccessOrExit(err);

    // Generate and encode manufacturer attestation device signature.
    if (reqType == kReqType_GetInitialOpDeviceCert)
    {
        err = CertProvDelegate->GenerateNodeManufAttestSig(tbsHash, SHA256::kHashLength, writer);
        SuccessOrExit(err);
    }

    err = writer.EndContainer(containerType);
    SuccessOrExit(err);

    err = writer.Finalize();
    SuccessOrExit(err);

    State = kState_GetCertificateRequestGenerated;

exit:
    return err;
}

WEAVE_ERROR WeaveCertProvClient::ProcessGetCertificateResponse(PacketBuffer *msgBuf)
{
    WEAVE_ERROR err;
    TLVReader reader;
    TLVWriter writer;
    TLVType outerContainer;
    uint16_t dataLen = msgBuf->DataLength();
    uint16_t availableDataLen = msgBuf->AvailableDataLength();
    uint8_t * dataStart = msgBuf->Start();
    uint8_t * dataMove = dataStart + availableDataLen;
    uint8_t * cert;
    uint16_t certLen;
    uint8_t * relatedCerts = NULL;
    uint16_t relatedCertsLen = 0;

    VerifyOrExit(State == kState_GetCertificateRequestGenerated, err = WEAVE_ERROR_INCORRECT_STATE);

    // Move message data to the end of message buffer.
    memmove(dataMove, dataStart, dataLen);

    reader.Init(dataMove, dataLen);

    writer.Init(dataStart, dataLen + availableDataLen);

    err = reader.Next(kTLVType_Structure, AnonymousTag);
    SuccessOrExit(err);

    err = reader.EnterContainer(outerContainer);
    SuccessOrExit(err);

    err = reader.Next(kTLVType_Structure, ContextTag(kTag_GetCertRespMsg_OpDeviceCert));
    SuccessOrExit(err);

    err = writer.CopyContainer(ProfileTag(kWeaveProfile_Security, kTag_WeaveCertificate), reader);
    SuccessOrExit(err);

    cert = dataStart;
    certLen = writer.GetLengthWritten();

    err = reader.Next(kTLVType_Array, ContextTag(kTag_GetCertRespMsg_RelatedCerts));

    // Optional Weave intermediate certificates.
    if (err == WEAVE_NO_ERROR)
    {
        relatedCerts = cert + certLen;

        err = writer.CopyContainer(ProfileTag(kWeaveProfile_Security, kTag_WeaveCertificateList), reader);
        SuccessOrExit(err);

        relatedCertsLen = writer.GetLengthWritten() - certLen;

        err = reader.Next();
    }

    err = reader.VerifyEndOfContainer();
    SuccessOrExit(err);

    err = reader.ExitContainer(outerContainer);
    SuccessOrExit(err);

    err = reader.Next();
    VerifyOrExit(err == WEAVE_END_OF_TLV, err = WEAVE_ERROR_UNEXPECTED_TLV_ELEMENT);

    VerifyOrExit(reader.GetLengthRead() == dataLen, err = WEAVE_ERROR_INVALID_MESSAGE_LENGTH);

    // Call the delegate to replace local node Weave operational certificate.
    err = CertProvDelegate->StoreServiceAssignedNodeOperationalCert(cert, certLen, relatedCerts, relatedCertsLen);
    SuccessOrExit(err);

    State = kState_Complete;

exit:
    return err;
}

} // namespace CertProvisioning
} // namespace Security
} // namespace Profiles
} // namespace Weave
} // namespace nl
