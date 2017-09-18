/*
 *
 *    Copyright (c) 2013-2017 Nest Labs, Inc.
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
 *      This file implements message objects for initiators and
 *      responders for the Weave Certificate Authenticated Session
 *      Establishment (CASE) protocol.
 *
 */

#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif
#include <stdint.h>

#include <Weave/Core/WeaveCore.h>
#include "WeaveCASE.h"
#include <Weave/Core/WeaveTLV.h>
#include <Weave/Profiles/WeaveProfiles.h>
#include <Weave/Profiles/security/WeaveSecurity.h>
#include <Weave/Profiles/security/WeaveSig.h>
#include <Weave/Profiles/security/WeavePrivateKey.h>
#include <Weave/Support/crypto/HashAlgos.h>
#include <Weave/Support/crypto/EllipticCurve.h>
#include <Weave/Support/CodeUtils.h>


namespace nl {
namespace Weave {
namespace Profiles {
namespace Security {
namespace CASE {

using namespace nl::Weave::Profiles;
using namespace nl::Weave::Crypto;
using namespace nl::Weave::Encoding;
using namespace nl::Weave::TLV;


// Encode the initial portion of Weave CASE BeginSessionRequest message, not including the DH public key,
// the certificate info, the payload or the signature.
WEAVE_ERROR BeginSessionRequestMessage::EncodeHead(PacketBuffer *msgBuf)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint8_t *p = msgBuf->Start();
    uint8_t bufSize = msgBuf->MaxDataLength();

    VerifyOrExit(AlternateConfigCount < kMaxAlternateProtocolConfigs, err = WEAVE_ERROR_INVALID_ARGUMENT);
    VerifyOrExit(AlternateCurveCount < kMaxAlternateCurveIds, err = WEAVE_ERROR_INVALID_ARGUMENT);

    // Verify we have enough room to do our job.
    VerifyOrExit(bufSize > HeadLength(), err = WEAVE_ERROR_BUFFER_TOO_SMALL);

    // Encode the control header.
    *p++ = (EncryptionType & kCASEHeader_EncryptionTypeMask) |
           ((PerformKeyConfirm) ? kCASEHeader_PerformKeyConfirmFlag : 0);

    // Encode the alternate config count, alternate curve count and DH public key length.
    *p++ = AlternateConfigCount;
    *p++ = AlternateCurveCount;
    *p++ = ECDHPublicKey.ECPointLen;

    // Encode the certificate information length.
    LittleEndian::Write16(p, CertInfoLength);

    // Encode the payload length.
    LittleEndian::Write16(p, PayloadLength);

    // Encode the proposed protocol config.
    LittleEndian::Write32(p, ProtocolConfig);

    // Encode the proposed elliptic curve.
    LittleEndian::Write32(p, CurveId);

    // Encode the session id.
    LittleEndian::Write16(p, SessionKeyId);

    // Encode the list of alternate configs.
    for (uint8_t i = 0; i < AlternateConfigCount; i++)
        LittleEndian::Write32(p, AlternateConfigs[i]);

    // Encode the list of alternate curves.
    for (uint8_t i = 0; i < AlternateCurveCount; i++)
        LittleEndian::Write32(p, AlternateCurveIds[i]);

exit:
    return err;
}

// Decode the initial portion of a Weave CASE BeginSessionRequest message.
WEAVE_ERROR BeginSessionRequestMessage::DecodeHead(PacketBuffer *msgBuf)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint8_t *p = msgBuf->Start();
    uint16_t msgLen = msgBuf->DataLength();
    uint16_t msgLenWithoutSig;
    uint8_t controlHeader;

    // Verify we can read the fixed length portion of the message without running into the end of the buffer.
    VerifyOrExit(msgLen > 18, err = WEAVE_ERROR_MESSAGE_INCOMPLETE);

    // Parse and decode the control header.
    controlHeader = *p++;
    EncryptionType = controlHeader & kCASEHeader_EncryptionTypeMask;
    PerformKeyConfirm = ((controlHeader & kCASEHeader_PerformKeyConfirmFlag) != 0);
    VerifyOrExit((controlHeader & kCASEHeader_ControlHeaderUnusedBits) == 0, err = WEAVE_ERROR_INVALID_ARGUMENT);

