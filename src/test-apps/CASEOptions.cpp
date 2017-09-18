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
 *      Implementation of CASEConfig object, which provides an implementation of the WeaveCASEAuthDelegate
 *      interface for use in test applications.
 *
 */

#include "stdio.h"

#include "ToolCommon.h"
#include <Weave/Support/CodeUtils.h>
#include <Weave/Core/WeaveTLV.h>
#include <Weave/Profiles/WeaveProfiles.h>
#include <Weave/Profiles/security/WeaveSecurityDebug.h>
#include <Weave/Profiles/service-provisioning/ServiceProvisioning.h>
#include <Weave/Support/NestCerts.h>
#include <Weave/Support/ErrorStr.h>
#include <Weave/Support/ASN1.h>
#include <Weave/Support/Base64.h>
#include "CASEOptions.h"

using namespace nl::Weave;
using namespace nl::Weave::TLV;
using namespace nl::Weave::ASN1;
using namespace nl::Weave::Profiles;
using namespace nl::Weave::Profiles::Security;
using namespace nl::Weave::Profiles::Security::CASE;

CASEOptions gCASEOptions;

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
    nl::Weave::TLV::TLVReader reader;
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

bool ParseCASEConfig(const char *str, uint32_t& output)
{
    uint32_t configNum;

    if (!ParseInt(str, configNum))
        return false;

    switch (configNum)
    {
    case 1:
        output = kCASEConfig_Config1;
        return true;
    case 2:
        output = kCASEConfig_Config2;
        return true;
    default:
        return false;
    }
}

// Parse a sequence of zero or more unsigned integers corresponding to a list
// of allowed CASE configurations.  Integer values must be separated by either
// a comma or a space.
bool ParseAllowedCASEConfigs(const char *strConst, uint8_t& output)
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
            output |= kCASEAllowedConfig_Config1;
        else if (configNum == 2)
            output |= kCASEAllowedConfig_Config2;
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

CASEOptions::CASEOptions()
{
    static OptionDef optionDefs[] =
    {
#if WEAVE_CONFIG_ENABLE_CASE_INITIATOR || WEAVE_CONFIG_ENABLE_CASE_RESPONDER
        { "node-cert",              kArgumentRequired,      kToolCommonOpt_NodeCert             },
        { "node-key",               kArgumentRequired,      kToolCommonOpt_NodeKey              },
        { "ca-cert",                kArgumentRequired,      kToolCommonOpt_CACert               },
        { "no-ca-cert",             kNoArgument,            kToolCommonOpt_NoCACert             },
        { "case-config",            kArgumentRequired,      kToolCommonOpt_CASEConfig           },
        { "allowed-case-configs",   kArgumentRequired,      kToolCommonOpt_AllowedCASEConfigs   },
        { "debug-case",             kNoArgument,            kToolCommonOpt_DebugCASE            },
#if WEAVE_CONFIG_SECURITY_TEST_MODE
        { "case-use-known-key",     kNoArgument,            kToolCommonOpt_CASEUseKnownECDHKey  },
#endif // WEAVE_CONFIG_SECURITY_TEST_MODE
#endif // WEAVE_CONFIG_ENABLE_CASE_INITIATOR || WEAVE_CONFIG_ENABLE_CASE_RESPONDER
        { NULL }
    };
    OptionDefs = optionDefs;

    HelpGroupName = "CASE OPTIONS";

    OptionHelp =
#if WEAVE_CONFIG_ENABLE_CASE_INITIATOR || WEAVE_CONFIG_ENABLE_CASE_RESPONDER
        "  --node-cert <cert-file>\n"
        "       File containing a Weave certificate to be used to authenticate the node\n"
        "       when establishing a CASE session. The file can contain either raw TLV or\n"
        "       base-64.\n"
        "\n"
        "  --node-key <key-file>\n"
        "       File containing a private key to be used to authenticate the node\n"
        "       when establishing a CASE session. The file can contain either raw TLV or\n"
        "       base-64.\n"
        "\n"
        "  --ca-cert <cert-file>\n"
        "       File containing a Weave CA certificate to be included along with the\n"
        "       node's certificate when establishing a CASE session. The file can contain\n"
        "       either raw TLV or base-64.\n"
        "\n"
        "  --no-ca-cert\n"
        "       Do not send an intermediate certificate when establishing a CASE session.\n"
        "\n"
        "  --case-config <int>\n"
        "       Proposed the specified CASE configuration when initiating a CASE session.\n"
        "\n"
        "  --allowed-case-configs <int>[,<int>]\n"
        "       Accept the specified set of CASE configurations when either initiating or\n"
        "       responding to a CASE session.\n"
        "\n"
        "  --debug-case\n"
        "       Enable CASE debug messages.\n"
#if WEAVE_CONFIG_SECURITY_TEST_MODE
        "\n"
        "  --case-use-known-key\n"
        "       Enable use of known ECDH key in CASE.\n"
#endif // WEAVE_CONFIG_SECURITY_TEST_MODE
        "\n"
#endif // WEAVE_CONFIG_ENABLE_CASE_INITIATOR || WEAVE_CONFIG_ENABLE_CASE_RESPONDER
        "";

    // Defaults
    InitiatorCASEConfig = kCASEConfig_NotSpecified;
    AllowedCASEConfigs = 0; // 0 causes code to use default value provided by WeaveSecurityManager
    NodeCert = NULL;
    NodeCertLength = 0;
    NodePrivateKey = NULL;
    NodePrivateKeyLength = 0;
    NodeIntermediateCert = NULL;
    NodeIntermediateCertLength = 0;
    ServiceConfig = NULL;
    ServiceConfigLength = 0;
    NodePayload = NULL;
    NodePayloadLength = 0;
    Debug = false;
#if WEAVE_CONFIG_SECURITY_TEST_MODE
    UseKnownECDHKey = false;
#endif
}

