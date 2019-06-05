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
 *      Implementation of CertProvOptions object, which provides an implementation of the
 *      WeaveNodeOpAuthDelegate and WeaveNodeMfrAttestDelegate interfaces for use
 *      in test applications.
 *
 */

#include "stdio.h"

#include "ToolCommon.h"
#include "TestWeaveCertData.h"
#include <Weave/Support/CodeUtils.h>
#include <Weave/Core/WeaveTLV.h>
#include <Weave/Profiles/WeaveProfiles.h>
#include <Weave/Profiles/security/WeaveSig.h>
#include <Weave/Profiles/security/WeaveSecurityDebug.h>
#include <Weave/Profiles/security/WeavePrivateKey.h>
#include <Weave/Profiles/security/WeaveCertProvisioning.h>
#include <Weave/Profiles/service-provisioning/ServiceProvisioning.h>
#include <Weave/Support/TimeUtils.h>
#include <Weave/Support/NestCerts.h>
#include <Weave/Support/ErrorStr.h>
#include <Weave/Support/ASN1.h>
#include <Weave/Support/crypto/HashAlgos.h>
#include "CertProvOptions.h"

using namespace nl::Weave;
using namespace nl::Weave::TLV;
using namespace nl::Weave::ASN1;
using namespace nl::Weave::Profiles;
using namespace nl::Weave::Profiles::Security;
using namespace nl::Weave::Profiles::Security::CertProvisioning;

const uint8_t TestPairingToken[] =
{
    /*
    -----BEGIN PAIRING TOKEN-----
    1QAABAAJADUBMAEITi8yS0HXOtskAgQ3AyyBEERVTU1ZLUFDQ09VTlQtSUQYJgTLqPobJgVLNU9C
    NwYsgRBEVU1NWS1BQ0NPVU5ULUlEGCQHAiYIJQBaIzAKOQQr2dtaYu+6sVMqD5ljt4owxYpBKaUZ
    TksL837axemzNfB1GG1JXYbERCUHQbTTqe/utCrWCl2d4DWDKQEYNYIpASQCBRg1hCkBNgIEAgQB
    GBg1gTACCEI8lV9GHlLbGDWAMAIIQjyVX0YeUtsYNQwwAR0AimGGYj0XstLP0m05PeQlaeCR6gVq
    dc7dReuDzzACHHS0K6RtFGW3t3GaWq9k0ohgbrOxoDHKkm/K8kMYGDUCJgElAFojMAIcuvzjT4a/
    fDgScCv5oxC/T5vz7zAPpURNQjpnajADOQQr2dtaYu+6sVMqD5ljt4owxYpBKaUZTksL837axemz
    NfB1GG1JXYbERCUHQbTTqe/utCrWCl2d4BgY
    -----END PAIRING TOKEN-----
    */
    0xd5, 0x00, 0x00, 0x04, 0x00, 0x09, 0x00, 0x35, 0x01, 0x30, 0x01, 0x08, 0x4e, 0x2f, 0x32, 0x4b,
    0x41, 0xd7, 0x3a, 0xdb, 0x24, 0x02, 0x04, 0x37, 0x03, 0x2c, 0x81, 0x10, 0x44, 0x55, 0x4d, 0x4d,
    0x59, 0x2d, 0x41, 0x43, 0x43, 0x4f, 0x55, 0x4e, 0x54, 0x2d, 0x49, 0x44, 0x18, 0x26, 0x04, 0xcb,
    0xa8, 0xfa, 0x1b, 0x26, 0x05, 0x4b, 0x35, 0x4f, 0x42, 0x37, 0x06, 0x2c, 0x81, 0x10, 0x44, 0x55,
    0x4d, 0x4d, 0x59, 0x2d, 0x41, 0x43, 0x43, 0x4f, 0x55, 0x4e, 0x54, 0x2d, 0x49, 0x44, 0x18, 0x24,
    0x07, 0x02, 0x26, 0x08, 0x25, 0x00, 0x5a, 0x23, 0x30, 0x0a, 0x39, 0x04, 0x2b, 0xd9, 0xdb, 0x5a,
    0x62, 0xef, 0xba, 0xb1, 0x53, 0x2a, 0x0f, 0x99, 0x63, 0xb7, 0x8a, 0x30, 0xc5, 0x8a, 0x41, 0x29,
    0xa5, 0x19, 0x4e, 0x4b, 0x0b, 0xf3, 0x7e, 0xda, 0xc5, 0xe9, 0xb3, 0x35, 0xf0, 0x75, 0x18, 0x6d,
    0x49, 0x5d, 0x86, 0xc4, 0x44, 0x25, 0x07, 0x41, 0xb4, 0xd3, 0xa9, 0xef, 0xee, 0xb4, 0x2a, 0xd6,
    0x0a, 0x5d, 0x9d, 0xe0, 0x35, 0x83, 0x29, 0x01, 0x18, 0x35, 0x82, 0x29, 0x01, 0x24, 0x02, 0x05,
    0x18, 0x35, 0x84, 0x29, 0x01, 0x36, 0x02, 0x04, 0x02, 0x04, 0x01, 0x18, 0x18, 0x35, 0x81, 0x30,
    0x02, 0x08, 0x42, 0x3c, 0x95, 0x5f, 0x46, 0x1e, 0x52, 0xdb, 0x18, 0x35, 0x80, 0x30, 0x02, 0x08,
    0x42, 0x3c, 0x95, 0x5f, 0x46, 0x1e, 0x52, 0xdb, 0x18, 0x35, 0x0c, 0x30, 0x01, 0x1d, 0x00, 0x8a,
    0x61, 0x86, 0x62, 0x3d, 0x17, 0xb2, 0xd2, 0xcf, 0xd2, 0x6d, 0x39, 0x3d, 0xe4, 0x25, 0x69, 0xe0,
    0x91, 0xea, 0x05, 0x6a, 0x75, 0xce, 0xdd, 0x45, 0xeb, 0x83, 0xcf, 0x30, 0x02, 0x1c, 0x74, 0xb4,
    0x2b, 0xa4, 0x6d, 0x14, 0x65, 0xb7, 0xb7, 0x71, 0x9a, 0x5a, 0xaf, 0x64, 0xd2, 0x88, 0x60, 0x6e,
    0xb3, 0xb1, 0xa0, 0x31, 0xca, 0x92, 0x6f, 0xca, 0xf2, 0x43, 0x18, 0x18, 0x35, 0x02, 0x26, 0x01,
    0x25, 0x00, 0x5a, 0x23, 0x30, 0x02, 0x1c, 0xba, 0xfc, 0xe3, 0x4f, 0x86, 0xbf, 0x7c, 0x38, 0x12,
    0x70, 0x2b, 0xf9, 0xa3, 0x10, 0xbf, 0x4f, 0x9b, 0xf3, 0xef, 0x30, 0x0f, 0xa5, 0x44, 0x4d, 0x42,
    0x3a, 0x67, 0x6a, 0x30, 0x03, 0x39, 0x04, 0x2b, 0xd9, 0xdb, 0x5a, 0x62, 0xef, 0xba, 0xb1, 0x53,
    0x2a, 0x0f, 0x99, 0x63, 0xb7, 0x8a, 0x30, 0xc5, 0x8a, 0x41, 0x29, 0xa5, 0x19, 0x4e, 0x4b, 0x0b,
    0xf3, 0x7e, 0xda, 0xc5, 0xe9, 0xb3, 0x35, 0xf0, 0x75, 0x18, 0x6d, 0x49, 0x5d, 0x86, 0xc4, 0x44,
    0x25, 0x07, 0x41, 0xb4, 0xd3, 0xa9, 0xef, 0xee, 0xb4, 0x2a, 0xd6, 0x0a, 0x5d, 0x9d, 0xe0, 0x18,
    0x18,
};

const uint16_t TestPairingTokenLength = sizeof(TestPairingToken);

const uint8_t TestPairingInitData[] =
{
    0x6E, 0x3C, 0x71, 0x5B, 0xE0, 0x19, 0xD4, 0x35, 0x83, 0x29, 0x01, 0x18, 0x35, 0x82, 0x29, 0x01,
    0x24, 0x02, 0x05, 0x18, 0x35, 0x84, 0x29, 0x01, 0x36, 0x02, 0x04, 0x02, 0x04, 0x01,
};

const uint16_t TestPairingInitDataLength = sizeof(TestPairingInitData);

const uint32_t TestDevice1_MfrAttest_HMACKeyId = 0xCAFECAFE;

const uint8_t TestDevice1_MfrAttest_HMACMetaData[] =
{
    0x2a, 0xd6, 0x0a, 0x29, 0x01, 0x6E, 0x71, 0x29, 0x01, 0x18, 0x35
};