    // Parse the alternate config count, alternate curve count and DH public key length.
    AlternateConfigCount = *p++;
    VerifyOrExit(AlternateConfigCount <= kMaxAlternateProtocolConfigs, err = WEAVE_ERROR_INVALID_ARGUMENT);
    AlternateCurveCount = *p++;
    VerifyOrExit(AlternateCurveCount <= kMaxAlternateCurveIds, err = WEAVE_ERROR_INVALID_ARGUMENT);
    ECDHPublicKey.ECPointLen = *p++;

    // Parse the certificate information length.
    CertInfoLength = LittleEndian::Read16(p);

    // Parse the payload length.
    PayloadLength = LittleEndian::Read16(p);

    // Parse the proposed protocol config.
    ProtocolConfig = LittleEndian::Read32(p);

    // Parse the proposed curve id.
    CurveId = LittleEndian::Read32(p);

    // Parse the session key id.
    SessionKeyId = LittleEndian::Read16(p);

    // Verify the overall message length is consistent with the claimed field sizes.
    msgLenWithoutSig = HeadLength() + ECDHPublicKey.ECPointLen + CertInfoLength + PayloadLength;
    VerifyOrExit(msgLen > msgLenWithoutSig, err = WEAVE_ERROR_MESSAGE_INCOMPLETE);

    // Parse the alternate configs list.
    for (uint8_t i = 0; i < AlternateConfigCount; i++)
        AlternateConfigs[i] = LittleEndian::Read32(p);

    // Parse the alternate curves list.
    for (uint8_t i = 0; i < AlternateCurveCount; i++)
        AlternateCurveIds[i] = LittleEndian::Read32(p);

    // Save a pointer to the DH public key.
    ECDHPublicKey.ECPoint = p;
    p += ECDHPublicKey.ECPointLen;

    // Save a pointer to the initiator's certificate information.
    CertInfo = p;
    p += CertInfoLength;

    // Save a pointer to the payload data.
    Payload = p;
    p += PayloadLength;

    // Save a pointer to the signature and compute the signature length.
    Signature = p;
    SignatureLength = msgLen - msgLenWithoutSig;

exit:
    return err;
}

// Determine if the given config is among the list of alternate configs.
bool BeginSessionRequestMessage::IsAltConfig(uint32_t config) const
{
    for (uint8_t i = 0; i < AlternateConfigCount; i++)
        if (AlternateConfigs[i] == config)
            return true;
    return false;
}

// Encode the initial portion of a Weave CASE BeginSessionResponse message.
WEAVE_ERROR BeginSessionResponseMessage::EncodeHead(PacketBuffer *msgBuf)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint8_t *p = msgBuf->Start();
    uint8_t bufSize = msgBuf->MaxDataLength();
    uint8_t controlHeader = 0;

    // Verify we have enough room to do our job.
    VerifyOrExit(bufSize > HeadLength(), err = WEAVE_ERROR_BUFFER_TOO_SMALL);

    // If performing key confirmation, encode the length of the key confirmation hash value
    // in the upper bits of the control header.
    if (PerformKeyConfirm)
    {
        switch (KeyConfirmHashLength)
        {
        case 20:
            controlHeader |= kCASEKeyConfirmHashLength_20Bytes;
            break;
        case 32:
            controlHeader |= kCASEKeyConfirmHashLength_32Bytes;
            break;
        default:
            ExitNow(err = WEAVE_ERROR_INVALID_ARGUMENT);
        }
    }

    // Encode the control header.
    *p++ = controlHeader;

    // Encode the DH public key length.
    *p++ = ECDHPublicKey.ECPointLen;

    // Encode the certificate information length.
    LittleEndian::Write16(p, CertInfoLength);

