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
 *      This file implements methods for converting a Weave
 *      TLV-encoded certificate to a standard X.509 certificate.
 *
 */

#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif
#include <stdint.h>

#include <Weave/Support/NLDLLUtil.h>
#include <Weave/Core/WeaveCore.h>
#include <Weave/Profiles/WeaveProfiles.h>
#include <Weave/Profiles/security/WeaveSecurity.h>
#include <Weave/Profiles/security/WeaveCert.h>
#include <Weave/Profiles/security/WeaveSig.h>
#include <Weave/Support/crypto/EllipticCurve.h>
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

// Return true if a TLV tag represents a certificate extension.
inline bool IsCertificateExtensionTag(uint64_t tag)
{
    if (IsContextTag(tag))
    {
        uint32_t tagNum = TagNumFromTag(tag);
        return (tagNum >= kCertificateExtensionTagsStart && tagNum <= kCertificateExtensionTagsEnd);
    }
    else
        return false;
}

static WEAVE_ERROR DecodeConvertDN(TLVReader& reader, ASN1Writer& writer, WeaveDN& dn)
{
    WEAVE_ERROR err;
    TLVType outerContainer;
    TLVType elemType;
    uint64_t tlvTag;
    uint32_t tlvTagNum;
    OID attrOID;
    uint32_t asn1Tag;
    const uint8_t *asn1AttrVal;
    uint32_t asn1AttrValLen;
    uint8_t weaveIdStr[17];

    // Enter the Path TLV element that represents the DN in TLV format.
    err = reader.EnterContainer(outerContainer);
    SuccessOrExit(err);

    // Read the first TLV element in the Path.  This represents the first RDN in the original ASN.1 DN.
    //
    // NOTE: Although Weave certificate encoding allows for DNs containing multiple RDNs, and/or multiple
    // attributes per RDN, this implementation only supports DNs with a single RDN that contains exactly
    // one attribute.
    //
    err = reader.Next();
    SuccessOrExit(err);

    // Get the TLV tag, make sure it is a context tag and extract the context tag number.
    tlvTag = reader.GetTag();
    VerifyOrExit(IsContextTag(tlvTag), err = WEAVE_ERROR_INVALID_TLV_TAG);
    tlvTagNum = TagNumFromTag(tlvTag);

    // Get the element type.
    elemType = reader.GetType();

    // Derive the OID of the corresponding ASN.1 attribute from the TLV tag number.
    // The numeric value of the OID is encoded in the bottom 7 bits of the TLV tag number.
    // This eliminates the need for a translation table/switch statement but has the
    // effect of tying the two encodings together.
    //
    // NOTE: In the event that the computed OID value is not one that we recognize
    // (specifically, is not in the table of OIDs defined in ASN1OID.h) then the
    // macro call below that encodes the attribute's object id (ASN1_ENCODE_OBJECT_ID)
    // will fail for lack of the OID's encoded representation.  Given this there's no
    // need to test the validity of the OID here.
    //
    attrOID = (OID)(kOIDCategory_AttributeType | (tlvTagNum & 0x7f));

    // Save the attribute OID in the caller's DN structure.
    dn.AttrOID = attrOID;

    // If the attribute is one of the Weave-defined X.509 attributes that contains a Weave id...
    if (IsWeaveIdX509Attr(attrOID))
    {
        // Verify that the underlying TLV data type is unsigned integer.
        VerifyOrExit(elemType == kTLVType_UnsignedInteger, err = WEAVE_ERROR_WRONG_TLV_TYPE);

        // Read the value of the weave id.
        uint64_t weaveId;
        err = reader.Get(weaveId);
        SuccessOrExit(err);

        // Generate the string representation of the id that will appear in the ASN.1 attribute.
        // For weave ids the string representation is *always* 16 uppercase hex characters.
        snprintf((char *)weaveIdStr, sizeof(weaveIdStr), "%016llX", (unsigned long long)weaveId);
        asn1AttrVal = weaveIdStr;
        asn1AttrValLen = 16;

        // The ASN.1 tag for Weave id attributes is always UTF8String.
        asn1Tag = kASN1UniversalTag_UTF8String;

        // Save the weave id value in the caller's DN structure.
        dn.AttrValue.WeaveId = weaveId;
    }

    // Otherwise the attribute is either one of the supported X.509 attributes or a Weave-defined
    // attribute that is *not* a Weave id...
    else
    {
        // Verify that the underlying data type is UTF8 string.
        VerifyOrExit(elemType == kTLVType_UTF8String, err = WEAVE_ERROR_WRONG_TLV_TYPE);

        // Get a pointer the underlying string data, plus its length.
        err = reader.GetDataPtr(asn1AttrVal);
        SuccessOrExit(err);
        asn1AttrValLen = reader.GetLength();

        // Determine the appropriate ASN.1 tag for the DN attribute.
        // - Weave-defined attributes are always UTF8Strings.
        // - DomainComponent is always an IA5String.
        // - For all other ASN.1 defined attributes, bit 0x80 in the TLV tag value conveys whether the attribute
        //   is a UTF8String or a PrintableString (in some cases the certificate generator has a choice).
        if (IsWeaveX509Attr(attrOID))
            asn1Tag = kASN1UniversalTag_UTF8String;
        else if (attrOID == kOID_AttributeType_DomainComponent)
            asn1Tag = kASN1UniversalTag_IA5String;
        else
            asn1Tag = (tlvTagNum & 0x80) ? kASN1UniversalTag_PrintableString : kASN1UniversalTag_UTF8String;

        // Save the string value in the caller's DN structure.
        dn.AttrValue.String.Value = asn1AttrVal;
        dn.AttrValue.String.Len = asn1AttrValLen;
    }

    // Verify that there are no further elements in the DN.
    err = reader.VerifyEndOfContainer();
    SuccessOrExit(err);

    err = reader.ExitContainer(outerContainer);
    SuccessOrExit(err);

    // Write the ASN.1 representation of the DN...

    // RDNSequence ::= SEQUENCE OF RelativeDistinguishedName
    ASN1_START_SEQUENCE {

        // RelativeDistinguishedName ::= SET SIZE (1..MAX) OF AttributeTypeAndValue
        ASN1_START_SET {

            // AttributeTypeAndValue ::= SEQUENCE
            ASN1_START_SEQUENCE {

                // type AttributeType
                // AttributeType ::= OBJECT IDENTIFIER
                ASN1_ENCODE_OBJECT_ID(attrOID);

                // value AttributeValue
                // AttributeValue ::= ANY -- DEFINED BY AttributeType
                err = writer.PutString(asn1Tag, (const char *)asn1AttrVal, asn1AttrValLen);
                SuccessOrExit(err);

            } ASN1_END_SEQUENCE;

        } ASN1_END_SET;

    } ASN1_END_SEQUENCE;

exit:
    return err;
}

