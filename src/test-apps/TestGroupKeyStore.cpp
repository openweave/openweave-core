/*
 *
 *    Copyright (c) 2016-2017 Nest Labs, Inc.
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
 *      Test platform key store object.
 *
 */

#include "TestGroupKeyStore.h"
#include <Weave/Support/TimeUtils.h>
#include <Weave/Core/WeaveKeyIds.h>
#include <Weave/Core/WeaveCore.h>
#include <Weave/Support/CodeUtils.h>
#include <Weave/Support/crypto/WeaveCrypto.h>
#include <Weave/Profiles/security/WeavePasscodes.h>

using namespace nl::Weave;
using namespace nl::Weave::Crypto;
using namespace nl::Weave::Profiles::Security::AppKeys;
using namespace nl::Weave::Profiles::Security::Passcodes;

uint32_t sLastUsedEpochKeyId = WeaveKeyId::kNone;
uint32_t sCurrentUTCTime = 0x0;

/* ************************************************************ */
/*  Constituent Key Material.                                   */
/* ************************************************************ */

const uint8_t sFabricSecret[] =
{
    0xFA, 0x00, 0xFA, 0x01, 0xFA, 0x02, 0xFA, 0x03, 0xFA, 0x04, 0xFA, 0x05, 0xFA, 0x06, 0xFA, 0x07,
    0xFA, 0x08, 0xFA, 0x09, 0xFA, 0x0A, 0xFA, 0x0B, 0xFA, 0x0C, 0xFA, 0x0D, 0xFA, 0x0E, 0xFA, 0x0F,
    0xFA, 0x10, 0xFA, 0x11,
};
const uint8_t sFabricSecretLen = sizeof(sFabricSecret);

const uint8_t sServiceRootKey[] =
{
    0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F,
    0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF,
};
const uint8_t sServiceRootKeyLen = sizeof(sServiceRootKey);

const uint32_t sInvalidRootKeyNumber = 3;
const uint32_t sInvalidRootKeyId = WeaveKeyId::MakeRootKeyId(sInvalidRootKeyNumber);

const uint32_t sEpochKey0_Number = 0;
const uint32_t sEpochKey0_KeyId = WeaveKeyId::MakeEpochKeyId(sEpochKey0_Number);
const uint32_t sEpochKey0_StartTime = 0x56E34CF0; // 3/11/2016, 2:55:44 PM
const uint8_t sEpochKey0_Key[] =
{
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
    0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF,
};
const uint8_t sEpochKey0_KeyLen = sizeof(sEpochKey0_Key);

const uint32_t sEpochKey1_Number = 1;
const uint32_t sEpochKey1_KeyId = WeaveKeyId::MakeEpochKeyId(sEpochKey1_Number);
const uint32_t sEpochKey1_StartTime = sEpochKey0_StartTime + nl::kSecondsPerDay;
const uint8_t sEpochKey1_Key[] =
{
    0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F,
    0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F,
};
const uint8_t sEpochKey1_KeyLen = sizeof(sEpochKey1_Key);

const uint32_t sEpochKey2_Number = 2;
const uint32_t sEpochKey2_KeyId = WeaveKeyId::MakeEpochKeyId(sEpochKey2_Number);
const uint32_t sEpochKey2_StartTime = sEpochKey1_StartTime + nl::kSecondsPerDay;
const uint8_t sEpochKey2_Key[] =
{
    0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF,
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,
};
const uint8_t sEpochKey2_KeyLen = sizeof(sEpochKey2_Key);

const uint32_t sEpochKey3_Number = 3;
const uint32_t sEpochKey3_KeyId = WeaveKeyId::MakeEpochKeyId(sEpochKey3_Number);
const uint32_t sEpochKey3_StartTime = sEpochKey2_StartTime + nl::kSecondsPerDay;
const uint8_t sEpochKey3_Key[] =
{
    0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF,
    0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F,
};
const uint8_t sEpochKey3_KeyLen = sizeof(sEpochKey3_Key);

const uint32_t sEpochKey4_Number = 4;
const uint32_t sEpochKey4_KeyId = WeaveKeyId::MakeEpochKeyId(sEpochKey4_Number);
const uint32_t sEpochKey4_StartTime = sEpochKey3_StartTime + nl::kSecondsPerDay;
const uint8_t sEpochKey4_Key[] =
{
    0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F,
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x20
};
const uint8_t sEpochKey4_KeyLen = sizeof(sEpochKey4_Key);

