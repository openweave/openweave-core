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
 *      This file implements utility functions for reading, writing,
 *      parsing, resigning, encoding, and decoding Weave certificates.
 *
 */

#define __STDC_FORMAT_MACROS

#include <ctype.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include "weave-tool.h"

using namespace nl::Weave::Profiles::Security;
using namespace nl::Weave::ASN1;

enum {
    kMaxWeaveCertInflationFactor = 5 // Maximum ratio of the size of buffer needed to hold an X.509 certificate
                                     // relative to the size of buffer needed to hold its Weave counterpart.
                                     // This value (5) is conservatively big given that certificates contain large
                                     // amounts of incompressible data.  In practice, the factor is going to be
                                     // much closer to 1.5.
};

bool ReadCert(const char *fileName, X509 *& cert)
{
    CertFormat origCertFmt;
    return ReadCert(fileName, cert, origCertFmt);
}

bool ReadCert(const char *fileName, X509 *& cert, CertFormat& origCertFmt)
{
    bool res = true;
    WEAVE_ERROR err;
    uint8_t *certBuf = NULL;
    const uint8_t *p;
    uint32_t certLen;
    CertFormat curCertFmt;

    cert = NULL;

    if (!ReadFileIntoMem(fileName, certBuf, certLen))
        ExitNow(res = false);

    curCertFmt = origCertFmt = DetectCertFormat(certBuf, certLen);

    if (curCertFmt == kCertFormat_X509_PEM)
    {
        if (!X509PEMToDER(certBuf, certLen))
            ExitNow(res = false);
        curCertFmt = kCertFormat_X509_DER;
    }
    else if (curCertFmt == kCertFormat_Weave_Base64)
    {
        if (!Base64Decode(certBuf, certLen, certBuf, certLen, certLen))
            ExitNow(res = false);
        curCertFmt = kCertFormat_Weave_Raw;
    }

    if (curCertFmt == kCertFormat_Weave_Raw)
    {
        uint32_t convertedCertLen = certLen * kMaxWeaveCertInflationFactor;
        uint8_t *convertedCertBuf = (uint8_t *)malloc((size_t)convertedCertLen);

        err = ConvertWeaveCertToX509Cert(certBuf, certLen, convertedCertBuf, convertedCertLen, convertedCertLen);
        if (err != WEAVE_NO_ERROR)
        {
            free(convertedCertBuf);
            fprintf(stderr, "weave: Error converting certificate: %s\n", nl::ErrorStr(err));
            ExitNow(res = false);
        }

        free(certBuf);
        certBuf = convertedCertBuf;
        certLen = convertedCertLen;

        curCertFmt= kCertFormat_X509_DER;
    }

    p = certBuf;
    cert = d2i_X509(NULL, &p, certLen);
    if (cert == NULL)
        ReportOpenSSLErrorAndExit("d2i_X509", res = false);

exit:
    if (cert != NULL && !res) {
        X509_free(cert);
        cert = NULL;
    }
    if (certBuf != NULL) {
        free(certBuf);
    }
    return res;
}

bool ReadCertPEM(const char *fileName, X509 *& cert)
{
    bool res = true;
    FILE *file = NULL;

    file = fopen(fileName, "r");
    if (file == NULL)
    {
        fprintf(stderr, "Unable to open %s: %s\n", fileName, strerror(errno));
        return false;
    }

    cert = NULL;
    if (PEM_read_X509(file, &cert, NULL, NULL) == NULL)
    {
        fprintf(stderr, "Unable to read %s\n", fileName);
        ReportOpenSSLErrorAndExit("PEM_read_X509", res = false);
    }

exit:
    if (file != NULL)
        fclose(file);
    return res;
}

bool ReadWeaveCert(const char *fileName, uint8_t *& certBuf, uint32_t& certLen)
{
    bool res = true;
    CertFormat certFmt;

    certBuf = NULL;
    certLen = 0;

    if (!ReadFileIntoMem(fileName, certBuf, certLen))
        ExitNow(res = false);

    certFmt = DetectCertFormat(certBuf, certLen);
    if (certFmt != kCertFormat_Weave_Raw && certFmt != kCertFormat_Weave_Base64)
    {
        fprintf(stderr, "weave: Error reading %s\nUnrecognized certificate format\n", fileName);
        ExitNow(res = false);
    }

    if (certFmt == kCertFormat_Weave_Base64)
    {
        uint8_t *rawCertBuf = Base64Decode(certBuf, certLen, NULL, 0, certLen);
        if (rawCertBuf == NULL)
        {
            ExitNow(res = false);
        }
        free(certBuf);
        certBuf = rawCertBuf;
    }

exit:
    if (!res && certBuf != NULL)
    {
        free(certBuf);
        certBuf = NULL;
        certLen = 0;
    }
    return res;
}