static WEAVE_ERROR DecodeConvertValidity(TLVReader& reader, ASN1Writer& writer, WeaveCertificateData& certData)
{
    WEAVE_ERROR err;
    ASN1UniversalTime asn1Time;
    uint32_t packedTime;

    ASN1_START_SEQUENCE {

        err = reader.Next(kTLVType_UnsignedInteger, ContextTag(kTag_NotBefore));
        SuccessOrExit(err);
        err = reader.Get(packedTime);
        SuccessOrExit(err);
        certData.NotBeforeDate = PackedCertTimeToDate(packedTime);
        err = UnpackCertTime(packedTime, asn1Time);
        SuccessOrExit(err);

        ASN1_ENCODE_TIME(asn1Time);

        err = reader.Next(kTLVType_UnsignedInteger, ContextTag(kTag_NotAfter));
        SuccessOrExit(err);
        err = reader.Get(packedTime);
        SuccessOrExit(err);
        certData.NotAfterDate = PackedCertTimeToDate(packedTime);
        err = UnpackCertTime(packedTime, asn1Time);
        SuccessOrExit(err);

        ASN1_ENCODE_TIME(asn1Time);

    } ASN1_END_SEQUENCE;

exit:
    return err;
}

static WEAVE_ERROR DecodeConvertSubjectPublicKeyInfo(TLVReader& reader, ASN1Writer& writer, WeaveCertificateData& certData)
{
    WEAVE_ERROR err;
    OID pubKeyAlgoOID, pubKeyCurveOID = kOID_NotSpecified;
    uint32_t weavePubKeyAlgoId, weaveCurveId, len;

    err = reader.Next(kTLVType_UnsignedInteger, ContextTag(kTag_PublicKeyAlgorithm));
    SuccessOrExit(err);
    err = reader.Get(weavePubKeyAlgoId);
    SuccessOrExit(err);

    pubKeyAlgoOID = (OID)(kOIDCategory_PubKeyAlgo | weavePubKeyAlgoId);
    certData.PubKeyAlgoOID = pubKeyAlgoOID;

    if (pubKeyAlgoOID == kOID_PubKeyAlgo_ECPublicKey ||
        pubKeyAlgoOID == kOID_PubKeyAlgo_ECDH ||
        pubKeyAlgoOID == kOID_PubKeyAlgo_ECMQV)
    {
        err = reader.Next(kTLVType_UnsignedInteger, ContextTag(kTag_EllipticCurveIdentifier));
        SuccessOrExit(err);
        err = reader.Get(weaveCurveId);
        SuccessOrExit(err);

        // Support old form of Nest curve ids that did not include vendor.
        if (weaveCurveId < 65536)
            weaveCurveId |= (kWeaveVendor_NestLabs << 16);

        certData.PubKeyCurveId = weaveCurveId;
        pubKeyCurveOID = WeaveCurveIdToOID(certData.PubKeyCurveId);
    }

    else
        VerifyOrExit(pubKeyAlgoOID == kOID_PubKeyAlgo_RSAEncryption, err = WEAVE_ERROR_UNSUPPORTED_CERT_FORMAT);

    // subjectPublicKeyInfo SubjectPublicKeyInfo,
    ASN1_START_SEQUENCE {

        // algorithm AlgorithmIdentifier,
        // AlgorithmIdentifier ::= SEQUENCE
        ASN1_START_SEQUENCE {

            // algorithm OBJECT IDENTIFIER,
            ASN1_ENCODE_OBJECT_ID(pubKeyAlgoOID);

            // parameters ANY DEFINED BY algorithm OPTIONAL
            if (pubKeyAlgoOID == kOID_PubKeyAlgo_RSAEncryption)
            {
                // Per RFC4055, RSA parameters must be an explicit NULL.
                ASN1_ENCODE_NULL;
            }
            else
            {
                // EcpkParameters ::= CHOICE {
                //     ecParameters  ECParameters,
                //     namedCurve    OBJECT IDENTIFIER,
                //     implicitlyCA  NULL }
                //
                // (Only namedCurve supported).
                //
                ASN1_ENCODE_OBJECT_ID(pubKeyCurveOID);
            }

        } ASN1_END_SEQUENCE;

        // subjectPublicKey BIT STRING
        if (pubKeyAlgoOID == kOID_PubKeyAlgo_RSAEncryption)
        {
            TLVType pubKeyOuterContainer;

            err = reader.Next(kTLVType_Structure, ContextTag(kTag_RSAPublicKey));
            SuccessOrExit(err);
            err = reader.EnterContainer(pubKeyOuterContainer);
            SuccessOrExit(err);

            // Per RFC3279, RSA public key is an encapsulated DER encoding of RSAPublicKey within
            // the subjectPublicKey BitString
            ASN1_START_BIT_STRING_ENCAPSULATED {

                // RSAPublicKey ::= SEQUENCE
                ASN1_START_SEQUENCE {

                    // modulus INTEGER
                    err = reader.Next(kTLVType_ByteString, ContextTag(kTag_RSAPublicKey_Modulus));
                    SuccessOrExit(err);
                    err = writer.PutValue(kASN1TagClass_Universal, kASN1UniversalTag_Integer, false, reader);
                    SuccessOrExit(err);

                    // publicExponent INTEGER
                    err = reader.Next(kTLVType_SignedInteger, ContextTag(kTag_RSAPublicKey_PublicExponent));
                    SuccessOrExit(err);
                    int64_t exp;
                    err = reader.Get(exp);
                    SuccessOrExit(err);
                    err = writer.PutInteger(exp);
                    SuccessOrExit(err);

                } ASN1_END_SEQUENCE;

            } ASN1_END_ENCAPSULATED;

            err = reader.ExitContainer(pubKeyOuterContainer);
            SuccessOrExit(err);

            // TODO: extract RSA public key
        }

        else
        {
            err = reader.Next(kTLVType_ByteString, ContextTag(kTag_EllipticCurvePublicKey));
            SuccessOrExit(err);

            err = reader.GetDataPtr((const uint8_t *&)certData.PublicKey.EC.ECPoint);
            SuccessOrExit(err);

            len = reader.GetLength();
            VerifyOrExit(len <= UINT16_MAX, err = WEAVE_ERROR_UNSUPPORTED_CERT_FORMAT);

            certData.PublicKey.EC.ECPointLen = len;

            // For EC certs, the subjectPublicKey BIT STRING contains the X9.62 encoded EC point.
            err = writer.PutBitString(0, certData.PublicKey.EC.ECPoint, (uint16_t)certData.PublicKey.EC.ECPointLen);
            SuccessOrExit(err);
        }

    } ASN1_END_SEQUENCE;

exit:
    return err;
}

