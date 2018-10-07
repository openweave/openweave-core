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
 *      Supporting classes for acting as a client in the key export protocol.
 *
 */

#include <Weave/Core/WeaveCore.h>
#include <Weave/Support/NestCerts.h>
#include <Weave/Profiles/security/WeaveAccessToken.h>
#include <Weave/Profiles/security/WeaveSig.h>
#include "WeaveKeyExportClient.h"

// Key export client classes only available in contexts that support malloc and system time.
#if HAVE_MALLOC && HAVE_FREE && HAVE_TIME_H

#include <time.h>
#include <stdlib.h>

namespace nl {
namespace Weave {
namespace Profiles {
namespace Security {
namespace KeyExport {

/**
 * @fn bool WeaveStandAloneKeyExportClient::ProposedConfig() const
 *
 * Get the key export protocol configuration that will be proposed
 * in the next key export request.
 */

/**
 * @fn void WeaveStandAloneKeyExportClient::ProposedConfig(uint8_t val)
 *
 * Set the key export protocol configuration that will be proposed in
 * the next key export request.
 */

/**
 * @fn bool WeaveStandAloneKeyExportClient::AllowNestDevelopmentDevices() const
 *
 * Get the current value of a flag indicating whether devices with Nest development
 * certificates will be trusted to respond to key export requests.
 */

/**
 * @fn void WeaveStandAloneKeyExportClient::AllowNestDevelopmentDevices(bool val)
 *
 * Set a flag indicating whether devices with Nest development certificates should
 * be trusted to respond to key export requests.
 */

/**
 * @fn bool WeaveStandAloneKeyExportClient::AllowSHA1DeviceCerts() const
 *
 * Get the current value of a flag indicating whether devices with SHA-1 signed
 * certificates will be trusted to respond to key export requests.
 */

/**
 * @fn void WeaveStandAloneKeyExportClient::AllowSHA1DeviceCerts(bool val)
 *
 * Set a flag indicating whether devices with SHA-1 signed certificates should be
 * trusted to respond to key export requests.
 */

/**
 * Initialize the WeaveStandAloneKeyExportClient object.
 */
void WeaveStandAloneKeyExportClient::Init(void)
{
    mKeyExportObj.Init(this, NULL);
    Reset();
    mAllowNestDevDevices = false;
    mAllowSHA1DeviceCerts = false;
}

/**
 * Reset the state of WeaveStandAloneKeyExportClient object.
 */
void WeaveStandAloneKeyExportClient::Reset(void)
{
    mKeyExportObj.Reset();
    mKeyExportObj.SetAllowedConfigs(kKeyExportSupportedConfig_All);
    mProposedConfig = kKeyExportConfig_Config2;
    mKeyId = WeaveKeyId::kNone;
    mResponderNodeId = kNodeIdNotSpecified;
    mClientCert = NULL;
    mClientCertLen = 0;
    mClientKey = NULL;
    mClientKeyLen = 0;
    mAccessToken = NULL;
    mAccessTokenLen = 0;
}

/**
 * Generate a key export request given a client certificate and private key.
 *
 * @param keyId                         The Weave key id of the key to be exported.
 * @param responderNodeId               The Weave node id of the device to which the request will be forwarded; or
 *                                      kNodeIdNotSpecified (0) if the particular device id is unknown.
 * @param clientCert                    A buffer containing a Weave certificate identifying the client making the request.
 *                                      The certificate is expected to be encoded in Weave TLV format.
 * @param clientCertLen                 The length of the client certificate pointed at by clientCert.
 * @param clientKey                     A buffer containing the private key associated with the client certificate.
 *                                      The private key is expected to be encoded in Weave TLV format.
 * @param clientKeyLen                  The length of the encoded private key pointed at by clientKey.
 * @param reqBuf                        A buffer into which the generated key export request should be written.
 * @param reqBufSize                    The size of the buffer (in bytes) pointed at by reqBuf.
 * @param reqLen                        A reference to an integer that will be set to the length of the generated request.
 *                                      Note that is value is only set when the method succeeds.
 *
 * @retval #WEAVE_NO_ERROR              If the method succeeded.
 * @retval #WEAVE_ERROR_INCORRECT_STATE If another key export operation is already in progress or complete.
 * @retval #WEAVE_ERROR_BUFFER_TOO_SMALL
 *                                      If the supplied buffer is too small to hold the generated request.
 * @retval other                        Other Weave or platform error codes.
 */
WEAVE_ERROR WeaveStandAloneKeyExportClient::GenerateKeyExportRequest(uint32_t keyId, uint64_t responderNodeId, const uint8_t* clientCert,
        uint16_t clientCertLen, const uint8_t* clientKey, uint16_t clientKeyLen, uint8_t* reqBuf, uint16_t reqBufSize, uint16_t& reqLen)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    // Verify there isn't an export already in progress.
    VerifyOrExit(mKeyExportObj.State() == WeaveKeyExport::kState_Reset ||
                 mKeyExportObj.State() == WeaveKeyExport::kState_InitiatorReconfigureProcessed,
                 err = WEAVE_ERROR_INCORRECT_STATE);