bool LoadWeaveCert(const char *fileName, bool isTrused, WeaveCertificateSet& certSet, uint8_t *& certBuf)
{
    bool res = true;
    WEAVE_ERROR err;
    WeaveCertificateData *cert;
    uint32_t certLen;

    if (certSet.CertCount == certSet.MaxCerts)
    {
        fprintf(stderr, "weave: Too many input certificates.\n");
        return false;
    }

    res = ReadWeaveCert(fileName, certBuf, certLen);
    if (!res)
        ExitNow();

    err = certSet.LoadCert(certBuf, certLen, (isTrused) ? 0 : kDecodeFlag_GenerateTBSHash, cert);
    if (err != WEAVE_NO_ERROR)
    {
        fprintf(stderr, "weave: Error reading %s\n%s\n", fileName, nl::ErrorStr(err));
        ExitNow(res = false);
    }

    if (isTrused)
        cert->CertFlags |= kCertFlag_IsTrusted;

exit:
    if (!res && certBuf != NULL)
    {
        free(certBuf);
        certBuf = NULL;
    }
    return res;
}

bool WriteCert(X509 *cert, FILE *file, const char *fileName, CertFormat certFmt)
{
    bool res = true;
    uint8_t *weaveCert = NULL;
    uint32_t weaveCertLen;
    char *weaveCertBase64 = NULL;

    if (certFmt == kCertFormat_X509_PEM)
    {
        if (PEM_write_X509(file, cert) == 0)
            ReportOpenSSLErrorAndExit("PEM_write_X509", res = false);
    }

    else if (certFmt == kCertFormat_X509_DER)
    {
        if (i2d_X509_fp(file, cert) == 0)
            ReportOpenSSLErrorAndExit("i2d_X509_fp", res = false);
    }

    else if (certFmt == kCertFormat_Weave_Raw || certFmt == kCertFormat_Weave_Base64)
    {
        if (!WeaveEncodeCert(cert, weaveCert, weaveCertLen))
            ExitNow(res = false);

        if (certFmt == kCertFormat_Weave_Raw)
        {
            if (fwrite(weaveCert, 1, weaveCertLen, file) != weaveCertLen)
            {
                fprintf(stderr, "weave: ERROR: Unable to write to %s\n%s\n", fileName, strerror(ferror(file) ? errno : ENOSPC));
                ExitNow(res = false);
            }
        }

        else
        {
            weaveCertBase64 = Base64Encode(weaveCert, weaveCertLen);
            if (weaveCertBase64 == NULL)
                ExitNow(res = false);

            if (fputs(weaveCertBase64, file) == EOF)
            {
                fprintf(stderr, "weave: ERROR: Unable to write output certificate file (%s)\n%s\n", fileName, strerror(ferror(file) ? errno : ENOSPC));
                ExitNow(res = false);
            }
        }
    }

exit:
    if (weaveCert != NULL)
        free(weaveCert);
    if (weaveCertBase64 != NULL)
        free(weaveCertBase64);
    return res;
}

bool WeaveEncodeCert(X509 *cert, uint8_t *& encodedCert, uint32_t& encodedCertLen)
{
    bool res = true;
    WEAVE_ERROR err;
    uint8_t *derEncodedCert = NULL;
    int32_t derEncodedCertLen;

    encodedCert = NULL;

    derEncodedCertLen = i2d_X509(cert, &derEncodedCert);
    if (derEncodedCertLen < 0)
        ReportOpenSSLErrorAndExit("i2d_X509", res = false);

    encodedCert = (uint8_t *)malloc(derEncodedCertLen);
    if (encodedCert == NULL)
    {
        fprintf(stderr, "Failed to encode certificate: Memory allocation failure\n");
        ExitNow(res = false);
    }

    err = ConvertX509CertToWeaveCert(derEncodedCert, derEncodedCertLen, encodedCert, derEncodedCertLen, encodedCertLen);
    if (err != WEAVE_NO_ERROR)
    {
        fprintf(stderr, "Failed to create device certificate: ConvertX509CertToWeaveCert() failed\n%s\n", nl::ErrorStr(err));
        ExitNow(res = false);
    }

exit:
    if (encodedCert != NULL && !res)
    {
        free(encodedCert);
        encodedCert = NULL;
    }
    return res;
}