static WEAVE_ERROR DecodeConvertAuthorityKeyIdentifierExtension(TLVReader& reader, ASN1Writer& writer, WeaveCertificateData& certData)
{
    WEAVE_ERROR err, nextRes;
    uint32_t len;

    certData.CertFlags |= kCertFlag_ExtPresent_AuthKeyId;

    // AuthorityKeyIdentifier ::= SEQUENCE
    ASN1_START_SEQUENCE {

        // keyIdentifier [0] IMPLICIT KeyIdentifier OPTIONAL,
        // KeyIdentifier ::= OCTET STRING
        if (reader.GetTag() == ContextTag(kTag_AuthorityKeyIdentifier_KeyIdentifier))
        {
            VerifyOrExit(reader.GetType() == kTLVType_ByteString, err = WEAVE_ERROR_WRONG_TLV_TYPE);

            err = reader.GetDataPtr(certData.AuthKeyId.Id);
            SuccessOrExit(err);

            len = reader.GetLength();
            VerifyOrExit(len <= UINT8_MAX, err = WEAVE_ERROR_UNSUPPORTED_CERT_FORMAT);

            certData.AuthKeyId.Len = len;

            err = writer.PutOctetString(kASN1TagClass_ContextSpecific, 0, certData.AuthKeyId.Id, certData.AuthKeyId.Len);
            SuccessOrExit(err);

            nextRes = reader.Next();
            VerifyOrExit(nextRes == WEAVE_NO_ERROR || nextRes == WEAVE_END_OF_TLV, err = nextRes);
        }

        // NOTE: kTag_AuthorityKeyIdentifier_Issuer and kTag_AuthorityKeyIdentifier_SerialNumber currently unsupported.

    } ASN1_END_SEQUENCE;

exit:
    return err;
}