const uint16_t TestDevice1_MfrAttest_HMACMetaDataLength = sizeof(TestDevice1_MfrAttest_HMACMetaData);

const uint8_t TestDevice1_MfrAttest_HMACKey[] =
{
    0xd9, 0xdb, 0x5a, 0x62, 0xE0, 0x19, 0xD4, 0x35, 0x83, 0x29, 0x01, 0x18, 0x35, 0x82, 0x29, 0x01,
    0x24, 0x02, 0x05, 0x18, 0x36, 0x02, 0x04, 0x02, 0x04, 0x01, 0x29, 0x01, 0x0b, 0xf3, 0xa0, 0x31
};

const uint16_t TestDevice1_MfrAttest_HMACKeyLength = sizeof(TestDevice1_MfrAttest_HMACKey);

DeviceCredentialsStore gDeviceCredsStore;

CertProvOptions gCertProvOptions;

DeviceCredentialsStore::DeviceCredentialsStore()
{
    mDeviceId = kNodeIdNotSpecified;
    mDevicePrivateKeyLen = 0;
    mDeviceCertLen = 0;
    mDeviceICACertsLen = 0;
}

WEAVE_ERROR DeviceCredentialsStore::StoreDeviceCert(const uint8_t * cert, uint16_t certLen)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    VerifyOrExit(cert != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);
    VerifyOrExit(certLen <= sizeof(mDeviceCert), err = WEAVE_ERROR_BUFFER_TOO_SMALL);

    memcpy(mDeviceCert, cert, certLen);
    mDeviceCertLen = certLen;

exit:
    return err;
}

WEAVE_ERROR DeviceCredentialsStore::StoreDeviceICACerts(const uint8_t * certs, uint16_t certsLen)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    VerifyOrExit(certs != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);
    VerifyOrExit(certsLen <= sizeof(mDeviceICACerts), err = WEAVE_ERROR_BUFFER_TOO_SMALL);

    memcpy(mDeviceICACerts, certs, certsLen);
    mDeviceICACertsLen = certsLen;

exit:
    return err;
}

WEAVE_ERROR DeviceCredentialsStore::StoreDevicePrivateKey(const uint8_t * key, uint16_t keyLen)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    VerifyOrExit(key != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);
    VerifyOrExit(keyLen <= sizeof(mDevicePrivateKey), err = WEAVE_ERROR_BUFFER_TOO_SMALL);

    memcpy(mDevicePrivateKey, key, keyLen);
    mDevicePrivateKeyLen = keyLen;

exit:
    return err;
}

WEAVE_ERROR DeviceCredentialsStore::GetDeviceId(uint64_t & deviceId)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    VerifyOrExit(mDeviceId != kNodeIdNotSpecified, err = WEAVE_ERROR_WRONG_NODE_ID);

    deviceId = mDeviceId;

exit:
    return err;
}

WEAVE_ERROR DeviceCredentialsStore::GetDeviceCert(const uint8_t *& cert, uint16_t & certLen)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    VerifyOrExit(mDeviceCertLen > 0, err = WEAVE_ERROR_CERT_NOT_FOUND);

    cert = mDeviceCert;
    certLen = mDeviceCertLen;

exit:
    return err;
}

WEAVE_ERROR DeviceCredentialsStore::GetDeviceICACerts(const uint8_t *& cert, uint16_t & certLen)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    VerifyOrExit(mDeviceICACertsLen > 0, err = WEAVE_ERROR_CERT_NOT_FOUND);

    cert = mDeviceICACerts;
    certLen = mDeviceICACertsLen;

exit:
    return err;
}

WEAVE_ERROR DeviceCredentialsStore::GetDevicePrivateKey(const uint8_t *& key, uint16_t & keyLen)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    VerifyOrExit(mDevicePrivateKeyLen > 0, err = WEAVE_ERROR_KEY_NOT_FOUND);

    key = mDevicePrivateKey;
    keyLen = mDevicePrivateKeyLen;

exit:
    return err;
}

void DeviceCredentialsStore::ClearDeviceId(void)
{
    mDeviceId = kNodeIdNotSpecified;
}

void DeviceCredentialsStore::ClearDeviceCert(void)
{
    ClearSecretData(mDeviceCert, sizeof(mDeviceCert));
    mDeviceCertLen = 0;
}

void DeviceCredentialsStore::ClearDeviceICACerts(void)
{
    ClearSecretData(mDeviceICACerts, sizeof(mDeviceICACerts));
    mDeviceICACertsLen = 0;
}

void DeviceCredentialsStore::ClearDevicePrivateKey(void)
{
    ClearSecretData(mDevicePrivateKey, sizeof(mDevicePrivateKey));
    mDevicePrivateKeyLen = 0;
}

void DeviceCredentialsStore::ClearDeviceCredentials(void)
{
    ClearDeviceId();
    ClearDeviceCert();
    ClearDeviceICACerts();
    ClearDevicePrivateKey();
}

bool DeviceCredentialsStore::DeviceIdExists(void)
{
    return (mDeviceId != kNodeIdNotSpecified);
}

bool DeviceCredentialsStore::DeviceCertExists(void)
{
    return (mDeviceCertLen > 0);
}

bool DeviceCredentialsStore::DeviceICACertsExists(void)
{
    return (mDeviceICACertsLen > 0);
}

bool DeviceCredentialsStore::DevicePrivateKeyExists(void)
{
    return (mDevicePrivateKeyLen > 0);
}

bool DeviceCredentialsStore::DeviceCredentialsExist(void)
{
    return (DeviceIdExists() && DeviceCertExists() && DevicePrivateKeyExists());
}

static WEAVE_ERROR GenerateDeviceECDSASignature(const uint8_t *hash, uint8_t hashLen, EncodedECDSASignature& ecdsaSig)
{
    WEAVE_ERROR err;
    const uint8_t * key = NULL;
    uint16_t keyLen;
    uint32_t weaveCurveId;
    EncodedECPrivateKey devicePrivKey;
    EncodedECPublicKey devicePubKey;

    err = gDeviceCredsStore.GetDevicePrivateKey(key, keyLen);
    SuccessOrExit(err);

    err = DecodeWeaveECPrivateKey(key, keyLen, weaveCurveId, devicePubKey, devicePrivKey);
    SuccessOrExit(err);

    err = nl::Weave::Crypto::GenerateECDSASignature(WeaveCurveIdToOID(weaveCurveId),
                                                    hash, hashLen, devicePrivKey, ecdsaSig);
    SuccessOrExit(err);

exit:
    return err;
}

WEAVE_ERROR DeviceCredentialsStore::GenerateAndStoreDeviceCredentials(uint64_t deviceId)
{
    WEAVE_ERROR err;
    uint8_t weaveKey[kWeaveDevicePrivateKeyBufSize];
    uint32_t weaveKeyLen;
    uint8_t weaveCert[kWeaveDeviceCertBufSize];
    uint16_t weaveCertLen;
    uint8_t privKey[EncodedECPrivateKey::kMaxValueLength];
    uint8_t pubKey[EncodedECPublicKey::kMaxValueLength];
    EncodedECPublicKey devicePubKey;
    EncodedECPrivateKey devicePrivKey;

    // If not specified, generate random device Id.
    if (deviceId == kNodeIdNotSpecified)
    {
        err = GenerateWeaveNodeId(deviceId);
        SuccessOrExit(err);
    }

    // Store generated device Id.
    err = StoreDeviceId(deviceId);
    SuccessOrExit(err);

    devicePrivKey.PrivKey = privKey;
    devicePrivKey.PrivKeyLen = sizeof(privKey);

    devicePubKey.ECPoint = pubKey;
    devicePubKey.ECPointLen = sizeof(pubKey);

    // Generate random EC private/public key pair.
    err = GenerateECDHKey(WeaveCurveIdToOID(WEAVE_CONFIG_OPERATIONAL_DEVICE_CERT_CURVE_ID), devicePubKey, devicePrivKey);
    SuccessOrExit(err);

    // Encode Weave device EC private/public key pair into EllipticCurvePrivateKey TLV structure.
    err = EncodeWeaveECPrivateKey(WEAVE_CONFIG_OPERATIONAL_DEVICE_CERT_CURVE_ID, &devicePubKey, devicePrivKey,
                                  weaveKey, sizeof(weaveKey), weaveKeyLen);
    SuccessOrExit(err);

    // Store generated device private key.
    err = StoreDevicePrivateKey(weaveKey, weaveKeyLen);
    SuccessOrExit(err);

    // Generate self-signed operational device certificate.
    err = GenerateOperationalDeviceCert(deviceId, devicePubKey, weaveCert, sizeof(weaveCert), weaveCertLen, GenerateDeviceECDSASignature);
    SuccessOrExit(err);

    // Store generated device certificate.
    err = StoreDeviceCert(weaveCert, weaveCertLen);
    SuccessOrExit(err);

exit:
    if (err != WEAVE_NO_ERROR)
    {
        ClearDeviceCredentials();
    }
    return err;
}

