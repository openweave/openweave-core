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
 *      This file implements utility functions for outputting
 *      information related to Weave security.
 *
 *      @note These function symbols are only available when
 *            WEAVE_CONFIG_ENABLE_SECURITY_DEBUG_FUNCS has been
 *            asserted.
 *
 */

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif

#include <inttypes.h>
#include <stdio.h>

#include <Weave/Core/WeaveCore.h>
#include <Weave/Profiles/security/WeaveSecurityDebug.h>
#include <Weave/Support/ErrorStr.h>

namespace nl {
namespace Weave {
namespace Profiles {
namespace Security {

using namespace nl::Weave::ASN1;
using namespace nl::Weave::TLV;
using namespace nl::Weave::Profiles;
using namespace nl::Weave::Crypto;

#if WEAVE_CONFIG_ENABLE_SECURITY_DEBUG_FUNCS

static void Indent(FILE *out, uint16_t count)
{
    while (count--)
        fputc(' ', out);
}

static void PrintHexField(FILE *out, const char *name, uint16_t indent, uint16_t count, const uint8_t *data)
{
    Indent(out, indent);
    fprintf(out, "%s: ", name);

    for (uint16_t i = 0; i < count; i++)
    {
        if (i % 16 == 0)
        {
            fprintf(out, "\n");
            Indent(out, indent + 2);
        }

        fprintf(out, "%02X ", data[i]);
    }

    fprintf(out, "\n");
}

static void PrintCertType(FILE *out, uint8_t certType)
{
    const char *certTypeStr;

    switch (certType)
    {
    case kCertType_NotSpecified:
        certTypeStr = "Not specified";
        break;
    case kCertType_General:
        certTypeStr = "General";
        break;
    case kCertType_Device:
        certTypeStr = "Device";
        break;
    case kCertType_ServiceEndpoint:
        certTypeStr = "Service Endpoint";
        break;
    case kCertType_FirmwareSigning:
        certTypeStr = "Firmware Signing";
        break;
    case kCertType_AccessToken:
        certTypeStr = "Access Token";
        break;
    case kCertType_CA:
        certTypeStr = "CA";
        break;
    default:
        if (certType < kCertType_AppDefinedBase)
            fprintf(out, "Unknown (0x%02X)", certType);
        else
            fprintf(out, "Application Defined (0x%02X)", certType);
        return;
    }

    fputs(certTypeStr, out);
}

NL_DLL_EXPORT void PrintCert(FILE *out, const WeaveCertificateData& cert, const WeaveCertificateSet *certSet, uint16_t indent, bool verbose)
{
    Indent(out, indent);
    fprintf(out, "Subject: ");
    if ((cert.CertFlags & kCertFlag_UnsupportedSubjectDN) == 0)
        PrintWeaveDN(out, cert.SubjectDN);
    else
        fprintf(out, "(unsupported DN format)");
    fprintf(out, "\n");

    Indent(out, indent);
    fprintf(out, "Issuer: ");
    if ((cert.CertFlags & kCertFlag_UnsupportedIssuerDN) == 0)
        PrintWeaveDN(out, cert.IssuerDN);
    else
        fprintf(out, "(unsupported DN format)");
    fprintf(out, "\n");

    if (cert.CertFlags & kCertFlag_ExtPresent_SubjectKeyId)
    {
        Indent(out, indent);
        fprintf(out, "Subject Key Id: ");
        for (uint16_t i = 0; i < cert.SubjectKeyId.Len; i++)
            fprintf(out, "%02X", cert.SubjectKeyId.Id[i]);
        fprintf(out, "\n");
    }

    if (cert.CertFlags & kCertFlag_ExtPresent_AuthKeyId)
    {
        Indent(out, indent);
        fprintf(out, "Authority Key Id: ");
        for (uint16_t i = 0; i < cert.AuthKeyId.Len; i++)
            fprintf(out, "%02X", cert.AuthKeyId.Id[i]);
        if (certSet != NULL)
        {
            WeaveCertificateData *authCert = certSet->FindCert(cert.AuthKeyId);
            if (authCert != NULL)
                fprintf(out, " (Cert %u)", (unsigned)(authCert - certSet->Certs));
            else
                fprintf(out, " (no match)");
        }
        fprintf(out, "\n");
    }

    Indent(out, indent);
    fprintf(out, "Validity:\n");
    Indent(out, indent + 2);
    fprintf(out, "Not Before: "); PrintPackedDate(out, cert.NotBeforeDate); fprintf(out, "\n");
    Indent(out, indent + 2);
    fprintf(out, "Not After: "); PrintPackedDate(out, cert.NotAfterDate); fprintf(out, "\n");

    if (cert.CertType != kCertType_NotSpecified)
    {
        Indent(out, indent);
        fprintf(out, "Type: ");
        PrintCertType(out, cert.CertType);
        fprintf(out, "\n");
    }

    if (cert.CertFlags & kCertFlag_IsCA)
    {
        Indent(out, indent);
        fprintf(out, "Is CA: true\n");
    }

    if (cert.CertFlags & kCertFlag_PathLenConstPresent)
    {
        Indent(out, indent);
        fprintf(out, "Path Length Constraint: %u\n", (unsigned)cert.PathLenConstraint);
    }

    if (cert.CertFlags & kCertFlag_IsTrusted)
    {
        Indent(out, indent);
        fprintf(out, "Is Trusted: true\n");
    }

    if (cert.CertFlags & kCertFlag_ExtPresent_KeyUsage)
    {
        Indent(out, indent);
        fprintf(out, "Key Usage: ");
        if (cert.KeyUsageFlags & kKeyUsageFlag_DigitalSignature)
            fprintf(out, "DigitalSignature ");
        if (cert.KeyUsageFlags & kKeyUsageFlag_NonRepudiation)
            fprintf(out, "NonRepudiation ");
        if (cert.KeyUsageFlags & kKeyUsageFlag_KeyEncipherment)
            fprintf(out, "KeyEncipherment ");
        if (cert.KeyUsageFlags & kKeyUsageFlag_DataEncipherment)
            fprintf(out, "DataEncipherment ");
        if (cert.KeyUsageFlags & kKeyUsageFlag_KeyAgreement)
            fprintf(out, "KeyAgreement ");
        if (cert.KeyUsageFlags & kKeyUsageFlag_KeyCertSign)
            fprintf(out, "KeyCertSign ");
        if (cert.KeyUsageFlags & kKeyUsageFlag_CRLSign)
            fprintf(out, "CRLSign ");
        if (cert.KeyUsageFlags & kKeyUsageFlag_EncipherOnly)
            fprintf(out, "EncipherOnly ");
        if (cert.KeyUsageFlags & kKeyUsageFlag_DecipherOnly)
            fprintf(out, "DecipherOnly ");
        fprintf(out, "\n");
    }

    if (cert.CertFlags & kCertFlag_ExtPresent_ExtendedKeyUsage)
    {
        Indent(out, indent);
        fprintf(out, "Key Purpose: ");
        if (cert.KeyPurposeFlags & kKeyPurposeFlag_ServerAuth)
            fprintf(out, "ServerAuth ");
        if (cert.KeyPurposeFlags & kKeyPurposeFlag_ClientAuth)
            fprintf(out, "ClientAuth ");
        if (cert.KeyPurposeFlags & kKeyPurposeFlag_CodeSigning)
            fprintf(out, "CodeSigning ");
        if (cert.KeyPurposeFlags & kKeyPurposeFlag_EmailProtection)
            fprintf(out, "EmailProtection ");
        if (cert.KeyPurposeFlags & kKeyPurposeFlag_TimeStamping)
            fprintf(out, "TimeStamping ");
        if (cert.KeyPurposeFlags & kKeyPurposeFlag_OCSPSigning)
            fprintf(out, "OCSPSigning ");
        fprintf(out, "\n");
    }

    Indent(out, indent);
    fprintf(out, "Public Key Algorithm: %s\n", GetOIDName(cert.PubKeyAlgoOID));

    Indent(out, indent);
    fprintf(out, "Signature Algorithm: %s\n", GetOIDName(cert.SigAlgoOID));

    if (cert.PubKeyAlgoOID == kOID_PubKeyAlgo_ECPublicKey ||
        cert.PubKeyAlgoOID == kOID_PubKeyAlgo_ECDH ||
        cert.PubKeyAlgoOID == kOID_PubKeyAlgo_ECMQV)
    {
        Indent(out, indent);
        fprintf(out, "Curve Id: %s\n", GetOIDName(WeaveCurveIdToOID(cert.PubKeyCurveId)));

        if (verbose)
        {
            PrintHexField(out, "Public Key", indent, cert.PublicKey.EC.ECPointLen, cert.PublicKey.EC.ECPoint);

            Indent(out, indent);
            fprintf(out, "Signature:\n");
            PrintHexField(out, "r", indent + 2, cert.Signature.EC.RLen, cert.Signature.EC.R);
            PrintHexField(out, "s", indent + 2, cert.Signature.EC.SLen, cert.Signature.EC.S);
        }
    }
}

NL_DLL_EXPORT void PrintCertValidationResults(FILE *out, const WeaveCertificateSet& certSet, const ValidationContext& validContext, uint16_t indent)
{
#if WEAVE_CONFIG_DEBUG_CERT_VALIDATION

    WEAVE_ERROR *certValidRes = validContext.CertValidationResults;

    for (uint8_t i = 0; i < certSet.CertCount && i < validContext.CertValidationResultsLen; i++)
    {
        const WeaveCertificateData& cert = certSet.Certs[i];
        Indent(out, indent);
        if (certValidRes[i] == WEAVE_NO_ERROR)
            printf("Cert %d: Validation successful\n", i);
        else if (certValidRes[i] == WEAVE_CERT_NOT_USED)
            printf("Cert %d: Not used during validation\n", i);
        else
            printf("Cert %d: %s\n", i, nl::ErrorStr(certValidRes[i]));
        PrintCert(out, cert, &certSet, indent + 2, false);
        if (&cert == validContext.TrustAnchor)
        {
            Indent(out, indent + 2);
            printf("Is Trust Anchor: true\n");
        }
        printf("\n");
    }

#endif // WEAVE_CONFIG_DEBUG_CERT_VALIDATION
}

void PrintWeaveDN(FILE *out, const WeaveDN& dn)
{
    char valueStr[1024];
    const char *certDesc = NULL;

    if (IsWeaveIdX509Attr(dn.AttrOID))
    {
        snprintf(valueStr, sizeof(valueStr), "%016" PRIX64, dn.AttrValue.WeaveId);
        certDesc = DescribeWeaveCertId(dn.AttrOID, dn.AttrValue.WeaveId);
    }
    else
    {
        uint32_t len = dn.AttrValue.String.Len;
        if (len > sizeof(valueStr) - 1)
            len = sizeof(valueStr) - 1;
        memcpy(valueStr, dn.AttrValue.String.Value, len);
        valueStr[len] = 0;
    }

    fprintf(out, "%s=%s", nl::Weave::ASN1::GetOIDName((OID)dn.AttrOID), valueStr);
    if (certDesc != NULL)
        fprintf(out, " (%s)", certDesc);
}

WEAVE_ERROR PrintWeaveDN(FILE *out, TLVReader & reader)
{
    WEAVE_ERROR err;
    WeaveDN dn;

    err = DecodeWeaveDN(reader, dn);
    SuccessOrExit(err);

    PrintWeaveDN(out, dn);

exit:
    return err;
}

void PrintPackedTime(FILE *out, uint32_t t)
{
    nl::Weave::ASN1::ASN1UniversalTime asn1Time;
    UnpackCertTime(t, asn1Time);
    fprintf(out, "%04" PRId16 "/%02" PRId8 "/%02" PRId8 " %02" PRId8 ":%02" PRId8 ":%02" PRId8 "",
            asn1Time.Year, asn1Time.Month, asn1Time.Day,
            asn1Time.Hour, asn1Time.Minute, asn1Time.Second);
}

void PrintPackedDate(FILE *out, uint16_t t)
{
    nl::Weave::ASN1::ASN1UniversalTime asn1Time;
    UnpackCertTime(PackedCertDateToTime(t), asn1Time);
    fprintf(out, "%04" PRId16 "/%02" PRId8 "/%02" PRId8,
            asn1Time.Year, asn1Time.Month, asn1Time.Day);
}

const char *DescribeWeaveCertId(OID attrOID, uint64_t weaveCertId)
{
    switch (attrOID)
    {
    case kOID_AttributeType_WeaveCAId:
        if (weaveCertId == 0x18B430EE00000001ULL)
            return "Nest Production Root";
        if (weaveCertId == 0x18B430EE00000002ULL)
            return "Nest Production Device CA";
        if (weaveCertId == 0x18B430EE00000003ULL)
            return "Nest Production Service Endpoint CA";
        if (weaveCertId == 0x18B430EE00000004ULL)
            return "Nest Production Firmware Signing CA";
        if (weaveCertId == 0x18B430EEEE000001ULL)
            return "Nest Development Root";
        if (weaveCertId == 0x18B430EEEE000002ULL)
            return "Nest Development Device CA";
        if (weaveCertId == 0x18B430EEEE000003ULL)
            return "Nest Development Service Endpoint CA";
        if (weaveCertId == 0x18B430EEEE000004ULL)
            return "Nest Development Firmware Signing CA";
        return NULL;
    case kOID_AttributeType_WeaveDeviceId:
        return "Device";
    case kOID_AttributeType_WeaveServiceEndpointId:
        if (weaveCertId == 0x18B4300200000001ULL)
            return "Nest Directory Endpoint";
        if (weaveCertId == 0x18B4300200000002ULL)
            return "Nest Software Update Endpoint";
        if (weaveCertId == 0x18B4300200000003ULL)
            return "Nest Data Management Endpoint";
        if (weaveCertId == 0x18B4300200000004ULL)
            return "Nest Log Upload Endpoint";
        if (weaveCertId == 0x18B4300200000005ULL)
            return "Nest Time Service Endpoint";
        if (weaveCertId == 0x18B4300200000010ULL)
            return "Nest Service Provisioning Endpoint";
        if (weaveCertId == 0x18B4300200000011ULL)
            return "Nest Weave Tunnel Endpoint";
        if (weaveCertId == 0x18B4300200000012ULL)
            return "Nest Service Router Endpoint";
        if (weaveCertId == 0x18B4300200000013ULL)
            return "Nest File Download Endpoint";
        if (weaveCertId == 0x18B4300200000014ULL)
            return "Nest Bastion Service Endpoint";
        return NULL;
    case kOID_AttributeType_WeaveSoftwarePublisherId:
        if (weaveCertId == 0x18B4300301000001ULL)
            return "Nest Production Firmware Signing";
        if (weaveCertId == 0x18B4300302000001ULL)
            return "Nest Development Firmware Signing";
    default:
        return NULL;
    }
}

WEAVE_ERROR PrintCertArray(FILE * out, TLVReader & reader, uint16_t indent)
{
    WEAVE_ERROR err;
    TLVType outerContainerType;
    uint32_t certNum = 1;
    WeaveCertificateData cert;

    if (reader.GetType() == kTLVType_NotSpecified)
    {
        err = reader.Next();
        SuccessOrExit(err);
    }

    VerifyOrExit(reader.GetType() == kTLVType_Array, err = WEAVE_ERROR_INVALID_ARGUMENT);

    err = reader.EnterContainer(outerContainerType);
    SuccessOrExit(err);

    while ((err = reader.Next()) == WEAVE_NO_ERROR)
    {
        VerifyOrExit(reader.GetType() == kTLVType_Structure, err = WEAVE_ERROR_INVALID_ARGUMENT);

        Indent(out, indent);
        fprintf(out, "Certificate %" PRId32 ":\n", certNum);

        err = DecodeWeaveCert(reader, cert);
        SuccessOrExit(err);

        err = DetermineCertType(cert);
        SuccessOrExit(err);

        PrintCert(out, cert, NULL, indent + 2, true);

        certNum++;
    }

    VerifyOrExit(err == WEAVE_END_OF_TLV, /**/);

    err = reader.ExitContainer(kTLVType_Array);
    SuccessOrExit(err);

exit:
    return err;
}

WEAVE_ERROR PrintECDSASignature(FILE * out, TLVReader & reader, uint16_t indent)
{
    WEAVE_ERROR err;
    TLVType outerContainerType;

    if (reader.GetType() == kTLVType_NotSpecified)
    {
        err = reader.Next();
        SuccessOrExit(err);
    }

    // Verify the start of the ECDSASignture structure.
    VerifyOrExit(reader.GetType() == kTLVType_Structure, err = WEAVE_ERROR_INVALID_ARGUMENT);

    err = reader.EnterContainer(outerContainerType);
    SuccessOrExit(err);

    while ((err = reader.Next()) == WEAVE_NO_ERROR)
    {
        uint64_t tag;
        uint8_t tagNum;
        const char * label;
        const uint8_t * data;

        tag = reader.GetTag();

        if (!IsContextTag(tag))
        {
            continue;
        }

        tagNum = TagNumFromTag(tag);

        switch (tagNum)
        {
        case kTag_ECDSASignature_r:
            label = "r";
            break;
        case kTag_ECDSASignature_s:
            label = "s";
            break;
        default:
            continue;
        }

        VerifyOrExit(reader.GetType() == kTLVType_ByteString, err = WEAVE_ERROR_INVALID_ARGUMENT);

        err = reader.GetDataPtr(data);
        SuccessOrExit(err);

        PrintHexField(out, label, indent, reader.GetLength(), data);
    }

    VerifyOrExit(err == WEAVE_END_OF_TLV, /**/);

    err = reader.ExitContainer(kTLVType_Structure);
    SuccessOrExit(err);

exit:
    return err;
}

WEAVE_ERROR PrintCertReference(FILE * out, TLVReader & reader, uint16_t indent)
{
    WEAVE_ERROR err;
    TLVType outerContainerType;

    if (reader.GetType() == kTLVType_NotSpecified)
    {
        err = reader.Next();
        SuccessOrExit(err);
    }

    // Verify the start of the WeaveECDSASignture structure.
    VerifyOrExit(reader.GetType() == kTLVType_Structure, err = WEAVE_ERROR_INVALID_ARGUMENT);

    err = reader.EnterContainer(outerContainerType);
    SuccessOrExit(err);

    while ((err = reader.Next()) == WEAVE_NO_ERROR)
    {
        uint64_t tag;
        uint8_t tagNum;
        const uint8_t * data;
        uint32_t dataLen;

        tag = reader.GetTag();

        if (!IsContextTag(tag))
        {
            continue;
        }

        tagNum = TagNumFromTag(tag);

        switch (tagNum)
        {
        case kTag_WeaveCertificateRef_Subject:
            Indent(out, indent);
            fprintf(out, "Subject DN: ");
            err = PrintWeaveDN(out, reader);
            SuccessOrExit(err);
            break;
        case kTag_WeaveCertificateRef_PublicKeyId:
            VerifyOrExit(reader.GetType() == kTLVType_ByteString, err = WEAVE_ERROR_INVALID_ARGUMENT);
            err = reader.GetDataPtr(data);
            SuccessOrExit(err);
            dataLen = reader.GetLength();
            Indent(out, indent);
            fprintf(out, "Public Key Id: ");
            for (uint32_t i = 0; i < dataLen; i++)
                fprintf(out, "%02X", data[i]);
            fprintf(out, "\n");
            break;
        default:
            break;
        }
    }

    VerifyOrExit(err == WEAVE_END_OF_TLV, /**/);

    err = reader.ExitContainer(kTLVType_Structure);
    SuccessOrExit(err);

exit:
    return err;
}

WEAVE_ERROR PrintWeaveSignature(FILE * out, TLVReader & reader, uint16_t indent)
{
    WEAVE_ERROR err;
    TLVType outerContainerType;
    bool sigAlgoPrinted = false;

    if (reader.GetType() == kTLVType_NotSpecified)
    {
        err = reader.Next();
        SuccessOrExit(err);
    }

    // Verify the start of the WeaveSignature structure.
    VerifyOrExit(reader.GetType() == kTLVType_Structure, err = WEAVE_ERROR_INVALID_ARGUMENT);

    err = reader.EnterContainer(outerContainerType);
    SuccessOrExit(err);

    while ((err = reader.Next()) == WEAVE_NO_ERROR)
    {
        uint64_t tag = reader.GetTag();
        uint8_t tagNum;

        if (!IsContextTag(tag))
        {
            continue;
        }

        tagNum = TagNumFromTag(tag);

        if (!sigAlgoPrinted && tagNum != kTag_WeaveSignature_SignatureAlgorithm)
        {
            Indent(out, indent);
            fprintf(out, "Signature Algorithm: ECDSAWithSHA1 (implicit)\n");
            sigAlgoPrinted = true;
        }

        switch (tagNum)
        {
        case kTag_WeaveSignature_ECDSASignatureData:
            Indent(out, indent);
            fprintf(out, "ECDSA Signature:\n");
            err = PrintECDSASignature(out, reader, indent + 2);
            SuccessOrExit(err);
            break;

        case kTag_WeaveSignature_SigningCertificateRef:
            Indent(out, indent);
            fprintf(out, "Signing Certificate Reference:\n");
            err = PrintCertReference(out, reader, indent + 2);
            SuccessOrExit(err);
            break;

        case kTag_WeaveSignature_RelatedCertificates:
            Indent(out, indent);
            fprintf(out, "Related Certificates:\n");
            err = PrintCertArray(out, reader, indent + 2);
            SuccessOrExit(err);
            break;

        case kTag_WeaveSignature_SignatureAlgorithm:
        {
            uint16_t sigAlgo;

            VerifyOrExit(reader.GetType() == kTLVType_UnsignedInteger, err = WEAVE_ERROR_INVALID_ARGUMENT);

            err = reader.Get(sigAlgo);
            SuccessOrExit(err);

            Indent(out, indent);
            fprintf(out, "Signature Algorithm: %s\n", GetOIDName(sigAlgo));

            sigAlgoPrinted = true;

            break;
        }

        default:
            break;
        }
    }

    VerifyOrExit(err == WEAVE_END_OF_TLV, /**/);

    err = reader.ExitContainer(kTLVType_Structure);
    SuccessOrExit(err);

exit:
    return err;
}

#endif // WEAVE_CONFIG_ENABLE_SECURITY_DEBUG_FUNCS

} // namespace Security
} // namespace Profiles
} // namespace Weave
} // namespace nl
