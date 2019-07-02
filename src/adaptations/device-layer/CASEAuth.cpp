/*
 *
 *    Copyright (c) 2018 Nest Labs, Inc.
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
 *          Provides an implementation for the OpenWeave WeaveCASEAuthDelegate
 *          interface.
 */

// TODO: Generalize this code to work on systems without malloc.

#include <Weave/DeviceLayer/internal/WeaveDeviceLayerInternal.h>
#include <Weave/Core/WeaveTLV.h>
#include <Weave/Profiles/WeaveProfiles.h>
#include <Weave/Profiles/security/WeaveSecurity.h>
#include <Weave/Profiles/security/WeaveCert.h>
#include <Weave/Profiles/security/WeaveCASE.h>
#include <Weave/Profiles/service-provisioning/ServiceProvisioning.h>
#include <Weave/Support/NestCerts.h>
#include <Weave/Support/ASN1.h>
#include <Weave/Support/TimeUtils.h>
#include <new>

using namespace ::nl;
using namespace ::nl::Weave;
using namespace ::nl::Weave::TLV;
using namespace ::nl::Weave::Profiles;
using namespace ::nl::Weave::Profiles::Security;

using namespace ::nl::Weave::DeviceLayer;
using namespace ::nl::Weave::DeviceLayer::Internal;

namespace nl {
namespace Weave {
namespace DeviceLayer {

namespace {

using ::nl::Weave::Platform::Security::MemoryAlloc;
using ::nl::Weave::Platform::Security::MemoryFree;

class CASEAuthDelegate : public WeaveCASEAuthDelegate
{
public:
    enum {
        kMaxValidationCerts = 7,            // Maximum number of certificates that can be present during certificate validation.
        kCertDecodeBufferSize = 1024        // Size of temporary buffer used during certificate decoding and signature validation.
    };

    CASEAuthDelegate()
    : mPrivKeyBuf(NULL), mServiceConfigBuf(NULL)
    {
    }

