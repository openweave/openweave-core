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

#include <Weave/DeviceLayer/internal/WeaveDeviceLayerInternal.h>
#include <Weave/DeviceLayer/internal/CertificateProvisioningClient.h>
#include <Weave/Core/WeaveTLV.h>
#include <Weave/Profiles/common/CommonProfile.h>
#include <Weave/Profiles/security/WeaveCert.h>
#include <Weave/Profiles/security/WeavePrivateKey.h>
#include <Weave/Profiles/security/WeaveSig.h>

using namespace ::nl;
using namespace ::nl::Weave;
using namespace ::nl::Weave::TLV;
using namespace ::nl::Weave::Profiles;
using namespace ::nl::Weave::Profiles::Security;
using namespace ::nl::Weave::Profiles::Security::CertProvisioning;
using namespace ::nl::Weave::Platform::Security;

namespace nl {
namespace Weave {
namespace DeviceLayer {
namespace Internal {

using ::nl::Weave::Platform::Security::MemoryAlloc;
using ::nl::Weave::Platform::Security::MemoryFree;

#if WEAVE_DEVICE_CONFIG_ENABLE_OPERATIONAL_DEVICE_CREDENTIALS

static EncodedECPrivateKey sDevicePrivKey;

static WEAVE_ERROR sGenerateDeviceECDSASignature(const uint8_t *hash, uint8_t hashLen, EncodedECDSASignature& ecdsaSig)
{
    return nl::Weave::Crypto::GenerateECDSASignature(WeaveCurveIdToOID(WEAVE_CONFIG_OPERATIONAL_DEVICE_CERT_CURVE_ID),
                                                     hash, hashLen, sDevicePrivKey, ecdsaSig);
}

/**
 * Generates and stores Weave operational device credentials:
 *   -- Weave device Id (if needed)
 *   -- Weave device self-signed certificate
 *   -- Weave device private key
 *
 * @retval #WEAVE_NO_ERROR         If Weave device credentials were successfully generated and stored.
 */
WEAVE_ERROR GenerateAndStoreOperationalDeviceCredentials(uint64_t deviceId)
{
    enum {
        kWeaveDeviceKeyBufSize  = 128,
        kWeaveDeviceCertBufSize = 300,
    };
    WEAVE_ERROR err;
    uint8_t key[kWeaveDeviceKeyBufSize];
    uint32_t keyLen;
    uint8_t cert[kWeaveDeviceCertBufSize];
    uint16_t certLen;
    uint8_t privKey[EncodedECPrivateKey::kMaxValueLength];
    uint8_t pubKey[EncodedECPublicKey::kMaxValueLength];
    EncodedECPublicKey devicePubKey;

    // If not specified, generate random device Id.
    if (deviceId == kNodeIdNotSpecified)
    {
        err = GenerateWeaveNodeId(deviceId);
        SuccessOrExit(err);
    }

    sDevicePrivKey.PrivKey = privKey;
    sDevicePrivKey.PrivKeyLen = sizeof(privKey);

    devicePubKey.ECPoint = pubKey;
    devicePubKey.ECPointLen = sizeof(pubKey);

    // Generate random EC private/public key pair.
    err = GenerateECDHKey(WeaveCurveIdToOID(WEAVE_CONFIG_OPERATIONAL_DEVICE_CERT_CURVE_ID), devicePubKey, sDevicePrivKey);
    SuccessOrExit(err);

    // Encode Weave device EC private/public key pair into EllipticCurvePrivateKey TLV structure.
    err = EncodeWeaveECPrivateKey(WEAVE_CONFIG_OPERATIONAL_DEVICE_CERT_CURVE_ID, &devicePubKey, sDevicePrivKey, key, sizeof(key), keyLen);
    SuccessOrExit(err);

    // Generate self-signed operational device certificate.
    err = GenerateOperationalDeviceCert(deviceId, devicePubKey, cert, sizeof(cert), certLen, sGenerateDeviceECDSASignature);
    SuccessOrExit(err);

    // Store generated device credentials.
    err = ConfigurationMgr().StoreDeviceCredentials(deviceId, cert, certLen, key, keyLen);
    SuccessOrExit(err);

exit:
    return err;
}

WEAVE_ERROR CertificateProvisioningClient::Init(uint8_t reqType)
{
    return Init(reqType, NULL);
}

/**
 * Initialize certificate provisioning client.
 *
 *  @param[in]  reqType              Get certificate request type.
 *  @param[in]  encodeReqAuthInfo    A pointer to a function that generates ECDSA signature on the given
 *                                   certificate hash using operational device private key.
 *
 *  @retval #WEAVE_NO_ERROR          If certificate provisioning client was successfully initialized.
 */
WEAVE_ERROR CertificateProvisioningClient::Init(uint8_t reqType, EncodeReqAuthInfoFunct encodeReqAuthInfo)
{
    mReqType = reqType;
    mDoManufAttest = (reqType == WeaveCertProvEngine::kReqType_GetInitialOpDeviceCert) ? true : false;

    mEncodeReqAuthInfo = encodeReqAuthInfo;

    mBinding = NULL;
    mWaitingForServiceConnectivity = false;

    return WEAVE_NO_ERROR;
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
void CertificateProvisioningClient::CertProvClientEventHandler(void * appState, WeaveCertProvEngine::EventType eventType, const WeaveCertProvEngine::InEventParam & inParam, WeaveCertProvEngine::OutEventParam & outParam)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    CertificateProvisioningClient * client = static_cast<CertificateProvisioningClient *>(appState);
    WeaveCertProvEngine * certProvEngine = inParam.Source;

    switch (eventType)
    {
    case WeaveCertProvEngine::kEvent_PrepareAuthorizeInfo:
    {
        if (client->mEncodeReqAuthInfo != NULL)
        {
            WeaveLogProgress(DeviceLayer, "Preparing authorization information for the GetCertificateRequest message");

            err = client->mEncodeReqAuthInfo(*inParam.PrepareAuthorizeInfo.Writer);
            SuccessOrExit(err);
        }

        break;
    }

    case WeaveCertProvEngine::kEvent_ResponseReceived:
    {
        if (inParam.ResponseReceived.ReplaceCert)
        {
            // Store service issued operational device certificate.
            err = ConfigurationMgr().StoreDeviceCertificate(inParam.ResponseReceived.Cert, inParam.ResponseReceived.CertLen);
            SuccessOrExit(err);

            if (inParam.ResponseReceived.RelatedCerts != NULL)
            {
                // Store device intermediate CA certificates related to the service issued operational device certificate.
                err = ConfigurationMgr().StoreDeviceICACerts(inParam.ResponseReceived.RelatedCerts, inParam.ResponseReceived.RelatedCertsLen);
                SuccessOrExit(err);
            }

            // Post an event alerting other subsystems that the device now has new certificate.
            {
                WeaveDeviceEvent event;
                event.Type = DeviceEventType::kDeviceCredentialsChange;
                event.DeviceCredentialsChange.AreCredentialsProvisioned = true;
                PlatformMgr().PostEvent(&event);
            }

            WeaveLogProgress(DeviceLayer, "Stored new operational device certificate received from the CA service");
        }
        else
        {
            WeaveLogProgress(DeviceLayer, "CA service reported: no need to replace operational device certificate");
        }

        certProvEngine->AbortCertificateProvisioning();

        break;
    }

    case WeaveCertProvEngine::kEvent_CommunicationError:
    {
        if (inParam.CommunicationError.Reason == WEAVE_ERROR_STATUS_REPORT_RECEIVED)
        {
            WeaveLogError(DeviceLayer, "Received status report from the CA service: %s",
                          nl::StatusReportStr(inParam.CommunicationError.RcvdStatusReport->mProfileId, inParam.CommunicationError.RcvdStatusReport->mStatusCode));
        }
        else
        {
            WeaveLogError(DeviceLayer, "Failed to prepare/send GetCertificateRequest message: %s", ErrorStr(inParam.CommunicationError.Reason));
        }

        certProvEngine->AbortCertificateProvisioning();

        break;
    }

    default:
        WeaveLogError(DeviceLayer, "Unrecognized certificate provisioning API event");
        break;
    }

exit:
    if (eventType == WeaveCertProvEngine::kEvent_PrepareAuthorizeInfo)
        outParam.PrepareAuthorizeInfo.Error = err;
    else if (eventType == WeaveCertProvEngine::kEvent_ResponseReceived)
        outParam.ResponseReceived.Error = err;
}

// ===== Methods that implement the WeaveNodeOpAuthDelegate interface

WEAVE_ERROR CertificateProvisioningClient::EncodeOpCert(TLVWriter & writer, uint64_t tag)
{
    WEAVE_ERROR err;
    uint8_t * cert = NULL;
    size_t certLen = 0;

    // Determine the length of the operational device certificate.
    err = ConfigurationMgr().GetDeviceCertificate((uint8_t *)NULL, 0, certLen);
    SuccessOrExit(err);

    // Fail if no operational device certificate has been configured.
    VerifyOrExit(certLen != 0, err = WEAVE_ERROR_CERT_NOT_FOUND);

    // Create a temporary buffer to hold the operational device certificate.
    cert = static_cast<uint8_t *>(MemoryAlloc(certLen));
    VerifyOrExit(cert != NULL, err = WEAVE_ERROR_NO_MEMORY);

    // Read the operational device certificate.
    err = ConfigurationMgr().GetDeviceCertificate(cert, certLen, certLen);
    SuccessOrExit(err);

    // Copy encoded operational device certificate.
    err = writer.CopyContainer(tag, cert, certLen);
    SuccessOrExit(err);

exit:
    if (cert != NULL)
    {
        MemoryFree(cert);
    }
    return err;
}

WEAVE_ERROR CertificateProvisioningClient::EncodeOpRelatedCerts(TLVWriter & writer, uint64_t tag)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint8_t * caCerts = NULL;
    size_t caCertsLen = 0;