    // Save supplied information for building request.
    mKeyId = keyId;
    mResponderNodeId = responderNodeId;
    mClientCert = clientCert;
    mClientCertLen = clientCertLen;
    mClientKey = clientKey;
    mClientKeyLen = clientKeyLen;

    // Call the key export object to generate a key export request.
    err = mKeyExportObj.GenerateKeyExportRequest(reqBuf, reqBufSize, reqLen, mProposedConfig, keyId, true);
    SuccessOrExit(err);

    // These values are no longer needed, so clear them to prevent inadvertent use.
    mClientCert = NULL;
    mClientCertLen = 0;
    mClientKey = NULL;
    mClientKeyLen = 0;

exit:
    return err;
}

/**
 * Generate a key export request given an access token.
 *
 * @param keyId                         The Weave key id of the key to be exported.
 * @param responderNodeId               The Weave node id of the device to which the request will be forwarded; or
 *                                      kNodeIdNotSpecified (0) if the particular device id is unknown.
 * @param accessToken                   A buffer containing a Weave access token identifying the client making the request.
 *                                      The access token is expected to be encoded in Weave TLV format.
 * @param accessTokenLen                The length of the access token pointed at by accessToken.
 * @param reqBuf                        A buffer into which the generated key export request should be written.
 * @param reqBufSize                    The size of the buffer (in bytes) pointed at by reqBuf.
 * @param reqLen                        A reference to an integer that will be set to the length of the generated request.
 *                                      Note that is value is only set when the method succeeds.
 *
 * @retval #WEAVE_NO_ERROR              If the method succeeded.
 * @retval #WEAVE_ERROR_INCORRECT_STATE If another key export operation is already in progress or complete.
 * @retval #WEAVE_ERROR_BUFFER_TOO_SMALL
 *                                      If the supplied buffer is too small to hold the generated request.
 * @retval other                        Other Weave or platform error codes.
 */
WEAVE_ERROR WeaveStandAloneKeyExportClient::GenerateKeyExportRequest(uint32_t keyId, uint64_t responderNodeId,
        const uint8_t *accessToken, uint16_t accessTokenLen,
        uint8_t *reqBuf, uint16_t reqBufSize, uint16_t& reqLen)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    // Verify there isn't an export already in progress.
    VerifyOrExit(mKeyExportObj.State() == WeaveKeyExport::kState_Reset ||
                 mKeyExportObj.State() == WeaveKeyExport::kState_InitiatorReconfigureProcessed,
                 err = WEAVE_ERROR_INCORRECT_STATE);

    // Save supplied information for building request.
    mKeyId = keyId;
    mResponderNodeId = responderNodeId;
    mAccessToken = accessToken;
    mAccessTokenLen = accessTokenLen;

