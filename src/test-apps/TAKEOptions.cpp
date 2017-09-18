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
 *      Implementation of TAKEConfig object, which provides an implementation of the WeaveTAKEAuthDelegate
 *      interface for use in test applications.
 *
 */


#include "ToolCommon.h"
#include <Weave/Support/ASN1.h>
#include "TAKEOptions.h"

TAKEOptions gTAKEOptions;
MockTAKEChallengerDelegate gMockTAKEChallengerDelegate;
MockTAKETokenDelegate gMockTAKETokenDelegate;

TAKEOptions::TAKEOptions()
{
    static OptionDef optionDefs[] =
    {
#if WEAVE_CONFIG_ENABLE_TAKE_INITIATOR || WEAVE_CONFIG_ENABLE_TAKE_RESPONDER
        { "take-reauth", kNoArgument, kToolCommonOpt_TAKEReauth           },
#endif
        { NULL }
    };
    OptionDefs = optionDefs;

    HelpGroupName = "TAKE OPTIONS";

    OptionHelp =
#if WEAVE_CONFIG_ENABLE_TAKE_INITIATOR || WEAVE_CONFIG_ENABLE_TAKE_RESPONDER
        "  --take-reauth\n"
        "       Pre-populate the challenger token data store with the AK and\n"
        "       encrypted-AK for the token such that the initial TAKE interaction\n"
        "       is a re-authentication.\n"
        "\n"
#endif
        "";

    // Defaults
    static const uint8_t ik[] = { 0x05, 0x26, 0xAD, 0xB7, 0xBB, 0xD7, 0x82, 0x52, 0x78, 0x2D, 0x60, 0xD6, 0x40, 0xFD, 0xE6, 0xF9 };
    static const uint8_t challengerId[] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F };
    static uint8_t tPub[] = { 0x04, 0x55, 0x7B, 0x11, 0x55, 0xE5, 0xE2, 0x59, 0xB1, 0x98, 0xB2, 0x56, 0x13, 0xE3, 0x5B, 0xA7, 0x91, 0x5C, 0xB1, 0x4A, 0x8D, 0xC4, 0x08, 0x99, 0x03, 0x8F, 0x51, 0xB4, 0xAE, 0xC4, 0xA8, 0x95, 0x1F, 0xF6, 0x65, 0xFF, 0x21, 0x12, 0x3E, 0x8E, 0x1C, 0x36, 0x60, 0xB3, 0x3D, 0xB3, 0x02, 0x5B, 0xA5, 0xB7, 0xD9, 0xFE, 0xA2, 0xB1, 0x01, 0x42, 0x13 };
    static const uint8_t tPriv[] = { 0x54, 0x7A, 0x86, 0xF5, 0x6E, 0xFF, 0xDC, 0x52, 0x22, 0x13, 0xBA, 0x8C, 0x00, 0x88, 0x0A, 0x9C, 0x62, 0x1D, 0xCB, 0xA5, 0xD1, 0xD7, 0x70, 0xDF, 0x23, 0x40, 0x7D, 0x18 };
    static const uint8_t irk[] = { 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F };
    static const uint8_t masterKey[] = { 0x11, 0xFF, 0xF1, 0x1F, 0xD1, 0x3F, 0xB1, 0x5F, 0x91, 0x7F, 0x71, 0x9F, 0x51, 0xBF, 0x31, 0xDF, 0x11, 0xFF, 0xF1, 0x1F, 0xD1, 0x3F, 0xB1, 0x5F, 0x91, 0x7F, 0x71, 0x9F, 0x51, 0xBF, 0x31, 0xDF };
    static const uint8_t ak[] = { 0x9F, 0x0F, 0x92, 0xE3, 0xB9, 0x04, 0x96, 0xA1, 0xCB, 0x7C, 0x94, 0x99, 0xAB, 0x34, 0xDD, 0x04 };
    static const uint8_t encAK[] = { 0xE6, 0xC4, 0x03, 0xE8, 0xEE, 0xA3, 0x80, 0x56, 0xE0, 0xB1, 0x9C, 0xE9, 0xE3, 0xA6, 0xD8, 0x3A };
    IK = ik;
    ChallengerId = challengerId;
    ChallengerIdLen = sizeof(challengerId);
    TPub = tPub;
    TPubLen = sizeof(tPub);
    TPriv = tPriv;
    TPrivLen = sizeof(tPriv);
    IRK = irk;
    MasterKey = masterKey;
    AK = ak;
    EncAK = encAK;
    ForceReauth = false;
}

