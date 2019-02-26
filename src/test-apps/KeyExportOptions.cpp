/*
 *
 *    Copyright (c) 2017 Nest Labs, Inc.
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
 *      Implementation of KeyExportConfig object, which provides an implementation of the WeaveKeyExportDelegate
 *      interface for use in test applications.
 *
 */

#include "stdio.h"

#include "ToolCommon.h"
#include <Weave/Support/CodeUtils.h>
#include <Weave/Core/WeaveTLV.h>
#include <Weave/Profiles/WeaveProfiles.h>
#include <Weave/Profiles/security/WeaveAccessToken.h>
#include <Weave/Profiles/security/WeaveSig.h>
#include <Weave/Profiles/security/WeaveSecurityDebug.h>
#include <Weave/Profiles/service-provisioning/ServiceProvisioning.h>
#include <Weave/Support/NestCerts.h>
#include <Weave/Support/ErrorStr.h>
#include <Weave/Support/ASN1.h>
#include "KeyExportOptions.h"

using namespace nl::Weave;
using namespace nl::Weave::TLV;
using namespace nl::Weave::ASN1;
using namespace nl::Weave::Profiles;
using namespace nl::Weave::Profiles::Security;
using namespace nl::Weave::Profiles::Security::KeyExport;

static uint8_t sAccessToken[] =
{
    /*
    -----BEGIN ACCESS TOKEN-----
    1QAABAAJADUBMAEITi8yS0HXOtskAgQ3AyyBEERVTU1ZLUFDQ09VTlQtSUQYJgTLqPobJgVLNU9C
    NwYsgRBEVU1NWS1BQ0NPVU5ULUlEGCQHAiYIJQBaIzAKOQQr2dtaYu+6sVMqD5ljt4owxYpBKaUZ
    TksL837axemzNfB1GG1JXYbERCUHQbTTqe/utCrWCl2d4DWDKQEYNYIpASQCBRg1hCkBNgIEAgQB
    GBg1gTACCEI8lV9GHlLbGDWAMAIIQjyVX0YeUtsYNQwwAR0AimGGYj0XstLP0m05PeQlaeCR6gVq
    dc7dReuDzzACHHS0K6RtFGW3t3GaWq9k0ohgbrOxoDHKkm/K8kMYGDUCJgElAFojMAIcuvzjT4a/
    fDgScCv5oxC/T5vz7zAPpURNQjpnajADOQQr2dtaYu+6sVMqD5ljt4owxYpBKaUZTksL837axemz
    NfB1GG1JXYbERCUHQbTTqe/utCrWCl2d4BgY
    -----END ACCESS TOKEN-----
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

static uint16_t sAccessTokenLength = sizeof(sAccessToken);

KeyExportOptions gKeyExportOptions;

// Parse a sequence of zero or more unsigned integers corresponding to a list
// of allowed KeyExport configurations.  Integer values must be separated by either
// a comma or a space.
bool ParseAllowedKeyExportConfigs(const char *strConst, uint8_t& output)
{
    bool res = true;
    char *str = strdup(strConst);
    uint32_t configNum;

    output = 0;

    for (char *p = str; p != NULL; )
    {
        char *sep = strchr(p, ',');
        if (sep == NULL)
            sep = strchr(p, ' ');

        if (sep != NULL)
            *sep = 0;

        if (!ParseInt(p, configNum))
        {
            res = false;
            break;
        }

        if (configNum == 1)
            output |= kKeyExportSupportedConfig_Config1;
        else if (configNum == 2)
            output |= kKeyExportSupportedConfig_Config2;
        else
        {
            res = false;
            break;
        }

        p = (sep != NULL) ? sep + 1 : NULL;
    }

    free(str);

    return res;
}

KeyExportOptions::KeyExportOptions()
{
    static OptionDef optionDefs[] =
    {
#if WEAVE_CONFIG_ENABLE_KEY_EXPORT_INITIATOR || WEAVE_CONFIG_ENABLE_KEY_EXPORT_RESPONDER
        { "allowed-key-export-configs", kArgumentRequired,      kToolCommonOpt_AllowedKeyExportConfigs },
        { "access-token",               kArgumentRequired,      kToolCommonOpt_AccessToken             },
#endif
        { NULL }
    };
    OptionDefs = optionDefs;

    HelpGroupName = "KEY EXPORT OPTIONS";

    OptionHelp =
#if WEAVE_CONFIG_ENABLE_KEY_EXPORT_INITIATOR || WEAVE_CONFIG_ENABLE_KEY_EXPORT_RESPONDER
        "  --allowed-key-export-configs <int>[,<int>]\n"
        "       Accept the specified set of key export configurations when either initiating or\n"
        "       responding to a key export request.\n"
        "\n"
        "  --access-token <access-token-file>\n"
        "       File containing a Weave Access Token to be used to authenticate the key\n"
        "       export request. (Must be in Weave TLV format). In not specified the default\n"
        "       test access token is used.\n"
        "\n"
#endif
      "";

    // Defaults
    AllowedKeyExportConfigs = 0; // 0 causes code to use default value provided by WeaveSecurityManager
    mAccessToken = NULL;
    mAccessTokenLength = 0;
}

bool KeyExportOptions::HandleOption(const char *progName, OptionSet *optSet, int id, const char *name, const char *arg)
{
    uint32_t len;

    switch (id)
    {
#if WEAVE_CONFIG_ENABLE_KEY_EXPORT_INITIATOR || WEAVE_CONFIG_ENABLE_KEY_EXPORT_RESPONDER
    case kToolCommonOpt_AllowedKeyExportConfigs:
        if (!ParseAllowedKeyExportConfigs(arg, AllowedKeyExportConfigs))
        {
            PrintArgError("%s: Invalid value specified for allowed KeyExport configs: %s\n", progName, arg);
            return false;
        }
        break;
    case kToolCommonOpt_AccessToken:
        mAccessToken = ReadFileArg(arg, len);
        if (mAccessToken == NULL)
            return false;
        mAccessTokenLength = len;
        break;
#endif // WEAVE_CONFIG_ENABLE_KEY_EXPORT_INITIATOR || WEAVE_CONFIG_ENABLE_KEY_EXPORT_RESPONDER

    default:
        PrintArgError("%s: INTERNAL ERROR: Unhandled option: %s\n", progName, name);
        return false;
    }

    return true;
}

enum
{
    // Max Device Private Key Size -- Size of the temporary buffer used to hold
    // a device's TLV encoded private key.
    kMaxDevicePrivateKeySize = 300,

    // Max Validation Certs -- This controls the maximum number of certificates
    // that can be involved in the validation of an image signature. It must
    // include room for the signing cert, the trust anchors and any intermediate
    // certs included in the signature object.
    kMaxCerts = 10,

    // Certificate Decode Buffer Size -- Size of the temporary buffer used to decode
    // certs. The buffer must be big enough to hold the ASN1 DER encoding of the
    // TBSCertificate portion of the largest cert involved in signature verification.
    // Note that all certificates included in the signature are decoded using this
    // buffer, even if they are ultimately not involved in verifying the image
    // signature.
    kCertDecodeBufferSize = 1024
};

#if !WEAVE_CONFIG_LEGACY_KEY_EXPORT_DELEGATE

WEAVE_ERROR KeyExportOptions::GetNodeCertSet(WeaveKeyExport * keyExport, WeaveCertificateSet & certSet)
{
    return GetNodeCertSet(keyExport->IsInitiator(), certSet);
}

WEAVE_ERROR KeyExportOptions::ReleaseNodeCertSet(WeaveKeyExport * keyExport, WeaveCertificateSet & certSet)
{
    return ReleaseNodeCertSet(keyExport->IsInitiator(), certSet);
}

WEAVE_ERROR KeyExportOptions::GenerateNodeSignature(WeaveKeyExport * keyExport, const uint8_t * msgHash, uint8_t msgHashLen,
    TLVWriter & writer)
{
    WEAVE_ERROR err;
    const uint8_t * privKey = NULL;
    uint16_t privKeyLen;

    err = GetNodePrivateKey(keyExport->IsInitiator(), privKey, privKeyLen);
    SuccessOrExit(err);

    err = GenerateAndEncodeWeaveECDSASignature(writer, TLV::ContextTag(kTag_WeaveSignature_ECDSASignatureData), msgHash, msgHashLen, privKey, privKeyLen);
    SuccessOrExit(err);

exit:
    if (privKey != NULL)
    {
        WEAVE_ERROR relErr = ReleaseNodePrivateKey(keyExport->IsInitiator(), privKey);
        err = (err == WEAVE_NO_ERROR) ? relErr : err;
    }
    return err;
}

WEAVE_ERROR KeyExportOptions::BeginCertValidation(WeaveKeyExport * keyExport, ValidationContext & validCtx,
        WeaveCertificateSet & certSet)
{
    return BeginCertValidation(keyExport->IsInitiator(), certSet, validCtx);
}

WEAVE_ERROR KeyExportOptions::HandleCertValidationResult(WeaveKeyExport * keyExport, ValidationContext & validCtx,
        WeaveCertificateSet & certSet, uint32_t requestedKeyId)
{
    return HandleCertValidationResult(keyExport->IsInitiator(), certSet, validCtx, NULL, keyExport->MessageInfo(), requestedKeyId);
}

WEAVE_ERROR KeyExportOptions::EndCertValidation(WeaveKeyExport * keyExport, ValidationContext & validCtx,
        WeaveCertificateSet & certSet)
{
    return EndCertValidation(keyExport->IsInitiator(), certSet, validCtx);
}

WEAVE_ERROR KeyExportOptions::ValidateUnsignedKeyExportMessage(WeaveKeyExport * keyExport, uint32_t requestedKeyId)
{
    // Unsigned key export messages are not supported.
    return keyExport->IsInitiator()
            ? WEAVE_ERROR_UNAUTHORIZED_KEY_EXPORT_RESPONSE
            : WEAVE_ERROR_UNAUTHORIZED_KEY_EXPORT_REQUEST;
}

#endif // !WEAVE_CONFIG_LEGACY_KEY_EXPORT_DELEGATE

// Get the key export certificate set for the local node.
// This method is responsible for initializing certificate set and loading all certificates
// that will be included in the signature of the message.
WEAVE_ERROR KeyExportOptions::GetNodeCertSet(bool isInitiator, WeaveCertificateSet& certSet)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    WeaveCertificateData *cert;
    bool certSetInitialized = false;

    err = certSet.Init(kMaxCerts, kCertDecodeBufferSize, nl::Weave::Platform::Security::MemoryAlloc, nl::Weave::Platform::Security::MemoryFree);
    SuccessOrExit(err);
    certSetInitialized = true;

    if (isInitiator)
    {
#if WEAVE_CONFIG_ENABLE_KEY_EXPORT_INITIATOR
        const uint8_t *accessToken = mAccessToken;
        uint16_t accessTokenLen = mAccessTokenLength;

        if (accessToken == NULL || accessTokenLen == 0)
        {
            accessToken = sAccessToken;
            accessTokenLen = sAccessTokenLength;
        }

        err = LoadAccessTokenCerts(accessToken, accessTokenLen, certSet, 0, cert);
        SuccessOrExit(err);

#else // !WEAVE_CONFIG_ENABLE_KEY_EXPORT_INITIATOR
        ExitNow(err = WEAVE_ERROR_INVALID_ARGUMENT);
#endif // WEAVE_CONFIG_ENABLE_KEY_EXPORT_INITIATOR
    }
    else
    {
#if WEAVE_CONFIG_ENABLE_KEY_EXPORT_RESPONDER
        // Responder uses the same device certificate that is specified for CASE.
        const uint8_t *nodeCert = gCASEOptions.NodeCert;
        uint16_t nodeCertLen = gCASEOptions.NodeCertLength;

        if (nodeCert == NULL || nodeCertLen == 0)
        {
            GetTestNodeCert(FabricState.LocalNodeId, nodeCert, nodeCertLen);
        }
        if (nodeCert == NULL || nodeCertLen == 0)
        {
            printf("ERROR: Node certificate not configured\n");
            ExitNow(err = WEAVE_ERROR_CERT_NOT_FOUND);
        }

        err = certSet.LoadCert(nodeCert, nodeCertLen, 0, cert);
        SuccessOrExit(err);

#else // !WEAVE_CONFIG_ENABLE_KEY_EXPORT_RESPONDER
        ExitNow(err = WEAVE_ERROR_INVALID_ARGUMENT);
#endif // WEAVE_CONFIG_ENABLE_KEY_EXPORT_RESPONDER
    }

exit:
    if (err != WEAVE_NO_ERROR && certSetInitialized)
        certSet.Release();

    return err;
}

// Called when the key export engine is done with the certificate set returned by GetNodeCertSet().
WEAVE_ERROR KeyExportOptions::ReleaseNodeCertSet(bool isInitiator, WeaveCertificateSet& certSet)
{
    certSet.Release();

    return WEAVE_NO_ERROR;
}

// Get the local node's private key.
WEAVE_ERROR KeyExportOptions::GetNodePrivateKey(bool isInitiator, const uint8_t *& weavePrivKey, uint16_t& weavePrivKeyLen)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint8_t *privKeyBuf = NULL;

    if (isInitiator)
    {
#if WEAVE_CONFIG_ENABLE_KEY_EXPORT_INITIATOR
        const uint8_t *accessToken = mAccessToken;
        uint16_t accessTokenLen = mAccessTokenLength;

        if (accessToken == NULL || accessTokenLen == 0)
        {
            accessToken = sAccessToken;
            accessTokenLen = sAccessTokenLength;
        }

        // Allocate a buffer to hold the private key.
        privKeyBuf = (uint8_t *)nl::Weave::Platform::Security::MemoryAlloc(kMaxDevicePrivateKeySize);
        VerifyOrExit(privKeyBuf != NULL, err = WEAVE_ERROR_NO_MEMORY);

        // Extract the private key from the access token, converting the encoding to a EllipticCurvePrivateKey TLV object.
        err = ExtractPrivateKeyFromAccessToken(accessToken, accessTokenLen, privKeyBuf, accessTokenLen, weavePrivKeyLen);
        SuccessOrExit(err);

        // Pass the extracted key back to the caller.
        weavePrivKey = privKeyBuf;
        privKeyBuf = NULL;

#else // !WEAVE_CONFIG_ENABLE_KEY_EXPORT_INITIATOR
        ExitNow(err = WEAVE_ERROR_INVALID_ARGUMENT);
#endif // WEAVE_CONFIG_ENABLE_KEY_EXPORT_INITIATOR
    }
    else
    {
#if WEAVE_CONFIG_ENABLE_KEY_EXPORT_RESPONDER
        // Responder uses the same device private key that was specified for CASE authentication.
        weavePrivKey = gCASEOptions.NodePrivateKey;
        weavePrivKeyLen = gCASEOptions.NodePrivateKeyLength;

        if (weavePrivKey == NULL || weavePrivKeyLen == 0)
        {
            GetTestNodePrivateKey(FabricState.LocalNodeId, weavePrivKey, weavePrivKeyLen);
        }
        if (weavePrivKey == NULL || weavePrivKeyLen == 0)
        {
            printf("ERROR: Node private key not configured\n");
            ExitNow(err = WEAVE_ERROR_KEY_NOT_FOUND);
        }

#else // !WEAVE_CONFIG_ENABLE_KEY_EXPORT_RESPONDER
        ExitNow(err = WEAVE_ERROR_INVALID_ARGUMENT);
#endif // WEAVE_CONFIG_ENABLE_KEY_EXPORT_RESPONDER
    }

exit:
    if (privKeyBuf != NULL)
        nl::Weave::Platform::Security::MemoryFree(privKeyBuf);

    return err;
}

// Called when the key export engine is done with the buffer returned by GetNodePrivateKey().
WEAVE_ERROR KeyExportOptions::ReleaseNodePrivateKey(bool isInitiator, const uint8_t *& weavePrivKey)
{
#if WEAVE_CONFIG_ENABLE_KEY_EXPORT_INITIATOR
    if (isInitiator && weavePrivKey != NULL)
    {
        nl::Weave::Platform::Security::MemoryFree((void *)weavePrivKey);
        weavePrivKey = NULL;
    }
#endif

    return WEAVE_NO_ERROR;
}

// Prepare the supplied certificate set and validation context for use in validating the certificate of a peer.
// This method is responsible for loading the trust anchors into the certificate set.
WEAVE_ERROR KeyExportOptions::BeginCertValidation(bool isInitiator, WeaveCertificateSet& certSet, ValidationContext& validContext)
{
    WEAVE_ERROR err;
    WeaveCertificateData *cert;
    bool certSetInitialized = false;

    err = certSet.Init(kMaxCerts, kCertDecodeBufferSize, nl::Weave::Platform::Security::MemoryAlloc, nl::Weave::Platform::Security::MemoryFree);
    SuccessOrExit(err);
    certSetInitialized = true;

    if (isInitiator)
    {
#if WEAVE_CONFIG_ENABLE_KEY_EXPORT_INITIATOR
        err = certSet.LoadCert(nl::NestCerts::Development::Root::Cert, nl::NestCerts::Development::Root::CertLength, 0, cert);
        SuccessOrExit(err);
        cert->CertFlags |= kCertFlag_IsTrusted;

        err = certSet.LoadCert(nl::NestCerts::Production::Root::Cert, nl::NestCerts::Production::Root::CertLength, 0, cert);
        SuccessOrExit(err);
        cert->CertFlags |= kCertFlag_IsTrusted;

        err = certSet.LoadCert(nl::NestCerts::Development::DeviceCA::Cert, nl::NestCerts::Development::DeviceCA::CertLength, kDecodeFlag_GenerateTBSHash, cert);
        SuccessOrExit(err);

        err = certSet.LoadCert(nl::NestCerts::Production::DeviceCA::Cert, nl::NestCerts::Production::DeviceCA::CertLength, kDecodeFlag_GenerateTBSHash, cert);
        SuccessOrExit(err);

#else // !WEAVE_CONFIG_ENABLE_KEY_EXPORT_INITIATOR
        ExitNow(err = WEAVE_ERROR_INVALID_ARGUMENT);
#endif // WEAVE_CONFIG_ENABLE_KEY_EXPORT_INITIATOR
    }
    else
    {
#if WEAVE_CONFIG_ENABLE_KEY_EXPORT_RESPONDER
        const uint8_t *accessToken = mAccessToken;
        uint16_t accessTokenLen = mAccessTokenLength;

        if (accessToken == NULL || accessTokenLen == 0)
        {
            accessToken = sAccessToken;
            accessTokenLen = sAccessTokenLength;
        }

        err = LoadAccessTokenCerts(accessToken, accessTokenLen, certSet, 0, cert);
        SuccessOrExit(err);
        cert->CertFlags |= kCertFlag_IsTrusted;

#else // !WEAVE_CONFIG_ENABLE_KEY_EXPORT_RESPONDER
        ExitNow(err = WEAVE_ERROR_INVALID_ARGUMENT);
#endif // WEAVE_CONFIG_ENABLE_KEY_EXPORT_RESPONDER
    }

    // Initialize the validation context.
    memset(&validContext, 0, sizeof(validContext));
    validContext.EffectiveTime = SecondsSinceEpochToPackedCertTime(time(NULL));
    validContext.RequiredKeyUsages = kKeyUsageFlag_DigitalSignature;
    validContext.ValidateFlags = kValidateFlag_IgnoreNotAfter;

exit:
    if (err != WEAVE_NO_ERROR && certSetInitialized)
        certSet.Release();

    return err;
}

// Called with the results of validating the peer's certificate.
// Responder verifies that requestor is authorized to export the specified key.
WEAVE_ERROR KeyExportOptions::HandleCertValidationResult(bool isInitiator, WeaveCertificateSet& certSet, ValidationContext& validContext,
                                                         const IPPacketInfo *pktInfo, const WeaveMessageInfo *msgInfo, uint32_t requestedKeyId)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    WeaveCertificateData *peerCert = validContext.SigningCert;

    if (isInitiator)
    {
#if WEAVE_CONFIG_ENABLE_KEY_EXPORT_INITIATOR
        // Verify that it is device certificate and its subject matches the responder node id.
        VerifyOrExit(peerCert->SubjectDN.AttrOID == ASN1::kOID_AttributeType_WeaveDeviceId &&
                     peerCert->SubjectDN.AttrValue.WeaveId == msgInfo->SourceNodeId, err = WEAVE_ERROR_UNAUTHORIZED_KEY_EXPORT_RESPONSE);

#else // !WEAVE_CONFIG_ENABLE_KEY_EXPORT_INITIATOR
        ExitNow(err = WEAVE_ERROR_INVALID_ARGUMENT);
#endif // WEAVE_CONFIG_ENABLE_KEY_EXPORT_INITIATOR
    }
    else
    {
#if WEAVE_CONFIG_ENABLE_KEY_EXPORT_RESPONDER
        // Verify that requested key is Client Root Key and that peer's signing certificate
        // has all the correct attributes of access token certificate:
        //   -- it is trusted.
        //   -- it is self-signed.
        //   -- it has CommonName attribute type.
        VerifyOrExit((requestedKeyId == WeaveKeyId::kClientRootKey) &&
                     (peerCert->CertFlags & kCertFlag_IsTrusted) &&
                     peerCert->IssuerDN.IsEqual(peerCert->SubjectDN) &&
                     peerCert->AuthKeyId.IsEqual(peerCert->SubjectKeyId) &&
                     peerCert->SubjectDN.AttrOID == ASN1::kOID_AttributeType_CommonName, err = WEAVE_ERROR_UNAUTHORIZED_KEY_EXPORT_REQUEST);

#else // !WEAVE_CONFIG_ENABLE_KEY_EXPORT_RESPONDER
        ExitNow(err = WEAVE_ERROR_INVALID_ARGUMENT);
#endif // WEAVE_CONFIG_ENABLE_KEY_EXPORT_RESPONDER
    }

exit:
    return err;
}

// Called when peer certificate validation is complete.
WEAVE_ERROR KeyExportOptions::EndCertValidation(bool isInitiator, WeaveCertificateSet& certSet, ValidationContext& validContext)
{
    certSet.Release();

    return WEAVE_NO_ERROR;
}

// Called by requestor and responder to verify that received message was appropriately secured when the message isn't signed.
WEAVE_ERROR KeyExportOptions::ValidateUnsignedKeyExportMessage(bool isInitiator, const IPPacketInfo *pktInfo, const WeaveMessageInfo *msgInfo, uint32_t requestedKeyId)
{
    // Unsigned key export messages are not supported.
    return isInitiator ? WEAVE_ERROR_UNAUTHORIZED_KEY_EXPORT_RESPONSE :
                         WEAVE_ERROR_UNAUTHORIZED_KEY_EXPORT_REQUEST;
}