    if (ConfigurationMgr().DeviceICACertsProvisioned())
    {
        // Determine the length of the operational device intermediate certificates.
        err = ConfigurationMgr().GetDeviceICACerts((uint8_t *)NULL, 0, caCertsLen);
        SuccessOrExit(err);

        // Fail if no operational device intermediate certificates array has been configured.
        VerifyOrExit(caCertsLen != 0, err = WEAVE_ERROR_CERT_NOT_FOUND);

        // Create a temporary buffer to hold the operational device intermediate certificates.
        caCerts = static_cast<uint8_t *>(MemoryAlloc(caCertsLen));
        VerifyOrExit(caCerts != NULL, err = WEAVE_ERROR_NO_MEMORY);

        // Read the operational device intermediate certificates.
        err = ConfigurationMgr().GetDeviceCertificate(caCerts, caCertsLen, caCertsLen);
        SuccessOrExit(err);

        // Copy encoded operational device intermediate certificates array.
        err = writer.CopyContainer(tag, caCerts, caCertsLen);
        SuccessOrExit(err);
    }

exit:
    if (caCerts != NULL)
    {
        MemoryFree(caCerts);
    }
    return err;
}

WEAVE_ERROR CertificateProvisioningClient::GenerateAndEncodeOpSig(const uint8_t * hash, uint8_t hashLen, TLVWriter & writer, uint64_t tag)
{
    WEAVE_ERROR err;
    uint8_t * privKey = NULL;
    size_t privKeyLen = 0;

    // Determine the length of the operational device private key.
    err = ConfigurationMgr().GetDevicePrivateKey((uint8_t *)NULL, 0, privKeyLen);
    SuccessOrExit(err);

    // Fail if no operational device private key has been configured.
    VerifyOrExit(privKeyLen != 0, err = WEAVE_ERROR_KEY_NOT_FOUND);

    // Create a temporary buffer to hold the operational device private key.
    privKey = static_cast<uint8_t *>(MemoryAlloc(privKeyLen));
    VerifyOrExit(privKey != NULL, err = WEAVE_ERROR_NO_MEMORY);

    // Read the operational device private key.
    err = ConfigurationMgr().GetDevicePrivateKey(privKey, privKeyLen, privKeyLen);
    SuccessOrExit(err);

    // Generate and encode operational device signature.
    err = GenerateAndEncodeWeaveECDSASignature(writer, tag, hash, hashLen, privKey, privKeyLen);
    SuccessOrExit(err);

exit:
    if (privKey != NULL)
    {
        MemoryFree(privKey);
    }
    return err;
}

// ===== Methods that implement the WeaveNodeManufAttestDelegate interface

WEAVE_ERROR CertificateProvisioningClient::EncodeMAInfo(TLVWriter & writer)
{
    WEAVE_ERROR err;
    uint8_t * cert = NULL;
    size_t certLen = 0;

    // Determine the length of the manufacturer attestation device certificate.
    err = ConfigurationMgr().GetManufAttestDeviceCertificate((uint8_t *)NULL, 0, certLen);
    SuccessOrExit(err);

    // Fail if no manufacturer attestation device certificate has been configured.
    VerifyOrExit(certLen != 0, err = WEAVE_ERROR_CERT_NOT_FOUND);

    // Create a temporary buffer to hold the manufacturer attestation device certificate.
    cert = static_cast<uint8_t *>(MemoryAlloc(certLen));
    VerifyOrExit(cert != NULL, err = WEAVE_ERROR_NO_MEMORY);

    // Read the manufacturer attestation device certificate.
    err = ConfigurationMgr().GetManufAttestDeviceCertificate(cert, certLen, certLen);
    SuccessOrExit(err);

    // Copy encoded manufacturer attestation device certificate.
    err = writer.CopyContainer(ContextTag(kTag_GetCertReqMsg_ManufAttest_WeaveCert), cert, certLen);
    SuccessOrExit(err);

    if (ConfigurationMgr().ManufAttestDeviceICACertsProvisioned())
    {
        size_t caCertsLen = 0;

        // Determine the length of the manufacturer attestation device intermediate CA certificates.
        err = ConfigurationMgr().GetManufAttestDeviceICACerts((uint8_t *)NULL, 0, caCertsLen);
        SuccessOrExit(err);

        // If needed, allocate larger buffer to hold the intermediate CA certificates.
        if (caCertsLen > certLen)
        {
            MemoryFree(cert);

            cert = static_cast<uint8_t *>(MemoryAlloc(caCertsLen));
            VerifyOrExit(cert != NULL, err = WEAVE_ERROR_NO_MEMORY);
        }

        // Read the manufacturer attestation device intermediate CA certificates.
        err = ConfigurationMgr().GetManufAttestDeviceICACerts(cert, caCertsLen, caCertsLen);
        SuccessOrExit(err);

        // Copy encoded manufacturer attestation device intermediate CA certificates.
        err = writer.CopyContainer(ContextTag(kTag_GetCertReqMsg_ManufAttest_WeaveRelCerts), cert, caCertsLen);
        SuccessOrExit(err);
    }

exit:
    if (cert != NULL)
    {
        MemoryFree(cert);
    }
    return err;
}

WEAVE_ERROR CertificateProvisioningClient::GenerateAndEncodeMASig(const uint8_t * data, uint16_t dataLen, TLVWriter & writer)
{
    WEAVE_ERROR err;
    uint8_t * privKey = NULL;
    size_t privKeyLen = 0;
    nl::Weave::Platform::Security::SHA256 sha256;
    uint8_t hash[SHA256::kHashLength];

    // Determine the length of the manufacturer attestation device private key.
    err = ConfigurationMgr().GetDevicePrivateKey((uint8_t *)NULL, 0, privKeyLen);
    SuccessOrExit(err);

    // Fail if no manufacturer attestation device private key has been configured.
    VerifyOrExit(privKeyLen != 0, err = WEAVE_ERROR_KEY_NOT_FOUND);

    // Create a temporary buffer to hold the manufacturer attestation device private key.
    privKey = static_cast<uint8_t *>(MemoryAlloc(privKeyLen));
    VerifyOrExit(privKey != NULL, err = WEAVE_ERROR_NO_MEMORY);

    // Read the manufacturer attestation device private key.
    err = ConfigurationMgr().GetDevicePrivateKey(privKey, privKeyLen, privKeyLen);
    SuccessOrExit(err);

    // Calculate data hash.
    sha256.Begin();
    sha256.AddData(data, dataLen);
    sha256.Finish(hash);

    // Encode manufacturer attestation device signature algorithm: ECDSAWithSHA256.
    err = writer.Put(ContextTag(kTag_GetCertReqMsg_ManufAttestSigAlgo), static_cast<uint16_t>(ASN1::kOID_SigAlgo_ECDSAWithSHA256));
    SuccessOrExit(err);

    // Generate and encode manufacturer attestation device signature.
    err = GenerateAndEncodeWeaveECDSASignature(writer, ContextTag(kTag_GetCertReqMsg_ManufAttestSig_ECDSA),
                                               hash, SHA256::kHashLength, privKey, privKeyLen);
    SuccessOrExit(err);

exit:
    if (privKey != NULL)
    {
        MemoryFree(privKey);
    }
    return err;
}

// ===== Members for internal use by this class only.

void CertificateProvisioningClient::OnPlatformEvent(const WeaveDeviceEvent * event)
{
    // If a tunnel to the service has been established...
    // OR if service connectivity has been established (e.g. via Thread)...
    if ((event->Type == DeviceEventType::kServiceTunnelStateChange &&
         event->ServiceTunnelStateChange.Result == kConnectivity_Established) ||
        (event->Type == DeviceEventType::kServiceConnectivityChange &&
         event->ServiceConnectivityChange.Overall.Result == kConnectivity_Established))
    {
        // If the system is waiting for the service connectivity to be established,
        // initiate the Certificate Provisioning now.
        if (mWaitingForServiceConnectivity)
        {
            StartCertificateProvisioning();
        }
    }
}

void CertificateProvisioningClient::StartCertificateProvisioning(void)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    // If the system does not currently have a tunnel established with the service,
    // AND the system does not have service connectivity by some other means (e.g. Thread)
    // wait a period of time for connectivity to be established.
    if (!ConnectivityMgr().HaveServiceConnectivity() && !ConnectivityMgr().IsServiceTunnelConnected())
    {
        mWaitingForServiceConnectivity = true;

        err = SystemLayer.StartTimer(WEAVE_DEVICE_CONFIG_CERTIFICATE_PROVISIONING_CONNECTIVITY_TIMEOUT,
                                     HandleServiceConnectivityTimeout, this);
        SuccessOrExit(err);
        ExitNow();

        WeaveLogProgress(DeviceLayer, "Waiting for service connectivity to complete RegisterServicePairDevice action");
    }