static WEAVE_ERROR DecodeConvertSubjectKeyIdentifierExtension(TLVReader& reader, ASN1Writer& writer, WeaveCertificateData& certData)
{
    WEAVE_ERROR err;
    uint32_t len;

    certData.CertFlags |= kCertFlag_ExtPresent_SubjectKeyId;

    // SubjectKeyIdentifier ::= KeyIdentifier
    // KeyIdentifier ::= OCTET STRING
    VerifyOrExit(reader.GetType() == kTLVType_ByteString, err = WEAVE_ERROR_WRONG_TLV_TYPE);
    VerifyOrExit(reader.GetTag() == ContextTag(kTag_SubjectKeyIdentifier_KeyIdentifier), err = WEAVE_ERROR_UNEXPECTED_TLV_ELEMENT);

    len = reader.GetLength();
    VerifyOrExit(len <= UINT8_MAX, err = WEAVE_ERROR_UNSUPPORTED_CERT_FORMAT);

    certData.SubjectKeyId.Len = len;

    err = reader.GetDataPtr(certData.SubjectKeyId.Id);
    SuccessOrExit(err);

    err = writer.PutOctetString(certData.SubjectKeyId.Id, certData.SubjectKeyId.Len);
    SuccessOrExit(err);

exit:
    return err;
}

static WEAVE_ERROR DecodeConvertKeyUsageExtension(TLVReader& reader, ASN1Writer& writer, WeaveCertificateData& certData)
{
    WEAVE_ERROR err;
    uint16_t keyUsageBits;

    certData.CertFlags |= kCertFlag_ExtPresent_KeyUsage;

    // KeyUsage ::= BIT STRING
    VerifyOrExit(reader.GetTag() == ContextTag(kTag_KeyUsage_KeyUsage), err = WEAVE_ERROR_UNEXPECTED_TLV_ELEMENT);
    err = reader.Get(keyUsageBits);
    SuccessOrExit(err);
    ASN1_ENCODE_BIT_STRING(keyUsageBits);

    certData.KeyUsageFlags = keyUsageBits;

exit:
    return err;
}

static WEAVE_ERROR DecodeConvertBasicConstraintsExtension(TLVReader& reader, ASN1Writer& writer, WeaveCertificateData& certData)
{
    WEAVE_ERROR err, nextRes;

    certData.CertFlags |= kCertFlag_ExtPresent_BasicConstraints;

    // BasicConstraints ::= SEQUENCE
    ASN1_START_SEQUENCE {

        // cA BOOLEAN DEFAULT FALSE
        if (reader.GetTag() == ContextTag(kTag_BasicConstraints_IsCA))
        {
            bool isCA;

            err = reader.Get(isCA);
            SuccessOrExit(err);

            if (isCA)
            {
                ASN1_ENCODE_BOOLEAN(true);
                certData.CertFlags |= kCertFlag_IsCA;
            }

            nextRes = reader.Next();
            VerifyOrExit(nextRes == WEAVE_NO_ERROR || nextRes == WEAVE_END_OF_TLV, err = nextRes);
        }

        // pathLenConstraint INTEGER (0..MAX) OPTIONAL
        if (reader.GetTag() == ContextTag(kTag_BasicConstraints_PathLenConstraint))
        {
            uint8_t pathLenConstraint;

            err = reader.Get(pathLenConstraint);
            SuccessOrExit(err);

            ASN1_ENCODE_INTEGER((int64_t)pathLenConstraint);

            certData.PathLenConstraint = pathLenConstraint;

            certData.CertFlags |= kCertFlag_PathLenConstPresent;
        }

    } ASN1_END_SEQUENCE;

exit:
    return err;
}