WEAVE_ERROR DeviceCredentialsStore::GenerateAndReplaceCurrentDeviceCert(void)
{
    WEAVE_ERROR err;
    const uint8_t * currentCert;
    uint16_t currentCertLen;
    uint8_t cert[kWeaveDeviceCertBufSize];
    uint16_t certLen;
    uint8_t icaCert[kWeaveDeviceCertBufSize];
    uint16_t icaCertLen;
    WeaveCertificateSet certSet;
    bool certSetInitialized = false;
    WeaveCertificateData *certData = NULL;

    // Get current certificate data.
    {
        err = GetDeviceCert(currentCert, currentCertLen);
        SuccessOrExit(err);

        err = certSet.Init(1, nl::TestCerts::kTestCertBufSize);
        SuccessOrExit(err);

        certSetInitialized = true;

        // Load Weave operational device certificate.
        err = certSet.LoadCert(currentCert, currentCertLen, kDecodeFlag_GenerateTBSHash, certData);
        SuccessOrExit(err);
    }

    // Generate service assigned test device certificate.
    err = GenerateTestDeviceCert(certData->SubjectDN.AttrValue.WeaveId, certData->PublicKey.EC,
                                 cert, sizeof(cert), certLen);
    SuccessOrExit(err);

    err = StoreDeviceCert(cert, certLen);
    SuccessOrExit(err);

    // Write test intermediate CA certificate in a Weave TLV array form.
    {
        TLVWriter writer;
        TLVType containerType;

        writer.Init(icaCert, sizeof(icaCert));

        err = writer.StartContainer(ProfileTag(kWeaveProfile_Security, kTag_WeaveCertificateList), kTLVType_Array, containerType);
        SuccessOrExit(err);

        err = writer.CopyContainer(AnonymousTag, nl::NestCerts::Development::DeviceCA::Cert, nl::NestCerts::Development::DeviceCA::CertLength);
        SuccessOrExit(err);

        err = writer.EndContainer(containerType);
        SuccessOrExit(err);

        err = writer.Finalize();
        SuccessOrExit(err);

        icaCertLen = writer.GetLengthWritten();
    }

    err = StoreDeviceICACerts(icaCert, icaCertLen);
    SuccessOrExit(err);

exit:
    if (certSetInitialized)
        certSet.Release();

    return err;
}

NL_DLL_EXPORT WEAVE_ERROR GenerateTestDeviceCert(uint64_t deviceId, EncodedECPublicKey& devicePubKey,
                                                 uint8_t *cert, uint16_t certBufSize, uint16_t& certLen)
{
    return GenerateTestDeviceCert(deviceId, devicePubKey,
                                  nl::TestCerts::sTestCert_CA_Weave, nl::TestCerts::sTestCertLength_CA_Weave,
                                  nl::TestCerts::sTestCert_CA_PrivateKey_Weave, nl::TestCerts::sTestCertLength_CA_PrivateKey_Weave,
                                  cert, certBufSize, certLen);
}