bool MakeDeviceCert(uint64_t devId, X509 *caCert, EVP_PKEY *caKey, const char *curveName, const struct tm& validFrom, uint32_t validDays,
                    const EVP_MD *sigHashAlgo, X509 *& devCert, EVP_PKEY *& devKey)
{
    bool res = true;
    bool keySupplied = (devKey != NULL);

    devCert = NULL;

    if (!keySupplied && !GenerateKeyPair(curveName, devKey))
        ExitNow(res = false);

    devCert = X509_new();
    if (devCert == NULL)
        ReportOpenSSLErrorAndExit("X509_new", res = false);

    if (!X509_set_version(devCert, 2))
        ReportOpenSSLErrorAndExit("X509_set_version", res = false);

    if (!SetCertSerialNumber(devCert))
        ExitNow(res = false);

    // Set the certificate validity time.
    if (!SetValidityTime(devCert, validFrom, validDays))
        ExitNow(res = false);

    if (!X509_set_pubkey(devCert, devKey))
        ReportOpenSSLErrorAndExit("X509_set_pubkey", res = false);

    if (!SetWeaveCertSubjectName(devCert, gNIDWeaveDeviceId, devId))
        ExitNow(res = false);

    if (!X509_set_issuer_name(devCert, X509_get_subject_name(caCert)))
        ReportOpenSSLErrorAndExit("X509_set_issuer_name", res = false);

    if (!AddExtension(devCert, NID_basic_constraints, "critical,CA:FALSE") ||
        !AddExtension(devCert, NID_key_usage, "critical,digitalSignature,keyEncipherment") ||
        !AddExtension(devCert, NID_ext_key_usage, "critical,clientAuth,serverAuth"))
        ExitNow(res = false);

    if (!AddSubjectKeyId(devCert))
        ExitNow(res = false);

    if (!AddAuthorityKeyId(devCert, caCert))
        ExitNow(res = false);

    if (!X509_sign(devCert, caKey, sigHashAlgo))
        ReportOpenSSLErrorAndExit("X509_sign", res = false);

exit:
    if (devCert != NULL && !res)
    {
        X509_free(devCert);
        devCert = NULL;
    }
    if (!keySupplied && devKey != NULL && !res)
    {
        EVP_PKEY_free(devKey);
        devKey = NULL;
    }
    return res;
}

bool MakeCACert(uint64_t newCertId, EVP_PKEY *newCertKey, X509 *caCert, EVP_PKEY *caKey, const struct tm& validFrom, uint32_t validDays,
                const EVP_MD *sigHashAlgo, X509 *& newCert)
{
    bool res = true;

    // Create new certificate object.
    newCert = X509_new();
    if (newCert == NULL)
        ReportOpenSSLErrorAndExit("X509_new", res = false);

    // If a signing CA certificate was not provided, then arrange to generate a self-signed
    // certificate.
    if (caCert == NULL)
    {
        caCert = newCert;
        caKey = newCertKey;
    }

    // Set the certificate version (must be 2, a.k.a. v3).
    if (!X509_set_version(newCert, 2))
        ReportOpenSSLErrorAndExit("X509_set_version", res = false);

    // Generate a serial number for the cert.
    if (!SetCertSerialNumber(newCert))
        ExitNow(res = false);

    // Set the certificate validity time.
    if (!SetValidityTime(newCert, validFrom, validDays))
        ExitNow(res = false);

    // Set the certificate's public key.
    if (!X509_set_pubkey(newCert, newCertKey))
        ReportOpenSSLErrorAndExit("X509_set_pubkey", res = false);

    // Create a subject name consisting of a single RDN containing the WeaveCAId attribute.
    if (!SetWeaveCertSubjectName(newCert, gNIDWeaveCAId, newCertId))
        ExitNow(res = false);

    // Set the issuer name for the certificate. In the case of a self-signed cert, this will be
    // the new cert's subject name.
    if (!X509_set_issuer_name(newCert, X509_get_subject_name(caCert)))
        ReportOpenSSLErrorAndExit("X509_set_issuer_name", res = false);

    // Add the appropriate extensions for a CA certificate.
    if (!AddExtension(newCert, NID_basic_constraints, "critical,CA:TRUE") ||
        !AddExtension(newCert, NID_key_usage, "critical,keyCertSign,cRLSign"))
        ExitNow(res = false);

    // Generate a subject key id for the certificate.
    if (!AddSubjectKeyId(newCert))
        ExitNow(res = false);

    // Add the authority key id from the signing certificate. For self-signed cert's this will
    // be the same as new cert's subject key id.
    if (!AddAuthorityKeyId(newCert, caCert))
        ExitNow(res = false);

    // Sign the new certificate.
    if (!X509_sign(newCert, caKey, sigHashAlgo))
        ReportOpenSSLErrorAndExit("X509_sign", res = false);

exit:
    if (newCert != NULL && !res)
    {
        X509_free(newCert);
        newCert = NULL;
    }
    return res;
}