static WEAVE_ERROR DecodeConvertExtendedKeyUsageExtension(TLVReader& reader, ASN1Writer& writer, WeaveCertificateData& certData)
{
    WEAVE_ERROR err, nextRes;
    TLVType outerContainer;

    certData.CertFlags |= kCertFlag_ExtPresent_ExtendedKeyUsage;

    // ExtKeyUsageSyntax ::= SEQUENCE SIZE (1..MAX) OF KeyPurposeId
    ASN1_START_SEQUENCE {

        VerifyOrExit(reader.GetTag() == ContextTag(kTag_ExtendedKeyUsage_KeyPurposes), err = WEAVE_ERROR_WRONG_TLV_TYPE);
        VerifyOrExit(reader.GetType() == kTLVType_Array, err = WEAVE_ERROR_WRONG_TLV_TYPE);

        err = reader.EnterContainer(outerContainer);
        SuccessOrExit(err);

        while ((nextRes = reader.Next(kTLVType_UnsignedInteger, AnonymousTag)) == WEAVE_NO_ERROR)
        {
            OID keyPurposeOID;
            uint16_t weaveKeyPurposeId;

            err = reader.Get(weaveKeyPurposeId);
            SuccessOrExit(err);

            keyPurposeOID = (OID)(kOIDCategory_KeyPurpose | weaveKeyPurposeId);

            // KeyPurposeId ::= OBJECT IDENTIFIER
            ASN1_ENCODE_OBJECT_ID(keyPurposeOID);

            // TODO: eliminate switch statement and generate key purpose flag by shifting 1 by lower bits of OID.

            switch (keyPurposeOID)
            {
            case kOID_KeyPurpose_ServerAuth:       certData.KeyPurposeFlags |= kKeyPurposeFlag_ServerAuth;      break;
            case kOID_KeyPurpose_ClientAuth:       certData.KeyPurposeFlags |= kKeyPurposeFlag_ClientAuth;      break;
            case kOID_KeyPurpose_CodeSigning:      certData.KeyPurposeFlags |= kKeyPurposeFlag_CodeSigning;     break;
            case kOID_KeyPurpose_EmailProtection:  certData.KeyPurposeFlags |= kKeyPurposeFlag_EmailProtection; break;
            case kOID_KeyPurpose_TimeStamping:     certData.KeyPurposeFlags |= kKeyPurposeFlag_TimeStamping;    break;
            case kOID_KeyPurpose_OCSPSigning:      certData.KeyPurposeFlags |= kKeyPurposeFlag_OCSPSigning;     break;
            default:
                break;
            }
        }

        VerifyOrExit(nextRes == WEAVE_END_OF_TLV, err = nextRes);

        err = reader.ExitContainer(outerContainer);
        SuccessOrExit(err);

    } ASN1_END_SEQUENCE;

exit:
    return err;
}