const uint32_t sEpochKey5_Number = 5;
const uint32_t sEpochKey5_KeyId = WeaveKeyId::MakeEpochKeyId(sEpochKey5_Number);
const uint32_t sEpochKey5_StartTime = sEpochKey4_StartTime + nl::kSecondsPerDay;
const uint8_t sEpochKey5_Key[] =
{
    0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F,
    0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x90
};
const uint8_t sEpochKey5_KeyLen = sizeof(sEpochKey5_Key);

const uint32_t sAppGroupMasterKey0_Number = 0;
const uint32_t sAppGroupMasterKey0_KeyId = WeaveKeyId::MakeAppGroupMasterKeyId(sAppGroupMasterKey0_Number);
const uint32_t sAppGroupMasterKey0_GlobalId = 0x80808080;
const uint8_t sAppGroupMasterKey0_Key[] =
{
    0xDF, 0xDE, 0xDD, 0xDC, 0xDB, 0xDA, 0xD9, 0xD8, 0xD7, 0xD6, 0xD5, 0xD4, 0xD3, 0xD2, 0xD1, 0xD0,
    0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F,
};
const uint8_t sAppGroupMasterKey0_KeyLen = sizeof(sAppGroupMasterKey0_Key);

const uint32_t sAppGroupMasterKey4_Number = 4;
const uint32_t sAppGroupMasterKey4_KeyId = WeaveKeyId::MakeAppGroupMasterKeyId(sAppGroupMasterKey4_Number);
const uint32_t sAppGroupMasterKey4_GlobalId = 0x84848484;
const uint8_t sAppGroupMasterKey4_Key[] =
{
    0x3F, 0x3E, 0x3D, 0x3C, 0x3B, 0x3A, 0x39, 0x38, 0x37, 0x36, 0x35, 0x34, 0x33, 0x32, 0x31, 0x30,
    0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF,
};
const uint8_t sAppGroupMasterKey4_KeyLen = sizeof(sAppGroupMasterKey4_Key);

const uint32_t sAppGroupMasterKey10_Number = 10;
const uint32_t sAppGroupMasterKey10_KeyId = WeaveKeyId::MakeAppGroupMasterKeyId(sAppGroupMasterKey10_Number);
const uint32_t sAppGroupMasterKey10_GlobalId = 0x8A8A8A8A;
const uint8_t sAppGroupMasterKey10_Key[] =
{
    0xEF, 0xEE, 0xED, 0xEC, 0xEB, 0xEA, 0xE9, 0xE8, 0xE7, 0xE6, 0xE5, 0xE4, 0xE3, 0xE2, 0xE1, 0xE0,
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,
};
const uint8_t sAppGroupMasterKey10_KeyLen = sizeof(sAppGroupMasterKey10_Key);

const uint32_t sAppGroupMasterKey54_Number = 54;
const uint32_t sAppGroupMasterKey54_KeyId = WeaveKeyId::MakeAppGroupMasterKeyId(sAppGroupMasterKey54_Number);
const uint32_t sAppGroupMasterKey54_GlobalId = 0xB6B6B6B6;
const uint8_t sAppGroupMasterKey54_Key[] =
{
    0x7F, 0x7E, 0x7D, 0x7C, 0x7B, 0x7A, 0x79, 0x78, 0x77, 0x76, 0x75, 0x74, 0x73, 0x72, 0x71, 0x70,
    0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF,
};
const uint8_t sAppGroupMasterKey54_KeyLen = sizeof(sAppGroupMasterKey54_Key);

const uint32_t sAppGroupMasterKey7_Number = 7;
const uint32_t sAppGroupMasterKey7_KeyId = WeaveKeyId::MakeAppGroupMasterKeyId(sAppGroupMasterKey7_Number);
const uint32_t sAppGroupMasterKey7_GlobalId = 0x37373737;
const uint8_t sAppGroupMasterKey7_Key[] =
{
    0x7F, 0x7E, 0x7D, 0x7C, 0x7B, 0x7A, 0x79, 0x78, 0x77, 0x76, 0x75, 0x74, 0x73, 0x72, 0x71, 0x70,
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x30
};
const uint8_t sAppGroupMasterKey7_KeyLen = sizeof(sAppGroupMasterKey7_Key);

/* ************************************************************ */
/*  Derived Keys.                                               */
/* ************************************************************ */