NL_DLL_EXPORT WEAVE_ERROR GenerateTestDeviceCert(uint64_t deviceId, EncodedECPublicKey& devicePubKey,
                                                 const uint8_t *caCert, uint16_t caCertLen,
                                                 const uint8_t *caKey, uint16_t caKeyLen,
                                                 uint8_t *cert, uint16_t certBufSize, uint16_t& certLen)
{
    WEAVE_ERROR err;
    TLVWriter writer;
    TLVType containerType;
    TLVType containerType2;
    TLVType containerType3;
    uint8_t *certDecodeBuf = NULL;
    WeaveCertificateSet certSet;
    bool certSetInitialized = false;
    WeaveCertificateData *caCertData = NULL;
    WeaveCertificateData *certData = NULL;

    VerifyOrExit(devicePubKey.ECPoint != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);
    VerifyOrExit(caCert != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);
    VerifyOrExit(caKey != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);
    VerifyOrExit(cert != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);

    // Get CA certificate data.
    {
        err = certSet.Init(1, nl::TestCerts::kTestCertBufSize);
        SuccessOrExit(err);

        certSetInitialized = true;

        // Load Weave operational device certificate.
        err = certSet.LoadCert(caCert, caCertLen, kDecodeFlag_GenerateTBSHash, caCertData);
        SuccessOrExit(err);
    }

    writer.Init(cert, certBufSize);

    err = writer.StartContainer(ProfileTag(kWeaveProfile_Security, kTag_WeaveCertificate), kTLVType_Structure, containerType);
    SuccessOrExit(err);

    // Certificate serial number.
    {
        enum
        {
            kCertSerialNumber_Length          = 8,      // Length of the certificate serial number.
            kCertSerialNumber_FirstByteMask   = 0x7F,   // Mask applied on the first byte of the key Id value.
            kCertSerialNumber_FirstBytePrefix = 0x40,   // 4-bit Type value (0100) added at the beginning of the key Id value.
        };
        uint8_t certSerialNumber[kCertSerialNumber_Length];

        // Generate a random value to be used as the serial number.
        err = nl::Weave::Platform::Security::GetSecureRandomData(certSerialNumber, kCertSerialNumber_Length);
        SuccessOrExit(err);

        // Apply mask to avoid negative numbers.
        certSerialNumber[0] &= kCertSerialNumber_FirstByteMask;

        // Apply mask to guarantee the first byte is not zero.
        certSerialNumber[0] |= kCertSerialNumber_FirstBytePrefix;

        err = writer.PutBytes(ContextTag(kTag_SerialNumber), certSerialNumber, sizeof(certSerialNumber));
        SuccessOrExit(err);
    }

    // Weave signature algorithm.
    err = writer.Put(ContextTag(kTag_SignatureAlgorithm), static_cast<uint8_t>(kOID_SigAlgo_ECDSAWithSHA256 & ~kOIDCategory_Mask));
    SuccessOrExit(err);

    // Certificate issuer Id.
    {
        err = writer.StartContainer(ContextTag(kTag_Issuer), kTLVType_Path, containerType2);
        SuccessOrExit(err);

        err = writer.Put(ContextTag(kOID_AttributeType_WeaveDeviceId & kOID_Mask), caCertData->SubjectDN.AttrValue.WeaveId);
        SuccessOrExit(err);

        err = writer.EndContainer(containerType2);
        SuccessOrExit(err);
    }

    // Certificate validity times.
    err = writer.Put(ContextTag(kTag_NotBefore), PackedCertDateToTime(WEAVE_CONFIG_OP_DEVICE_CERT_VALID_DATE_NOT_BEFORE));
    SuccessOrExit(err);

    // Certificate validity period is 10 year.
    err = writer.Put(ContextTag(kTag_NotAfter), PackedCertDateToTime(WEAVE_CONFIG_OP_DEVICE_CERT_VALID_DATE_NOT_BEFORE + (10 * 12 * 31)));
    SuccessOrExit(err);

    // Certificate subject Id.
    {
        err = writer.StartContainer(ContextTag(kTag_Subject), kTLVType_Path, containerType2);
        SuccessOrExit(err);

        err = writer.Put(ContextTag(kOID_AttributeType_WeaveDeviceId & kOID_Mask), deviceId);
        SuccessOrExit(err);

        err = writer.EndContainer(containerType2);
        SuccessOrExit(err);
    }

    // EC public key algorithm.
    err = writer.Put(ContextTag(kTag_PublicKeyAlgorithm), static_cast<uint8_t>(kOID_PubKeyAlgo_ECPublicKey & kOID_Mask));
    SuccessOrExit(err);

    // EC public key curve Id.
    err = writer.Put(ContextTag(kTag_EllipticCurveIdentifier), static_cast<uint32_t>(WEAVE_CONFIG_OPERATIONAL_DEVICE_CERT_CURVE_ID));
    SuccessOrExit(err);

    // EC public key.
    err = writer.PutBytes(ContextTag(kTag_EllipticCurvePublicKey), devicePubKey.ECPoint, devicePubKey.ECPointLen);
    SuccessOrExit(err);

    // Certificate extension: basic constraints.
    {
        err = writer.StartContainer(ContextTag(kTag_BasicConstraints), kTLVType_Structure, containerType2);
        SuccessOrExit(err);

        // This extension is critical.
        err = writer.PutBoolean(ContextTag(kTag_BasicConstraints_Critical), true);
        SuccessOrExit(err);

        err = writer.EndContainer(containerType2);
        SuccessOrExit(err);
    }

    // Certificate extension: key usage.
    {
        err = writer.StartContainer(ContextTag(kTag_KeyUsage), kTLVType_Structure, containerType2);
        SuccessOrExit(err);

        // This extension is critical.
        err = writer.PutBoolean(ContextTag(kTag_KeyUsage_Critical), true);
        SuccessOrExit(err);

        err = writer.Put(ContextTag(kTag_KeyUsage_KeyUsage), static_cast<uint16_t>(kKeyUsageFlag_DigitalSignature | kKeyUsageFlag_KeyEncipherment));
        SuccessOrExit(err);

        err = writer.EndContainer(containerType2);
        SuccessOrExit(err);
    }

    // Certificate extension: extended key usage.
    {
        err = writer.StartContainer(ContextTag(kTag_ExtendedKeyUsage), kTLVType_Structure, containerType2);
        SuccessOrExit(err);

        // This extension is critical.
        err = writer.PutBoolean(ContextTag(kTag_ExtendedKeyUsage_Critical), true);
        SuccessOrExit(err);

        err = writer.StartContainer(ContextTag(kTag_ExtendedKeyUsage_KeyPurposes), kTLVType_Array, containerType3);
        SuccessOrExit(err);

        // Key purpose is client authentication.
        err = writer.Put(AnonymousTag, static_cast<uint8_t>(kOID_KeyPurpose_ClientAuth & kOID_Mask));
        SuccessOrExit(err);

        // Key purpose is server authentication.
        err = writer.Put(AnonymousTag, static_cast<uint8_t>(kOID_KeyPurpose_ServerAuth & kOID_Mask));
        SuccessOrExit(err);

        err = writer.EndContainer(containerType3);
        SuccessOrExit(err);

        err = writer.EndContainer(containerType2);
        SuccessOrExit(err);
    }

    // Certificate extension: subject key identifier.
    {
        /* Use "truncated" SHA-1 hash. Per RFC5280:
         *
         *     "(2) The keyIdentifier is composed of a four-bit type field with the value 0100 followed
         *     by the least significant 60 bits of the SHA-1 hash of the value of the BIT STRING
         *     subjectPublicKey (excluding the tag, length, and number of unused bits)."
         */
        enum
        {
            kCertKeyId_Length          = 8,      // Length of the certificate key identifier.
            kCertKeyId_FirstByte       = nl::Weave::Platform::Security::SHA1::kHashLength - kCertKeyId_Length,
                                                 // First byte of the SHA1 hash that used to generate certificate keyId.
            kCertKeyId_FirstByteMask   = 0x0F,   // Mask applied on the first byte of the key Id value.
            kCertKeyId_FirstBytePrefix = 0x40,   // 4-bit Type value (0100) added at the beginning of the key Id value.
        };
        nl::Weave::Platform::Security::SHA1 sha1;
        uint8_t hash[nl::Weave::Platform::Security::SHA1::kHashLength];
        uint8_t *certKeyId = &hash[kCertKeyId_FirstByte];

        sha1.Begin();
        sha1.AddData(devicePubKey.ECPoint, devicePubKey.ECPointLen);
        sha1.Finish(hash);

        certKeyId[0] &= kCertKeyId_FirstByteMask;
        certKeyId[0] |= kCertKeyId_FirstBytePrefix;

        err = writer.StartContainer(ContextTag(kTag_SubjectKeyIdentifier), kTLVType_Structure, containerType2);
        SuccessOrExit(err);

        err = writer.PutBytes(ContextTag(kTag_SubjectKeyIdentifier_KeyIdentifier), certKeyId, kCertKeyId_Length);
        SuccessOrExit(err);

        err = writer.EndContainer(containerType2);
        SuccessOrExit(err);
    }

    // Certificate extension: authority key identifier.
    {
        err = writer.StartContainer(ContextTag(kTag_AuthorityKeyIdentifier), kTLVType_Structure, containerType2);
        SuccessOrExit(err);

        err = writer.PutBytes(ContextTag(kTag_AuthorityKeyIdentifier_KeyIdentifier), caCertData->SubjectKeyId.Id, caCertData->SubjectKeyId.Len);
        SuccessOrExit(err);

        err = writer.EndContainer(containerType2);
        SuccessOrExit(err);
    }

    // Start the ECDSASignature structure.
    // Note that the ECDSASignature tag is added here but the actual certificate data (S and R values)
    // will be written later. This is needed to prevent DecodeConvertTBSCert() function from failing.
    // This function expects to read new non-hashable element after all TBS data is converted.
    err = writer.StartContainer(ContextTag(kTag_ECDSASignature), kTLVType_Structure, containerType2);
    SuccessOrExit(err);

    {
        enum
        {
            kCertDecodeBufferSize      = 1024,   // Maximum ASN1 encoded size of the operational device certificate.
        };
        TLVReader reader;
        ASN1Writer tbsWriter;
        TLVType readContainerType;

        reader.Init(cert, certBufSize);

        // Parse the beginning of the WeaveSignature structure.
        err = reader.Next(kTLVType_Structure, ProfileTag(kWeaveProfile_Security, kTag_WeaveCertificate));
        SuccessOrExit(err);

        // Enter the certificate structure.
        err = reader.EnterContainer(readContainerType);
        SuccessOrExit(err);

        // Allocate decode memory buffer.
        certDecodeBuf = static_cast<uint8_t *>(nl::Weave::Platform::Security::MemoryAlloc(kCertDecodeBufferSize));
        VerifyOrExit(certDecodeBuf != NULL, err = WEAVE_ERROR_NO_MEMORY);

        // Allocate certificate data  structure.
        certData = static_cast<WeaveCertificateData *>(nl::Weave::Platform::Security::MemoryAlloc(sizeof(WeaveCertificateData)));
        VerifyOrExit(certData != NULL, err = WEAVE_ERROR_NO_MEMORY);

        // Initialize an ASN1Writer and convert the TBS (to-be-signed) portion of
        // the certificate to ASN.1 DER encoding.
        tbsWriter.Init(certDecodeBuf, kCertDecodeBufferSize);
        err = DecodeConvertTBSCert(reader, tbsWriter, *certData);
        SuccessOrExit(err);

        // Finish writing the ASN.1 DER encoding of the TBS certificate.
        err = tbsWriter.Finalize();
        SuccessOrExit(err);

        // Generate a SHA hash of the encoded TBS certificate.
        nl::Weave::Platform::Security::SHA256 sha256;
        sha256.Begin();
        sha256.AddData(certDecodeBuf, tbsWriter.GetLengthWritten());
        sha256.Finish(certData->TBSHash);

        uint32_t caCurveId;
        EncodedECPublicKey caPubKey;
        EncodedECPrivateKey caPrivKey;

        // Decode the CA private key.
        err = DecodeWeaveECPrivateKey(caKey, caKeyLen, caCurveId, caPubKey, caPrivKey);
        SuccessOrExit(err);

        // Reuse already allocated decode buffer to hold the generated signature value.
        EncodedECDSASignature ecdsaSig;
        ecdsaSig.R = certDecodeBuf;
        ecdsaSig.RLen = EncodedECDSASignature::kMaxValueLength;
        ecdsaSig.S = certDecodeBuf + EncodedECDSASignature::kMaxValueLength;
        ecdsaSig.SLen = EncodedECDSASignature::kMaxValueLength;

        // Generate an ECDSA signature for the given message hash.
        err = nl::Weave::Crypto::GenerateECDSASignature(WeaveCurveIdToOID(caCurveId), certData->TBSHash, nl::Weave::Platform::Security::SHA256::kHashLength, caPrivKey, ecdsaSig);
        SuccessOrExit(err);

        // Write the R value.
        err = writer.PutBytes(ContextTag(kTag_ECDSASignature_r), ecdsaSig.R, ecdsaSig.RLen);
        SuccessOrExit(err);

        // Write the S value.
        err = writer.PutBytes(ContextTag(kTag_ECDSASignature_s), ecdsaSig.S, ecdsaSig.SLen);
        SuccessOrExit(err);
    }

    err = writer.EndContainer(containerType2);
    SuccessOrExit(err);

    err = writer.EndContainer(containerType);
    SuccessOrExit(err);

    err = writer.Finalize();
    SuccessOrExit(err);

    certLen = static_cast<uint16_t>(writer.GetLengthWritten());

exit:
    if (certDecodeBuf != NULL)
        nl::Weave::Platform::Security::MemoryFree(certDecodeBuf);

    if (certSetInitialized)
        certSet.Release();

    return err;
}