static WEAVE_ERROR DecodeConvertExtension(TLVReader& reader, ASN1Writer& writer, WeaveCertificateData& certData)
{
    WEAVE_ERROR err, nextRes;
    TLVType outerContainer;
    uint64_t extensionTagNum = TagNumFromTag(reader.GetTag());
    OID extensionOID;

    if (extensionTagNum == kTag_AuthorityKeyIdentifier)
        extensionOID = kOID_Extension_AuthorityKeyIdentifier;
    else if (extensionTagNum == kTag_SubjectKeyIdentifier)
        extensionOID = kOID_Extension_SubjectKeyIdentifier;
    else if (extensionTagNum == kTag_KeyUsage)
        extensionOID = kOID_Extension_KeyUsage;
    else if (extensionTagNum == kTag_BasicConstraints)
        extensionOID = kOID_Extension_BasicConstraints;
    else if (extensionTagNum == kTag_ExtendedKeyUsage)
        extensionOID = kOID_Extension_ExtendedKeyUsage;
    else
        ExitNow(err = WEAVE_ERROR_UNEXPECTED_TLV_ELEMENT);

    err = reader.EnterContainer(outerContainer);
    SuccessOrExit(err);

    // Extension ::= SEQUENCE
    ASN1_START_SEQUENCE {

        // extnID OBJECT IDENTIFIER,
        ASN1_ENCODE_OBJECT_ID(extensionOID);

        // critical BOOLEAN DEFAULT FALSE,
        nextRes = reader.Next();
        VerifyOrExit(nextRes == WEAVE_NO_ERROR || nextRes == WEAVE_END_OF_TLV, err = nextRes);
        if (reader.GetTag() == ContextTag(kTag_BasicConstraints_Critical))
        {
            bool critical;
            err = reader.Get(critical);
            SuccessOrExit(err);
            if (critical)
                ASN1_ENCODE_BOOLEAN(true);

            nextRes = reader.Next();
            VerifyOrExit(nextRes == WEAVE_NO_ERROR || nextRes == WEAVE_END_OF_TLV, err = nextRes);
        }

        // extnValue OCTET STRING
        //           -- contains the DER encoding of an ASN.1 value
        //           -- corresponding to the extension type identified
        //           -- by extnID
        ASN1_START_OCTET_STRING_ENCAPSULATED {

            if (extensionTagNum == kTag_AuthorityKeyIdentifier)
                err = DecodeConvertAuthorityKeyIdentifierExtension(reader, writer, certData);
            else if (extensionTagNum == kTag_SubjectKeyIdentifier)
                err = DecodeConvertSubjectKeyIdentifierExtension(reader, writer, certData);
            else if (extensionTagNum == kTag_KeyUsage)
                err = DecodeConvertKeyUsageExtension(reader, writer, certData);
            else if (extensionTagNum == kTag_BasicConstraints)
                err = DecodeConvertBasicConstraintsExtension(reader, writer, certData);
            else if (extensionTagNum == kTag_ExtendedKeyUsage)
                err = DecodeConvertExtendedKeyUsageExtension(reader, writer, certData);
            else
                err = WEAVE_ERROR_UNSUPPORTED_CERT_FORMAT;
            SuccessOrExit(err);

        } ASN1_END_ENCAPSULATED;

    } ASN1_END_SEQUENCE;

    // Verify that all elements in the extension structure were consumed.
    err = reader.VerifyEndOfContainer();
    SuccessOrExit(err);

    err = reader.ExitContainer(outerContainer);
    SuccessOrExit(err);

exit:
    return err;
}

static WEAVE_ERROR DecodeConvertExtensions(TLVReader& reader, ASN1Writer& writer, WeaveCertificateData& certData)
{
    WEAVE_ERROR err;
    uint64_t tag;

    // extensions [3] EXPLICIT Extensions OPTIONAL
    ASN1_START_CONSTRUCTED(kASN1TagClass_ContextSpecific, 3) {

        // Extensions ::= SEQUENCE SIZE (1..MAX) OF Extension
        ASN1_START_SEQUENCE {

            while (true)
            {
                err = DecodeConvertExtension(reader, writer, certData);
                SuccessOrExit(err);

                // Break the loop if the next certificate element is NOT an extension.
                err = reader.Next();
                SuccessOrExit(err);
                tag = reader.GetTag();
                if (!IsCertificateExtensionTag(tag))
                    break;
            }

        } ASN1_END_SEQUENCE;

    } ASN1_END_CONSTRUCTED;

exit:
    return err;
}

static WEAVE_ERROR DecodeConvertRSASignature(TLVReader& reader, ASN1Writer& writer, WeaveCertificateData& certData)
{
    WEAVE_ERROR err;

    VerifyOrExit(reader.GetType() == kTLVType_ByteString, err = WEAVE_ERROR_WRONG_TLV_TYPE);
    VerifyOrExit(reader.GetTag() == ContextTag(kTag_RSASignature), err = WEAVE_ERROR_UNEXPECTED_TLV_ELEMENT);

    err = writer.PutBitString(0, reader);
    SuccessOrExit(err);

exit:
    return err;
}

static WEAVE_ERROR DecodeConvertECDSASignature(TLVReader& reader, ASN1Writer& writer, WeaveCertificateData& certData)
{
    WEAVE_ERROR err;
    EncodedECDSASignature& encodedSig = certData.Signature.EC;

    VerifyOrExit(reader.GetTag() == ContextTag(kTag_ECDSASignature), err = WEAVE_ERROR_UNEXPECTED_TLV_ELEMENT);

    err = DecodeWeaveECDSASignature(reader, encodedSig);
    SuccessOrExit(err);

    // signatureValue BIT STRING
    // Per RFC3279, the ECDSA signature value is encoded in DER encapsulated in the signatureValue BIT STRING.
    ASN1_START_BIT_STRING_ENCAPSULATED {

        // Ecdsa-Sig-Value ::= SEQUENCE
        ASN1_START_SEQUENCE {

            // r INTEGER
            err = writer.PutValue(kASN1TagClass_Universal, kASN1UniversalTag_Integer, false, encodedSig.R, encodedSig.RLen);
            SuccessOrExit(err);

            // s INTEGER
            err = writer.PutValue(kASN1TagClass_Universal, kASN1UniversalTag_Integer, false, encodedSig.S, encodedSig.SLen);
            SuccessOrExit(err);

        } ASN1_END_SEQUENCE;

    } ASN1_END_ENCAPSULATED;

exit:
    return err;
}

