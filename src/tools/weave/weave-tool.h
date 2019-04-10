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
 *      This file defines the public API symbols, constants, and interfaces
 *      for the 'weave' tool.
 *
 *      The 'weave' tool is a command line interface (CLI) utility
 *      used, primarily, for generating and manipulating Weave
 *      security certificate material.
 *
 */

#ifndef WEAVE_TOOL_H_
#define WEAVE_TOOL_H_

#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/objects.h>
#include <openssl/x509v3.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/bn.h>

#include <Weave/Core/WeaveCore.h>
#include <Weave/Support/CodeUtils.h>
#include <Weave/Profiles/security/WeaveSecurity.h>
#include <Weave/Profiles/security/WeavePrivateKey.h>
#include <Weave/Profiles/security/WeaveCert.h>
#include <Weave/Support/ASN1.h>
#include <Weave/Support/ASN1OID.h>
#include <Weave/Support/ErrorStr.h>
#include <Weave/Support/nlargparser.hpp>
#include <Weave/WeaveVersion.h>

using namespace nl::ArgParser;

using nl::Weave::ASN1::OID;

#define COPYRIGHT_STRING "Copyright (c) 2019 Google LLC.\nCopyright (c) 2013-2017 Nest Labs, Inc.\nAll rights reserved.\n"

#define MAX_CERT_SIZE 65536
#define MAX_KEY_SIZE 65536

enum CertFormat
{
    kCertFormat_Unknown                         = 0,
    kCertFormat_X509_DER,
    kCertFormat_X509_PEM,
    kCertFormat_Weave_Raw,
    kCertFormat_Weave_Base64
};

enum KeyFormat
{
    kKeyFormat_Unknown                          = 0,
    kKeyFormat_DER,
    kKeyFormat_DER_PKCS8,
    kKeyFormat_PEM,
    kKeyFormat_PEM_PKCS8,
    kKeyFormat_Weave_Raw,
    kKeyFormat_Weave_Base64
};

extern bool Cmd_GenCACert(int argc, char *argv[]);
extern bool Cmd_GenDeviceCert(int argc, char *argv[]);
extern bool Cmd_GenCodeSigningCert(int argc, char *argv[]);
extern bool Cmd_GenServiceEndpointCert(int argc, char *argv[]);
extern bool Cmd_GenGeneralCert(int argc, char *argv[]);
extern bool Cmd_ConvertCert(int argc, char *argv[]);
extern bool Cmd_ConvertKey(int argc, char *argv[]);
extern bool Cmd_ConvertProvisioningData(int argc, char *argv[]);
extern bool Cmd_ResignCert(int argc, char *argv[]);
extern bool Cmd_MakeServiceConfig(int argc, char *argv[]);
extern bool Cmd_MakeAccessToken(int argc, char *argv[]);
extern bool Cmd_GenProvisioningData(int argc, char *argv[]);
extern bool Cmd_ValidateCert(int argc, char *argv[]);
extern bool Cmd_PrintCert(int argc, char *argv[]);
extern bool Cmd_PrintSig(int argc, char *argv[]);
extern bool Cmd_PrintTLV(int argc, char *argv[]);