bool MakeCodeSigningCert(uint64_t newCertId, EVP_PKEY *newCertKey, X509 *caCert, EVP_PKEY *caKey, const struct tm& validFrom, uint32_t validDays,
                         const EVP_MD *sigHashAlgo, X509 *& newCert)
{
    bool res = true;

    // Create new certificate object.
    newCert = X509_new();
    if (newCert == NULL)
        ReportOpenSSLErrorAndExit("X509_new", res = false);

    // Set the certificate version (must be 2, a.k.a. v3).
    if (!X509_set_version(newCert, 2))
        ReportOpenSSLErrorAndExit("X509_set_version", res = false);

    // Generate a serial number for the cert.
    if (!SetCertSerialNumber(newCert))
        ExitNow(res = false);

    // Set the certificate validity time.
    if (!SetValidityTime(newCert, validFrom, validDays))
        ExitNow(res = false);

    // Set the certificate's public key.
    if (!X509_set_pubkey(newCert, newCertKey))
        ReportOpenSSLErrorAndExit("X509_set_pubkey", res = false);

    // Create a subject name consisting of a single RDN containing the WeaveSoftwarePublisherId attribute.
    if (!SetWeaveCertSubjectName(newCert, gNIDWeaveSoftwarePublisherId, newCertId))
        ExitNow(res = false);

    // Set the issuer name for the certificate. In the case of a self-signed cert, this will be
    // the new cert's subject name.
    if (!X509_set_issuer_name(newCert, X509_get_subject_name(caCert)))
        ReportOpenSSLErrorAndExit("X509_set_issuer_name", res = false);

    // Add the appropriate extensions for a code signing certificate.
    if (!AddExtension(newCert, NID_basic_constraints, "critical,CA:FALSE") ||
        !AddExtension(newCert, NID_key_usage, "critical,digitalSignature") ||
        !AddExtension(newCert, NID_ext_key_usage, "critical,codeSigning"))
        ExitNow(res = false);

    // Generate a subject key id for the certificate.
    if (!AddSubjectKeyId(newCert))
        ExitNow(res = false);

    // Add the authority key id from the signing certificate. For self-signed cert's this will
    // be the same as new cert's subject key id.
    if (!AddAuthorityKeyId(newCert, caCert))
        ExitNow(res = false);

    // Sign the new certificate.
    if (!X509_sign(newCert, caKey, sigHashAlgo))
        ReportOpenSSLErrorAndExit("X509_sign", res = false);

exit:
    if (newCert != NULL && !res)
    {
        X509_free(newCert);
        newCert = NULL;
    }
    return res;
}