    // Encode the payload length.
    LittleEndian::Write16(p, PayloadLength);

exit:
    return err;
}

// Decode a Weave CASE BeginSessionResponse message.
WEAVE_ERROR BeginSessionResponseMessage::DecodeHead(PacketBuffer *msgBuf)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    const uint8_t *p = msgBuf->Start();
    uint16_t msgLen = msgBuf->DataLength();
    uint16_t msgLenWithoutSig;
    uint8_t controlHeader;

    // Verify we can read up to the public key field field without running into the end of the message.
    VerifyOrExit(msgLen > 6, err = WEAVE_ERROR_MESSAGE_INCOMPLETE);

    // Parse and validate the control header.
    controlHeader = *p++;
    VerifyOrExit((controlHeader & ~kCASEHeader_KeyConfirmHashLengthMask) == 0, err = WEAVE_ERROR_INVALID_ARGUMENT);

    // Parse the various length fields.
    ECDHPublicKey.ECPointLen = *p++;
    CertInfoLength = LittleEndian::Read16(p);
    PayloadLength = LittleEndian::Read16(p);

    // Determine the length of the key confirmation hash field, if present.
    switch (controlHeader & kCASEHeader_KeyConfirmHashLengthMask)
    {
    case kCASEKeyConfirmHashLength_0Bytes:
        PerformKeyConfirm = false;
        KeyConfirmHashLength = 0;
        break;
    case kCASEKeyConfirmHashLength_20Bytes:
        PerformKeyConfirm = true;
        KeyConfirmHashLength = 20;
        break;
    case kCASEKeyConfirmHashLength_32Bytes:
        PerformKeyConfirm = true;
        KeyConfirmHashLength = 32;
        break;
    default:
        ExitNow(err = WEAVE_ERROR_INVALID_ARGUMENT);
    }

    // Verify the overall message length is consistent with the claimed field sizes.
    msgLenWithoutSig = 6 + ECDHPublicKey.ECPointLen + CertInfoLength + PayloadLength + KeyConfirmHashLength;
    VerifyOrExit(msgLen > msgLenWithoutSig, err = WEAVE_ERROR_MESSAGE_INCOMPLETE);

    // Computed the signature field length.
    SignatureLength = msgLen - msgLenWithoutSig;

    // Save a pointer to the DH public key.
    ECDHPublicKey.ECPoint = (uint8_t *)p;
    p += ECDHPublicKey.ECPointLen;

    // Save a pointer to the initiator's certificate information.
    CertInfo = p;
    p += CertInfoLength;

    // Save a pointer to the payload data.
    Payload = p;
    p += PayloadLength;

    // Save a pointer to the signature and compute the signature length.
    Signature = p;
    p += SignatureLength;

    // Save a pointer to the start of the key confirmation hash, if present.
    KeyConfirmHash = (PerformKeyConfirm) ? p : NULL;

exit:
    return err;
}


WEAVE_ERROR ReconfigureMessage::Encode(PacketBuffer *msgBuf)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint8_t *p = msgBuf->Start();
    uint8_t bufSize = msgBuf->MaxDataLength();

    // Verify we have enough room to do our job.
    VerifyOrExit(bufSize >= 8, err = WEAVE_ERROR_BUFFER_TOO_SMALL);

    // Encode the proposed protocol config.
    LittleEndian::Write32(p, ProtocolConfig);

    // Encode the proposed elliptic curve.
    LittleEndian::Write32(p, CurveId);

    // Set the message length.
    msgBuf->SetDataLength(8);

exit:
    return err;
}

WEAVE_ERROR ReconfigureMessage::Decode(PacketBuffer *msgBuf, ReconfigureMessage& msg)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    const uint8_t *p = msgBuf->Start();
    uint16_t msgLen = msgBuf->DataLength();

    // Verify the size of the message.
    VerifyOrExit(msgLen >= 8, err = WEAVE_ERROR_MESSAGE_INCOMPLETE);
    VerifyOrExit(msgLen == 8, err = WEAVE_ERROR_MESSAGE_TOO_LONG);

    msg.ProtocolConfig = LittleEndian::Read32(p);
    msg.CurveId = LittleEndian::Read32(p);

exit:
    return err;
}


} // namespace CASE
} // namespace Security
} // namespace Profiles
} // namespace Weave
} // namespace nl