bool CASEOptions::HandleOption(const char *progName, OptionSet *optSet, int id, const char *name, const char *arg)
{
    switch (id)
    {
#if WEAVE_CONFIG_ENABLE_CASE_INITIATOR || WEAVE_CONFIG_ENABLE_CASE_RESPONDER

    case kToolCommonOpt_NodeCert:
        if (!ReadCertFile(arg, (uint8_t *&)NodeCert, NodeCertLength))
            return false;
        break;
    case kToolCommonOpt_NodeKey:
        if (!ReadPrivateKeyFile(arg, (uint8_t *&)NodePrivateKey, NodePrivateKeyLength))
            return false;
        break;
    case kToolCommonOpt_CACert:
        if (!ReadCertFile(arg, (uint8_t *&)NodeIntermediateCert, NodeIntermediateCertLength))
            return false;
        break;
    case kToolCommonOpt_NoCACert:
        NodeIntermediateCert = NULL;
        NodeIntermediateCertLength = 0;
        break;
    case kToolCommonOpt_CASEConfig:
        if (!ParseCASEConfig(arg, InitiatorCASEConfig))
        {
            PrintArgError("%s: Invalid value specified for CASE config: %s\n", progName, arg);
            return false;
        }
        break;
    case kToolCommonOpt_AllowedCASEConfigs:
        if (!ParseAllowedCASEConfigs(arg, AllowedCASEConfigs))
        {
            PrintArgError("%s: Invalid value specified for allowed CASE configs: %s\n", progName, arg);
            return false;
        }
        break;
    case kToolCommonOpt_DebugCASE:
        Debug = true;
        break;
#if WEAVE_CONFIG_SECURITY_TEST_MODE
    case kToolCommonOpt_CASEUseKnownECDHKey:
        UseKnownECDHKey = true;
        break;
#endif //WEAVE_CONFIG_SECURITY_TEST_MODE

#endif // WEAVE_CONFIG_ENABLE_CASE_INITIATOR || WEAVE_CONFIG_ENABLE_CASE_RESPONDER

    default:
        PrintArgError("%s: INTERNAL ERROR: Unhandled option: %s\n", progName, name);
        return false;
    }

    return true;
}

bool CASEOptions::ReadCertFile(const char *fileName, uint8_t *& certBuf, uint16_t& certLen)
{
    uint32_t len;

    static const char *certB64Prefix = "1QAABAAB";
    static const size_t certB64PrefixLen = sizeof(certB64Prefix) - 1;

    // Read the specified file into a malloced buffer.
    certBuf = ReadFileArg(fileName, len, UINT16_MAX);
    if (certBuf == NULL)
        return false;

    // If the certificate is in base-64 format, convert it to raw TLV.
    if (len > certB64PrefixLen && memcmp(certBuf, certB64Prefix, certB64PrefixLen) == 0)
    {
        len = nl::Base64Decode((const char *)certBuf, len, (uint8_t *)certBuf);
        if (len == UINT16_MAX)
        {
            printf("Invalid certificate format: %s\n", fileName);
            free(certBuf);
            certBuf = NULL;
            return false;
        }
    }

    certLen = (uint16_t)len;

    return true;
}

