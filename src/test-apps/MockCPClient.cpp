/*
 *
 *    Copyright (c) 2020 Google LLC.
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

#include "ToolCommon.h"
#include "MockCPClient.h"
#include "MockDDServer.h"
#include <Weave/Core/WeaveTLV.h>
#include <Weave/Profiles/common/CommonProfile.h>
#include <Weave/Profiles/security/WeaveCert.h>
#include <Weave/Profiles/security/WeavePrivateKey.h>
#include <Weave/Profiles/security/WeaveSig.h>
#include <Weave/Profiles/service-directory/ServiceDirectory.h>
#include <Weave/Profiles/service-provisioning/ServiceProvisioning.h>
#include "CASEOptions.h"

using namespace ::nl;
using namespace ::nl::Weave;
using namespace ::nl::Weave::TLV;
using namespace ::nl::Weave::Profiles;
using namespace ::nl::Weave::Profiles::Security;
using namespace ::nl::Weave::Profiles::Security::CertProvisioning;
using namespace ::nl::Weave::Profiles::ServiceProvisioning;

using namespace nl::Weave::Encoding;

extern MockCertificateProvisioningClient MockCPClient;

MockCertificateProvisioningClient::MockCertificateProvisioningClient(void)
{
    mReqType = WeaveCertProvEngine::kReqType_NotSpecified;
    mEncodeReqAuthInfo = NULL;
    mOnCertProvDone = NULL;
    mRequesterState = NULL;

    mExchangeMgr = NULL;
    mBinding = NULL;

    mDeviceId = kNodeIdNotSpecified;
    mDeviceCert = NULL;
    mDeviceCertLen = 0;
    mDeviceIntermediateCACerts = NULL;
    mDeviceIntermediateCACertsLen = 0;
    mDevicePrivateKey = NULL;
    mDevicePrivateKeyLen = 0;
}

WEAVE_ERROR MockCertificateProvisioningClient::Init(WeaveExchangeManager *exchangeMgr)
{
    WEAVE_ERROR err;

    mExchangeMgr = exchangeMgr;

    static char defaultWOCAServerAddr[64];
    strcpy(defaultWOCAServerAddr, "127.0.0.1");

#if WEAVE_CONFIG_ENABLE_TARGETED_LISTEN
    if (exchangeMgr->FabricState->ListenIPv4Addr == IPAddress::Any)
    {
        if (exchangeMgr->FabricState->ListenIPv6Addr != IPAddress::Any)
            exchangeMgr->FabricState->ListenIPv6Addr.ToString(defaultWOCAServerAddr, sizeof(defaultWOCAServerAddr));
    }
    else
        exchangeMgr->FabricState->ListenIPv4Addr.ToString(defaultWOCAServerAddr, sizeof(defaultWOCAServerAddr));
#endif

    WOCAServerEndPointId = exchangeMgr->FabricState->LocalNodeId;
    WOCAServerAddr = defaultWOCAServerAddr;
    WOCAServerUseCASE = false;

    err = GenerateAndStoreOperationalDeviceCredentials(exchangeMgr->FabricState->LocalNodeId);
    SuccessOrExit(err);

exit:
    return err;
}

WEAVE_ERROR MockCertificateProvisioningClient::Shutdown(void)
{
    mReqType = WeaveCertProvEngine::kReqType_NotSpecified;
    mEncodeReqAuthInfo = NULL;
    mOnCertProvDone = NULL;
    mRequesterState = NULL;

    mExchangeMgr = NULL;
    mBinding = NULL;

    ClearOperationalDeviceCredentials();

    return WEAVE_NO_ERROR;
}

void MockCertificateProvisioningClient::Reset(void)
{
    ClearOperationalDeviceCredentials();
}

static WEAVE_ERROR GenerateOperationalECDSASignature(const uint8_t *hash, uint8_t hashLen, EncodedECDSASignature& ecdsaSig)
{
    WEAVE_ERROR err;
    uint8_t * weavePrivKey = NULL;
    uint16_t weavePrivKeyLen = 0;
    uint32_t weaveCurveId;
    EncodedECPublicKey pubKey;
    EncodedECPrivateKey privKey;

    // Read the private key.
    err = MockCPClient.GetDevicePrivateKey(weavePrivKey, weavePrivKeyLen);
    SuccessOrExit(err);

    // Decode operational device private/public keys from private key TLV structure.
    err = Profiles::Security::DecodeWeaveECPrivateKey(weavePrivKey, weavePrivKeyLen, weaveCurveId, pubKey, privKey);
    SuccessOrExit(err);

    // Generate operational device signature.
    err = nl::Weave::Crypto::GenerateECDSASignature(Profiles::Security::WeaveCurveIdToOID(weaveCurveId),
                                                    hash, hashLen, privKey, ecdsaSig);
    SuccessOrExit(err);

exit:
    return err;
}

WEAVE_ERROR MockCertificateProvisioningClient::GenerateAndStoreOperationalDeviceCredentials(uint64_t deviceId)
{
    enum
    {
        kWeaveDeviceCertBufSize           = 300,    // Size of buffer needed to hold Weave device certificate.
        kWeaveDevicePrivateKeyBufSize     = 128,    // Size of buffer needed to hold Weave device private key.
    };

    WEAVE_ERROR err;
    uint8_t weavePrivKey[kWeaveDevicePrivateKeyBufSize];
    uint32_t weavePrivKeyLen;
    uint8_t weaveCert[kWeaveDeviceCertBufSize];
    uint16_t weaveCertLen;
    uint8_t privKeyBuf[EncodedECPrivateKey::kMaxValueLength];
    uint8_t pubKeyBuf[EncodedECPublicKey::kMaxValueLength];
    EncodedECPublicKey pubKey;
    EncodedECPrivateKey privKey;

    // If not specified, generate random device Id.
    if (deviceId == nl::Weave::kNodeIdNotSpecified)
    {
        err = GenerateWeaveNodeId(deviceId);
        SuccessOrExit(err);
    }

    // Store generated device Id.
    err = StoreDeviceId(deviceId);
    SuccessOrExit(err);

    privKey.PrivKey = privKeyBuf;
    privKey.PrivKeyLen = sizeof(privKeyBuf);

    pubKey.ECPoint = pubKeyBuf;
    pubKey.ECPointLen = sizeof(pubKeyBuf);

    // Generate random EC private/public key pair.
    err = GenerateECDHKey(Profiles::Security::WeaveCurveIdToOID(WEAVE_CONFIG_OPERATIONAL_DEVICE_CERT_CURVE_ID), pubKey, privKey);
    SuccessOrExit(err);

    // Encode Weave device EC private/public key pair into EllipticCurvePrivateKey TLV structure.
    err = EncodeWeaveECPrivateKey(WEAVE_CONFIG_OPERATIONAL_DEVICE_CERT_CURVE_ID, &pubKey, privKey,
                                  weavePrivKey, sizeof(weavePrivKey), weavePrivKeyLen);
    SuccessOrExit(err);

    // Store generated operational device private key.
    err = StoreDevicePrivateKey(weavePrivKey, weavePrivKeyLen);
    SuccessOrExit(err);

    // Generate self-signed operational device certificate.
    err = Profiles::Security::GenerateOperationalDeviceCert(deviceId, pubKey, weaveCert, sizeof(weaveCert), weaveCertLen, GenerateOperationalECDSASignature);
    SuccessOrExit(err);

    // Store generated operational device certificate.
    err = StoreDeviceCertificate(weaveCert, weaveCertLen);
    SuccessOrExit(err);

exit:
    if (err != WEAVE_NO_ERROR)
    {
        ClearOperationalDeviceCredentials();
    }
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
void MockCertificateProvisioningClient::CertProvClientEventHandler(void * appState, WeaveCertProvEngine::EventType eventType, const WeaveCertProvEngine::InEventParam & inParam, WeaveCertProvEngine::OutEventParam & outParam)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    MockCertificateProvisioningClient * client = static_cast<MockCertificateProvisioningClient *>(appState);
    WeaveCertProvEngine * certProvEngine = inParam.Source;
    uint32_t statusProfileId = 0;
    uint16_t statusCode = 0;

    switch (eventType)
    {
    case WeaveCertProvEngine::kEvent_PrepareAuthorizeInfo:
    {
        if (client->mEncodeReqAuthInfo != NULL)
        {
            printf("Preparing authorization information for the GetCertificateRequest message\n");

            err = client->mEncodeReqAuthInfo(client->mRequesterState, *inParam.PrepareAuthorizeInfo.Writer);
            SuccessOrExit(err);
        }
        break;
    }

    case WeaveCertProvEngine::kEvent_ResponseReceived:
    {
        if (inParam.ResponseReceived.ReplaceCert)
        {
            printf("Storing WOCA server issued operational device certificate\n");

            // Store service issued operational device certificate.
            err = client->StoreDeviceCertificate(inParam.ResponseReceived.Cert, inParam.ResponseReceived.CertLen);
            SuccessOrExit(err);

            if (inParam.ResponseReceived.RelatedCerts != NULL)
            {
                // Store device intermediate CA certificates related to the service issued operational device certificate.
                err = client->StoreDeviceIntermediateCACerts(inParam.ResponseReceived.RelatedCerts, inParam.ResponseReceived.RelatedCertsLen);
                SuccessOrExit(err);
            }
        }
        else
        {
            printf("WOCA server reported: no need to replace current operational device certificate\n");
        }
        break;
    }

    case WeaveCertProvEngine::kEvent_CommunicationError:
    {
        err = inParam.CommunicationError.Reason;
        if (err == WEAVE_ERROR_STATUS_REPORT_RECEIVED)
        {
            statusProfileId = inParam.CommunicationError.RcvdStatusReport->mProfileId;
            statusCode = inParam.CommunicationError.RcvdStatusReport->mStatusCode;

            printf("Received status report from the WOCA server: %s\n", nl::StatusReportStr(statusProfileId, statusCode));
        }
        else
        {
            printf("Failed to prepare/send GetCertificateRequest message: %s\n", ErrorStr(err));
        }
        break;
    }

    default:
        printf("Unrecognized certificate provisioning API event\n");
        break;
    }

exit:
    if (eventType == WeaveCertProvEngine::kEvent_PrepareAuthorizeInfo)
    {
        outParam.PrepareAuthorizeInfo.Error = err;
    }
    else
    {
        certProvEngine->AbortCertificateProvisioning();
        client->HandleCertificateProvisioningResult(err, statusProfileId, statusCode);
    }
}

// ===== Methods that implement the WeaveNodeOpAuthDelegate interface

WEAVE_ERROR MockCertificateProvisioningClient::EncodeOpCert(TLVWriter & writer, uint64_t tag)
{
    WEAVE_ERROR err;
    uint8_t * cert = NULL;
    uint16_t certLen = 0;

    // Read the operational device certificate.
    err = GetDeviceCertificate(cert, certLen);
    SuccessOrExit(err);

    // Copy encoded operational device certificate.
    err = writer.CopyContainer(tag, cert, certLen);
    SuccessOrExit(err);

exit:
    return err;
}

WEAVE_ERROR MockCertificateProvisioningClient::EncodeOpRelatedCerts(TLVWriter & writer, uint64_t tag)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint8_t * icaCerts = NULL;
    uint16_t icaCertsLen = 0;

    // Read the operational device intermediate CA certificates.
    err = GetDeviceIntermediateCACerts(icaCerts, icaCertsLen);
    if (err == WEAVE_ERROR_CA_CERT_NOT_FOUND)
    {
        // Exit without error if operational device intermediate CA certificates is not configured.
        ExitNow(err = WEAVE_NO_ERROR);
    }
    SuccessOrExit(err);

    // Copy encoded operational device intermediate CA certificates.
    err = writer.CopyContainer(tag, icaCerts, icaCertsLen);
    SuccessOrExit(err);

exit:
    return err;
}

WEAVE_ERROR MockCertificateProvisioningClient::GenerateAndEncodeOpSig(const uint8_t * hash, uint8_t hashLen, TLVWriter & writer, uint64_t tag)
{
    WEAVE_ERROR err;
    uint8_t * privKey = NULL;
    uint16_t privKeyLen = 0;

    // Read the operational device private key.
    err = GetDevicePrivateKey(privKey, privKeyLen);
    SuccessOrExit(err);

    // Generate and encode operational device signature.
    err = GenerateAndEncodeWeaveECDSASignature(writer, tag, hash, hashLen, privKey, privKeyLen);
    SuccessOrExit(err);

exit:
    return err;
}

// ===== Methods that implement the WeaveNodeMfrAttestDelegate interface

WEAVE_ERROR MockCertificateProvisioningClient::EncodeMAInfo(TLVWriter & writer)
{
    WEAVE_ERROR err;
    uint8_t * cert = NULL;
    uint16_t certLen = 0;

    // Read the manufacturer assigned device certificate.
    err = GetManufacturerDeviceCertificate(cert, certLen);
    SuccessOrExit(err);

    // Copy encoded manufacturer attestation device certificate.
    err = writer.CopyContainer(ContextTag(kTag_GetCertReqMsg_MfrAttest_WeaveCert), cert, certLen);
    SuccessOrExit(err);

    // Determine if present and the length of the manufacturer assigned device intermediate CA certificates.
    err = GetManufacturerDeviceIntermediateCACerts(cert, certLen);
    if (cert == NULL || certLen == 0)
    {
        // Exit without error if manufacturer assigned intermediate CA certificates is not configured.
        ExitNow(err = WEAVE_NO_ERROR);
    }
    SuccessOrExit(err);

    // Copy encoded manufacturer attestation device intermediate CA certificates.
    err = writer.CopyContainer(ContextTag(kTag_GetCertReqMsg_MfrAttest_WeaveRelCerts), cert, certLen);
    SuccessOrExit(err);

exit:
    return err;
}

WEAVE_ERROR MockCertificateProvisioningClient::GenerateAndEncodeMASig(const uint8_t * data, uint16_t dataLen, TLVWriter & writer)
{
    WEAVE_ERROR err;
    uint8_t * privKey = NULL;
    uint16_t privKeyLen = 0;
    nl::Weave::Platform::Security::SHA256 sha256;
    uint8_t hash[SHA256::kHashLength];

    // Read the manufacturer attestation device private key.
    err = GetManufacturerDevicePrivateKey(privKey, privKeyLen);
    SuccessOrExit(err);

    // Calculate data hash.
    sha256.Begin();
    sha256.AddData(data, dataLen);
    sha256.Finish(hash);

    // Encode manufacturer attestation device signature algorithm: ECDSAWithSHA256.
    err = writer.Put(ContextTag(kTag_GetCertReqMsg_MfrAttestSigAlgo), static_cast<uint16_t>(ASN1::kOID_SigAlgo_ECDSAWithSHA256));
    SuccessOrExit(err);

    // Generate and encode manufacturer attestation device signature.
    err = GenerateAndEncodeWeaveECDSASignature(writer, ContextTag(kTag_GetCertReqMsg_MfrAttestSig_ECDSA),
                                               hash, SHA256::kHashLength, privKey, privKeyLen);
    SuccessOrExit(err);

exit:
    return err;
}

WEAVE_ERROR MockCertificateProvisioningClient::StartCertificateProvisioning(uint8_t reqType, EncodeReqAuthInfoFunct encodeReqAuthInfo,
                                                                            void * requesterState, HandleCertificateProvisioningResultFunct onCertProvDone)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    IPAddress endPointAddr;

    VerifyOrExit(IPAddress::FromString(WOCAServerAddr, endPointAddr), err = WEAVE_ERROR_INVALID_ADDRESS);

    mReqType = reqType;
    mEncodeReqAuthInfo = encodeReqAuthInfo;
    mRequesterState = requesterState;
    mOnCertProvDone = onCertProvDone;

    mDoMfrAttest = (reqType == WeaveCertProvEngine::kReqType_GetInitialOpDeviceCert) ? true : false;

    printf("Initiating communication with Certificate Provisioning service\n");

    // Create a binding and begin the process of preparing it for talking to the Certificate Provisioning service.
    mBinding = mExchangeMgr->NewBinding(HandleCertificateProvisioningBindingEvent, this);
    VerifyOrExit(mBinding != NULL, err = WEAVE_ERROR_NO_MEMORY);

    if (WOCAServerUseCASE)
    {
        err = mBinding->BeginConfiguration()
                .Target_NodeId(WOCAServerEndPointId)
                .TargetAddress_IP(endPointAddr)
                .Transport_UDP_WRM()
                .Security_SharedCASESession()
                .PrepareBinding();
    }
    else
    {
        err = mBinding->BeginConfiguration()
                .Target_NodeId(WOCAServerEndPointId)
                .TargetAddress_IP(endPointAddr)
                .Transport_UDP_WRM()
                .Security_None()
                .PrepareBinding();
    }
    SuccessOrExit(err);

exit:
    return err;
}

// ===== Members for internal use by this class only.

void MockCertificateProvisioningClient::HandleCertificateProvisioningBindingEvent(void *const appState,
                                                                                  const nl::Weave::Binding::EventType event,
                                                                                  const nl::Weave::Binding::InEventParam &inParam,
                                                                                  nl::Weave::Binding::OutEventParam &outParam)
{
    MockCertificateProvisioningClient *client = static_cast<MockCertificateProvisioningClient *>(appState);
    uint32_t statusProfileId = 0;
    uint16_t statusCode = 0;

    switch (event)
    {
    case Binding::kEvent_BindingReady:
        printf("Certificate Provisioning client binding ready\n");

        client->SendGetCertificateRequest();
        break;

    case Binding::kEvent_PrepareFailed:
        printf("Certificate Provisioning client binding prepare failed: %s\n", nl::ErrorStr(inParam.PrepareFailed.Reason));

        if (inParam.PrepareFailed.StatusReport != NULL)
        {
            statusProfileId = inParam.PrepareFailed.StatusReport->mProfileId;
            statusCode = inParam.PrepareFailed.StatusReport->mStatusCode;
        }
        else
        {
            statusProfileId = kWeaveProfile_Security;
            statusCode = Profiles::ServiceProvisioning::kStatusCode_ServiceCommunicationError;
        }

        client->HandleCertificateProvisioningResult(inParam.PrepareFailed.Reason, statusProfileId, statusCode);
        break;

    case nl::Weave::Binding::kEvent_BindingFailed:
        printf("Certificate Provisioning client binding failed: %s\n", nl::ErrorStr(inParam.BindingFailed.Reason));

        statusProfileId = kWeaveProfile_Security;
        statusCode = Profiles::ServiceProvisioning::kStatusCode_ServiceCommunicationError;

        client->HandleCertificateProvisioningResult(inParam.BindingFailed.Reason, statusProfileId, statusCode);
        break;

    default:
        Binding::DefaultEventHandler(appState, event, inParam, outParam);
        break;
    }
}

void MockCertificateProvisioningClient::SendGetCertificateRequest(void)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    err = mCertProvEngine.Init(mBinding, this, this, CertProvClientEventHandler, this);
    SuccessOrExit(err);

    printf("Sending GetCertificateRequest to the Weave Operational Certificate Provisioning (WOCA) Server\n");

    err = mCertProvEngine.StartCertificateProvisioning(mReqType, mDoMfrAttest);
    SuccessOrExit(err);

exit:
    if (err != WEAVE_NO_ERROR)
    {
        HandleCertificateProvisioningResult(err, kWeaveProfile_Common, Profiles::Common::kStatus_InternalError);
    }
}

void MockCertificateProvisioningClient::HandleCertificateProvisioningResult(WEAVE_ERROR localErr, uint32_t statusProfileId, uint16_t statusCode)
{
    // Close the binding if necessary.
    if (mBinding != NULL)
    {
        mBinding->Close();
        mBinding = NULL;
    }

    if (localErr != WEAVE_NO_ERROR)
    {
        printf("Certificate Provisioning failed with %s: %s\n",
                 (localErr == WEAVE_ERROR_STATUS_REPORT_RECEIVED) ? "status report from service" : "local error",
                 (localErr == WEAVE_ERROR_STATUS_REPORT_RECEIVED) ? ::nl::StatusReportStr(statusProfileId, statusCode)
                                                             : ::nl::ErrorStr(localErr));

        // Choose an appropriate StatusReport to return if not already given.
        if (statusProfileId == 0 && statusCode == 0)
        {
            if (localErr == WEAVE_ERROR_TIMEOUT)
            {
                statusProfileId = kWeaveProfile_Security;
                statusCode = Profiles::ServiceProvisioning::kStatusCode_ServiceCommunicationError;
            }
            else
            {
                statusProfileId = kWeaveProfile_Common;
                statusCode = Profiles::Common::kStatus_InternalError;
            }
        }
    }

    // CallBack to the Calling Application.
    if (mOnCertProvDone != NULL)
        mOnCertProvDone(mRequesterState, localErr, statusProfileId, statusCode);
}

// ===== Persisted Operational Device Credentials.

WEAVE_ERROR MockCertificateProvisioningClient::GetDeviceId(uint64_t & deviceId)
{
    deviceId = mDeviceId;

    return WEAVE_NO_ERROR;
}

WEAVE_ERROR MockCertificateProvisioningClient::GetDeviceCertificate(uint8_t *& cert, uint16_t & certLen)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    VerifyOrExit(mDeviceCert != NULL, err = WEAVE_ERROR_CERT_NOT_FOUND);

    cert = mDeviceCert;
    certLen = mDeviceCertLen;

exit:
    return err;
}

WEAVE_ERROR MockCertificateProvisioningClient::GetDeviceIntermediateCACerts(uint8_t *& certs, uint16_t & certsLen)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    VerifyOrExit(mDeviceIntermediateCACerts != NULL, err = WEAVE_ERROR_CA_CERT_NOT_FOUND);

    certs = mDeviceIntermediateCACerts;
    certsLen = mDeviceIntermediateCACertsLen;

exit:
    return err;
}

WEAVE_ERROR MockCertificateProvisioningClient::GetDevicePrivateKey(uint8_t *& key, uint16_t & keyLen)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    VerifyOrExit(mDevicePrivateKey != NULL, err = WEAVE_ERROR_KEY_NOT_FOUND);

    key = mDevicePrivateKey;
    keyLen = mDevicePrivateKeyLen;

exit:
    return err;
}

WEAVE_ERROR MockCertificateProvisioningClient::StoreDeviceId(uint64_t deviceId)
{
    mDeviceId = deviceId;

    return WEAVE_NO_ERROR;
}

WEAVE_ERROR MockCertificateProvisioningClient::StoreDeviceCertificate(const uint8_t * cert, uint16_t certLen)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint8_t *certCopy = NULL;

    certCopy = (uint8_t *)malloc(certLen);
    VerifyOrExit(certCopy != NULL, err = WEAVE_ERROR_NO_MEMORY);
    memcpy(certCopy, cert, certLen);

    if (mDeviceCert != NULL)
        free(mDeviceCert);

    mDeviceCert = certCopy;
    mDeviceCertLen = certLen;

    // Setup to use operational device certificate in subsequence CASE sessions.
    gCASEOptions.NodeCert = mDeviceCert;
    gCASEOptions.NodeCertLength = mDeviceCertLen;

exit:
    if (err != WEAVE_NO_ERROR && certCopy != NULL)
        free(certCopy);
    return err;
}

WEAVE_ERROR MockCertificateProvisioningClient::StoreDeviceIntermediateCACerts(const uint8_t * certs, uint16_t certsLen)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint8_t *certsCopy = NULL;

    certsCopy = (uint8_t *)malloc(certsLen);
    VerifyOrExit(certsCopy != NULL, err = WEAVE_ERROR_NO_MEMORY);
    memcpy(certsCopy, certs, certsLen);

    if (mDeviceIntermediateCACerts != NULL)
        free(mDeviceIntermediateCACerts);

    mDeviceIntermediateCACerts = certsCopy;
    mDeviceIntermediateCACertsLen = certsLen;

    // Setup to use operational device intermediate CA certificates in subsequence CASE sessions.
    gCASEOptions.NodeIntermediateCert = mDeviceIntermediateCACerts;
    gCASEOptions.NodeIntermediateCertLength = mDeviceIntermediateCACertsLen;

exit:
    if (err != WEAVE_NO_ERROR && certsCopy != NULL)
        free(certsCopy);
    return err;
}

WEAVE_ERROR MockCertificateProvisioningClient::StoreDevicePrivateKey(const uint8_t * key, uint16_t keyLen)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint8_t *keyCopy = NULL;

    keyCopy = (uint8_t *)malloc(keyLen);
    VerifyOrExit(keyCopy != NULL, err = WEAVE_ERROR_NO_MEMORY);
    memcpy(keyCopy, key, keyLen);

    if (mDevicePrivateKey != NULL)
        free(mDevicePrivateKey);

    mDevicePrivateKey = keyCopy;
    mDevicePrivateKeyLen = keyLen;

    // Setup to use operational device private key in subsequence CASE sessions.
    gCASEOptions.NodePrivateKey = mDevicePrivateKey;
    gCASEOptions.NodePrivateKeyLength = mDevicePrivateKeyLen;

exit:
    if (err != WEAVE_NO_ERROR && keyCopy != NULL)
        free(keyCopy);
    return err;
}

void MockCertificateProvisioningClient::ClearOperationalDeviceCredentials(void)
{
    mDeviceId = kNodeIdNotSpecified;
    if (mDeviceCert != NULL)
    {
        free(mDeviceCert);
        mDeviceCert = NULL;
    }
    mDeviceCertLen = 0;
    if (mDeviceIntermediateCACerts != NULL)
    {
        free(mDeviceIntermediateCACerts);
        mDeviceIntermediateCACerts = NULL;
    }
    mDeviceIntermediateCACertsLen = 0;
    if (mDevicePrivateKey != NULL)
    {
        free(mDevicePrivateKey);
        mDevicePrivateKey = NULL;
    }
    mDevicePrivateKeyLen = 0;

    gCASEOptions.NodeCert = NULL;
    gCASEOptions.NodeCertLength = 0;
    gCASEOptions.NodeIntermediateCert = NULL;
    gCASEOptions.NodeIntermediateCertLength = 0;
    gCASEOptions.NodePrivateKey = NULL;
    gCASEOptions.NodePrivateKeyLength = 0;
}

WEAVE_ERROR MockCertificateProvisioningClient::GetManufacturerDeviceCertificate(uint8_t *& cert, uint16_t & certLen)
{
    cert = TestDevice1_Cert;
    certLen = TestDevice1_CertLength;

    return WEAVE_NO_ERROR;
}

WEAVE_ERROR MockCertificateProvisioningClient::GetManufacturerDeviceIntermediateCACerts(uint8_t *& certs, uint16_t & certsLen)
{
    certs = NULL;
    certsLen = 0;

    return WEAVE_NO_ERROR;
}

WEAVE_ERROR MockCertificateProvisioningClient::GetManufacturerDevicePrivateKey(uint8_t *& key, uint16_t & keyLen)
{
    key = TestDevice1_PrivateKey;
    keyLen = TestDevice1_PrivateKeyLength;

    return WEAVE_NO_ERROR;
}