    mWaitingForServiceConnectivity = false;
    SystemLayer.CancelTimer(HandleServiceConnectivityTimeout, this);

    WeaveLogProgress(DeviceLayer, "Initiating communication with Service Provisioning service");

    // Create a binding and begin the process of preparing it for talking to the Certificate Provisioning
    // service. When this completes HandleCertProvBindingEvent will be called with a BindingReady event.
    mBinding = nl::Weave::DeviceLayer::ExchangeMgr.NewBinding(HandleCertProvBindingEvent, NULL);
    VerifyOrExit(mBinding != NULL, err = WEAVE_ERROR_NO_MEMORY);
    err = mBinding->BeginConfiguration()
            .Target_ServiceEndpoint(WEAVE_DEVICE_CONFIG_CERTIFICATE_PROVISIONING_ENDPOINT_ID)
            .Transport_UDP_WRM()
            .Exchange_ResponseTimeoutMsec(WEAVE_DEVICE_CONFIG_GET_CERTIFICATE_REQUEST_TIMEOUT)
            .Security_SharedCASESession()
            .PrepareBinding();
    SuccessOrExit(err);

    err = mCertProvEngine.Init(mBinding, this, this, CertProvClientEventHandler, this);
    SuccessOrExit(err);

exit:
    if (err != WEAVE_NO_ERROR)
    {
        HandleCertificateProvisioningResult(err, kWeaveProfile_Common, Profiles::Common::kStatus_InternalError);
    }
}

void CertificateProvisioningClient::SendGetCertificateRequest(void)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    WeaveLogProgress(DeviceLayer, "Sending GetCertificateRequest to Certificate Provisioning service");