bool CASEOptions::ReadPrivateKeyFile(const char *fileName, uint8_t *& keyBuf, uint16_t& keyLen)
{
    uint32_t len;

    static const char *keyB64Prefix = "1QAABAAC";
    static const size_t keyB64PrefixLen = sizeof(keyB64Prefix) - 1;

    // Read the specified file into a malloced buffer.
    keyBuf = ReadFileArg(fileName, len, UINT16_MAX);
    if (keyBuf == NULL)
        return false;

    // If the private key is in base-64 format, convert it to raw TLV.
    if (len > keyB64PrefixLen && memcmp(keyBuf, keyB64Prefix, keyB64PrefixLen) == 0)
    {
        len = nl::Base64Decode((const char *)keyBuf, len, (uint8_t *)keyBuf);
        if (len == UINT16_MAX)
        {
            printf("Invalid private key format: %s\n", fileName);
            free(keyBuf);
            keyBuf = NULL;
            return false;
        }
    }

    keyLen = (uint16_t)len;

    return true;
}

// Get the CASE Certificate Information structure for the local node.
WEAVE_ERROR CASEOptions::GetNodeCertInfo(bool isInitiator, uint8_t *buf, uint16_t bufSize, uint16_t& certInfoLen)
{
    const uint8_t *nodeCert = NodeCert;
    uint16_t nodeCertLen = NodeCertLength;
    const uint8_t *intCert = NodeIntermediateCert;
    uint16_t intCertLen = NodeIntermediateCertLength;

    if (nodeCert == NULL || nodeCertLen == 0)
    {
        GetTestNodeCert(FabricState.LocalNodeId, nodeCert, nodeCertLen);
    }
    if (nodeCert == NULL || nodeCertLen == 0)
    {
        printf("ERROR: Node certificate not configured\n");
        return WEAVE_ERROR_CERT_NOT_FOUND;
    }
    if (intCert == NULL || intCertLen == 0)
    {
        intCert = nl::NestCerts::Development::DeviceCA::Cert;
        intCertLen = nl::NestCerts::Development::DeviceCA::CertLength;
    }
    return MakeCertInfo(buf, bufSize, certInfoLen, nodeCert, nodeCertLen, intCert, intCertLen);
}

// Get the local node's private key.
WEAVE_ERROR CASEOptions::GetNodePrivateKey(bool isInitiator, const uint8_t *& weavePrivKey, uint16_t& weavePrivKeyLen)
{
    weavePrivKey = NodePrivateKey;
    weavePrivKeyLen = NodePrivateKeyLength;

    if (weavePrivKey == NULL || weavePrivKeyLen == 0)
    {
        GetTestNodePrivateKey(FabricState.LocalNodeId, weavePrivKey, weavePrivKeyLen);
    }
    if (weavePrivKey == NULL || weavePrivKeyLen == 0)
    {
        printf("ERROR: Node private key not configured\n");
        return WEAVE_ERROR_KEY_NOT_FOUND;
    }
    return WEAVE_NO_ERROR;
}

// Called when the CASE engine is done with the buffer returned by GetNodePrivateKey().
WEAVE_ERROR CASEOptions::ReleaseNodePrivateKey(const uint8_t *weavePrivKey)
{
    // nothing to do
    return WEAVE_NO_ERROR;
}

// Get payload information, if any, to be included in the message to the peer.
WEAVE_ERROR CASEOptions::GetNodePayload(bool isInitiator, uint8_t *buf, uint16_t bufSize, uint16_t& payloadLen)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    if (NodePayload != NULL)
    {
        VerifyOrExit(NodePayloadLength <= bufSize, err = WEAVE_ERROR_BUFFER_TOO_SMALL);

        memcpy(buf, NodePayload, NodePayloadLength);
        payloadLen = NodePayloadLength;
    }

    else
    {
        WeaveDeviceDescriptor deviceDesc;
        uint32_t encodedDescLen;

        gDeviceDescOptions.GetDeviceDesc(deviceDesc);

        err = WeaveDeviceDescriptor::EncodeTLV(deviceDesc, buf, bufSize, encodedDescLen);
        SuccessOrExit(err);

        payloadLen = encodedDescLen;
    }

exit:
    return err;
}