    // Call the key export object to generate a key export request.
    err = mKeyExportObj.GenerateKeyExportRequest(reqBuf, reqBufSize, reqLen, mProposedConfig, keyId, true);
    SuccessOrExit(err);

    // These values are no longer needed, so clear them to prevent inadvertent use.
    mAccessToken = NULL;
    mAccessTokenLen = 0;

exit:
    return err;
}

/**
 * Process the response to a previously-generated key export request.
 *
 * @param resp                          A buffer containing the key export response to be processed.
 * @param respLen                       The length of the key export response, in bytes.
 * @param responderNodeId               The Weave node id of the device from which the response was received; or
 *                                      kNodeIdNotSpecified (0) if the particular device id is unknown.
 * @param exportedKeyBuf                A buffer into which the exported key data should be written.
 * @param exportedKeyBufSize            The size (in bytes) of the buffer pointed at by exportedKeyBuf.
 * @param exportedKeyLen                A reference to an integer that will be set to the length of the exported
 *                                      key. Note that this field will only be set when the method succeeds.
 * @param exportedKeyId                 A reference to an integer that will be set to the Weave key id of
 *                                      the exported key. Note that this field will only be set when the method
 *                                      succeeds.
 *
 * @retval #WEAVE_NO_ERROR              If the method succeeded.
 * @retval #WEAVE_ERROR_INCORRECT_STATE If a key export operation is not in progress.
 * @retval #WEAVE_ERROR_BUFFER_TOO_SMALL
 *                                      If the supplied buffer is too small to hold the exported key.
 * @retval other                        Other Weave or platform error codes.
 */
WEAVE_ERROR WeaveStandAloneKeyExportClient::ProcessKeyExportResponse(const uint8_t *resp, uint16_t respLen, uint64_t responderNodeId,
        uint8_t *exportedKeyBuf, uint16_t exportedKeyBufSize, uint16_t &exportedKeyLen, uint32_t &exportedKeyId)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    // Verify there's a key export already in progress.
    VerifyOrExit(mKeyExportObj.State() == WeaveKeyExport::kState_InitiatorRequestGenerated, err = WEAVE_ERROR_INCORRECT_STATE);

    // If provided, verify the responding node id matches the expected value.
    VerifyOrExit(mResponderNodeId == kAnyNodeId || responderNodeId == mResponderNodeId, err = WEAVE_ERROR_WRONG_NODE_ID);

    // Call the key export object to process the key export response.
    err = mKeyExportObj.ProcessKeyExportResponse(resp, respLen, NULL, exportedKeyBuf, exportedKeyBufSize, exportedKeyLen, exportedKeyId);
    SuccessOrExit(err);

exit:
    return err;
}

/**
 * Process a reconfigure message received in response to a previously-generated key export request.
 *
 * @param reconfig                      A buffer containing a Weave key export reconfigure message, as returned
 *                                      by the device.
 * @param reconfigLen                   The length of the reconfigure message pointed at by reconfig.
 *
 * @retval #WEAVE_NO_ERROR              If the method succeeded.
 * @retval #WEAVE_ERROR_INCORRECT_STATE If a key export operation is not in progress.
 * @retval other                        Other Weave or platform error codes.
 */
WEAVE_ERROR WeaveStandAloneKeyExportClient::ProcessKeyExportReconfigure(const uint8_t *reconfig, uint16_t reconfigLen)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    // Verify there's a key export already in progress.
    VerifyOrExit(mKeyExportObj.State() == WeaveKeyExport::kState_InitiatorRequestGenerated, err = WEAVE_ERROR_INCORRECT_STATE);