// Fabric root key derived from device fabric secret.
const uint8_t sFabricRootKey[] =
{
    0x9F, 0xCD, 0x16, 0x87, 0x32, 0x5F, 0xE9, 0x09, 0xD8, 0xA0, 0xB2, 0x94, 0xC7, 0x81, 0x05, 0xAD,
    0x93, 0xA6, 0x92, 0x9B, 0x85, 0x4B, 0x4F, 0x59, 0x30, 0xF6, 0xB7, 0x1D, 0xCB, 0xED, 0xE1, 0xB6,
};
const uint8_t sFabricRootKeyLen = sizeof(sFabricRootKey);

// Client root key derived from device fabric secret.
const uint8_t sClientRootKey[] =
{
    0xA2, 0x58, 0x83, 0x0C, 0xEE, 0xF6, 0x4F, 0x12, 0x21, 0x3C, 0xFA, 0xA1, 0xF0, 0xA5, 0xFC, 0x69,
    0x26, 0x69, 0xC6, 0x47, 0x4C, 0x76, 0x38, 0xE6, 0xBE, 0xF9, 0xAD, 0x02, 0xD9, 0xD5, 0x4C, 0xAC,
};
const uint8_t sClientRootKeyLen = sizeof(sClientRootKey);


// Intermediate key derived from fabric root key and epoch key #2.
const uint32_t sIntermediateKeyId_FRK_E2 = WeaveKeyId::MakeAppIntermediateKeyId(WeaveKeyId::kFabricRootKey, sEpochKey2_KeyId, false);
const uint32_t sIntermediateKeyId_FRK_EC = WeaveKeyId::MakeAppIntermediateKeyId(WeaveKeyId::kFabricRootKey, WeaveKeyId::kNone, true);
const uint8_t sIntermediateKey_FRK_E2[] =
{
    0x52, 0x82, 0xD7, 0x8E, 0x4B, 0xF3, 0x46, 0xDB, 0x75, 0x1E, 0xD7, 0x8B, 0x47, 0x73, 0x8B, 0x02,
    0x8A, 0x56, 0xD6, 0xDF, 0x62, 0x9C, 0x67, 0xE2, 0xC4, 0x5C, 0x37, 0x9C, 0xA9, 0x30, 0xD7, 0xC8,
};
const uint8_t sIntermediateKeyLen_FRK_E2 = sizeof(sIntermediateKey_FRK_E2);


// Application static key derived from client root key and group master key #10.
const uint32_t sAppStaticKeyId_CRK_G10 = WeaveKeyId::MakeAppStaticKeyId(WeaveKeyId::kClientRootKey, sAppGroupMasterKey10_KeyId);
const uint8_t sAppStaticKeyDiversifier_CRK_G10[] =
{
    0x74, 0x98, 0x57, 0xFB, 0x21, 0xDB, 0x2B, 0x28, 0x4D, 0x8D, 0x40,
};
const uint8_t sAppStaticKeyDiversifierLen_CRK_G10 = sizeof(sAppStaticKeyDiversifier_CRK_G10);
const uint8_t sAppStaticKey_CRK_G10[] =
{
    0x68, 0xBB, 0x09, 0xA5, 0x04, 0x76, 0x1D, 0x68, 0x07, 0x78, 0xC7, 0xF8, 0x34, 0xA6, 0x71, 0x0E,
    0x7E, 0xA4, 0x89, 0x8F, 0x4D, 0x1D, 0xE5, 0x03, 0x64, 0xBA, 0xB4, 0xD7, 0x19, 0x76, 0xD8, 0x1B,
    0x0D, 0x29, 0xA4, 0xA6, 0x04, 0x3C, 0xF1, 0x87, 0xDD, 0x96, 0x55, 0x09, 0x6B, 0x64, 0x49, 0x70,
};
const uint8_t sAppStaticKeyLen_CRK_G10 = sizeof(sAppStaticKey_CRK_G10);


