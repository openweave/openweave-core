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

#if WEAVE_DEVICE_CONFIG_ENABLE_JUST_IN_TIME_PROVISIONING

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
    mDoMfrAttest = (reqType == WeaveCertProvEngine::kReqType_GetInitialOpDeviceCert) ? true : false;

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
                err = ConfigurationMgr().StoreDeviceIntermediateCACerts(inParam.ResponseReceived.RelatedCerts, inParam.ResponseReceived.RelatedCertsLen);
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
    uint8_t * icaCerts = NULL;
    size_t icaCertsLen = 0;

    // Determine if present and the length of the operational device intermediate CA certificates.
    err = ConfigurationMgr().GetDeviceIntermediateCACerts((uint8_t *)NULL, 0, icaCertsLen);
    if (err == WEAVE_DEVICE_ERROR_CONFIG_NOT_FOUND)
    {
        // Exit without error if operational device intermediate CA certificates is not configured.
        ExitNow(err = WEAVE_NO_ERROR);
    }
    SuccessOrExit(err);

    VerifyOrExit(icaCertsLen != 0, err = WEAVE_ERROR_CERT_NOT_FOUND);

    // Create a temporary buffer to hold the operational device intermediate CA certificates.
    icaCerts = static_cast<uint8_t *>(MemoryAlloc(icaCertsLen));
    VerifyOrExit(icaCerts != NULL, err = WEAVE_ERROR_NO_MEMORY);

    // Read the operational device intermediate CA certificates.
    err = ConfigurationMgr().GetDeviceCertificate(icaCerts, icaCertsLen, icaCertsLen);
    SuccessOrExit(err);

    // Copy encoded operational device intermediate CA certificates.
    err = writer.CopyContainer(tag, icaCerts, icaCertsLen);
    SuccessOrExit(err);

exit:
    if (icaCerts != NULL)
    {
        MemoryFree(icaCerts);
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

// ===== Methods that implement the WeaveNodeMfrAttestDelegate interface

WEAVE_ERROR CertificateProvisioningClient::EncodeMAInfo(TLVWriter & writer)
{
    WEAVE_ERROR err;
    uint8_t * cert = NULL;
    size_t certLen = 0;
    size_t icaCertsLen = 0;

    // Determine the length of the manufacturer assigned device certificate.
    err = ConfigurationMgr().GetManufacturerDeviceCertificate((uint8_t *)NULL, 0, certLen);
    SuccessOrExit(err);

    // Fail if no manufacturer attestation device certificate has been configured.
    VerifyOrExit(certLen != 0, err = WEAVE_ERROR_CERT_NOT_FOUND);

    // Create a temporary buffer to hold the manufacturer attestation device certificate.
    cert = static_cast<uint8_t *>(MemoryAlloc(certLen));
    VerifyOrExit(cert != NULL, err = WEAVE_ERROR_NO_MEMORY);

    // Read the manufacturer assigned device certificate.
    err = ConfigurationMgr().GetManufacturerDeviceCertificate(cert, certLen, certLen);
    SuccessOrExit(err);

    // Copy encoded manufacturer attestation device certificate.
    err = writer.CopyContainer(ContextTag(kTag_GetCertReqMsg_MfrAttest_WeaveCert), cert, certLen);
    SuccessOrExit(err);

    // Determine if present and the length of the manufacturer assigned device intermediate CA certificates.
    err = ConfigurationMgr().GetManufacturerDeviceIntermediateCACerts((uint8_t *)NULL, 0, icaCertsLen);
    if (err == WEAVE_DEVICE_ERROR_CONFIG_NOT_FOUND)
    {
        // Exit without error if manufacturer assigned intermediate CA certificates is not configured.
        ExitNow(err = WEAVE_NO_ERROR);
    }
    SuccessOrExit(err);

    {
        VerifyOrExit(icaCertsLen != 0, err = WEAVE_ERROR_CERT_NOT_FOUND);

        // If needed, allocate larger buffer to hold the intermediate CA certificates.
        if (icaCertsLen > certLen)
        {
            MemoryFree(cert);

            cert = static_cast<uint8_t *>(MemoryAlloc(icaCertsLen));
            VerifyOrExit(cert != NULL, err = WEAVE_ERROR_NO_MEMORY);
        }

        // Read the manufacturer assigned device intermediate CA certificates.
        err = ConfigurationMgr().GetManufacturerDeviceIntermediateCACerts(cert, icaCertsLen, icaCertsLen);
        SuccessOrExit(err);

        // Copy encoded manufacturer attestation device intermediate CA certificates.
        err = writer.CopyContainer(ContextTag(kTag_GetCertReqMsg_MfrAttest_WeaveRelCerts), cert, icaCertsLen);
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
    err = ConfigurationMgr().GetManufacturerDevicePrivateKey((uint8_t *)NULL, 0, privKeyLen);
    SuccessOrExit(err);

    // Fail if no manufacturer attestation device private key has been configured.
    VerifyOrExit(privKeyLen != 0, err = WEAVE_ERROR_KEY_NOT_FOUND);

    // Create a temporary buffer to hold the manufacturer attestation device private key.
    privKey = static_cast<uint8_t *>(MemoryAlloc(privKeyLen));
    VerifyOrExit(privKey != NULL, err = WEAVE_ERROR_NO_MEMORY);

    // Read the manufacturer attestation device private key.
    err = ConfigurationMgr().GetManufacturerDevicePrivateKey(privKey, privKeyLen, privKeyLen);
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

    err = mCertProvEngine.StartCertificateProvisioning(mReqType, mDoMfrAttest);
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

#endif // WEAVE_DEVICE_CONFIG_ENABLE_JUST_IN_TIME_PROVISIONING

} // namespace Internal
} // namespace DeviceLayer
} // namespace Weave
} // namespace nl