bool MakeServiceEndpointCert(uint64_t newCertId, EVP_PKEY *newCertKey, X509 *caCert, EVP_PKEY *caKey, const struct tm& validFrom, uint32_t validDays,
                             const EVP_MD *sigHashAlgo, X509 *& newCert)
{
    bool res = true;

    // Create new certificate object.
    newCert = X509_new();
    if (newCert == NULL)
        ReportOpenSSLErrorAndExit("X509_new", res = false);

    // Set the certificate version (must be 2, a.k.a. v3).
    if (!X509_set_version(newCert, 2))
        ReportOpenSSLErrorAndExit("X509_set_version", res = false);

    // Generate a serial number for the cert.
    if (!SetCertSerialNumber(newCert))
        ExitNow(res = false);

    // Set the certificate validity time.
    if (!SetValidityTime(newCert, validFrom, validDays))
        ExitNow(res = false);

    // Set the certificate's public key.
    if (!X509_set_pubkey(newCert, newCertKey))
        ReportOpenSSLErrorAndExit("X509_set_pubkey", res = false);

    // Create a subject name consisting of a single RDN containing the WeaveServiceEndpointId attribute.
    if (!SetWeaveCertSubjectName(newCert, gNIDWeaveServiceEndpointId, newCertId))
        ExitNow(res = false);

    // Set the issuer name for the certificate.
    if (!X509_set_issuer_name(newCert, X509_get_subject_name(caCert)))
        ReportOpenSSLErrorAndExit("X509_set_issuer_name", res = false);

    // Add the appropriate extensions for a service endpoint certificate.
    if (!AddExtension(newCert, NID_basic_constraints, "critical,CA:FALSE") ||
        !AddExtension(newCert, NID_key_usage, "critical,digitalSignature,keyEncipherment") ||
        !AddExtension(newCert, NID_ext_key_usage, "critical,clientAuth,serverAuth"))
        ExitNow(res = false);

    // Generate a subject key id for the certificate.
    if (!AddSubjectKeyId(newCert))
        ExitNow(res = false);

    // Add the authority key id from the signing certificate. For self-signed cert's this will
    // be the same as new cert's subject key id.
    if (!AddAuthorityKeyId(newCert, caCert))
        ExitNow(res = false);

    // Sign the new certificate.
    if (!X509_sign(newCert, caKey, sigHashAlgo))
        ReportOpenSSLErrorAndExit("X509_sign", res = false);

exit:
    if (newCert != NULL && !res)
    {
        X509_free(newCert);
        newCert = NULL;
    }
    return res;
}

bool MakeGeneralCert(const char *subject, EVP_PKEY *newCertKey, X509 *caCert, EVP_PKEY *caKey, const struct tm& validFrom, uint32_t validDays,
                     const EVP_MD *sigHashAlgo, X509 *& newCert)
{
    bool res = true;

    // Create new certificate object.
    newCert = X509_new();
    if (newCert == NULL)
        ReportOpenSSLErrorAndExit("X509_new", res = false);

    // If a signing CA certificate was not provided, then arrange to generate a self-signed
    // certificate.
    if (caCert == NULL)
    {
        caCert = newCert;
        caKey = newCertKey;
    }

    // Set the certificate version (must be 2, a.k.a. v3).
    if (!X509_set_version(newCert, 2))
        ReportOpenSSLErrorAndExit("X509_set_version", res = false);

    // Generate a serial number for the cert.
    if (!SetCertSerialNumber(newCert))
        ExitNow(res = false);

    // Set the certificate validity time.
    if (!SetValidityTime(newCert, validFrom, validDays))
        ExitNow(res = false);

    // Set the certificate's public key.
    if (!X509_set_pubkey(newCert, newCertKey))
        ReportOpenSSLErrorAndExit("X509_set_pubkey", res = false);

    // Create a subject name consisting of a single RDN containing the specific subject name.
    if (!SetCertSubjectName(newCert, NID_commonName, subject))
        ExitNow(res = false);

    // Set the issuer name for the certificate. In the case of a self-signed cert, this will be
    // the new cert's subject name.
    if (!X509_set_issuer_name(newCert, X509_get_subject_name(caCert)))
        ReportOpenSSLErrorAndExit("X509_set_issuer_name", res = false);

    // Add the appropriate extensions for a non-CA certificate.
    if (!AddExtension(newCert, NID_basic_constraints, "critical,CA:FALSE") ||
        !AddExtension(newCert, NID_key_usage, "critical,digitalSignature,keyEncipherment") ||
        !AddExtension(newCert, NID_ext_key_usage, "critical,clientAuth,serverAuth"))
        ExitNow(res = false);

    // Generate a subject key id for the certificate.
    if (!AddSubjectKeyId(newCert))
        ExitNow(res = false);

    // Add the authority key id from the signing certificate. For self-signed cert's this will
    // be the same as new cert's subject key id.
    if (!AddAuthorityKeyId(newCert, caCert))
        ExitNow(res = false);

    // Sign the new certificate.
    if (!X509_sign(newCert, caKey, sigHashAlgo))
        ReportOpenSSLErrorAndExit("X509_sign", res = false);

exit:
    if (newCert != NULL && !res)
    {
        X509_free(newCert);
        newCert = NULL;
    }
    return res;
}