    virtual WEAVE_ERROR GetNodeCertInfo(bool isInitiator, uint8_t *buf, uint16_t bufSize, uint16_t& certInfoLen);
    virtual WEAVE_ERROR GetNodePrivateKey(bool isInitiator, const uint8_t *& weavePrivKey, uint16_t & weavePrivKeyLen);
    virtual WEAVE_ERROR ReleaseNodePrivateKey(const uint8_t *weavePrivKey);
    virtual WEAVE_ERROR GetNodePayload(bool isInitiator, uint8_t *buf, uint16_t bufSize, uint16_t& payloadLen);
    virtual WEAVE_ERROR BeginCertValidation(bool isInitiator, WeaveCertificateSet& certSet, ValidationContext& validContext);
    virtual WEAVE_ERROR HandleCertValidationResult(bool isInitiator, WEAVE_ERROR& validRes, WeaveCertificateData *peerCert,
            uint64_t peerNodeId, WeaveCertificateSet& certSet, ValidationContext& validContext);
    virtual WEAVE_ERROR EndCertValidation(WeaveCertificateSet& certSet, ValidationContext& validContext);

private:
    uint8_t *mPrivKeyBuf;
    uint8_t *mServiceConfigBuf;
};

CASEAuthDelegate gCASEAuthDelegate;

WEAVE_ERROR AddCertToContainer(TLVWriter& writer, uint64_t tag, const uint8_t *cert, uint16_t certLen);
WEAVE_ERROR MakeCertInfo(uint8_t *buf, uint16_t bufSize, uint16_t& certInfoLen,
        const uint8_t *entityCert, uint16_t entityCertLen,
        const uint8_t *intermediateCert, uint16_t intermediateCertLen);
WEAVE_ERROR LoadCertsFromServiceConfig(const uint8_t *serviceConfig, uint16_t serviceConfigLen, WeaveCertificateSet& certSet);

} // unnamed namespace

namespace Internal {

WEAVE_ERROR InitCASEAuthDelegate()
{
    new (&gCASEAuthDelegate) CASEAuthDelegate();
    SecurityMgr.SetCASEAuthDelegate(&gCASEAuthDelegate);
    return WEAVE_NO_ERROR;
}

} // namespace Internal

namespace {

WEAVE_ERROR CASEAuthDelegate::GetNodeCertInfo(bool isInitiator, uint8_t *buf, uint16_t bufSize, uint16_t& certInfoLen)
{
    WEAVE_ERROR err;
    uint8_t * certBuf = NULL;
    size_t certLen;

    // Determine the length of the device certificate.
    err = ConfigurationMgr().GetDeviceCertificate((uint8_t *)NULL, 0, certLen);
    SuccessOrExit(err);

    // Fail if no certificate has been configured.
    VerifyOrExit(certLen != 0, err = WEAVE_ERROR_CERT_NOT_FOUND);

    // Create a temporary buffer to hold the certificate.
    certBuf = (uint8_t *)MemoryAlloc(certLen);
    VerifyOrExit(certBuf != NULL, err = WEAVE_ERROR_NO_MEMORY);

    // Read the certificate
    err = ConfigurationMgr().GetDeviceCertificate(certBuf, certLen, certLen);
    SuccessOrExit(err);

    // Encode a CASECertificateInformation TLV structure containing the device certificate.
    err = MakeCertInfo(buf, bufSize, certInfoLen, certBuf, certLen, NULL, 0);
    SuccessOrExit(err);

exit:
    if (certBuf != NULL)
    {
        MemoryFree(certBuf);
    }
    return err;
}

WEAVE_ERROR CASEAuthDelegate::GetNodePrivateKey(bool isInitiator, const uint8_t *& weavePrivKey, uint16_t & weavePrivKeyLen)
{
    WEAVE_ERROR err;
    size_t privKeyLen;

    // Determine the length of the private key.
    err = ConfigurationMgr().GetDevicePrivateKey((uint8_t *)NULL, 0, privKeyLen);
    SuccessOrExit(err);

    // Fail if no private key has been configured.
    VerifyOrExit(privKeyLen != 0, err = WEAVE_ERROR_CERT_NOT_FOUND);

    // Create a temporary buffer to hold the private key.
    mPrivKeyBuf = (uint8_t *)MemoryAlloc(privKeyLen);
    VerifyOrExit(mPrivKeyBuf != NULL, err = WEAVE_ERROR_NO_MEMORY);

    // Read the private key.
    err = ConfigurationMgr().GetDevicePrivateKey(mPrivKeyBuf, privKeyLen, privKeyLen);
    SuccessOrExit(err);

    weavePrivKey = mPrivKeyBuf;
    weavePrivKeyLen = privKeyLen;

exit:
    if (err != WEAVE_NO_ERROR && mPrivKeyBuf != NULL)
    {
        MemoryFree(mPrivKeyBuf);
        mPrivKeyBuf = NULL;
    }
    return err;
}

WEAVE_ERROR CASEAuthDelegate::ReleaseNodePrivateKey(const uint8_t *weavePrivKey)
{
    if (mPrivKeyBuf != NULL)
    {
        MemoryFree(mPrivKeyBuf);
        mPrivKeyBuf = NULL;
    }
    return WEAVE_NO_ERROR;
}

WEAVE_ERROR CASEAuthDelegate::GetNodePayload(bool isInitiator, uint8_t *buf, uint16_t bufSize, uint16_t& payloadLen)
{
    WEAVE_ERROR err;
    size_t deviceDescLen;

    err = ConfigurationMgr().GetDeviceDescriptorTLV(buf, (size_t)bufSize, deviceDescLen);
    SuccessOrExit(err);

    payloadLen = deviceDescLen;

exit:
    return err;
}

WEAVE_ERROR CASEAuthDelegate::BeginCertValidation(bool isInitiator, WeaveCertificateSet& certSet, ValidationContext& validContext)
{
    WEAVE_ERROR err;
    nl::Weave::ASN1::ASN1UniversalTime validTime;
    WeaveCertificateData *cert;
    size_t serviceConfigLen;
    uint64_t nowMS;

    // Initialize the certificate set object arranging for it to call the platform
    // memory allocation functions when it need memory.
    err = certSet.Init(kMaxValidationCerts, kCertDecodeBufferSize, MemoryAlloc, MemoryFree);
    SuccessOrExit(err);

    // Determine if the device has been provisioned for talking to a service by querying the configuration manager
    // for the length of the service configuration data.  If service configuration data is present, then the device
    // has been service provisioned.
    err = ConfigurationMgr().GetServiceConfig(NULL, 0, serviceConfigLen);
    if (err != WEAVE_DEVICE_ERROR_CONFIG_NOT_FOUND)
        SuccessOrExit(err);

    // If the device has been service provisioned...
    if (err != WEAVE_DEVICE_ERROR_CONFIG_NOT_FOUND)
    {
        // Allocate a buffer to hold the service config data.
        mServiceConfigBuf = (uint8_t *)MemoryAlloc(serviceConfigLen);
        VerifyOrExit(mServiceConfigBuf != NULL, err = WEAVE_ERROR_NO_MEMORY);

        // Read the service config data.
        err = ConfigurationMgr().GetServiceConfig(mServiceConfigBuf, serviceConfigLen, serviceConfigLen);
        SuccessOrExit(err);

        // Load the list of trusted root certificate from the service config.
        err = LoadCertsFromServiceConfig(mServiceConfigBuf, serviceConfigLen, certSet);
        SuccessOrExit(err);

        // Scan the list of trusted certs loaded from the service config.  If the list contains a general
        // certificate with a CommonName subject, presume this certificate is the access token certificate
        // and mark it as such.
        for (uint8_t i = 0; i < certSet.CertCount; i++)
        {
            cert = &certSet.Certs[i];
            if ((cert->CertFlags & kCertFlag_IsTrusted) != 0 &&
                cert->CertType == kCertType_General &&
                cert->SubjectDN.AttrOID == ASN1::kOID_AttributeType_CommonName)
            {
                cert->CertType = kCertType_AccessToken;
            }
        }
    }

    // Otherwise the device has not been service provisioned...
    else
    {
        // In security test mode, load a predefined set of root and intermediate certificates.
        // Otherwise fail with a NOT_SERVICE_PROVISIONED error.

#if WEAVE_CONFIG_SECURITY_TEST_MODE

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

#else

        ExitNow(err = WEAVE_DEVICE_ERROR_NOT_SERVICE_PROVISIONED);

#endif // WEAVE_CONFIG_SECURITY_TEST_MODE
    }

    memset(&validContext, 0, sizeof(validContext));

    // Set the effective time for certificate validation.
    //
    // If the system's real time clock is synchronized, use the current time.
    //
    // If the system's real time clock is NOT synchronized, use the firmware build time, and arrange to ignore
    // the "not before" date in the peer's certificate(s).
    //
    err = System::Layer::GetClock_RealTimeMS(nowMS);
    if (err == WEAVE_NO_ERROR)
    {
        SecondsSinceEpochToCalendarTime((uint32_t)(nowMS / 1000), validTime.Year, validTime.Month, validTime.Day,
                validTime.Hour, validTime.Minute, validTime.Second);
    }
    else if (err == WEAVE_SYSTEM_ERROR_REAL_TIME_NOT_SYNCED)
    {
        err = ConfigurationMgr().GetFirmwareBuildTime(validTime.Year, validTime.Month, validTime.Day,
                validTime.Hour, validTime.Minute, validTime.Second);
        SuccessOrExit(err);
        validContext.ValidateFlags |= kValidateFlag_IgnoreNotBefore;
        WeaveLogProgress(DeviceLayer, "Real time clock not synchronized; Using build time for cert validation");
    }
    else
    {
        ExitNow();
    }

    err = PackCertTime(validTime, validContext.EffectiveTime);
    SuccessOrExit(err);

    // Set the appropriate required key usages and purposes for the peer's certificates based
    // on whether we're initiating or responding.
    validContext.RequiredKeyUsages = kKeyUsageFlag_DigitalSignature;
    validContext.RequiredKeyPurposes = (isInitiator) ? kKeyPurposeFlag_ServerAuth : kKeyPurposeFlag_ClientAuth;

exit:
    if (err != WEAVE_NO_ERROR && mServiceConfigBuf != NULL)
    {
        MemoryFree(mServiceConfigBuf);
        mServiceConfigBuf = NULL;
    }
    return err;
}

WEAVE_ERROR CASEAuthDelegate::HandleCertValidationResult(bool isInitiator, WEAVE_ERROR& validRes, WeaveCertificateData *peerCert,
        uint64_t peerNodeId, WeaveCertificateSet& certSet, ValidationContext& validContext)
{
    // If the peer's certificate is otherwise valid...
    if (validRes == WEAVE_NO_ERROR)
    {
        // If the peer authenticated with a device certificate...
        if (peerCert->CertType == kCertType_Device)
        {
            // Get the node id from the certificate subject.
            uint64_t certId = peerCert->SubjectDN.AttrValue.WeaveId;

            // Verify the certificate node id matches the peer's node id.
            if (certId != peerNodeId)
                validRes = WEAVE_ERROR_WRONG_CERT_SUBJECT;
        }

        // If the peer authenticated with a service endpoint certificate...
        else if (peerCert->CertType == kCertType_ServiceEndpoint)
        {
            // Get the node id from the certificate subject.
            uint64_t certId = peerCert->SubjectDN.AttrValue.WeaveId;

            // Verify the certificate node id matches the peer's node id.
            if (certId != peerNodeId)
                validRes = WEAVE_ERROR_WRONG_CERT_SUBJECT;

            // Reject the peer if they are initiating the session.  Service endpoint certificates
            // cannot be used to initiate sessions to other nodes, only to respond.
            if (!isInitiator)
                validRes = WEAVE_ERROR_WRONG_CERT_TYPE;
        }

        // If the peer authenticated with an access token certificate...
        else if (peerCert->CertType == kCertType_AccessToken)
        {
            // Reject the peer if they are the session responder.  Access token certificates
            // can only be used to initiate sessions.
            if (isInitiator)
                validRes = WEAVE_ERROR_WRONG_CERT_TYPE;
        }

        // For all other certificate types, reject the session.
        else
        {
            validRes = WEAVE_ERROR_WRONG_CERT_TYPE;
        }
    }

    if (validRes == WEAVE_NO_ERROR)
    {
        WeaveLogProgress(DeviceLayer, "Certificate validation completed successfully");
    }
    else
    {
        WeaveLogError(DeviceLayer, "Certificate validation failed: %s", nl::ErrorStr(validRes));
    }

    return WEAVE_NO_ERROR;
}

WEAVE_ERROR CASEAuthDelegate::EndCertValidation(WeaveCertificateSet& certSet, ValidationContext& validContext)
{
    if (mServiceConfigBuf != NULL)
    {
        MemoryFree(mServiceConfigBuf);
        mServiceConfigBuf = NULL;
    }
    return WEAVE_NO_ERROR;
}

WEAVE_ERROR AddCertToContainer(TLVWriter& writer, uint64_t tag, const uint8_t *cert, uint16_t certLen)
{
    WEAVE_ERROR err;
    TLVReader reader;

    reader.Init(cert, certLen);

    err = reader.Next();
    SuccessOrExit(err);

    err = writer.PutPreEncodedContainer(tag, kTLVType_Structure, reader.GetReadPoint(), reader.GetRemainingLength());
    SuccessOrExit(err);

exit:
    return err;
}

WEAVE_ERROR MakeCertInfo(uint8_t *buf, uint16_t bufSize, uint16_t& certInfoLen,
                         const uint8_t *entityCert, uint16_t entityCertLen,
                         const uint8_t *intermediateCert, uint16_t intermediateCertLen)
{
    WEAVE_ERROR err;
    TLVWriter writer;
    TLVType container;

    writer.Init(buf, bufSize);
    writer.ImplicitProfileId = kWeaveProfile_Security;

    err = writer.StartContainer(ProfileTag(kWeaveProfile_Security, kTag_WeaveCASECertificateInformation), kTLVType_Structure, container);
    SuccessOrExit(err);

    err = AddCertToContainer(writer, ContextTag(kTag_CASECertificateInfo_EntityCertificate), entityCert, entityCertLen);
    SuccessOrExit(err);

    if (intermediateCert != NULL)
    {
        TLVType container2;

        err = writer.StartContainer(ContextTag(kTag_CASECertificateInfo_RelatedCertificates), kTLVType_Path, container2);
        SuccessOrExit(err);

        err = AddCertToContainer(writer, ProfileTag(kWeaveProfile_Security, kTag_WeaveCertificate), intermediateCert, intermediateCertLen);
        SuccessOrExit(err);

        err = writer.EndContainer(container2);
        SuccessOrExit(err);
    }

    err = writer.EndContainer(container);
    SuccessOrExit(err);

    err = writer.Finalize();
    SuccessOrExit(err);

    certInfoLen = writer.GetLengthWritten();

exit:
    return err;

}

WEAVE_ERROR LoadCertsFromServiceConfig(const uint8_t *serviceConfig, uint16_t serviceConfigLen, WeaveCertificateSet& certSet)
{
    WEAVE_ERROR err;
    TLVReader reader;
    TLVType topLevelContainer;

    reader.Init(serviceConfig, serviceConfigLen);
    reader.ImplicitProfileId = kWeaveProfile_ServiceProvisioning;

    err = reader.Next(kTLVType_Structure, ProfileTag(kWeaveProfile_ServiceProvisioning, ServiceProvisioning::kTag_ServiceConfig));
    SuccessOrExit(err);

    err = reader.EnterContainer(topLevelContainer);
    SuccessOrExit(err);

    err = reader.Next(kTLVType_Array, ContextTag(ServiceProvisioning::kTag_ServiceConfig_CACerts));
    SuccessOrExit(err);

    err = certSet.LoadCerts(reader, kDecodeFlag_IsTrusted);
    SuccessOrExit(err);

exit:
    return err;
}

} // unnamed namespace
} // namespace DeviceLayer
} // namespace Weave
} // namespace nl