// Application rotating key derived from service root key, epoch key #3 and group master key #54.
const uint32_t sAppRotatingKeyId_SRK_E3_G54 = WeaveKeyId::MakeAppRotatingKeyId(WeaveKeyId::kServiceRootKey, sEpochKey3_KeyId, sAppGroupMasterKey54_KeyId, false);
const uint8_t sAppRotatingKeyDiversifier_SRK_E3_G54[] =
{
    0x74, 0x98, 0x57, 0xFB, 0x21, 0xDB, 0x2B, 0x28, 0x4D, 0x8D, 0x40, 0x45, 0x46, 0x47, 0x48, 0x49,
};
const uint8_t sAppRotatingKeyDiversifierLen_SRK_E3_G54 = sizeof(sAppRotatingKeyDiversifier_SRK_E3_G54);
const uint8_t sAppRotatingKey_SRK_E3_G54[] =
{
    0x9B, 0x80, 0xEF, 0xFB, 0x6A, 0xC6, 0x94, 0xBD, 0xB8, 0xF5, 0x54, 0xFC, 0x8D, 0x8E, 0x54, 0xA2,
    0x8C, 0x19, 0xEE, 0x07, 0x89, 0xE9, 0x2A, 0x8F, 0xF7, 0x0F, 0xF5, 0xEA, 0x58, 0xAB, 0x60, 0x2C,
    0x38, 0x6E, 0xE6, 0xE0, 0x52, 0x21, 0xCE, 0xEA, 0xBE, 0x00, 0x55, 0xC8, 0xCE, 0x52, 0x7F, 0x5F,
    0x4C, 0xC3, 0x43, 0x20, 0xDC, 0xA0, 0x21, 0x46, 0x5B, 0xF8, 0xF4, 0x9D, 0x66, 0x36, 0x75, 0xA6,
};
const uint8_t sAppRotatingKeyLen_SRK_E3_G54 = sizeof(sAppRotatingKey_SRK_E3_G54);

/* ************************************************************** */
/*  Derived Application Keys - Also Used for Passcode Encryption. */
/* ************************************************************** */

// Passcode nonce.
const uint32_t sPasscodeEncryptionKeyNonce = 0xF4A825C9;
const uint8_t sPasscodeEncryptionKeyNonceLen = sizeof(sPasscodeEncryptionKeyNonce);

// Passcode encyption (and authentication) key diversifier.
const uint8_t sPasscodeEncryptionKeyDiversifier[] =
{
    kPasscodeEncKeyDiversifier[0], kPasscodeEncKeyDiversifier[1],
    kPasscodeEncKeyDiversifier[2], kPasscodeEncKeyDiversifier[3], kPasscode_Config2
};
const uint8_t sPasscodeEncryptionKeyDiversifierLen = sizeof(sPasscodeEncryptionKeyDiversifier);

// Passcode encryption (and authentication) ROTATING key derived from client root key, epoch key #0 and group master key #4.
const uint32_t sPasscodeEncRotatingKeyId_CRK_E0_G4 = WeaveKeyId::MakeAppRotatingKeyId(WeaveKeyId::kClientRootKey, sEpochKey0_KeyId, sAppGroupMasterKey4_KeyId, false);
const uint8_t sPasscodeEncRotatingKey_CRK_E0_G4[] =
{
    0x10, 0xB0, 0x5F, 0x61, 0x2A, 0x54, 0x4D, 0x3E, 0xC0, 0x0E, 0xBF, 0x06, 0x3E, 0x35, 0x65, 0xF2,
    0xEF, 0x06, 0x28, 0x96, 0x0B, 0x17, 0x50, 0x98, 0x1B, 0x18, 0x3A, 0xB8, 0xA5, 0xB6, 0x34, 0xF6,
    0x5A, 0xD4, 0x05, 0x36,
};
const uint8_t sPasscodeEncRotatingKeyLen_CRK_E0_G4 = sizeof(sPasscodeEncRotatingKey_CRK_E0_G4);

// Passcode encryption (and authentication) STATIC key derived from client root key and group master key #4.
const uint32_t sPasscodeEncStaticKeyId_CRK_G4 = WeaveKeyId::MakeAppStaticKeyId(WeaveKeyId::kClientRootKey, sAppGroupMasterKey4_KeyId);
const uint8_t sPasscodeEncStaticKey_CRK_G4[] =
{
    0x7E, 0x73, 0x33, 0x34, 0xE6, 0x68, 0x24, 0xDC, 0x2A, 0xD2, 0x1D, 0xD0, 0x1A, 0x19, 0x7C, 0x88,
    0xB1, 0xAE, 0x24, 0xE8, 0xB1, 0xD8, 0xC3, 0x62, 0x92, 0xE7, 0x78, 0x0E, 0x55, 0xA1, 0x31, 0x11,
    0xA2, 0x06, 0xF2, 0xBF,
};
const uint8_t sPasscodeEncStaticKeyLen_CRK_G4 = sizeof(sPasscodeEncStaticKey_CRK_G4);