bool ResignCert(X509 *cert, X509 *caCert, EVP_PKEY *caKey, const EVP_MD *sigHashAlgo)
{
    bool res = true;
    int authKeyIdExtLoc = -1;

    if (!SetCertSerialNumber(cert))
        ExitNow(res = false);

    if (!X509_set_issuer_name(cert, X509_get_subject_name(caCert)))
        ReportOpenSSLErrorAndExit("X509_set_issuer_name", res = false);

    // Remove any existing authority key id
    authKeyIdExtLoc = X509_get_ext_by_NID(cert, NID_authority_key_identifier, -1);
    if (authKeyIdExtLoc != -1)
    {
        if (X509_delete_ext(cert, authKeyIdExtLoc) == NULL)
            ReportOpenSSLErrorAndExit("X509_delete_ext", res = false);
    }

    if (!AddAuthorityKeyId(cert, caCert))
        ExitNow(res = false);

    if (!X509_sign(cert, caKey, sigHashAlgo))
        ReportOpenSSLErrorAndExit("X509_sign", res = false);

exit:
    return res;
}

// ASN1_INTEGER_set_uint64() is a new method, which was introduced in OpenSSL version 1.1.0.
// This method is used in the SetCertSerialNumber() function to store the serial number
// as an ASN1 integer value within the certificate.
// For older versions of OpenSSL c2i_ASN1_INTEGER() method was available and it is used in
// the SetCertSerialNumber() function to achieve similar functionality.
#if (OPENSSL_VERSION_NUMBER >= 0x10100000L)

bool SetCertSerialNumber(X509 *cert)
{
    bool res = true;
    uint64_t rnd;
    ASN1_INTEGER *snInt = X509_get_serialNumber(cert);

    // Generate a random value to be used as the serial number.
    if (!RAND_bytes(reinterpret_cast<uint8_t *>(&rnd), sizeof(rnd)))
        ReportOpenSSLErrorAndExit("RAND_bytes", res = false);

    // Avoid negative numbers.
    rnd &= 0x7FFFFFFFFFFFFFFF;

    // Store the serial number as an ASN1 integer value within the certificate.
    if (ASN1_INTEGER_set_uint64(snInt, rnd) == 0)
        ReportOpenSSLErrorAndExit("ASN1_INTEGER_set_uint64", res = false);

exit:
    return res;
}

#else // (OPENSSL_VERSION_NUMBER < 0x10100000L)

bool SetCertSerialNumber(X509 *cert)
{
    bool res = true;
    uint8_t rnd[8];
    const uint8_t *rndPtr = rnd;
    ASN1_INTEGER *snInt = X509_get_serialNumber(cert);

    // Generate a random value to be used as the serial number.
    if (!RAND_bytes(rnd, sizeof(rnd)))
        ReportOpenSSLErrorAndExit("RAND_bytes", res = false);

    // Avoid negative numbers.
    rnd[0] = rnd[0] & 0x7F;

    // Store the serial number as an ASN1 integer value within the certificate.
    if (c2i_ASN1_INTEGER(&snInt, &rndPtr, sizeof(rnd)) == NULL)
        ReportOpenSSLErrorAndExit("c2i_ASN1_INTEGER", res = false);

exit:
    return res;
}

#endif // (OPENSSL_VERSION_NUMBER >= 0x10100000L)

bool SetCertSubjectName(X509 *cert, int attrNID, const char *subjectName)
{
    bool res = true;
    int len = strlen(subjectName);

    if (!X509_NAME_add_entry_by_NID(X509_get_subject_name(cert), attrNID, MBSTRING_UTF8, (unsigned char *)subjectName, len, -1, 0))
        ReportOpenSSLErrorAndExit("X509_NAME_add_entry_by_NID", res = false);

exit:
    return res;
}

bool SetWeaveCertSubjectName(X509 *cert, int attrNID, uint64_t id)
{
    char idStr[17];

    snprintf(idStr, sizeof(idStr), "%016" PRIX64 "", id);
    return SetCertSubjectName(cert, attrNID, idStr);
}