// Prepare the supplied certificate set and validation context for use in validating the certificate of a peer.
// This method is responsible for loading the trust anchors into the certificate set.
WEAVE_ERROR CASEOptions::BeginCertValidation(bool isInitiator, WeaveCertificateSet& certSet, ValidationContext& validContext)
{
    WEAVE_ERROR err;
    WeaveCertificateData *cert;

    err = certSet.Init(10, 1024);
    SuccessOrExit(err);

    if (ServiceConfig != NULL)
    {
        err = LoadCertsFromServiceConfig(ServiceConfig, ServiceConfigLength, certSet);
        SuccessOrExit(err);

        // Scan the list of trusted certs loaded from the service config.  If the list contains a general
        // certificate with a CommonName subject, presume this certificate is the access token certificate.
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

    else
    {
        err = certSet.LoadCert(nl::NestCerts::Development::Root::Cert, nl::NestCerts::Development::Root::CertLength, 0, cert);
        SuccessOrExit(err);
        cert->CertFlags |= kCertFlag_IsTrusted;

        err = certSet.LoadCert(nl::NestCerts::Production::Root::Cert, nl::NestCerts::Production::Root::CertLength, 0, cert);
        SuccessOrExit(err);
        cert->CertFlags |= kCertFlag_IsTrusted;

        const uint8_t *caCert;
        uint16_t caCertLen;

        GetTestCACert(TestMockRoot_CAId, caCert, caCertLen);

        err = certSet.LoadCert(caCert, caCertLen, 0, cert);
        SuccessOrExit(err);
        cert->CertFlags |= kCertFlag_IsTrusted;

        err = certSet.LoadCert(nl::NestCerts::Development::DeviceCA::Cert, nl::NestCerts::Development::DeviceCA::CertLength, kDecodeFlag_GenerateTBSHash, cert);
        SuccessOrExit(err);

        err = certSet.LoadCert(nl::NestCerts::Production::DeviceCA::Cert, nl::NestCerts::Production::DeviceCA::CertLength, kDecodeFlag_GenerateTBSHash, cert);
        SuccessOrExit(err);

        GetTestCACert(TestMockServiceEndpointCA_CAId, caCert, caCertLen);

        err = certSet.LoadCert(caCert, caCertLen, kDecodeFlag_GenerateTBSHash, cert);
        SuccessOrExit(err);
    }

    memset(&validContext, 0, sizeof(validContext));
    validContext.EffectiveTime = SecondsSinceEpochToPackedCertTime(time(NULL));
    validContext.RequiredKeyUsages = kKeyUsageFlag_DigitalSignature;
    validContext.RequiredKeyPurposes = (isInitiator) ? kKeyPurposeFlag_ServerAuth : kKeyPurposeFlag_ClientAuth;

    if (Debug)
    {
        validContext.CertValidationResults = (WEAVE_ERROR *)malloc(sizeof(WEAVE_ERROR) * certSet.MaxCerts);
        validContext.CertValidationResultsLen = certSet.MaxCerts;
    }

exit:
    return err;
}

// Called when peer certificate validation is complete.
WEAVE_ERROR CASEOptions::HandleCertValidationResult(bool isInitiator, WEAVE_ERROR& validRes, WeaveCertificateData *peerCert,
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

            // This is a work-around for DVT devices that were built with incorrect certificates.
            // Specifically, the device id in the certificate didn't include Nest's OUI (the first
            // 3 bytes of the EUI-64 that makes up the id). Here we grandfather these in by assuming
            // anything that has an OUI of 0 is in fact a Nest device.
            if ((certId & 0xFFFFFF0000000000ULL) == 0)
                certId |= 0x18b4300000000000ULL;

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

    if (Debug)
    {
        if (validRes == WEAVE_NO_ERROR)
            printf("Certificate validation completed successfully\n");
        else
            printf("Certificate validation failed: %s\n", nl::ErrorStr(validRes));

        if (peerCert != NULL)
            printf("Peer certificate: %d\n", (int)(peerCert - certSet.Certs));

        printf("\nValidation results:\n\n");
        PrintCertValidationResults(stdout, certSet, validContext, 2);
    }

    return WEAVE_NO_ERROR;
}

// Called when peer certificate validation is complete.
WEAVE_ERROR CASEOptions::EndCertValidation(WeaveCertificateSet& certSet, ValidationContext& validContext)
{
    if (Debug)
        free(validContext.CertValidationResults);
    return WEAVE_NO_ERROR;
}