bool TAKEOptions::HandleOption(const char *progName, OptionSet *optSet, int id, const char *name, const char *arg)
{
    switch (id)
    {
    case kToolCommonOpt_TAKEReauth:
        ForceReauth = true;
        break;
    default:
        PrintArgError("%s: INTERNAL ERROR: Unhandled option: %s\n", progName, name);
        return false;
    }

    return true;
}

WEAVE_ERROR TAKEOptions::PrepopulateTokenData()
{
    WEAVE_ERROR err;
    uint16_t authKeyLen = TAKE::kAuthenticationKeySize;
    uint16_t encryptedAuthKeyLen = kTokenEncryptedStateSize;

    err = gMockTAKEChallengerDelegate.StoreTokenAuthData(1, kTAKEConfig_Config1, AK, authKeyLen, EncAK, encryptedAuthKeyLen);
    SuccessOrExit(err);

exit:
    return err;
}

static uint8_t AuthenticationKeyBuffer[TAKE::kAuthenticationKeySize];
static uint8_t EncryptedAuthenticationKeyBuffer[TAKE::kTokenEncryptedStateSize];


MockTAKEChallengerDelegate::MockTAKEChallengerDelegate() :
    AuthenticationKeySet(false),
    Rewinded(false)
{
    return;
}

// Rewind Identification Key Iterator.
// Called to prepare for a new Identification Key search.
WEAVE_ERROR MockTAKEChallengerDelegate::RewindIdentificationKeyIterator()
{
    Rewinded = true;
    return WEAVE_NO_ERROR;
}

// Get next {tokenId, IK} pair.
// returns tokenId = kNodeIdNotSpecified if no more IKs are available.
WEAVE_ERROR MockTAKEChallengerDelegate::GetNextIdentificationKey(uint64_t & tokenId, uint8_t *identificationKey, uint16_t & identificationKeyLen)
{
    if (Rewinded)
    {
        if (identificationKeyLen < kIdentificationKeySize)
            return WEAVE_ERROR_BUFFER_TOO_SMALL;
        tokenId = 1;
        identificationKeyLen = kIdentificationKeySize;
        memcpy(identificationKey, gTAKEOptions.IK, identificationKeyLen);
        Rewinded = false;
    }
    else
    {
        tokenId = nl::Weave::kNodeIdNotSpecified;
    }
    return WEAVE_NO_ERROR;
}

// Get Token Authentication Data.
// Function returns {takeConfig = kTAKEConfig_Invalid, authKey = NULL, encAuthBlob = NULL} if Authentication Data associated with a specified Token
// is not stored on the device.
// On the function call authKeyLen and encAuthBlobLen inputs specify sizes of the authKey and encAuthBlob buffers, respectively.
// Function should update these parameters to reflect actual sizes.
WEAVE_ERROR MockTAKEChallengerDelegate::GetTokenAuthData(uint64_t tokenId, uint8_t &takeConfig, uint8_t *authKey, uint16_t &authKeyLen, uint8_t *encAuthBlob, uint16_t &encAuthBlobLen)
{
    if (tokenId == 1)
    {
        if (!AuthenticationKeySet)
        {
            takeConfig = kTAKEConfig_Invalid;
            authKey = NULL;
            encAuthBlob = NULL;
            return WEAVE_NO_ERROR;
        }
        if (authKeyLen < TAKE::kAuthenticationKeySize)
            return WEAVE_ERROR_BUFFER_TOO_SMALL;
        if (encAuthBlobLen < kTokenEncryptedStateSize)
            return WEAVE_ERROR_BUFFER_TOO_SMALL;

        takeConfig = kTAKEConfig_Config1;
        authKeyLen = TAKE::kAuthenticationKeySize;
        encAuthBlobLen = kTokenEncryptedStateSize;
        memcpy(authKey, AuthenticationKeyBuffer, authKeyLen);
        memcpy(encAuthBlob, EncryptedAuthenticationKeyBuffer, encAuthBlobLen);
    }
    else
    {
        return WEAVE_ERROR_INVALID_ARGUMENT;
    }
    return WEAVE_NO_ERROR;
}