WEAVE_ERROR ValidateWeaveDeviceCert(WeaveCertificateSet & certSet)
{
    WEAVE_ERROR err;
    WeaveCertificateData * cert = certSet.Certs;
    bool isSelfSigned = cert->IssuerDN.IsEqual(cert->SubjectDN);
    enum { kLastSecondOfDay = nl::kSecondsPerDay - 1 };

    // Verify that the certificate of device type.
    VerifyOrExit(cert->CertType == kCertType_Device, err = WEAVE_ERROR_WRONG_CERT_TYPE);

    // Verify correct subject attribute.
    VerifyOrExit(cert->SubjectDN.AttrOID == ASN1::kOID_AttributeType_WeaveDeviceId, err = WEAVE_ERROR_WRONG_CERT_SUBJECT);

    // Verify that the key usage extension exists in the certificate and that the corresponding usages are supported.
    VerifyOrExit((cert->CertFlags & kCertFlag_ExtPresent_KeyUsage) != 0 &&
                 (cert->KeyUsageFlags == (kKeyUsageFlag_DigitalSignature | kKeyUsageFlag_KeyEncipherment)),
                 err = WEAVE_ERROR_CERT_USAGE_NOT_ALLOWED);

    // Verify the validity time of the certificate.
    {
        uint32_t effectiveTime;
        ASN1UniversalTime effectiveTimeASN1 = {
            .Year   = 2020,
            .Month  = 01,
            .Day    = 01,
            .Hour   = 00,
            .Minute = 00,
            .Second = 00
        };
        err = PackCertTime(effectiveTimeASN1, effectiveTime);
        SuccessOrExit(err);

        VerifyOrExit(effectiveTime >= PackedCertDateToTime(cert->NotBeforeDate), err = WEAVE_ERROR_CERT_NOT_VALID_YET);

        VerifyOrExit(effectiveTime <= PackedCertDateToTime(cert->NotAfterDate) + kLastSecondOfDay, err = WEAVE_ERROR_CERT_EXPIRED);
    }

    // Verify that a hash of the 'to-be-signed' portion of the certificate has been computed. We will need this to
    // verify the cert's signature below.
    VerifyOrExit((cert->CertFlags & kCertFlag_TBSHashPresent) != 0, err = WEAVE_ERROR_INVALID_ARGUMENT);

    // Verify correct public key algorithm.
    VerifyOrExit(cert->PubKeyAlgoOID == kOID_PubKeyAlgo_ECPublicKey, err = WEAVE_ERROR_INVALID_ARGUMENT);

    // Verify correct key purpose.
    VerifyOrExit(cert->KeyPurposeFlags == (kKeyPurposeFlag_ServerAuth | kKeyPurposeFlag_ClientAuth), err = WEAVE_ERROR_INVALID_ARGUMENT);

    // Verify correct EC curve.
    VerifyOrExit((cert->PubKeyCurveId == kWeaveCurveId_prime256v1) ||
                 (cert->PubKeyCurveId == kWeaveCurveId_secp224r1), err = WEAVE_ERROR_UNSUPPORTED_ELLIPTIC_CURVE);

    if (isSelfSigned)
    {
        // Verify that the certificate is self-signed.
        VerifyOrExit(cert->AuthKeyId.IsEqual(cert->SubjectKeyId), err = WEAVE_ERROR_WRONG_CERT_SUBJECT);

        // Verify the signature algorithm.
        VerifyOrExit(cert->SigAlgoOID == kOID_SigAlgo_ECDSAWithSHA256, err = WEAVE_ERROR_WRONG_CERT_SIGNATURE_ALGORITHM);

        // Verify certificate signature.
        err = VerifyECDSASignature(WeaveCurveIdToOID(cert->PubKeyCurveId),
                                   cert->TBSHash, SHA256::kHashLength,
                                   cert->Signature.EC, cert->PublicKey.EC);
        SuccessOrExit(err);
    }
    else
    {
        CertificateKeyId caKeyId;
        EncodedECPublicKey caPublicKey;
        OID caCurveOID;
        uint8_t tbsHashLen;

        if (cert->IssuerDN.AttrValue.WeaveId == nl::NestCerts::Development::DeviceCA::CAId)
        {
            caKeyId.Id = nl::NestCerts::Development::DeviceCA::SubjectKeyId;
            caKeyId.Len = static_cast<uint8_t>(nl::NestCerts::Development::DeviceCA::SubjectKeyIdLength);

            caPublicKey.ECPoint = const_cast<uint8_t *>(nl::NestCerts::Development::DeviceCA::PublicKey);
            caPublicKey.ECPointLen = static_cast<uint16_t>(nl::NestCerts::Development::DeviceCA::PublicKeyLength);

            caCurveOID = WeaveCurveIdToOID(nl::NestCerts::Development::DeviceCA::CurveOID);
        }
        else if (cert->IssuerDN.AttrValue.WeaveId == nl::TestCerts::sTestCert_CA_Id)
        {
            caKeyId.Id = nl::TestCerts::sTestCert_CA_SubjectKeyId;
            caKeyId.Len = nl::TestCerts::sTestCertLength_CA_SubjectKeyId;

            caPublicKey.ECPoint = const_cast<uint8_t *>(nl::TestCerts::sTestCert_CA_PublicKey);
            caPublicKey.ECPointLen = static_cast<uint16_t>(nl::TestCerts::sTestCertLength_CA_PublicKey);

            caCurveOID = WeaveCurveIdToOID(nl::TestCerts::sTestCert_CA_CurveId);
        }
        else
        {
            ExitNow(err = WEAVE_ERROR_WRONG_CERT_SUBJECT);
        }

        // Verify that the certificate is signed by the device CA.
        VerifyOrExit(cert->AuthKeyId.IsEqual(caKeyId), err = WEAVE_ERROR_WRONG_CERT_SUBJECT);

        // Verify the signature algorithm.
        VerifyOrExit((cert->SigAlgoOID == kOID_SigAlgo_ECDSAWithSHA256) ||
                     (cert->SigAlgoOID == kOID_SigAlgo_ECDSAWithSHA1), err = WEAVE_ERROR_WRONG_CERT_SIGNATURE_ALGORITHM);

        tbsHashLen = ((cert->SigAlgoOID == kOID_SigAlgo_ECDSAWithSHA256) ? static_cast<uint8_t>(SHA256::kHashLength) : static_cast<uint8_t>(SHA1::kHashLength));

        // Verify certificate signature.
        err = VerifyECDSASignature(caCurveOID, cert->TBSHash, tbsHashLen, cert->Signature.EC, caPublicKey);
        SuccessOrExit(err);
    }

exit:
    return err;
}

/**
 *  Handler for Certificate Provisioning Client API events.
 *
 *  @param[in]  appState    A pointer to application-defined state information associated with the client object.
 *  @param[in]  eventType   Event ID passed by the event callback.
 *  @param[in]  inParam     Reference of input event parameters passed by the event callback.
 *  @param[in]  outParam    Reference of output event parameters passed by the event callback.
 *
 */