// Passcode fingerprint key (always STATIC) derived from client root key and group master key #4.
const uint32_t sPasscodeFingerprintKeyId_CRK_G4 = WeaveKeyId::MakeAppStaticKeyId(WeaveKeyId::kClientRootKey, sAppGroupMasterKey4_KeyId);
const uint8_t sPasscodeFingerprintKey_CRK_G4[] =
{
    0x64, 0xFF, 0xF9, 0xA8, 0xBC, 0x5F, 0x49, 0xF8, 0x46, 0xAA, 0xF2, 0x94, 0xC6, 0xC1, 0x3C, 0xC3,
    0xA5, 0xD3, 0x4F, 0x1D,
};
const uint8_t sPasscodeFingerprintKeyLen_CRK_G4 = sizeof(sPasscodeFingerprintKey_CRK_G4);

/* ************************************************************** */
/*  Platform Key Store.                                           */
/* ************************************************************** */

TestGroupKeyStore::TestGroupKeyStore(void)
{
    // Initialize class member variables.
    Init();

    // Initialize key store with default key values for testing purpose.
    // --- Initialize fabric secret.
    memcpy(Keys[0].Key, sFabricSecret, sFabricSecretLen);
    Keys[0].KeyLen = sFabricSecretLen;
    Keys[0].KeyId = WeaveKeyId::kFabricSecret;

    // --- Initialize service root key.
    memcpy(Keys[1].Key, sServiceRootKey, sServiceRootKeyLen);
    Keys[1].KeyLen = sServiceRootKeyLen;
    Keys[1].KeyId = WeaveKeyId::kServiceRootKey;

    // --- Initialize epoch keys.
    for (int i = 2; i < 2 + WEAVE_CONFIG_MAX_APPLICATION_EPOCH_KEYS; i++)
    {
        switch (i)
        {
        case 2 + 0:
            memcpy(Keys[i].Key, sEpochKey0_Key, sEpochKey0_KeyLen);
            Keys[i].KeyLen = sEpochKey0_KeyLen;
            Keys[i].StartTime = sEpochKey0_StartTime;
            Keys[i].KeyId = sEpochKey0_KeyId;
            break;
        case 2 + 1:
            memcpy(Keys[i].Key, sEpochKey1_Key, sEpochKey1_KeyLen);
            Keys[i].KeyLen = sEpochKey1_KeyLen;
            Keys[i].StartTime = sEpochKey1_StartTime;
            Keys[i].KeyId = sEpochKey1_KeyId;
            break;
        case 2 + 2:
            memcpy(Keys[i].Key, sEpochKey2_Key, sEpochKey2_KeyLen);
            Keys[i].KeyLen = sEpochKey2_KeyLen;
            Keys[i].StartTime = sEpochKey2_StartTime;
            Keys[i].KeyId = sEpochKey2_KeyId;
            break;
        case 2 + 3:
            memcpy(Keys[i].Key, sEpochKey3_Key, sEpochKey3_KeyLen);
            Keys[i].KeyLen = sEpochKey3_KeyLen;
            Keys[i].StartTime = sEpochKey3_StartTime;
            Keys[i].KeyId = sEpochKey3_KeyId;
            break;
        default:
            memset(Keys[i].Key, 0, Keys[i].MaxKeySize);
            Keys[i].KeyLen = 0;
            Keys[i].StartTime = 0;
            Keys[i].KeyId = WeaveKeyId::kNone;
            break;
        }
    }

    // --- Initialize group master keys.
    for (int i = 2 + WEAVE_CONFIG_MAX_APPLICATION_EPOCH_KEYS; i < 2 + WEAVE_CONFIG_MAX_APPLICATION_EPOCH_KEYS + WEAVE_CONFIG_MAX_APPLICATION_GROUPS; i++)
    {
        switch (i)
        {
        case 2 + WEAVE_CONFIG_MAX_APPLICATION_EPOCH_KEYS + 0:
            memcpy(Keys[i].Key, sAppGroupMasterKey0_Key, sAppGroupMasterKey0_KeyLen);
            Keys[i].KeyLen = sAppGroupMasterKey0_KeyLen;
            Keys[i].GlobalId = sAppGroupMasterKey0_GlobalId;
            Keys[i].KeyId = sAppGroupMasterKey0_KeyId;
            break;
        case 2 + WEAVE_CONFIG_MAX_APPLICATION_EPOCH_KEYS + 1:
            memcpy(Keys[i].Key, sAppGroupMasterKey4_Key, sAppGroupMasterKey4_KeyLen);
            Keys[i].KeyLen = sAppGroupMasterKey4_KeyLen;
            Keys[i].GlobalId = sAppGroupMasterKey4_GlobalId;
            Keys[i].KeyId = sAppGroupMasterKey4_KeyId;
            break;
        case 2 + WEAVE_CONFIG_MAX_APPLICATION_EPOCH_KEYS + 2:
            memcpy(Keys[i].Key, sAppGroupMasterKey10_Key, sAppGroupMasterKey10_KeyLen);
            Keys[i].KeyLen = sAppGroupMasterKey10_KeyLen;
            Keys[i].GlobalId = sAppGroupMasterKey10_GlobalId;
            Keys[i].KeyId = sAppGroupMasterKey10_KeyId;
            break;
        case 2 + WEAVE_CONFIG_MAX_APPLICATION_EPOCH_KEYS + 3:
            memcpy(Keys[i].Key, sAppGroupMasterKey54_Key, sAppGroupMasterKey54_KeyLen);
            Keys[i].KeyLen = sAppGroupMasterKey54_KeyLen;
            Keys[i].GlobalId = sAppGroupMasterKey54_GlobalId;
            Keys[i].KeyId = sAppGroupMasterKey54_KeyId;
            break;
        default:
            memset(Keys[i].Key, 0, Keys[i].MaxKeySize);
            Keys[i].KeyLen = 0;
            Keys[i].GlobalId = 0;
            Keys[i].KeyId = WeaveKeyId::kNone;
            break;
        }
    }
}


