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
 *      This file implements methods for converting a standard X.509
 *      certificate to a Weave TLV-encoded certificate.
 *
 */

#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif
#include <stdint.h>

#include <Weave/Core/WeaveCore.h>
#include <Weave/Profiles/WeaveProfiles.h>
#include <Weave/Profiles/security/WeaveSecurity.h>
#include <Weave/Profiles/security/WeaveCert.h>
#include <Weave/Support/ASN1.h>
#include <Weave/Support/ASN1Macros.h>
#include <Weave/Core/WeaveTLV.h>
#include <Weave/Support/CodeUtils.h>

namespace nl {
namespace Weave {
namespace Profiles {
namespace Security {

using namespace nl::Weave::ASN1;
using namespace nl::Weave::TLV;
using namespace nl::Weave::Profiles;


static WEAVE_ERROR ParseWeaveIdAttribute(ASN1Reader& reader, uint64_t& weaveIdOut)
{
    if (reader.ValueLen != 16)
        return ASN1_ERROR_INVALID_ENCODING;

    weaveIdOut = 0;
    for (int i = 0; i < 16; i++)
    {
        weaveIdOut <<= 4;
        uint8_t ch = reader.Value[i];
        if (ch >= '0' && ch <= '9')
            weaveIdOut |= (ch - '0');
        else if (ch >= 'A' && ch <= 'F')
            weaveIdOut |= (ch - 'A' + 10);
        else
            return ASN1_ERROR_INVALID_ENCODING;
    }

    return ASN1_NO_ERROR;
}

static WEAVE_ERROR ConvertDistinguishedName(ASN1Reader& reader, TLVWriter& writer, uint64_t tag)
{
    WEAVE_ERROR err;
    TLVType outerContainer;
    OID attrOID;

    err = writer.StartContainer(tag, kTLVType_Path, outerContainer);
    SuccessOrExit(err);

    // RDNSequence ::= SEQUENCE OF RelativeDistinguishedName
    ASN1_PARSE_ENTER_SEQUENCE {

        while ((err = reader.Next()) == ASN1_NO_ERROR)
        {
            // RelativeDistinguishedName ::= SET SIZE (1..MAX) OF AttributeTypeAndValue
            ASN1_ENTER_SET {

                // AttributeTypeAndValue ::= SEQUENCE
                ASN1_PARSE_ENTER_SEQUENCE {

                    // type AttributeType
                    // AttributeType ::= OBJECT IDENTIFIER
                    ASN1_PARSE_OBJECT_ID(attrOID);
                    VerifyOrExit(GetOIDCategory(attrOID) == kOIDCategory_AttributeType, err = ASN1_ERROR_INVALID_ENCODING);

                    // AttributeValue ::= ANY -- DEFINED BY AttributeType
                    ASN1_PARSE_ANY;

                    // Can only support UTF8String, PrintableString and IA5String.
                    VerifyOrExit(reader.Class == kASN1TagClass_Universal &&
                                 (reader.Tag == kASN1UniversalTag_PrintableString || reader.Tag == kASN1UniversalTag_UTF8String || reader.Tag == kASN1UniversalTag_IA5String),
                                 err = ASN1_ERROR_UNSUPPORTED_ENCODING);

                    // Weave id attributes must be UTF8Strings.
                    if (IsWeaveX509Attr(attrOID))
                        VerifyOrExit(reader.Tag == kASN1UniversalTag_UTF8String, err = ASN1_ERROR_INVALID_ENCODING);

                    // Derive the TLV tag number from the enum value assigned to the attribute type OID. For attributes that can be
                    // either UTF8String or PrintableString, use the high bit in the tag number to distinguish the two.
                    uint8_t tlvTagNum = attrOID & kOID_Mask;
                    if (reader.Tag == kASN1UniversalTag_PrintableString)
                        tlvTagNum |= 0x80;

                    // If the attribute is a Weave-defined attribute that contains a 64-bit Weave id...
                    if (IsWeaveIdX509Attr(attrOID))
                    {
                        // Parse the attribute string into a 64-bit weave id.
                        uint64_t weaveId;
                        err = ParseWeaveIdAttribute(reader, weaveId);
                        SuccessOrExit(err);

                        // Write the weave id into the TLV.
                        err = writer.Put(ContextTag(tlvTagNum), weaveId);
                        SuccessOrExit(err);
                    }

                    //
                    else
                    {
                        err = writer.PutString(ContextTag(tlvTagNum), (const char *)reader.Value, reader.ValueLen);
                        SuccessOrExit(err);
                    }

                } ASN1_EXIT_SEQUENCE;

                // Only one AttributeTypeAndValue allowed per RDN.
                err = reader.Next();
                if (err == ASN1_NO_ERROR)
                    ExitNow(err = ASN1_ERROR_UNSUPPORTED_ENCODING);
                if (err != ASN1_END)
                    ExitNow();

            } ASN1_EXIT_SET;
        }

    } ASN1_EXIT_SEQUENCE;

    err = writer.EndContainer(outerContainer);
    SuccessOrExit(err);

exit:
    return err;
}

static WEAVE_ERROR ConvertValidity(ASN1Reader& reader, TLVWriter& writer)
{
    WEAVE_ERROR err;
    ASN1UniversalTime notBeforeTime, notAfterTime;
    uint32_t packedNotBeforeTime, packedNotAfterTime;

    ASN1_PARSE_ENTER_SEQUENCE {

        ASN1_PARSE_TIME(notBeforeTime);
        err = PackCertTime(notBeforeTime, packedNotBeforeTime);
        SuccessOrExit(err);
        err = writer.Put(ContextTag(kTag_NotBefore), packedNotBeforeTime);
        SuccessOrExit(err);

        ASN1_PARSE_TIME(notAfterTime);
        err = PackCertTime(notAfterTime, packedNotAfterTime);
        SuccessOrExit(err);
        err = writer.Put(ContextTag(kTag_NotAfter), packedNotAfterTime);
        SuccessOrExit(err);

    } ASN1_EXIT_SEQUENCE;

exit:
    return err;
}

static WEAVE_ERROR ConvertAuthorityKeyIdentifierExtension(ASN1Reader& reader, TLVWriter& writer)
{
    WEAVE_ERROR err;

    // AuthorityKeyIdentifier ::= SEQUENCE
    ASN1_PARSE_ENTER_SEQUENCE {

        err = reader.Next();

        // keyIdentifier [0] IMPLICIT KeyIdentifier OPTIONAL,
        // KeyIdentifier ::= OCTET STRING
        if (err == ASN1_NO_ERROR && reader.Class == kASN1TagClass_ContextSpecific && reader.Tag == 0)
        {
            VerifyOrExit(reader.IsConstructed == false, err = ASN1_ERROR_INVALID_ENCODING);

            err = writer.PutBytes(ContextTag(kTag_AuthorityKeyIdentifier_KeyIdentifier), reader.Value, reader.ValueLen);
            SuccessOrExit(err);

            err = reader.Next();
        }

        // authorityCertIssuer [1] IMPLICIT GeneralNames OPTIONAL,
        // GeneralNames ::= SEQUENCE SIZE (1..MAX) OF GeneralName
        if (err == ASN1_NO_ERROR && reader.Class == kASN1TagClass_ContextSpecific && reader.Tag == 1)
        {
            ASN1_ENTER_CONSTRUCTED(kASN1TagClass_ContextSpecific, 1) {

                // GeneralName ::= CHOICE {
                //     directoryName [4] EXPLICIT Name
                // }
                ASN1_PARSE_ENTER_CONSTRUCTED(kASN1TagClass_ContextSpecific, 4) {

                    err = ConvertDistinguishedName(reader, writer, ContextTag(kTag_AuthorityKeyIdentifier_Issuer));
                    SuccessOrExit(err);

                } ASN1_EXIT_CONSTRUCTED;

                // Only one directoryName allowed.
                err = reader.Next();
                if (err == ASN1_NO_ERROR)
                    ExitNow(err = ASN1_ERROR_UNSUPPORTED_ENCODING);

            } ASN1_EXIT_CONSTRUCTED;

            err = reader.Next();
        }

        // authorityCertSerialNumber [2] IMPLICIT CertificateSerialNumber OPTIONAL
        // CertificateSerialNumber ::= INTEGER
        if (err == ASN1_NO_ERROR && reader.Class == kASN1TagClass_ContextSpecific && reader.Tag == 2)
        {
            err = writer.PutBytes(ContextTag(kTag_AuthorityKeyIdentifier_SerialNumber), reader.Value, reader.ValueLen);
            SuccessOrExit(err);

            err = reader.Next();
        }

        if (err != ASN1_END)
            SuccessOrExit(err);

    } ASN1_EXIT_SEQUENCE;

exit:
    return err;
}

static WEAVE_ERROR ConvertSubjectPublicKeyInfo(ASN1Reader& reader, TLVWriter& writer)
{
    WEAVE_ERROR err;
    OID keyAlgo, namedCurveOID;
    uint32_t weaveCurveId;

    // subjectPublicKeyInfo SubjectPublicKeyInfo,
    ASN1_PARSE_ENTER_SEQUENCE {

        // algorithm AlgorithmIdentifier,
        // AlgorithmIdentifier ::= SEQUENCE
        ASN1_PARSE_ENTER_SEQUENCE {

            // algorithm OBJECT IDENTIFIER,
            ASN1_PARSE_OBJECT_ID(keyAlgo);

            // Verify that the algorithm type is supported.
            if (keyAlgo != kOID_PubKeyAlgo_RSAEncryption &&
                keyAlgo != kOID_PubKeyAlgo_ECPublicKey &&
                keyAlgo != kOID_PubKeyAlgo_ECDH &&
                keyAlgo != kOID_PubKeyAlgo_ECMQV)
                ExitNow(err = ASN1_ERROR_UNSUPPORTED_ENCODING);

            uint8_t weaveKeyAlgo = (keyAlgo & kOID_Mask);
            err = writer.Put(ContextTag(kTag_PublicKeyAlgorithm), weaveKeyAlgo);
            SuccessOrExit(err);

            // parameters ANY DEFINED BY algorithm OPTIONAL
            if (keyAlgo == kOID_PubKeyAlgo_RSAEncryption)
            {
                // Per RFC4055, RSA parameters must be an explicit NULL.
                ASN1_PARSE_NULL;
            }

            else if (keyAlgo == kOID_PubKeyAlgo_ECPublicKey ||
                     keyAlgo == kOID_PubKeyAlgo_ECDH ||
                     keyAlgo == kOID_PubKeyAlgo_ECMQV)
            {

                // EcpkParameters ::= CHOICE {
                //     ecParameters  ECParameters,
                //     namedCurve    OBJECT IDENTIFIER,
                //     implicitlyCA  NULL }
                ASN1_PARSE_ANY;

                // ecParameters and implicitlyCA not supported.
                if (reader.Class == kASN1TagClass_Universal && reader.Tag == kASN1UniversalTag_Sequence)
                    ExitNow(err = ASN1_ERROR_UNSUPPORTED_ENCODING);
                if (reader.Class == kASN1TagClass_Universal && reader.Tag == kASN1UniversalTag_Null)
                    ExitNow(err = ASN1_ERROR_UNSUPPORTED_ENCODING);

                ASN1_VERIFY_TAG(kASN1TagClass_Universal, kASN1UniversalTag_ObjectId);

                ASN1_GET_OBJECT_ID(namedCurveOID);

                // Verify the curve name is recognized.
                VerifyOrExit(GetOIDCategory(namedCurveOID) == kOIDCategory_EllipticCurve, err = ASN1_ERROR_UNSUPPORTED_ENCODING);

                weaveCurveId = OIDToWeaveCurveId(namedCurveOID);
                err = writer.Put(ContextTag(kTag_EllipticCurveIdentifier), weaveCurveId);
                SuccessOrExit(err);
            }

        } ASN1_EXIT_SEQUENCE;

        // subjectPublicKey BIT STRING
        ASN1_PARSE_ELEMENT(kASN1TagClass_Universal, kASN1UniversalTag_BitString);
        if (keyAlgo == kOID_PubKeyAlgo_RSAEncryption)
        {
            // Per RFC3279, RSA public key is encapsulated DER encoding in subjectPublicKey BitString
            ASN1_ENTER_ENCAPSULATED(kASN1TagClass_Universal, kASN1UniversalTag_BitString) {
                TLVType outerContainer;

                err = writer.StartContainer(ContextTag(kTag_RSAPublicKey), kTLVType_Structure, outerContainer);
                SuccessOrExit(err);

                // RSAPublicKey ::= SEQUENCE
                ASN1_PARSE_ENTER_SEQUENCE {

                    // modulus INTEGER
                    ASN1_PARSE_ELEMENT(kASN1TagClass_Universal, kASN1UniversalTag_Integer);
                    err = writer.PutBytes(ContextTag(kTag_RSAPublicKey_Modulus), reader.Value, reader.ValueLen);
                    SuccessOrExit(err);

                    // publicExponent INTEGER
                    int64_t exp;
                    ASN1_PARSE_INTEGER(exp);
                    err = writer.Put(ContextTag(kTag_RSAPublicKey_PublicExponent), (uint64_t)exp);
                    SuccessOrExit(err);

                } ASN1_EXIT_SEQUENCE;

                err = writer.EndContainer(outerContainer);
                SuccessOrExit(err);

            } ASN1_EXIT_ENCAPSULATED;
        }
        else if (keyAlgo == kOID_PubKeyAlgo_ECPublicKey ||
                 keyAlgo == kOID_PubKeyAlgo_ECDH ||
                 keyAlgo == kOID_PubKeyAlgo_ECMQV)
        {
            // For EC certs, copy the X9.62 encoded EC point into the Weave certificate as a byte string.
            err = writer.PutBytes(ContextTag(kTag_EllipticCurvePublicKey), reader.Value + 1, reader.ValueLen - 1);
            SuccessOrExit(err);
        }

    } ASN1_EXIT_SEQUENCE;

exit:
    return err;
}

static WEAVE_ERROR ConvertExtension(ASN1Reader& reader, TLVWriter& writer)
{
    WEAVE_ERROR err;
    TLVType outerContainer, outerContainer2;
    OID extensionOID;
    bool critical = false;

    // Extension ::= SEQUENCE
    ASN1_ENTER_SEQUENCE {

        // extnID OBJECT IDENTIFIER,
        ASN1_PARSE_OBJECT_ID(extensionOID);

        if (extensionOID == kOID_Unknown)
            ExitNow(err = ASN1_ERROR_UNSUPPORTED_ENCODING);
        if (GetOIDCategory(extensionOID) != kOIDCategory_Extension)
            ExitNow(err = ASN1_ERROR_INVALID_ENCODING);

        // critical BOOLEAN DEFAULT FALSE,
        ASN1_PARSE_ANY;
        if (reader.Class == kASN1TagClass_Universal && reader.Tag == kASN1UniversalTag_Boolean)
        {
            ASN1_GET_BOOLEAN(critical);

            if (!critical)
                ExitNow(err = ASN1_ERROR_INVALID_ENCODING);

            ASN1_PARSE_ANY;
        }

        // extnValue OCTET STRING
        //           -- contains the DER encoding of an ASN.1 value
        //           -- corresponding to the extension type identified
        //           -- by extnID
        ASN1_ENTER_ENCAPSULATED(kASN1TagClass_Universal, kASN1UniversalTag_OctetString) {

            if (extensionOID == kOID_Extension_AuthorityKeyIdentifier)
            {
                err = writer.StartContainer(ContextTag(kTag_AuthorityKeyIdentifier), kTLVType_Structure, outerContainer);
                SuccessOrExit(err);

                if (critical)
                {
                    err = writer.PutBoolean(ContextTag(kTag_AuthorityKeyIdentifier_Critical), critical);
                    SuccessOrExit(err);
                }

                err = ConvertAuthorityKeyIdentifierExtension(reader, writer);
                SuccessOrExit(err);
            }
            else if (extensionOID == kOID_Extension_SubjectKeyIdentifier)
            {
                // SubjectKeyIdentifier ::= KeyIdentifier
                ASN1_PARSE_ELEMENT(kASN1TagClass_Universal, kASN1UniversalTag_OctetString);

                err = writer.StartContainer(ContextTag(kTag_SubjectKeyIdentifier), kTLVType_Structure, outerContainer);
                SuccessOrExit(err);

                if (critical)
                {
                    err = writer.PutBoolean(ContextTag(kTag_SubjectKeyIdentifier_Critical), critical);
                    SuccessOrExit(err);
                }

                err = writer.PutBytes(ContextTag(kTag_SubjectKeyIdentifier_KeyIdentifier), reader.Value, reader.ValueLen);
                SuccessOrExit(err);
            }
            else if (extensionOID == kOID_Extension_KeyUsage)
            {
                // KeyUsage ::= BIT STRING
                ASN1_PARSE_ELEMENT(kASN1TagClass_Universal, kASN1UniversalTag_BitString);

                err = writer.StartContainer(ContextTag(kTag_KeyUsage), kTLVType_Structure, outerContainer);
                SuccessOrExit(err);

                if (critical)
                {
                    err = writer.PutBoolean(ContextTag(kTag_SubjectKeyIdentifier_Critical), critical);
                    SuccessOrExit(err);
                }

                uint32_t keyUsageBits;
                err = reader.GetBitString(keyUsageBits);
                SuccessOrExit(err);

                err = writer.Put(ContextTag(kTag_KeyUsage_KeyUsage), keyUsageBits);
                SuccessOrExit(err);
            }
            else if (extensionOID == kOID_Extension_BasicConstraints)
            {
                // BasicConstraints ::= SEQUENCE
                ASN1_PARSE_ENTER_SEQUENCE {
                    bool isCA = false;
                    int64_t pathLenConstraint = -1;

                    // cA BOOLEAN DEFAULT FALSE
                    err = reader.Next();
                    if (err == ASN1_NO_ERROR && reader.Class == kASN1TagClass_Universal && reader.Tag == kASN1UniversalTag_Boolean)
                    {
                        ASN1_GET_BOOLEAN(isCA);

                        VerifyOrExit(isCA == true, err = ASN1_ERROR_INVALID_ENCODING);

                        err = reader.Next();
                    }

                    // pathLenConstraint INTEGER (0..MAX) OPTIONAL
                    if (err == ASN1_NO_ERROR && reader.Class == kASN1TagClass_Universal && reader.Tag == kASN1UniversalTag_Integer)
                    {
                        ASN1_GET_INTEGER(pathLenConstraint);

                        VerifyOrExit(pathLenConstraint >= 0, err = ASN1_ERROR_INVALID_ENCODING);
                    }

                    err = writer.StartContainer(ContextTag(kTag_BasicConstraints), kTLVType_Structure, outerContainer);
                    SuccessOrExit(err);

                    if (critical)
                    {
                        err = writer.PutBoolean(ContextTag(kTag_BasicConstraints_Critical), critical);
                        SuccessOrExit(err);
                    }

                    if (isCA)
                    {
                        err = writer.PutBoolean(ContextTag(kTag_BasicConstraints_IsCA), isCA);
                        SuccessOrExit(err);
                    }

                    if (pathLenConstraint != -1)
                    {
                        err = writer.Put(ContextTag(kTag_BasicConstraints_PathLenConstraint), (uint32_t)pathLenConstraint);
                        SuccessOrExit(err);
                    }

                } ASN1_EXIT_SEQUENCE;
            }
            else if (extensionOID == kOID_Extension_ExtendedKeyUsage)
            {
                err = writer.StartContainer(ContextTag(kTag_ExtendedKeyUsage), kTLVType_Structure, outerContainer);
                SuccessOrExit(err);

                if (critical)
                {
                    err = writer.PutBoolean(ContextTag(kTag_ExtendedKeyUsage_Critical), critical);
                    SuccessOrExit(err);
                }

                err = writer.StartContainer(ContextTag(kTag_ExtendedKeyUsage_KeyPurposes), kTLVType_Array, outerContainer2);
                SuccessOrExit(err);

                // ExtKeyUsageSyntax ::= SEQUENCE SIZE (1..MAX) OF KeyPurposeId
                ASN1_PARSE_ENTER_SEQUENCE {

                    while ((err = reader.Next()) == ASN1_NO_ERROR)
                    {
                        // KeyPurposeId ::= OBJECT IDENTIFIER
                        OID keyPurpose;
                        ASN1_GET_OBJECT_ID(keyPurpose);

                        VerifyOrExit(keyPurpose != kOID_Unknown, err = ASN1_ERROR_UNSUPPORTED_ENCODING);
                        VerifyOrExit(GetOIDCategory(keyPurpose) == kOIDCategory_KeyPurpose, err = ASN1_ERROR_INVALID_ENCODING);

                        err = writer.Put(AnonymousTag, (uint8_t)(keyPurpose & kOID_Mask));
                        SuccessOrExit(err);
                    }
                    if (err != ASN1_END)
                        SuccessOrExit(err);

                } ASN1_EXIT_SEQUENCE;

                err = writer.EndContainer(outerContainer2);
                SuccessOrExit(err);
            }
            else
                ExitNow(err = ASN1_ERROR_UNSUPPORTED_ENCODING);

            err = writer.EndContainer(outerContainer);
            SuccessOrExit(err);

        } ASN1_EXIT_ENCAPSULATED;

    } ASN1_EXIT_SEQUENCE;

exit:
    return err;
}

static WEAVE_ERROR ConvertExtensions(ASN1Reader& reader, TLVWriter& writer)
{
    WEAVE_ERROR err;

    // Extensions ::= SEQUENCE SIZE (1..MAX) OF Extension
    ASN1_PARSE_ENTER_SEQUENCE {

        while ((err = reader.Next()) == ASN1_NO_ERROR)
        {
            err = ConvertExtension(reader, writer);
            SuccessOrExit(err);
        }

        if (err != ASN1_END)
            SuccessOrExit(err);

    } ASN1_EXIT_SEQUENCE;

exit:
    return err;
}

static WEAVE_ERROR ConvertCertificate(ASN1Reader& reader, TLVWriter& writer)
{
    WEAVE_ERROR err;
    int64_t version;
    OID sigAlgo;
    TLVType containerType;

    err = writer.StartContainer(ProfileTag(kWeaveProfile_Security, kTag_WeaveCertificate), kTLVType_Structure, containerType);
    SuccessOrExit(err);

    // Certificate ::= SEQUENCE
    ASN1_PARSE_ENTER_SEQUENCE {

        // tbsCertificate TBSCertificate,
        // TBSCertificate ::= SEQUENCE
        ASN1_PARSE_ENTER_SEQUENCE {

            // version [0] EXPLICIT Version DEFAULT v1
            ASN1_PARSE_ENTER_CONSTRUCTED(kASN1TagClass_ContextSpecific, 0) {

                // Version ::= INTEGER { v1(0), v2(1), v3(2) }
                ASN1_PARSE_INTEGER(version);

                // Verify that the X.509 certificate version is v3
                VerifyOrExit(version == 2, err = ASN1_ERROR_UNSUPPORTED_ENCODING);

            } ASN1_EXIT_CONSTRUCTED;

            // serialNumber CertificateSerialNumber
            // CertificateSerialNumber ::= INTEGER
            ASN1_PARSE_ELEMENT(kASN1TagClass_Universal, kASN1UniversalTag_Integer);
            err = writer.PutBytes(ContextTag(kTag_SerialNumber), reader.Value, reader.ValueLen);
            SuccessOrExit(err);

            // signature AlgorithmIdentifier
            // AlgorithmIdentifier ::= SEQUENCE
            ASN1_PARSE_ENTER_SEQUENCE {

                // algorithm OBJECT IDENTIFIER,
                ASN1_PARSE_OBJECT_ID(sigAlgo);

                VerifyOrExit(GetOIDCategory(sigAlgo) == kOIDCategory_SigAlgo, err = ASN1_ERROR_INVALID_ENCODING);

                if (sigAlgo != kOID_SigAlgo_MD2WithRSAEncryption &&
                    sigAlgo != kOID_SigAlgo_MD5WithRSAEncryption &&
                    sigAlgo != kOID_SigAlgo_SHA1WithRSAEncryption &&
                    sigAlgo != kOID_SigAlgo_ECDSAWithSHA1 &&
                    sigAlgo != kOID_SigAlgo_ECDSAWithSHA256)
                    ExitNow(err = ASN1_ERROR_UNSUPPORTED_ENCODING);

                uint8_t weaveSigAlgo = sigAlgo & ~kOIDCategory_Mask;
                err = writer.Put(ContextTag(kTag_SignatureAlgorithm), weaveSigAlgo);
                SuccessOrExit(err);

                // parameters ANY DEFINED BY algorithm OPTIONAL
                // Per RFC3279, parameters for RSA must be NULL, parameters for ECDSAWithSHA1 must be absent.
                if (sigAlgo == kOID_SigAlgo_MD2WithRSAEncryption ||
                    sigAlgo == kOID_SigAlgo_MD5WithRSAEncryption ||
                    sigAlgo == kOID_SigAlgo_SHA1WithRSAEncryption)
                {
                    ASN1_PARSE_NULL;
                }

            } ASN1_EXIT_SEQUENCE;

            // issuer Name
            err = ConvertDistinguishedName(reader, writer, ContextTag(kTag_Issuer));
            SuccessOrExit(err);

            // validity Validity,
            err = ConvertValidity(reader, writer);
            SuccessOrExit(err);

            // subject Name,
            err = ConvertDistinguishedName(reader, writer, ContextTag(kTag_Subject));
            SuccessOrExit(err);

            err = ConvertSubjectPublicKeyInfo(reader, writer);
            SuccessOrExit(err);

            err = reader.Next();

            // issuerUniqueID [1] IMPLICIT UniqueIdentifier OPTIONAL,
            // Not supported.
            if (err == ASN1_NO_ERROR && reader.Class == kASN1TagClass_ContextSpecific && reader.Tag == 1)
                ExitNow(err = ASN1_ERROR_UNSUPPORTED_ENCODING);

            // subjectUniqueID [2] IMPLICIT UniqueIdentifier OPTIONAL,
            // Not supported.
            if (err == ASN1_NO_ERROR && reader.Class == kASN1TagClass_ContextSpecific && reader.Tag == 2)
                ExitNow(err = ASN1_ERROR_UNSUPPORTED_ENCODING);

            // extensions [3] EXPLICIT Extensions OPTIONAL
            if (err == ASN1_NO_ERROR && reader.Class == kASN1TagClass_ContextSpecific && reader.Tag == 3)
            {
                ASN1_ENTER_CONSTRUCTED(kASN1TagClass_ContextSpecific, 3) {

                    err = ConvertExtensions(reader, writer);
                    SuccessOrExit(err);

                } ASN1_EXIT_CONSTRUCTED;

                err = reader.Next();
            }

            if (err != ASN1_END)
                ExitNow();

        } ASN1_EXIT_SEQUENCE;

        // signatureAlgorithm AlgorithmIdentifier
        // Skip signatureAlgorithm since its the same as the "signature" field in TBSCertificate.
        ASN1_PARSE_ANY;

        // signatureValue BIT STRING
        ASN1_PARSE_ELEMENT(kASN1TagClass_Universal, kASN1UniversalTag_BitString);
        if (sigAlgo == kOID_SigAlgo_MD2WithRSAEncryption ||
            sigAlgo == kOID_SigAlgo_MD5WithRSAEncryption ||
            sigAlgo == kOID_SigAlgo_SHA1WithRSAEncryption)
        {
            err = writer.PutBytes(ContextTag(kTag_RSASignature), reader.Value + 1, reader.ValueLen - 1);
            SuccessOrExit(err);
        }
        else if (sigAlgo == kOID_SigAlgo_ECDSAWithSHA1 ||
                 sigAlgo == kOID_SigAlgo_ECDSAWithSHA256)
        {
            // Per RFC3279, the ECDSA signature value is encoded in DER encapsulated in the signatureValue BIT STRING.
            ASN1_ENTER_ENCAPSULATED(kASN1TagClass_Universal, kASN1UniversalTag_BitString) {
                TLVType outerContainer;

                err = writer.StartContainer(ContextTag(kTag_ECDSASignature), kTLVType_Structure, outerContainer);
                SuccessOrExit(err);

                // Ecdsa-Sig-Value ::= SEQUENCE
                ASN1_PARSE_ENTER_SEQUENCE {

                    // r INTEGER
                    ASN1_PARSE_ELEMENT(kASN1TagClass_Universal, kASN1UniversalTag_Integer);
                    err = writer.PutBytes(ContextTag(kTag_ECDSASignature_r), reader.Value, reader.ValueLen);
                    SuccessOrExit(err);

                    // s INTEGER
                    ASN1_PARSE_ELEMENT(kASN1TagClass_Universal, kASN1UniversalTag_Integer);
                    err = writer.PutBytes(ContextTag(kTag_ECDSASignature_s), reader.Value, reader.ValueLen);
                    SuccessOrExit(err);

                } ASN1_EXIT_SEQUENCE;

                err = writer.EndContainer(outerContainer);
                SuccessOrExit(err);

            } ASN1_EXIT_ENCAPSULATED;
        }

    } ASN1_EXIT_SEQUENCE;

    err = writer.EndContainer(containerType);
    SuccessOrExit(err);

exit:
    return err;
}

NL_DLL_EXPORT WEAVE_ERROR ConvertX509CertToWeaveCert(const uint8_t *x509Cert, uint32_t x509CertLen, uint8_t *weaveCertBuf, uint32_t weaveCertBufSize, uint32_t& weaveCertLen)
{
    WEAVE_ERROR err;
    ASN1Reader reader;
    TLVWriter writer;

    reader.Init(x509Cert, x509CertLen);

    writer.Init(weaveCertBuf, weaveCertBufSize);

    err = ConvertCertificate(reader, writer);
    SuccessOrExit(err);

    err = writer.Finalize();
    SuccessOrExit(err);

    weaveCertLen = writer.GetLengthWritten();

exit:
    return err;
}

} // namespace Security
} // namespace Profiles
} // namespace Weave
} // namespace nl