    err = mCertProvEngine.StartCertificateProvisioning(mReqType, mDoManufAttest);
    SuccessOrExit(err);

exit:
    if (err != WEAVE_NO_ERROR)
    {
        HandleCertificateProvisioningResult(err, kWeaveProfile_Common, Profiles::Common::kStatus_InternalError);
    }
}

void CertificateProvisioningClient::HandleCertificateProvisioningResult(WEAVE_ERROR err, uint32_t statusReportProfileId, uint16_t statusReportStatusCode)
{
    // Close the binding if necessary.
    if (mBinding != NULL)
    {
        mBinding->Close();
        mBinding = NULL;
    }

    if (err != WEAVE_NO_ERROR)
    {
        WeaveLogError(DeviceLayer, "Certificate Provisioning failed with %s: %s",
                 (err == WEAVE_ERROR_STATUS_REPORT_RECEIVED) ? "status report from service" : "local error",
                 (err == WEAVE_ERROR_STATUS_REPORT_RECEIVED)
                  ? ::nl::StatusReportStr(statusReportProfileId, statusReportStatusCode)
                  : ::nl::ErrorStr(err));

        // Choose an appropriate StatusReport to return if not already given.
        if (statusReportProfileId == 0 && statusReportStatusCode == 0)
        {
            if (err == WEAVE_ERROR_TIMEOUT)
            {
                statusReportProfileId = kWeaveProfile_Security;
                statusReportStatusCode = Profiles::Security::kStatusCode_ServiceCommunicationError;
            }
            else
            {
                statusReportProfileId = kWeaveProfile_Common;
                statusReportStatusCode = Profiles::Common::kStatus_InternalError;
            }
        }

        // TODO: Error CallBack to the the Calling Application.
    }
}

void CertificateProvisioningClient::HandleServiceConnectivityTimeout(System::Layer * aSystemLayer, void * aAppState, System::Error aErr)
{
    CertificateProvisioningClient *client = static_cast<CertificateProvisioningClient *>(aAppState);

    client->HandleCertificateProvisioningResult(WEAVE_ERROR_TIMEOUT, 0, 0);
}

void CertificateProvisioningClient::HandleCertProvBindingEvent(void * appState, Binding::EventType eventType,
            const Binding::InEventParam & inParam, Binding::OutEventParam & outParam)
{
    uint32_t statusReportProfileId;
    uint16_t statusReportStatusCode;
    CertificateProvisioningClient *client = static_cast<CertificateProvisioningClient *>(appState);

    switch (eventType)
    {
    case Binding::kEvent_BindingReady:
        client->SendGetCertificateRequest();
        break;
    case Binding::kEvent_PrepareFailed:
        if (inParam.PrepareFailed.StatusReport != NULL)
        {
            statusReportProfileId = inParam.PrepareFailed.StatusReport->mProfileId;
            statusReportStatusCode = inParam.PrepareFailed.StatusReport->mStatusCode;
        }
        else
        {
            statusReportProfileId = kWeaveProfile_Security;
            statusReportStatusCode = Profiles::Security::kStatusCode_ServiceCommunicationError;
        }
        client->HandleCertificateProvisioningResult(inParam.PrepareFailed.Reason,
                statusReportProfileId, statusReportStatusCode);
        break;
    default:
        Binding::DefaultEventHandler(appState, eventType, inParam, outParam);
        break;
    }
}

#endif // WEAVE_DEVICE_CONFIG_ENABLE_OPERATIONAL_DEVICE_CREDENTIALS

} // namespace Internal
} // namespace DeviceLayer
} // namespace Weave
} // namespace nl