bool SetCertTimeField(ASN1_TIME *s, const struct tm& value)
{
    char timeStr[16];

    // Encode the time as a string in the form YYYYMMDDHHMMSSZ.
    snprintf(timeStr, sizeof(timeStr), "%04d%02d%02d%02d%02d%02dZ",
             value.tm_year + 1900, value.tm_mon + 1, value.tm_mday,
             value.tm_hour, value.tm_min, value.tm_sec);

    // X.509/RFC-5280 mandates that times before 2050 UTC must be encoded as ASN.1 UTCTime values, while
    // times equal or greater than 2050 must be encoded as GeneralizedTime values.  The only difference
    // between the two is the number of digits in the year -- 4 for GeneralizedTime, 2 for UTCTime.
    //
    // The OpenSSL ASN1_TIME_set_string() function DOES NOT handle picking the correct format based
    // on the given year.  Thus the caller MUST pass a correctly formatted string or the resultant
    // certificate will be malformed.

    bool useUTCTime = ((value.tm_year + 1900) < 2050);

    if (!ASN1_TIME_set_string(s, timeStr + (useUTCTime ? 2 : 0)))
    {
        fprintf(stderr, "OpenSSL ASN1_TIME_set_string() failed\n");
        return false;
    }

    return true;
}

bool SetValidityTime(X509 *cert, const struct tm& validFrom, uint32_t validDays)
{
    struct tm validTo;
    time_t validToTime;

    // Compute the validity end date.
    // Note that this computation is done in local time, despite the fact that the certificate validity times are
    // UTC.  This is because the standard posix time functions do not make it easy to convert a struct tm containing
    // UTC to a time_t value without manipulating the TZ environment variable.
    validTo = validFrom;
    validTo.tm_mday += validDays;
    validTo.tm_sec -= 1; // Ensure validity period is exactly a multiple of a day.
    validTo.tm_isdst = -1;
    validToTime = mktime(&validTo);
    if (validToTime == (time_t)-1)
    {
        fprintf(stderr, "mktime() failed\n");
        return false;
    }
    localtime_r(&validToTime, &validTo);

    // Set the certificate's notBefore date.
    if (!SetCertTimeField(X509_get_notBefore(cert), validFrom))
        return false;

    // Set the certificate's notAfter date.
    if (!SetCertTimeField(X509_get_notAfter(cert), validTo))
        return false;

    return true;
}

bool AddExtension(X509 *devCert, int extNID, const char *extStr)
{
    bool res = true;
    X509_EXTENSION *ex;

    ex = X509V3_EXT_nconf_nid(NULL, NULL, extNID, (char *)extStr);
    if (ex == NULL)
        ReportOpenSSLErrorAndExit("X509V3_EXT_conf_nid", res = false);

    if (!X509_add_ext(devCert, ex, -1))
        ReportOpenSSLErrorAndExit("X509_add_ext", res = false);

exit:
    if (ex != NULL)
        X509_EXTENSION_free(ex);
    return res;
}

bool AddSubjectKeyId(X509 *cert)
{
    bool res = true;
    ASN1_OCTET_STRING *pkHashOS = NULL;
    ASN1_BIT_STRING *pk = X509_get0_pubkey_bitstr(cert);
    unsigned char pkHash[EVP_MAX_MD_SIZE];
    unsigned int pkHashLen;

    if (!EVP_Digest(pk->data, pk->length, pkHash, &pkHashLen, EVP_sha1(), NULL))
        ReportOpenSSLErrorAndExit("EVP_Digest", res = false);

    if (pkHashLen != 20)
    {
        fprintf(stderr, "Unexpected hash length returned from EVP_Digest()\n");
        ExitNow(res = false);
    }

    pkHashOS = ASN1_STRING_type_new(V_ASN1_OCTET_STRING);
    if (pkHashOS == NULL)
        ReportOpenSSLErrorAndExit("ASN1_STRING_type_new", res = false);

    /* Use "truncated" SHA-1 hash. Per RFC5280:
     *
     *     "(2) The keyIdentifier is composed of a four-bit type field with the value 0100 followed
     *     by the least significant 60 bits of the SHA-1 hash of the value of the BIT STRING
     *     subjectPublicKey (excluding the tag, length, and number of unused bits)."
     */
    pkHash[12] = 0x40 | (pkHash[12] & 0xF);

    if (!ASN1_STRING_set(reinterpret_cast<ASN1_STRING *>(pkHashOS), &pkHash[12], 8))
        ReportOpenSSLErrorAndExit("ASN1_STRING_set", res = false);

    if (!X509_add1_ext_i2d(cert, NID_subject_key_identifier, pkHashOS, 0, X509V3_ADD_APPEND))
        ReportOpenSSLErrorAndExit("X509_add1_ext_i2d", res = false);

exit:
    if (pkHashOS != NULL)
        ASN1_STRING_free(reinterpret_cast<ASN1_STRING *>(pkHashOS));
    return res;
}