WEAVE_ERROR TestGroupKeyStore::Clear(void)
{
    // Init() function clears member variables.
    Init();

    // Store cleared LastUsedEpochKeyId parameter.
    StoreLastUsedEpochKeyId();

    // Delete fabric secret.
    DeleteGroupKey(WeaveKeyId::kFabricSecret);

    // Delete service root key.
    DeleteGroupKey(WeaveKeyId::kServiceRootKey);

    // Delete all epoch keys.
    DeleteGroupKeysOfAType(WeaveKeyId::kType_AppEpochKey);

    // Delete all group master keys.
    DeleteGroupKeysOfAType(WeaveKeyId::kType_AppGroupMasterKey);

    return WEAVE_NO_ERROR;
}

WEAVE_ERROR TestGroupKeyStore::RetrieveGroupKey(uint32_t keyId, WeaveGroupKey& key)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    // Find and copy the requested key.
    for (int i = 0; i < MaxGroupKeyCount; i++)
    {
        if (Keys[i].KeyId == keyId)
        {
            memcpy(key.Key, Keys[i].Key, Keys[i].KeyLen);
            key.KeyLen = Keys[i].KeyLen;
            key.KeyId = Keys[i].KeyId;

            if (WeaveKeyId::IsAppEpochKey(keyId))
                key.StartTime = Keys[i].StartTime;
            else if (WeaveKeyId::IsAppGroupMasterKey(keyId))
                key.GlobalId = Keys[i].GlobalId;

            ExitNow();
        }
    }

    ExitNow(err = WEAVE_ERROR_KEY_NOT_FOUND);

exit:
    return err;
}

WEAVE_ERROR TestGroupKeyStore::StoreGroupKey(const WeaveGroupKey& key)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint8_t ind = MaxGroupKeyCount;

    // Verify the supported key type is specified.
    VerifyOrExit(key.KeyId == WeaveKeyId::kFabricSecret || key.KeyId == WeaveKeyId::kServiceRootKey ||
                 WeaveKeyId::IsAppEpochKey(key.KeyId) || WeaveKeyId::IsAppGroupMasterKey(key.KeyId), err = WEAVE_ERROR_INVALID_KEY_ID);

    // Find entry index for the new key storage. If key with the same key id already
    // in the store then override it, otherwise, just find empty entry.
    for (int i = 0; i < MaxGroupKeyCount; i++)
    {
        if (Keys[i].KeyId == key.KeyId)
        {
            ind = i;
            break;
        }
        if (ind == MaxGroupKeyCount && Keys[i].KeyId == WeaveKeyId::kNone)
        {
            ind = i;
        }
    }

    // Verify that the key entry was found.
    VerifyOrExit(ind < MaxGroupKeyCount, err = WEAVE_ERROR_TOO_MANY_KEYS);

    // Set the key.
    memcpy(Keys[ind].Key, key.Key, key.KeyLen);
    Keys[ind].KeyLen = key.KeyLen;
    Keys[ind].KeyId = key.KeyId;

    if (WeaveKeyId::IsAppEpochKey(key.KeyId))
        Keys[ind].StartTime = key.StartTime;
    else if (WeaveKeyId::IsAppGroupMasterKey(key.KeyId))
        Keys[ind].GlobalId = key.GlobalId;