WEAVE_ERROR DecodeConvertTBSCert(TLVReader& reader, ASN1Writer& writer, WeaveCertificateData& certData)
{
    WEAVE_ERROR err;
    uint64_t tag;
    uint32_t weaveSigAlgo;
    OID sigAlgoOID;

    // tbsCertificate TBSCertificate,
    // TBSCertificate ::= SEQUENCE
    ASN1_START_SEQUENCE {

        // version [0] EXPLICIT Version DEFAULT v1
        ASN1_START_CONSTRUCTED(kASN1TagClass_ContextSpecific, 0) {

            // Version ::= INTEGER { v1(0), v2(1), v3(2) }
            ASN1_ENCODE_INTEGER(2);

        } ASN1_END_CONSTRUCTED;

        err = reader.Next(kTLVType_ByteString, ContextTag(kTag_SerialNumber));
        SuccessOrExit(err);

        // serialNumber CertificateSerialNumber
        // CertificateSerialNumber ::= INTEGER
        err = writer.PutValue(kASN1TagClass_Universal, kASN1UniversalTag_Integer, false, reader);
        SuccessOrExit(err);

        // signature AlgorithmIdentifier
        // AlgorithmIdentifier ::= SEQUENCE
        ASN1_START_SEQUENCE {

            err = reader.Next(kTLVType_UnsignedInteger, ContextTag(kTag_SignatureAlgorithm));
            SuccessOrExit(err);

            err = reader.Get(weaveSigAlgo);
            SuccessOrExit(err);

            sigAlgoOID = (OID)(weaveSigAlgo | kOIDCategory_SigAlgo);
            ASN1_ENCODE_OBJECT_ID(sigAlgoOID);

            // parameters ANY DEFINED BY algorithm OPTIONAL
            // Per RFC3279, parameters for RSA must be NULL, parameters for ECDSAWithSHA1 must be absent.
            if (sigAlgoOID == kOID_SigAlgo_MD2WithRSAEncryption ||
                sigAlgoOID == kOID_SigAlgo_MD5WithRSAEncryption ||
                sigAlgoOID == kOID_SigAlgo_SHA1WithRSAEncryption)
            {
                ASN1_ENCODE_NULL;
            }

            certData.SigAlgoOID = sigAlgoOID;

        } ASN1_END_SEQUENCE;

        // issuer Name
        //
        // NOTE: Accept a core tag as well as a context tag to support early Weave certificates where
        // this field was encoded incorrectly.
        //
        err = reader.Next();
        SuccessOrExit(err);
        tag = reader.GetTag();
        VerifyOrExit(tag == CommonTag(kTag_Issuer) || tag == ContextTag(kTag_Issuer), err = WEAVE_ERROR_UNEXPECTED_TLV_ELEMENT);
        VerifyOrExit(reader.GetType() == kTLVType_Path, err = WEAVE_ERROR_WRONG_TLV_TYPE);
        err = DecodeConvertDN(reader, writer, certData.IssuerDN);
        SuccessOrExit(err);

        // validity Validity,
        err = DecodeConvertValidity(reader, writer, certData);
        SuccessOrExit(err);

        // subject Name,
        //
        // NOTE: Also accept core tag here.
        //
        err = reader.Next();
        SuccessOrExit(err);
        tag = reader.GetTag();
        VerifyOrExit(tag == CommonTag(kTag_Subject) || tag == ContextTag(kTag_Subject), err = WEAVE_ERROR_UNEXPECTED_TLV_ELEMENT);
        VerifyOrExit(reader.GetType() == kTLVType_Path, err = WEAVE_ERROR_WRONG_TLV_TYPE);
        err = DecodeConvertDN(reader, writer, certData.SubjectDN);
        SuccessOrExit(err);

        // subjectPublicKeyInfo SubjectPublicKeyInfo,
        err = DecodeConvertSubjectPublicKeyInfo(reader, writer, certData);
        SuccessOrExit(err);

        // If the next element is a certificate extension...
        err = reader.Next();
        SuccessOrExit(err);
        tag = reader.GetTag();
        if (IsCertificateExtensionTag(tag))
        {
            err = DecodeConvertExtensions(reader, writer, certData);
            SuccessOrExit(err);
        }

    } ASN1_END_SEQUENCE;

exit:
    return err;
}