// Store Token Authentication Data.
// This function should clear Authentication Data that was previously stored on the device for the specified Token (if any).
WEAVE_ERROR MockTAKEChallengerDelegate::StoreTokenAuthData(uint64_t tokenId, uint8_t takeConfig, const uint8_t *authKey, uint16_t authKeyLen, const uint8_t *encAuthBlob, uint16_t encAuthBlobLen)
{
    if (tokenId == 1 && takeConfig == kTAKEConfig_Config1)
    {
        if (authKeyLen < TAKE::kAuthenticationKeySize)
            return WEAVE_ERROR_BUFFER_TOO_SMALL;
        if (encAuthBlobLen < kTokenEncryptedStateSize)
            return WEAVE_ERROR_BUFFER_TOO_SMALL;

        memcpy(AuthenticationKeyBuffer, authKey, authKeyLen);
        memcpy(EncryptedAuthenticationKeyBuffer, encAuthBlob, encAuthBlobLen);

        AuthenticationKeySet = true;
    }
    else
    {
        return WEAVE_ERROR_INVALID_ARGUMENT;
    }
    return WEAVE_NO_ERROR;

}

// Clear Token Authentication Data.
// This function should be called if ReAuthentication phase with the Token Authentication Data stored on the device failed.
WEAVE_ERROR MockTAKEChallengerDelegate::ClearTokenAuthData(uint64_t tokenId)
{
    if (tokenId == 1 && AuthenticationKeySet)
    {
        AuthenticationKeySet = false;
        return WEAVE_NO_ERROR;
    }
    return WEAVE_ERROR_INVALID_ARGUMENT;
}

// Get Token public key.
// On the function call tokenPubKeyLen input specifies size of the tokenPubKey buffer. Function should update this parameter to reflect actual sizes.
WEAVE_ERROR MockTAKEChallengerDelegate::GetTokenPublicKey(uint64_t tokenId, OID& curveOID, EncodedECPublicKey& tokenPubKey)
{
    if (tokenId == 1)
    {
        if (tokenPubKey.ECPointLen < kConfig1_ECPointX962FormatSize)
            return WEAVE_ERROR_BUFFER_TOO_SMALL;

        tokenPubKey.ECPointLen = kConfig1_ECPointX962FormatSize;
        memcpy(tokenPubKey.ECPoint, gTAKEOptions.TPub, tokenPubKey.ECPointLen);
        curveOID = nl::Weave::ASN1::kOID_EllipticCurve_secp224r1;
        return WEAVE_NO_ERROR;
    }
    return WEAVE_ERROR_INVALID_ARGUMENT;
}


// Get the challenger ID.
WEAVE_ERROR MockTAKEChallengerDelegate::GetChallengerID(uint8_t *challengerID, uint8_t &challengerIDLen) const
{
    if (challengerIDLen < gTAKEOptions.ChallengerIdLen)
        return WEAVE_ERROR_BUFFER_TOO_SMALL;
    challengerIDLen = gTAKEOptions.ChallengerIdLen;

    memcpy(challengerID, gTAKEOptions.ChallengerId, challengerIDLen);

    return WEAVE_NO_ERROR;
}

// Get the token Master key. size: kTokenMasterKeySize
WEAVE_ERROR MockTAKETokenDelegate::GetTokenMasterKey(uint8_t *tokenMasterKey) const
{
    memcpy(tokenMasterKey, gTAKEOptions.MasterKey, kTokenMasterKeySize);
    return WEAVE_NO_ERROR;
}

// Get the Identification Root Key. size: kIdentificationRootKeySize
WEAVE_ERROR MockTAKETokenDelegate::GetIdentificationRootKey(uint8_t *identificationRootKey) const
{
    memcpy(identificationRootKey, gTAKEOptions.IRK, kIdentificationRootKeySize);
    return WEAVE_NO_ERROR;
}

// Get the token Private Key.
// On the function call tokenPrivKeyLen input specifies size of the tokenPrivKey buffer.
// Function should update this parameter to reflect actual sizes of the private key.
WEAVE_ERROR MockTAKETokenDelegate::GetTokenPrivateKey(OID& curveOID, EncodedECPrivateKey& tokenPrivKey)  const
{
    if (tokenPrivKey.PrivKeyLen < gTAKEOptions.TPrivLen)
        return WEAVE_ERROR_BUFFER_TOO_SMALL;
    tokenPrivKey.PrivKeyLen = gTAKEOptions.TPrivLen;
    memcpy(tokenPrivKey.PrivKey, gTAKEOptions.TPriv, gTAKEOptions.TPrivLen);

    curveOID = nl::Weave::ASN1::kOID_EllipticCurve_secp224r1;

    return WEAVE_NO_ERROR;
}

// Get TAKE Time.
// Function returns takeTime, which is Unix time rounded with 24 hour granularity
// i.e. number of days elapsed after 1 January 1970.
WEAVE_ERROR MockTAKETokenDelegate::GetTAKETime(uint32_t &takeTime) const
{
    takeTime = 17167; // number of days til 01/01/2017

    return WEAVE_NO_ERROR;
}