exit:
    return err;
}

WEAVE_ERROR TestGroupKeyStore::DeleteGroupKey(uint32_t keyId)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    // Find and delete the specified key.
    for (int i = 0; i < MaxGroupKeyCount; i++)
    {
        if (Keys[i].KeyId == keyId)
        {
            ClearSecretData(Keys[i].Key, Keys[i].KeyLen);
            Keys[i].KeyLen = 0;
            Keys[i].KeyId = WeaveKeyId::kNone;
            Keys[i].StartTime = 0;
            Keys[i].GlobalId = 0;

            ExitNow();
        }
    }

    ExitNow(err = WEAVE_ERROR_KEY_NOT_FOUND);

exit:
    return err;
}

WEAVE_ERROR TestGroupKeyStore::DeleteGroupKeysOfAType(uint32_t keyType)
{
    WEAVE_ERROR err;
    uint32_t keyIds[MaxGroupKeysOfATypeCount];
    uint8_t keyCount;

    // Enumerate all group key of the specified type.
    err = EnumerateGroupKeys(keyType, keyIds, sizeof(keyIds) / sizeof(uint32_t), keyCount);
    SuccessOrExit(err);

    // Delete one by one the group keys of the specified type.
    for (uint8_t i = 0; i < keyCount; i++)
    {
        err = DeleteGroupKey(keyIds[i]);
        SuccessOrExit(err);
    }

exit:
    return err;
}

WEAVE_ERROR TestGroupKeyStore::EnumerateGroupKeys(uint32_t keyType, uint32_t *keyIds, uint8_t keyIdsArraySize, uint8_t & keyCount)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    // Verify the supported key type is specified.
    VerifyOrExit(WeaveKeyId::IsGeneralKey(keyType) ||
                 WeaveKeyId::IsAppRootKey(keyType) ||
                 WeaveKeyId::IsAppEpochKey(keyType) ||
                 WeaveKeyId::IsAppGroupMasterKey(keyType), err = WEAVE_ERROR_INVALID_KEY_ID);

    // Initialize key count with zero.
    keyCount = 0;

    // Find all keys of the specified type.
    for (int i = 0; i < MaxGroupKeyCount; i++)
    {
        if (WeaveKeyId::GetType(Keys[i].KeyId) == keyType)
        {
            VerifyOrExit(keyIdsArraySize >= keyCount + 1, err = WEAVE_ERROR_BUFFER_TOO_SMALL);

            keyIds[keyCount] = Keys[i].KeyId;
            keyCount++;
        }
    }

exit:
    return err;
}

WEAVE_ERROR TestGroupKeyStore::RetrieveLastUsedEpochKeyId(void)
{
    LastUsedEpochKeyId = sLastUsedEpochKeyId;
    return WEAVE_NO_ERROR;
}

WEAVE_ERROR TestGroupKeyStore::StoreLastUsedEpochKeyId(void)
{
    sLastUsedEpochKeyId = LastUsedEpochKeyId;
    return WEAVE_NO_ERROR;
}

WEAVE_ERROR TestGroupKeyStore::GetCurrentUTCTime(uint32_t& utcTime)
{
    if (sCurrentUTCTime == 0x0)
    {
        return WEAVE_ERROR_TIME_NOT_SYNCED_YET;
    }
    else
    {
        utcTime = sCurrentUTCTime;
        return WEAVE_NO_ERROR;
    }
}

uint32_t TestGroupKeyStore::TestValue_LastUsedEpochKeyId(void)
{
    return LastUsedEpochKeyId;
}

uint32_t TestGroupKeyStore::TestValue_NextEpochKeyStartTime(void)
{
    return NextEpochKeyStartTime;
}