void CertProvOptions::CertProvClientEventHandler(void * appState, WeaveCertProvEngine::EventType eventType,
                                                 const WeaveCertProvEngine::InEventParam & inParam, WeaveCertProvEngine::OutEventParam & outParam)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    WeaveCertificateSet certSet;
    bool certSetInitialized = false;
    CertProvOptions * certProvOptions = static_cast<CertProvOptions *>(appState);
    WeaveCertProvEngine * certProvEngine = inParam.Source;
    Binding *binding = certProvEngine->GetBinding();
    uint64_t peerNodeId;
    IPAddress peerAddr;
    uint16_t peerPort;
    InterfaceId peerInterfaceId;

    if (binding != NULL)
    {
        peerNodeId = binding->GetPeerNodeId();
        binding->GetPeerIPAddress(peerAddr, peerPort, peerInterfaceId);
    }

    switch (eventType)
    {
    case WeaveCertProvEngine::kEvent_PrepareAuthorizeInfo:
    {
        if (binding != NULL)
            WeaveLogDetail(SecurityManager, "WeaveCertProvEngine::kEvent_PrepareAuthorizeInfo; to node %" PRIX64 " (%s)", peerNodeId, peerAddr);
        else
            WeaveLogDetail(SecurityManager, "WeaveCertProvEngine::kEvent_PrepareAuthorizeInfo");

        if (certProvOptions->IncludeAuthorizeInfo)
        {
            TLVWriter * writer = inParam.PrepareAuthorizeInfo.Writer;

            // Pairing Token.
            err = writer->PutBytes(ContextTag(kTag_GetCertReqMsg_Authorize_PairingToken), certProvOptions->PairingToken, certProvOptions->PairingTokenLen);
            SuccessOrExit(err);

            // Pairing Initialization Data.
            err = writer->PutBytes(ContextTag(kTag_GetCertReqMsg_Authorize_PairingInitData), certProvOptions->PairingInitData, certProvOptions->PairingInitDataLen);
            SuccessOrExit(err);
        }
        break;
    }

    case WeaveCertProvEngine::kEvent_ResponseReceived:
    {
        if (inParam.ResponseReceived.ReplaceCert)
        {
            enum
            {
                kMaxCerts                         = 4,      // Maximum number of certificates in the certificate verification chain.
                kCertDecodeBufSize                = 1024,   // Size of buffer needed to hold any of the test certificates
                                                            // (in either Weave or DER form), or to decode the certificates.
            };
            WeaveCertificateData *certData;
            const uint8_t * cert = inParam.ResponseReceived.Cert;
            uint16_t certLen = inParam.ResponseReceived.CertLen;
            const uint8_t * relatedCerts = inParam.ResponseReceived.RelatedCerts;
            uint16_t relatedCertsLen = inParam.ResponseReceived.RelatedCertsLen;

            if (binding != NULL)
                WeaveLogDetail(SecurityManager, "WeaveCertProvEngine::kEvent_ResponseReceived; from node %" PRIX64 " (%s)", peerNodeId, peerAddr);
            else
                WeaveLogDetail(SecurityManager, "WeaveCertProvEngine::kEvent_ResponseReceived");

            // This certificate validation step is added for testing purposes only.
            // In reality, device doesn't have to validate certificate issued by the CA service.
            {
                err = certSet.Init(kMaxCerts, kCertDecodeBufSize);
                SuccessOrExit(err);

                certSetInitialized = true;

                err = certSet.LoadCert(cert, certLen, kDecodeFlag_GenerateTBSHash, certData);
                SuccessOrExit(err);

                if (relatedCerts != NULL)
                {
                    err = certSet.LoadCerts(relatedCerts, relatedCertsLen, kDecodeFlag_GenerateTBSHash);
                    SuccessOrExit(err);
                }

                err = ValidateWeaveDeviceCert(certSet);
                SuccessOrExit(err);
            }

            // Store service assigned operational device certificate.
            err = gDeviceCredsStore.StoreDeviceCert(cert, certLen);
            SuccessOrExit(err);

            // Store service assigned device intermediate CA certificates.
            if (relatedCerts != NULL)
            {
                err = gDeviceCredsStore.StoreDeviceICACerts(relatedCerts, relatedCertsLen);
                SuccessOrExit(err);
            }
        }
        else
        {
            if (binding != NULL)
                WeaveLogDetail(SecurityManager, "WeaveCertProvEngine::kEvent_ResponseReceived; received status report from node %" PRIX64 " (%s): "
                               "No Need to Replace Operational Device Certificate", peerNodeId, peerAddr);
            else
                WeaveLogDetail(SecurityManager, "WeaveCertProvEngine::kEvent_ResponseReceived; received status report: No Need to Replace Operational Device Certificate");
        }

        certProvEngine->AbortCertificateProvisioning();

        break;
    }

    case WeaveCertProvEngine::kEvent_CommunicationError:
    {
        if (inParam.CommunicationError.Reason == WEAVE_ERROR_STATUS_REPORT_RECEIVED)
        {
            if (binding != NULL)
                WeaveLogError(SecurityManager, "WeaveCertProvEngine::kEvent_CommunicationError; received status report from node %" PRIX64 " (%s): %s", peerNodeId, peerAddr,
                              nl::StatusReportStr(inParam.CommunicationError.RcvdStatusReport->mProfileId, inParam.CommunicationError.RcvdStatusReport->mStatusCode));
            else
                WeaveLogError(SecurityManager, "WeaveCertProvEngine::kEvent_CommunicationError; received status report: %s",
                              nl::StatusReportStr(inParam.CommunicationError.RcvdStatusReport->mProfileId, inParam.CommunicationError.RcvdStatusReport->mStatusCode));
        }
        else
        {
            if (binding != NULL)
                WeaveLogError(SecurityManager, "WeaveCertProvEngine::kEvent_CommunicationError with node %" PRIX64 " (%s): %s",
                              peerNodeId, peerAddr, ErrorStr(inParam.CommunicationError.Reason));
            else
                WeaveLogError(SecurityManager, "WeaveCertProvEngine::kEvent_CommunicationError: %s",
                              ErrorStr(inParam.CommunicationError.Reason));
        }

        certProvEngine->AbortCertificateProvisioning();

        break;
    }

    default:
        if (binding != NULL)
            WeaveLogError(SecurityManager, "WeaveCertProvEngine: unrecognized API event with node %" PRIX64 " (%s)", peerNodeId, peerAddr);
        else
            WeaveLogError(SecurityManager, "WeaveCertProvEngine: unrecognized API event");
        break;
    }

exit:
    if (eventType == WeaveCertProvEngine::kEvent_PrepareAuthorizeInfo)
        outParam.PrepareAuthorizeInfo.Error = err;
    else if (eventType == WeaveCertProvEngine::kEvent_ResponseReceived)
        outParam.ResponseReceived.Error = err;

    if (certSetInitialized)
        certSet.Release();
}

bool ParseGetCertReqType(const char *str, uint8_t& output)
{
    int reqType;

    if (!ParseInt(str, reqType))
        return false;

    switch (reqType)
    {
    case 1:
        output = WeaveCertProvEngine::kReqType_GetInitialOpDeviceCert;
        return true;
    case 2:
        output = WeaveCertProvEngine::kReqType_RotateOpDeviceCert;
        return true;
    default:
        return false;
    }
}

bool ParseMfrAttestType(const char *str, uint8_t& output)
{
    int maType;

    if (!ParseInt(str, maType))
        return false;

    switch (maType)
    {
    case 1:
        output = kMfrAttestType_WeaveCert;
        return true;
    case 2:
        output = kMfrAttestType_X509Cert;
        return true;
    case 3:
        output = kMfrAttestType_HMAC;
        return true;
    default:
        return false;
    }
}