extern bool ReadCert(const char *fileName, X509 *& cert);
extern bool ReadCert(const char *fileName, X509 *& cert, CertFormat& origCertFmt);
extern bool ReadCertPEM(const char *fileName, X509 *& cert);
extern bool ReadWeaveCert(const char *fileName, uint8_t *& certBuf, uint32_t& certLen);
extern bool LoadWeaveCert(const char *fileName, bool isTrused, nl::Weave::Profiles::Security::WeaveCertificateSet& certSet, uint8_t *& certBuf);
extern bool WriteCert(X509 *cert, FILE *file, const char *fileName, CertFormat certFmt);
extern bool MakeDeviceCert(uint64_t devId, X509 *caCert, EVP_PKEY *caKey, const char *curveName, const struct tm& validFrom, uint32_t validDays, const EVP_MD *sigHashAlgo, X509 *& devCert, EVP_PKEY *& devKey);
extern bool MakeCACert(uint64_t newCertId, EVP_PKEY *newCertKey, X509 *caCert, EVP_PKEY *caKey, const struct tm& validFrom, uint32_t validDays, const EVP_MD *sigHashAlgo, X509 *& newCert);
extern bool MakeCodeSigningCert(uint64_t newCertId, EVP_PKEY *newCertKey, X509 *caCert, EVP_PKEY *caKey, const struct tm& validFrom, uint32_t validDays, const EVP_MD *sigHashAlgo, X509 *& newCert);
extern bool MakeServiceEndpointCert(uint64_t newCertId, EVP_PKEY *newCertKey, X509 *caCert, EVP_PKEY *caKey, const struct tm& validFrom, uint32_t validDays, const EVP_MD *sigHashAlgo, X509 *& newCert);
extern bool MakeGeneralCert(const char *subject, EVP_PKEY *newCertKey, X509 *caCert, EVP_PKEY *caKey, const struct tm& validFrom, uint32_t validDays, const EVP_MD *sigHashAlgo, X509 *& newCert);
extern bool ResignCert(X509 *cert, X509 *caCert, EVP_PKEY *caKey, const EVP_MD *sigHashAlgo);
extern bool SetCertSerialNumber(X509 *cert);
extern bool SetCertSubjectName(X509 *cert, int attrNID, const char *subjectName);
extern bool SetValidityTime(X509 *cert, const struct tm& validFrom, uint32_t validDays);
extern bool SetWeaveCertSubjectName(X509 *cert, int attrNID, uint64_t id);
extern bool AddExtension(X509 *devCert, int extNID, const char *extStr);
extern bool AddSubjectKeyId(X509 *cert);
extern bool AddAuthorityKeyId(X509 *cert, X509 *caCert);
extern bool WeaveEncodeCert(X509 *cert, uint8_t *& encodedCert, uint32_t& encodedCertLen);
extern bool DEREncodeCert(X509 *cert, uint8_t *& encodedCert, uint32_t& encodedCertLen);
extern bool X509PEMToDER(uint8_t *cert, uint32_t& certLen);
extern bool X509DERToPEM(uint8_t *cert, uint32_t& certLen, uint32_t bufLen);
extern CertFormat DetectCertFormat(uint8_t *cert, uint32_t certLen);

extern bool ReadPrivateKey(const char *fileName, const char *prompt, EVP_PKEY *& key);
extern bool ReadPublicKey(const char *fileName, EVP_PKEY *& key);
extern bool ReadWeavePrivateKey(const char *fileName, uint8_t *& key, uint32_t &keyLen);
extern bool DecodePrivateKey(const uint8_t *keyData, uint32_t keyDataLen, KeyFormat keyFormat, const char *keySource, const char *prompt, EVP_PKEY *& key);
extern bool DecodePublicKey(const uint8_t *keyData, uint32_t keyDataLen, KeyFormat keyFormat, const char *keySource, EVP_PKEY *& key);
extern bool DetectKeyFormat(FILE *keyFile, KeyFormat& keyFormat);
extern KeyFormat DetectKeyFormat(const uint8_t *key, uint32_t keyLen);
extern bool GenerateKeyPair(const char *curveName, EVP_PKEY *& key);
extern bool EncodePrivateKey(EVP_PKEY *key, KeyFormat keyFormat, uint8_t *& encodedKey, uint32_t& encodedKeyLen);
extern bool WeaveEncodePrivateKey(EVP_PKEY *key, uint8_t *& encodedKey, uint32_t& encodedKeyLen);
extern bool WeaveEncodeECPrivateKey(EC_KEY *key, bool includePubKey, uint8_t *& encodedKey, uint32_t& encodedKeyLen);

extern bool InitOpenSSL();
extern OID NIDToWeaveOID(int nid);
extern char *Base64Encode(const uint8_t *inData, uint32_t inDataLen);
extern uint8_t *Base64Encode(const uint8_t *inData, uint32_t inDataLen, uint8_t *outBuf, uint32_t outBufSize, uint32_t& outDataLen);
extern uint8_t *Base64Decode(const uint8_t *inData, uint32_t inDataLen, uint8_t *outBuf, uint32_t outBufSize, uint32_t& outDataLen);
extern bool IsBase64String(const char * str, uint32_t strLen);
extern bool ContainsPEMMarker(const char *marker, const uint8_t *data, uint32_t dataLen);
extern bool ParseEUI64(const char *str, uint64_t& nodeId);
extern bool ParseDateTime(const char *str, struct tm& date);
extern bool ReadFileIntoMem(const char *fileName, uint8_t *& data, uint32_t &dataLen);

extern int gNIDWeaveDeviceId;
extern int gNIDWeaveServiceEndpointId;
extern int gNIDWeaveCAId;
extern int gNIDWeaveSoftwarePublisherId;


#define ReportOpenSSLErrorAndExit(FUNCT_NAME, ACTION) \
do \
{ \
    fprintf(stderr, "OpenSSL %s() failed\n", FUNCT_NAME); \
    ERR_print_errors_fp(stderr); \
    ACTION; \
    goto exit; \
} while (0)


#endif /* WEAVE_TOOL_H_ */