    // Call the key export object to process the reconfigure message.
    err = mKeyExportObj.ProcessKeyExportReconfigure(reconfig, reconfigLen, mProposedConfig);
    SuccessOrExit(err);

exit:
    return err;
}

#if !WEAVE_CONFIG_LEGACY_KEY_EXPORT_DELEGATE

WEAVE_ERROR WeaveStandAloneKeyExportClient::GetNodeCertSet(WeaveKeyExport * keyExport, WeaveCertificateSet & certSet)
{
    return GetNodeCertSet(keyExport->IsInitiator(), certSet);
}

WEAVE_ERROR WeaveStandAloneKeyExportClient::ReleaseNodeCertSet(WeaveKeyExport * keyExport, WeaveCertificateSet & certSet)
{
    return ReleaseNodeCertSet(keyExport->IsInitiator(), certSet);
}

WEAVE_ERROR WeaveStandAloneKeyExportClient::GenerateNodeSignature(WeaveKeyExport * keyExport, const uint8_t * msgHash, uint8_t msgHashLen,
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

WEAVE_ERROR WeaveStandAloneKeyExportClient::BeginCertValidation(WeaveKeyExport * keyExport, ValidationContext & validCtx,
        WeaveCertificateSet & certSet)
{
    return BeginCertValidation(keyExport->IsInitiator(), certSet, validCtx);
}

WEAVE_ERROR WeaveStandAloneKeyExportClient::HandleCertValidationResult(WeaveKeyExport * keyExport, ValidationContext & validCtx,
        WeaveCertificateSet & certSet, uint32_t requestedKeyId)
{
    return HandleCertValidationResult(keyExport->IsInitiator(), certSet, validCtx, NULL, keyExport->MessageInfo(), requestedKeyId);
}

WEAVE_ERROR WeaveStandAloneKeyExportClient::EndCertValidation(WeaveKeyExport * keyExport, ValidationContext & validCtx,
        WeaveCertificateSet & certSet)
{
    return EndCertValidation(keyExport->IsInitiator(), certSet, validCtx);
}

WEAVE_ERROR WeaveStandAloneKeyExportClient::ValidateUnsignedKeyExportMessage(WeaveKeyExport * keyExport, uint32_t requestedKeyId)
{
    // Responder is expected to always sign response.
    return WEAVE_ERROR_UNAUTHORIZED_KEY_EXPORT_RESPONSE;
}

#endif // !WEAVE_CONFIG_LEGACY_KEY_EXPORT_DELEGATE

WEAVE_ERROR WeaveStandAloneKeyExportClient::GetNodeCertSet(bool isInitiator, WeaveCertificateSet& certSet)
{
    WEAVE_ERROR err;
    WeaveCertificateData *cert;
    bool certSetInitialized = false;

    enum
    {
        kMaxCerts = 10,                 // Max number of certificates offered by the client (1 client cert + 9 related certs).
        kCertDecodeBufferSize = 4096,   // Maximum DER encoded size of any of the client certificates.
    };

    VerifyOrExit(isInitiator, err = WEAVE_ERROR_INVALID_ARGUMENT);

    // Initialize the certificate set.
    err = certSet.Init(kMaxCerts, kCertDecodeBufferSize);
    SuccessOrExit(err);
    certSetInitialized = true;

    // Load the client's certificate(s) into the certificate set...
    if (mClientCert != NULL && mClientCertLen != 0)
    {
        err = certSet.LoadCert(mClientCert, mClientCertLen, 0, cert);
        SuccessOrExit(err);
    }
    else if (mAccessToken != NULL && mAccessTokenLen != 0)
    {
        const uint16_t decodeFlags = 0;
        err = LoadAccessTokenCerts(mAccessToken, mAccessTokenLen, certSet, decodeFlags, cert);
        SuccessOrExit(err);
    }
    else
    {
        ExitNow(err = WEAVE_ERROR_INVALID_ARGUMENT);
    }

exit:
    if (err != WEAVE_NO_ERROR && certSetInitialized)
    {
        certSet.Release();
    }

    return err;
}

WEAVE_ERROR WeaveStandAloneKeyExportClient::ReleaseNodeCertSet(bool isInitiator, WeaveCertificateSet& certSet)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    VerifyOrExit(isInitiator, err = WEAVE_ERROR_INVALID_ARGUMENT);

    certSet.Release();

exit:
    return err;
}

WEAVE_ERROR WeaveStandAloneKeyExportClient::GetNodePrivateKey(bool isInitiator, const uint8_t*& weavePrivKey, uint16_t& weavePrivKeyLen)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint8_t *privKeyBuf = NULL;