static WEAVE_ERROR DecodeConvertCert(TLVReader& reader, ASN1Writer& writer, WeaveCertificateData& certData)
{
    WEAVE_ERROR err;
    uint64_t tag;
    TLVType containerType;

    if (reader.GetType() == kTLVType_NotSpecified)
    {
        err = reader.Next();
        SuccessOrExit(err);
    }
    VerifyOrExit(reader.GetType() == kTLVType_Structure, err = WEAVE_ERROR_WRONG_TLV_TYPE);
    tag = reader.GetTag();
    VerifyOrExit(tag == ProfileTag(kWeaveProfile_Security, kTag_WeaveCertificate) || tag == AnonymousTag,
                 err = WEAVE_ERROR_UNEXPECTED_TLV_ELEMENT);

    // Record the starting point of the certificate's elements.
    certData.EncodedCert = reader.GetReadPoint();

    err = reader.EnterContainer(containerType);
    SuccessOrExit(err);

    // Certificate ::= SEQUENCE
    ASN1_START_SEQUENCE {

        // tbsCertificate TBSCertificate,
        err = DecodeConvertTBSCert(reader, writer, certData);
        SuccessOrExit(err);

        // signatureAlgorithm   AlgorithmIdentifier
        // AlgorithmIdentifier ::= SEQUENCE
        ASN1_START_SEQUENCE {

            ASN1_ENCODE_OBJECT_ID((OID)certData.SigAlgoOID);

            // parameters ANY DEFINED BY algorithm OPTIONAL
            // Per RFC3279, parameters for RSA must be NULL, parameters for ECDSAWithSHA1 must be absent.
            if (certData.SigAlgoOID == kOID_SigAlgo_MD2WithRSAEncryption ||
                certData.SigAlgoOID == kOID_SigAlgo_MD5WithRSAEncryption ||
                certData.SigAlgoOID == kOID_SigAlgo_SHA1WithRSAEncryption)
            {
                ASN1_ENCODE_NULL;
            }

        } ASN1_END_SEQUENCE;

        // signatureValue BIT STRING
        if (certData.SigAlgoOID == kOID_SigAlgo_MD2WithRSAEncryption ||
            certData.SigAlgoOID == kOID_SigAlgo_MD5WithRSAEncryption ||
            certData.SigAlgoOID == kOID_SigAlgo_SHA1WithRSAEncryption)
        {
            err = DecodeConvertRSASignature(reader, writer, certData);
            SuccessOrExit(err);
        }

        else
        {
            err = DecodeConvertECDSASignature(reader, writer, certData);
            SuccessOrExit(err);
        }

    } ASN1_END_SEQUENCE;

    // Verify no more elements in certificate.
    err = reader.VerifyEndOfContainer();
    SuccessOrExit(err);

    err = reader.ExitContainer(containerType);
    SuccessOrExit(err);

exit:
    return err;
}

NL_DLL_EXPORT WEAVE_ERROR ConvertWeaveCertToX509Cert(const uint8_t *weaveCert, uint32_t weaveCertLen, uint8_t *x509CertBuf, uint32_t x509CertBufSize, uint32_t& x509CertLen)
{
    WEAVE_ERROR err;
    TLVReader reader;
    ASN1Writer writer;
    WeaveCertificateData certData;

    reader.Init(weaveCert, weaveCertLen);

    writer.Init(x509CertBuf, x509CertBufSize);

    memset(&certData, 0, sizeof(certData));

    err = DecodeConvertCert(reader, writer, certData);
    SuccessOrExit(err);

    err = writer.Finalize();
    SuccessOrExit(err);

    x509CertLen = writer.GetLengthWritten();

exit:
    return err;
}

WEAVE_ERROR DecodeWeaveCert(const uint8_t *weaveCert, uint32_t weaveCertLen, WeaveCertificateData& certData)
{
    TLVReader reader;

    reader.Init(weaveCert, weaveCertLen);
    return DecodeWeaveCert(reader, certData);
}

WEAVE_ERROR DecodeWeaveCert(TLVReader& reader, WeaveCertificateData& certData)
{
    ASN1Writer writer;

    writer.InitNullWriter();
    memset(&certData, 0, sizeof(certData));
    return DecodeConvertCert(reader, writer, certData);
}

WEAVE_ERROR DecodeWeaveDN(TLVReader& reader, WeaveDN& dn)
{
    ASN1Writer writer;

    writer.InitNullWriter();
    memset(&dn, 0, sizeof(dn));
    return DecodeConvertDN(reader, writer, dn);
}


} // namespace Security
} // namespace Profiles
} // namespace Weave
} // namespace nl