bool AddAuthorityKeyId(X509 *cert, X509 *caCert)
{
    bool res = true;
    AUTHORITY_KEYID *akid;
    int isCritical, index = 0;

    akid = AUTHORITY_KEYID_new();
    if (akid == NULL)
    {
        fprintf(stderr, "Memory allocation failure\n");
        ExitNow(res = false);
    }

    akid->keyid = (ASN1_OCTET_STRING *)X509_get_ext_d2i(caCert, NID_subject_key_identifier, &isCritical, &index);
    if (akid->keyid == NULL)
        ReportOpenSSLErrorAndExit("X509_get_ext_d2i", res = false);

    if (!X509_add1_ext_i2d(cert, NID_authority_key_identifier, akid, 0, X509V3_ADD_APPEND))
        ReportOpenSSLErrorAndExit("X509_add1_ext_i2d", res = false);

exit:
    return res;
}

bool X509PEMToDER(uint8_t *cert, uint32_t& certLen)
{
    bool res = true;
    BIO *certBIO;
    char *name = NULL;
    char *header = NULL;
    uint8_t *data = NULL;
    long dataLen;

    certBIO = BIO_new_mem_buf((void*)cert, certLen);
    if (certBIO == NULL)
    {
        fprintf(stderr, "Memory allocation error\n");
        ExitNow(res = false);
    }

    if (!PEM_read_bio(certBIO, &name, &header, &data, &dataLen))
        ReportOpenSSLErrorAndExit("PEM_read_bio", res = false);

    memcpy(cert, data, dataLen);
    certLen = dataLen;

exit:
    if (certBIO != NULL)
        BIO_free(certBIO);
    if (name != NULL)
        OPENSSL_free(name);
    if (header != NULL)
        OPENSSL_free(header);
    if (data != NULL)
        OPENSSL_free(data);
    return res;
}

bool X509DERToPEM(uint8_t *cert, uint32_t& certLen, uint32_t bufLen)
{
    bool res = true;
    BIO *certBIO;
    BUF_MEM *pemBuf;

    certBIO = BIO_new(BIO_s_mem());
    if (certBIO == NULL)
    {
        fprintf(stderr, "Memory allocation error\n");
        ExitNow(res = false);
    }

    if (!PEM_write_bio(certBIO, "CERTIFICATE", (char *)"", cert, certLen))
        ReportOpenSSLErrorAndExit("PEM_write_bio", res = false);

    (void)BIO_flush(certBIO);

    BIO_get_mem_ptr(certBIO, &pemBuf);

    if (pemBuf->length > bufLen)
    {
        fprintf(stderr, "Certificate too big\n");
        ExitNow(res = false);
    }

    memcpy(cert, pemBuf->data, pemBuf->length);
    certLen = pemBuf->length;

exit:
    if (certBIO != NULL)
        BIO_free(certBIO);
    return res;
}

CertFormat DetectCertFormat(uint8_t *cert, uint32_t certLen)
{
    static const uint8_t weaveRawPrefix[] = { 0xD5, 0x00, 0x00, 0x04, 0x00, 0x01, 0x00 };
    static const char *weaveB64Prefix = "1QAABAAB";
    static const uint32_t weaveB64PrefixLen = sizeof(weaveB64Prefix) - 1;
    static const char *pemMarker = "-----BEGIN CERTIFICATE-----";

    if (certLen > sizeof(weaveRawPrefix) && memcmp(cert, weaveRawPrefix, sizeof(weaveRawPrefix)) == 0)
        return kCertFormat_Weave_Raw;

    if (certLen > weaveB64PrefixLen && memcmp(cert, weaveB64Prefix, weaveB64PrefixLen) == 0)
        return kCertFormat_Weave_Base64;

    if (ContainsPEMMarker(pemMarker, cert, certLen))
        return kCertFormat_X509_PEM;

    return kCertFormat_X509_DER;
}