CertProvOptions::CertProvOptions()
{
    static OptionDef optionDefs[] =
    {
        { "get-cert-req-type",      kArgumentRequired,      kToolCommonOpt_GetCertReqType        },
        { "pairing-token",          kArgumentRequired,      kToolCommonOpt_PairingToken          },
        { "send-auth-info",         kNoArgument,            kToolCommonOpt_SendAuthorizeInfo     },
        { "op-cert",                kArgumentRequired,      kToolCommonOpt_OpCert                },
        { "op-key",                 kArgumentRequired,      kToolCommonOpt_OpKey                 },
        { "op-ca-cert",             kArgumentRequired,      kToolCommonOpt_OpICACerts            },
        { "send-op-ca-cert",        kNoArgument,            kToolCommonOpt_SendOpICACerts        },
        { "ma-type",                kArgumentRequired,      kToolCommonOpt_MfrAttestType         },
        { "ma-node-id",             kArgumentRequired,      kToolCommonOpt_MfrAttestNodeId       },
        { "ma-cert",                kArgumentRequired,      kToolCommonOpt_MfrAttestCert         },
        { "ma-key",                 kArgumentRequired,      kToolCommonOpt_MfrAttestKey          },
        { "ma-ca-cert",             kArgumentRequired,      kToolCommonOpt_MfrAttestICACert1     },
        { "ma-ca-cert2",            kArgumentRequired,      kToolCommonOpt_MfrAttestICACert2     },
        { "send-ma-ca-cert",        kNoArgument,            kToolCommonOpt_SendMfrAttestICACerts },
        { NULL }
    };
    OptionDefs = optionDefs;

    HelpGroupName = "CERTIFICATE PROVISIONING OPTIONS";

    OptionHelp =
        "  --get-cert-req-type <int>\n"
        "       Get Certificate Request type. If not specified the default value is used.\n"
        "       Valid values are:\n"
        "           1 - get initial operational certificate (default).\n"
        "           2 - rotate operational certificate.\n"
        "\n"
        "  --pairing-token <pairing-token-file>\n"
        "       File containing a Weave Pairing Token to be used to authorize the certificate\n"
        "       provisioning request. If not specified the default test pairing token is used.\n"
        "\n"
        "  --send-auth-info\n"
        "       Include an authorization information in the Get Certificate Request message.\n"
        "\n"
        "  --op-cert <cert-file>\n"
        "       File containing a Weave Operational certificate to be used to authenticate the node\n"
        "       when establishing a CASE session. The file can contain either raw TLV or\n"
        "       base-64. If not specified the default test certificate is used.\n"
        "\n"
        "  --op-key <key-file>\n"
        "       File containing an Operational private key to be used to authenticate the node's\n"
        "       when establishing a CASE session. The file can contain either raw TLV or\n"
        "       base-64. If not specified the default test key is used.\n"
        "\n"
        "  --op-ca-cert <cert-file>\n"
        "       File containing a Weave Operational CA certificate to be included along with the\n"
        "       node's Operational certificate in the Get Certificat Request message. The file can contain\n"
        "       either raw TLV or base-64. If not specified the default test CA certificate is used.\n"
        "\n"
        "  --send-op-ca-cert\n"
        "       Include a Weave Operational CA certificate in the Get Certificat Request message.\n"
        "       This option is set automatically when op-ca-cert is specified.\n"
        "\n"
        "  --ma-type <int>\n"
        "       Device Manufacturer Attestation type. If not specified the default value is used.\n"
        "       Supported options are:\n"
        "           1 - Weave certificate (default).\n"
        "           2 - X509 RSA certificate.\n"
        "           3 - HMAC Attestation.\n"
        "\n"
        "  --ma-node-id <int>\n"
        "       Device Manufacturer Attestation node id. If not specified the default test device #1\n"
        "       node id is used.\n"
        "\n"
        "  --ma-cert <cert-file>\n"
        "       File containing a Weave Manufacturer Attestation certificate to be used to authenticate\n"
        "       the node's manufacturer. The file can contain either raw TLV or base-64. If not\n"
        "       specified the default test certificate is used.\n"
        "\n"
        "  --ma-key <key-file>\n"
        "       File containing a Manufacturer Attestation private key to be used to authenticate\n"
        "       the node's manufacturer. The file can contain either raw TLV orbase-64. If not\n"
        "       specified the default test key is used.\n"
        "\n"
        "  --ma-ca-cert <cert-file>\n"
        "       File containing a Weave Manufacturer Attestation CA certificate to be included along\n"
        "       with the node's Manufacturer Attestation certificate in the Get Certificat Request\n"
        "       message. The file can contain either raw TLV or base-64. If not specified the default\n"
        "       test CA certificate is used.\n"
        "\n"
        "  --ma-ca-cert2 <cert-file>\n"
        "       File containing a Weave Manufacturer Attestation second CA certificate to be included along\n"
        "       with the node's Manufacturer Attestation certificate in the Get Certificat Request\n"
        "       message. The file can contain either raw TLV or base-64. If not specified the default\n"
        "       test CA certificate is used.\n"
        "\n"
        "  --send-ma-ca-cert\n"
        "       Include a Weave Manufacturer Attestation CA certificate in the Get Certificat Request message.\n"
        "       This option is set automatically when ma-ca-cert is specified.\n"
      "";

    // Defaults
    DeviceId = kNodeIdNotSpecified;
    RequestType = WeaveCertProvEngine::kReqType_GetInitialOpDeviceCert;
    IncludeAuthorizeInfo = false;
    PairingToken = TestPairingToken;
    PairingTokenLen = TestPairingTokenLength;
    PairingInitData = TestPairingInitData;
    PairingInitDataLen = TestPairingInitDataLength;
    OperationalCert = NULL;
    OperationalCertLen = 0;
    OperationalPrivateKey = NULL;
    OperationalPrivateKeyLen = 0;
    IncludeOperationalICACerts = false;
    OperationalICACerts = NULL;
    OperationalICACertsLen = 0;
    MfrAttestType = kMfrAttestType_WeaveCert;
    MfrAttestDeviceId = TestDevice1_NodeId;
    MfrAttestCert = NULL;
    MfrAttestCertLen = 0;
    MfrAttestPrivateKey = NULL;
    MfrAttestPrivateKeyLen = 0;
    IncludeMfrAttestICACerts = false;
    MfrAttestICACert1 = NULL;
    MfrAttestICACert1Len = 0;
    MfrAttestICACert2 = NULL;
    MfrAttestICACert2Len = 0;
}

bool CertProvOptions::HandleOption(const char *progName, OptionSet *optSet, int id, const char *name, const char *arg)
{
    uint32_t len;

    switch (id)
    {
    case kToolCommonOpt_GetCertReqType:
        if (!ParseGetCertReqType(arg, RequestType))
        {
            PrintArgError("%s: Invalid value specified for GetCertificate request type: %s\n", progName, arg);
            return false;
        }
        break;

    case kToolCommonOpt_PairingToken:
        PairingToken = ReadFileArg(arg, len);
        if (PairingToken == NULL)
            return false;
        PairingTokenLen = len;
        break;

    case kToolCommonOpt_PairingInitData:
        PairingInitData = ReadFileArg(arg, len);
        if (PairingInitData == NULL)
            return false;
        PairingInitDataLen = len;
        break;

    case kToolCommonOpt_SendAuthorizeInfo:
        IncludeAuthorizeInfo = true;
        break;

    case kToolCommonOpt_OpCert:
        if (!ReadCertFile(arg, (uint8_t *&)OperationalCert, OperationalCertLen))
            return false;
        break;

    case kToolCommonOpt_OpKey:
        if (!ReadPrivateKeyFile(arg, (uint8_t *&)OperationalPrivateKey, OperationalPrivateKeyLen))
            return false;
        break;

    case kToolCommonOpt_OpICACerts:
        if (!ReadCertFile(arg, (uint8_t *&)OperationalICACerts, OperationalICACertsLen))
            return false;
        break;

   case kToolCommonOpt_SendOpICACerts:
        IncludeOperationalICACerts = true;
        break;

    case kToolCommonOpt_MfrAttestType:
        if (!ParseMfrAttestType(arg, MfrAttestType))
        {
            PrintArgError("%s: Invalid value specified for manufacturer attestation type: %s\n", progName, arg);
            return false;
        }
        break;

    case kToolCommonOpt_MfrAttestNodeId:
        if (!ParseNodeId(arg, MfrAttestDeviceId))
        {
            PrintArgError("%s: Invalid value specified for manufacturer attestation node id: %s\n", progName, arg);
            return false;
        }
        break;

    case kToolCommonOpt_MfrAttestCert:
        if (!ReadCertFile(arg, (uint8_t *&)MfrAttestCert, MfrAttestCertLen))
            return false;
        break;

    case kToolCommonOpt_MfrAttestKey:
        if (!ReadPrivateKeyFile(arg, (uint8_t *&)MfrAttestPrivateKey, MfrAttestPrivateKeyLen))
            return false;
        break;

    case kToolCommonOpt_MfrAttestICACert1:
        if (!ReadCertFile(arg, (uint8_t *&)MfrAttestICACert1, MfrAttestICACert1Len))
            return false;
        break;

    case kToolCommonOpt_MfrAttestICACert2:
        if (!ReadCertFile(arg, (uint8_t *&)MfrAttestICACert2, MfrAttestICACert2Len))
            return false;
        break;

    case kToolCommonOpt_SendMfrAttestICACerts:
        IncludeMfrAttestICACerts = true;
        break;

    default:
        PrintArgError("%s: INTERNAL ERROR: Unhandled option: %s\n", progName, name);
        return false;
    }

    return true;
}

// ===== Methods that implement the WeaveNodeOpAuthDelegate interface

WEAVE_ERROR CertProvOptions::EncodeOpCert(TLVWriter & writer, uint64_t tag)
{
    WEAVE_ERROR err;
    const uint8_t * cert = OperationalCert;
    uint16_t certLen = OperationalCertLen;

    if (cert == NULL || certLen == 0)
    {
        err = gDeviceCredsStore.GetDeviceCert(cert, certLen);
        SuccessOrExit(err);
    }

    err = writer.CopyContainer(tag, cert, certLen);
    SuccessOrExit(err);

exit:
    return err;
}

WEAVE_ERROR CertProvOptions::EncodeOpRelatedCerts(TLVWriter & writer, uint64_t tag)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    TLVType containerType;
    const uint8_t * cert = OperationalICACerts;
    uint16_t certLen = OperationalICACertsLen;

    if (IncludeOperationalICACerts)
    {
        if (cert == NULL || certLen == 0)
        {
            err = gDeviceCredsStore.GetDeviceICACerts(cert, certLen);
            SuccessOrExit(err);
        }

        err = writer.CopyContainer(tag, cert, certLen);
        SuccessOrExit(err);
    }

exit:
    return err;
}