    VerifyOrExit(isInitiator, err = WEAVE_ERROR_INVALID_ARGUMENT);

    // Return the client's private key.
    if (mClientKey != NULL && mClientKeyLen != 0)
    {
        weavePrivKey = mClientKey;
        weavePrivKeyLen = mClientKeyLen;
    }
    else if (mAccessToken != NULL && mAccessTokenLen != 0)
    {
        // Allocate a buffer to hold the private key.  Since the key is held within the access token, a
        // buffer as big as the access token will always be sufficient.
        privKeyBuf = (uint8_t *)malloc(mAccessTokenLen);
        VerifyOrExit(privKeyBuf != NULL, err = WEAVE_ERROR_NO_MEMORY);

        // Extract the private key from the access token, converting the encoding to a EllipticCurvePrivateKey TLV object.
        err = ExtractPrivateKeyFromAccessToken(mAccessToken, mAccessTokenLen, privKeyBuf, mAccessTokenLen, weavePrivKeyLen);
        SuccessOrExit(err);

        // Pass the extracted key back to the caller.
        weavePrivKey = privKeyBuf;
        privKeyBuf = NULL;
    }
    else
    {
        ExitNow(err = WEAVE_ERROR_INVALID_ARGUMENT);
    }

exit:
    if (privKeyBuf != NULL)
        free(privKeyBuf);
    return err;
}

WEAVE_ERROR WeaveStandAloneKeyExportClient::ReleaseNodePrivateKey(bool isInitiator, const uint8_t *& weavePrivKey)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    VerifyOrExit(isInitiator, err = WEAVE_ERROR_INVALID_ARGUMENT);

    if (mClientKey != NULL && mClientKeyLen != 0)
    {
        // Nothing to do.
    }
    else if (mAccessToken != NULL && mAccessTokenLen != 0)
    {
        // Free the buffer containing the extracted private key.
        free((void *)weavePrivKey);
    }
    else
    {
        ExitNow(err = WEAVE_ERROR_INVALID_ARGUMENT);
    }

exit:
    return err;
}