WEAVE_ERROR CertProvOptions::GenerateAndEncodeOpSig(const uint8_t * hash, uint8_t hashLen, TLVWriter & writer, uint64_t tag)
{
    WEAVE_ERROR err;
    const uint8_t * key = OperationalPrivateKey;
    uint16_t keyLen = OperationalPrivateKeyLen;

    if (key == NULL || keyLen == 0)
    {
        err = gDeviceCredsStore.GetDevicePrivateKey(key, keyLen);
        SuccessOrExit(err);
    }

    err = GenerateAndEncodeWeaveECDSASignature(writer, tag, hash, hashLen, key, keyLen);
    SuccessOrExit(err);

exit:
    return err;
}

// ===== Methods that implement the WeaveNodeMfrAttestDelegate interface

WEAVE_ERROR CertProvOptions::EncodeMAInfo(TLVWriter & writer)
{
    WEAVE_ERROR err;
    TLVType containerType;
    const uint8_t * cert = MfrAttestCert;
    uint16_t certLen = MfrAttestCertLen;
    const uint8_t * caCert = MfrAttestICACert1;
    uint16_t caCertLen = MfrAttestICACert1Len;
    const uint8_t * caCert2 = MfrAttestICACert2;
    uint16_t caCert2Len = MfrAttestICACert2Len;

    if (MfrAttestType == kMfrAttestType_WeaveCert)
    {
        if (cert == NULL || certLen == 0)
        {
            if (!GetTestNodeCert(MfrAttestDeviceId, cert, certLen))
            {
                printf("ERROR: Node manufacturer attestation certificate not configured\n");
                ExitNow(err = WEAVE_ERROR_CERT_NOT_FOUND);
            }
        }

        err = writer.CopyContainer(ContextTag(kTag_GetCertReqMsg_MfrAttest_WeaveCert), cert, certLen);
        SuccessOrExit(err);

        if (IncludeMfrAttestICACerts)
        {
            if (caCert == NULL || caCertLen == 0)
            {
                caCert = nl::NestCerts::Development::DeviceCA::Cert;
                caCertLen = nl::NestCerts::Development::DeviceCA::CertLength;
            }

            err = writer.StartContainer(ContextTag(kTag_GetCertReqMsg_MfrAttest_WeaveRelCerts), kTLVType_Array, containerType);
            SuccessOrExit(err);

            err = writer.CopyContainer(AnonymousTag, caCert, caCertLen);
            SuccessOrExit(err);

            err = writer.EndContainer(containerType);
            SuccessOrExit(err);
        }
    }
    else if (MfrAttestType == kMfrAttestType_X509Cert)
    {
        if (cert == NULL || certLen == 0)
        {
            cert  = TestDevice1_X509_RSA_Cert;
            certLen  = TestDevice1_X509_RSA_CertLength;
        }

        // Copy the test device manufacturer attestation X509 RSA certificate into supplied TLV writer.
        err = writer.PutBytes(ContextTag(kTag_GetCertReqMsg_MfrAttest_X509Cert), cert, certLen);
        SuccessOrExit(err);

        if (IncludeMfrAttestICACerts)
        {
            if (caCert == NULL || caCertLen == 0)
            {
                caCert = TestDevice1_X509_RSA_ICACert1;
                caCertLen = TestDevice1_X509_RSA_ICACert1Length;

                caCert2 = TestDevice1_X509_RSA_ICACert2;
                caCert2Len = TestDevice1_X509_RSA_ICACert2Length;
            }

            // Start the RelatedCertificates array. This contains the list of certificates the signature verifier
            // will need to verify the signature.
            err = writer.StartContainer(ContextTag(kTag_GetCertReqMsg_MfrAttest_X509RelCerts), kTLVType_Array, containerType);
            SuccessOrExit(err);

            // Copy first Intermidiate CA (ICA) certificate.
            err = writer.PutBytes(AnonymousTag, caCert, caCertLen);
            SuccessOrExit(err);

            // Copy second Intermidiate CA (ICA) certificate.
            if (caCert2 != NULL && caCert2Len > 0)
            {
                err = writer.PutBytes(AnonymousTag, caCert2, caCert2Len);
                SuccessOrExit(err);
            }

            err = writer.EndContainer(containerType);
            SuccessOrExit(err);
        }
    }
    else if (MfrAttestType == kMfrAttestType_HMAC)
    {
        err = writer.Put(ContextTag(kTag_GetCertReqMsg_MfrAttest_HMACKeyId), TestDevice1_MfrAttest_HMACKeyId);
        SuccessOrExit(err);

        if (IncludeMfrAttestICACerts)
        {
            err = writer.PutBytes(ContextTag(kTag_GetCertReqMsg_MfrAttest_HMACMetaData), TestDevice1_MfrAttest_HMACMetaData, TestDevice1_MfrAttest_HMACMetaDataLength);
            SuccessOrExit(err);
        }
    }
    else
    {
        ExitNow(err = WEAVE_ERROR_INVALID_ARGUMENT);
    }

exit:
    return err;
}

WEAVE_ERROR CertProvOptions::GenerateAndEncodeMASig(const uint8_t * data, uint16_t dataLen, TLVWriter & writer)
{
    WEAVE_ERROR err;
    const uint8_t * key = MfrAttestPrivateKey;
    uint16_t keyLen = MfrAttestPrivateKeyLen;
    nl::Weave::Platform::Security::SHA256 sha256;
    uint8_t hash[SHA256::kHashLength];

    // Calculate data hash.
    if (MfrAttestType == kMfrAttestType_WeaveCert || MfrAttestType == kMfrAttestType_X509Cert)
    {
        sha256.Begin();
        sha256.AddData(data, dataLen);
        sha256.Finish(hash);
    }

    if (MfrAttestType == kMfrAttestType_WeaveCert)
    {
        if (key == NULL || keyLen == 0)
        {
            if (!GetTestNodePrivateKey(MfrAttestDeviceId, key, keyLen))
            {
                printf("ERROR: Node manufacturer attestation private key not configured\n");
                ExitNow(err = WEAVE_ERROR_KEY_NOT_FOUND);
            }
        }

        err = writer.Put(ContextTag(kTag_GetCertReqMsg_MfrAttestSigAlgo), static_cast<uint16_t>(ASN1::kOID_SigAlgo_ECDSAWithSHA256));
        SuccessOrExit(err);

        err = GenerateAndEncodeWeaveECDSASignature(writer, ContextTag(kTag_GetCertReqMsg_MfrAttestSig_ECDSA), hash, SHA256::kHashLength, key, keyLen);
        SuccessOrExit(err);
    }
    else if (MfrAttestType == kMfrAttestType_X509Cert)
    {
#if WEAVE_WITH_OPENSSL
        if (key == NULL || keyLen == 0)
        {
            key = TestDevice1_X509_RSA_PrivateKey;
            keyLen = TestDevice1_X509_RSA_PrivateKeyLength;
        }

        err = writer.Put(ContextTag(kTag_GetCertReqMsg_MfrAttestSigAlgo), static_cast<uint16_t>(ASN1::kOID_SigAlgo_SHA256WithRSAEncryption));
        SuccessOrExit(err);

        err = nl::Weave::Crypto::GenerateAndEncodeWeaveRSASignature(ASN1::kOID_SigAlgo_SHA256WithRSAEncryption,
                                                                    writer, ContextTag(kTag_GetCertReqMsg_MfrAttestSig_RSA),
                                                                    hash, SHA256::kHashLength, key, keyLen);
        SuccessOrExit(err);
#else
        printf("ERROR: Manufacturer Attestation X509 encoded certificates not supported.\n");
        ExitNow(err = WEAVE_ERROR_NOT_IMPLEMENTED);
#endif
    }
    else if (MfrAttestType == kMfrAttestType_HMAC)
    {
        if (key == NULL || keyLen == 0)
        {
            key = TestDevice1_MfrAttest_HMACKey;
            keyLen = TestDevice1_MfrAttest_HMACKeyLength;
        }

        err = writer.Put(ContextTag(kTag_GetCertReqMsg_MfrAttestSigAlgo), static_cast<uint16_t>(ASN1::kOID_SigAlgo_HMACWithSHA256));
        SuccessOrExit(err);

        err = GenerateAndEncodeWeaveHMACSignature(ASN1::kOID_SigAlgo_HMACWithSHA256, writer, ContextTag(kTag_GetCertReqMsg_MfrAttestSig_HMAC),
                                                  data, dataLen, key, keyLen);
        SuccessOrExit(err);
    }
    else
    {
        ExitNow(err = WEAVE_ERROR_INVALID_ARGUMENT);
    }

exit:
    return err;
}