WEAVE_ERROR WeaveStandAloneKeyExportClient::BeginCertValidation(bool isInitiator, WeaveCertificateSet& certSet, ValidationContext& validContext)
{
    WEAVE_ERROR err;
    WeaveCertificateData *cert;
    EncodedECPublicKey pubKey;
    bool certSetInitialized = false;

    enum
    {
        kMaxCerts = 10,                 // Max number of certificates expected to be sent by peer (1 peer cert + 9 related certs).
        kCertDecodeBufferSize = 4096,   // Maximum expected DER encoded size of any of the peer's certificates.
    };

    VerifyOrExit(isInitiator, err = WEAVE_ERROR_INVALID_ARGUMENT);

    err = certSet.Init(kMaxCerts, kCertDecodeBufferSize);
    SuccessOrExit(err);
    certSetInitialized = true;

    // Initialize the validation context.
    memset(&validContext, 0, sizeof(validContext));
    validContext.EffectiveTime = SecondsSinceEpochToPackedCertTime(time(NULL));
    validContext.RequiredKeyUsages = kKeyUsageFlag_DigitalSignature;
    validContext.ValidateFlags = kValidateFlag_IgnoreNotAfter;
    if (!mAllowSHA1DeviceCerts)
        validContext.ValidateFlags |= kValidateFlag_RequireSHA256;

    // Load Nest Production Root Public Key as a trusted root.
    pubKey.ECPoint = (uint8_t *)nl::NestCerts::Production::Root::PublicKey;
    pubKey.ECPointLen = nl::NestCerts::Production::Root::PublicKeyLength;
    err = certSet.AddTrustedKey(nl::NestCerts::Production::Root::CAId,
                                nl::NestCerts::Production::Root::CurveOID,
                                pubKey,
                                nl::NestCerts::Production::Root::SubjectKeyId,
                                nl::NestCerts::Production::Root::SubjectKeyIdLength);
    SuccessOrExit(err);

    // Load the Nest Production Device CA certificate so that it is available for chain validation.
    err = certSet.LoadCert(nl::NestCerts::Production::DeviceCA::Cert,
                           nl::NestCerts::Production::DeviceCA::CertLength,
                           kDecodeFlag_GenerateTBSHash, cert);
    SuccessOrExit(err);

// we need debug in order to get key export functionality working in mobile-ios
#if DEBUG

    // If DEBUG is enabled and mTrustPreProdDevices == true, arrange to accept key export responses
    // from pre-production devices built with Nest development certificates.

    if (mAllowNestDevDevices)
    {
        // Load Nest Development Root Public Key as a trusted root.
        pubKey.ECPoint = (uint8_t *)nl::NestCerts::Development::Root::PublicKey;
        pubKey.ECPointLen = nl::NestCerts::Development::Root::PublicKeyLength;
        err = certSet.AddTrustedKey(nl::NestCerts::Development::Root::CAId,
                                    nl::NestCerts::Development::Root::CurveOID,
                                    pubKey,
                                    nl::NestCerts::Development::Root::SubjectKeyId,
                                    nl::NestCerts::Development::Root::SubjectKeyIdLength);
        SuccessOrExit(err);

        // Load the Nest Development Device CA certificate so that it is available for chain validation.
        err = certSet.LoadCert(nl::NestCerts::Development::DeviceCA::Cert,
                               nl::NestCerts::Development::DeviceCA::CertLength,
                               kDecodeFlag_GenerateTBSHash, cert);
        SuccessOrExit(err);
    }

#endif //DEBUG

exit:
    if (err != WEAVE_NO_ERROR && certSetInitialized)
    {
        certSet.Release();
    }

    return err;
}

WEAVE_ERROR WeaveStandAloneKeyExportClient::HandleCertValidationResult(bool isInitiator, WeaveCertificateSet& certSet, ValidationContext& validContext,
        const IPPacketInfo *pktInfo, const WeaveMessageInfo *msgInfo, uint32_t requestedKeyId)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    // Verify the peer supplied a device certificate.
    VerifyOrExit(validContext.SigningCert->SubjectDN.AttrOID == ASN1::kOID_AttributeType_WeaveDeviceId, err = WEAVE_ERROR_WRONG_CERT_SUBJECT);

    // If a responder node id was specified, verify the certificate subject names that node.
    if (mResponderNodeId != kNodeIdNotSpecified)
        VerifyOrExit(validContext.SigningCert->SubjectDN.AttrValue.WeaveId == mResponderNodeId, err = WEAVE_ERROR_WRONG_CERT_SUBJECT);

exit:
    return err;
}

WEAVE_ERROR WeaveStandAloneKeyExportClient::EndCertValidation(bool isInitiator, WeaveCertificateSet& certSet, ValidationContext& validContext)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    VerifyOrExit(isInitiator, err = WEAVE_ERROR_INVALID_ARGUMENT);

    certSet.Release();

exit:
    return err;
}

WEAVE_ERROR WeaveStandAloneKeyExportClient::ValidateUnsignedKeyExportMessage(bool isInitiator, const IPPacketInfo *pktInfo,
        const WeaveMessageInfo *msgInfo, uint32_t requestedKeyId)
{
    // Responder is expected to always sign response.
    return WEAVE_ERROR_UNAUTHORIZED_KEY_EXPORT_RESPONSE;
}

} // namespace KeyExport
} // namespace Security
} // namespace Profiles
} // namespace Weave
} // namespace nl

#endif // HAVE_MALLOC && HAVE_FREE && HAVE_TIME_H
