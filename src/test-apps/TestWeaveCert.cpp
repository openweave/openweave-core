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
 *      Unit tests for Weave certificate functionality.
 *
 */

#include "ToolCommon.h"
#include <Weave/Core/WeaveCore.h>
#include <Weave/Core/WeaveTLV.h>
#include <Weave/Profiles/security/WeaveSecurity.h>
#include <Weave/Profiles/security/WeaveCert.h>

#include "TestWeaveCertData.h"

using namespace nl::Weave::TLV;
using namespace nl::Weave::Profiles::Security;
using namespace nl::Weave::ASN1;

using namespace nl::TestCerts;

#define VerifyOrFail(TST, MSG, ...) \
do { \
    if (!(TST)) \
    { \
        fprintf(stderr, "%s FAILED: ", __FUNCTION__); \
        fprintf(stderr, MSG, ## __VA_ARGS__); \
        fputs("\n", stderr); \
        exit(-1); \
    } \
} while (0)


#define SuccessOrFail(ERR, MSG, ...) \
do { \
    if ((ERR) != WEAVE_NO_ERROR) \
    { \
        fprintf(stderr, "%s FAILED: ", __FUNCTION__); \
        fprintf(stderr, MSG, ## __VA_ARGS__); \
        fputs(": ", stderr); \
        fputs(ErrorStr(ERR), stderr); \
        fputs("\n", stderr); \
        exit(-1); \
    } \
} while (0)


enum {
    kStandardCertsCount = 3
};

static void LoadStandardCerts(WeaveCertificateSet& certSet)
{
    LoadTestCert(certSet, kTestCert_Root | kDecodeFlag_IsTrusted);
    LoadTestCert(certSet, kTestCert_CA   | kDecodeFlag_GenerateTBSHash);
    LoadTestCert(certSet, kTestCert_Dev | kDecodeFlag_GenerateTBSHash);
}

void SetEffectiveTime(ValidationContext& validContext, uint16_t year, uint8_t mon, uint8_t day, uint8_t hour = 0, uint8_t min = 0, uint8_t sec = 0)
{
    WEAVE_ERROR err;
    ASN1UniversalTime effectiveTime;

    effectiveTime.Year = year;
    effectiveTime.Month = mon;
    effectiveTime.Day = day;
    effectiveTime.Hour = hour;
    effectiveTime.Minute = min;
    effectiveTime.Second = sec;
    err = PackCertTime(effectiveTime, validContext.EffectiveTime);
    SuccessOrFail(err, "PackCertTime() returned error");
}

void WeaveCertTest_WeaveToX509()
{
    WEAVE_ERROR err;
    const uint8_t *inCert;
    size_t inCertLen;
    const uint8_t *expectedOutCert;
    size_t expectedOutCertLen;
    uint8_t outCertBuf[kTestCertBufSize];
    uint32_t outCertLen;

    for (size_t i = 0; i < gNumTestCerts; i++)
    {
        int certSelector = gTestCerts[i];

        GetTestCert(certSelector, inCert, inCertLen);
        GetTestCert(certSelector | kTestCertLoadFlag_DERForm, expectedOutCert, expectedOutCertLen);

        err = ConvertWeaveCertToX509Cert(inCert, inCertLen, outCertBuf, sizeof(outCertBuf), outCertLen);
        SuccessOrFail(err, "%s Certificate: ConvertWeaveCertToX509Cert() returned error", GetTestCertName(certSelector));
        VerifyOrFail(outCertLen == expectedOutCertLen, "%s Certificate: ConvertWeaveCertToX509Cert() returned incorrect length", GetTestCertName(certSelector));
        VerifyOrFail(memcmp(outCertBuf, expectedOutCert, outCertLen) == 0, "%s Certificate: ConvertWeaveCertToX509Cert() returned incorrect certificate data", GetTestCertName(certSelector));
    }

    printf("%s passed\n", __FUNCTION__);
}

void WeaveCertTest_X509ToWeave()
{
    WEAVE_ERROR err;
    const uint8_t *inCert;
    size_t inCertLen;
    const uint8_t *expectedOutCert;
    size_t expectedOutCertLen;
    uint8_t outCertBuf[kTestCertBufSize];
    uint32_t outCertLen;

    for (size_t i = 0; i < gNumTestCerts; i++)
    {
        int certSelector = gTestCerts[i];

        GetTestCert(certSelector | kTestCertLoadFlag_DERForm, inCert, inCertLen);
        GetTestCert(certSelector, expectedOutCert, expectedOutCertLen);

        err = ConvertX509CertToWeaveCert(inCert, inCertLen, outCertBuf, sizeof(outCertBuf), outCertLen);
        SuccessOrFail(err, "Certificate Type %d: ConvertX509CertToWeaveCert() returned error", certSelector);
        VerifyOrFail(outCertLen == expectedOutCertLen, "Certificate Type %d: ConvertX509CertToWeaveCert() returned incorrect length", certSelector);
        VerifyOrFail(memcmp(outCertBuf, expectedOutCert, outCertLen) == 0, "Certificate Type %d: ConvertX509CertToWeaveCert() returned incorrect certificate data", certSelector);
    }

    printf("%s passed\n", __FUNCTION__);
}

void WeaveCertTest_CertValidation()
{
    WEAVE_ERROR err;
    WeaveCertificateSet certSet;
    ValidationContext validContext;
    enum { kMaxCertsPerTestCase = 10 };

    struct ValidationTestCase
    {
        int SubjectCertIndex;
        uint16_t ValidateFlags;
        uint8_t RequiredCertType;
        WEAVE_ERROR ExpectedResult;
        int ExpectedCertIndex;
        int ExpectedTrustAnchorIndex;
        int InputCerts[kMaxCertsPerTestCase];
    };

    enum
    {
        // Short-hand names to make the test cases table more concise.
        Root                  = kTestCert_Root,
        RootKey               = kTestCert_RootKey,
        RootSHA256            = kTestCert_Root_SHA256,
        CA                    = kTestCert_CA,
        CASHA256              = kTestCert_CA_SHA256,
        Dev                   = kTestCert_Dev,
        DevSHA256             = kTestCert_Dev_SHA256,
        SelfSigned            = kTestCert_SelfSigned,
        SelfSigned256         = kTestCert_SelfSigned_SHA256,
        ReqSHA256             = kValidateFlag_RequireSHA256,
        IsTrusted             = kDecodeFlag_IsTrusted,
        GenTBSHash            = kDecodeFlag_GenerateTBSHash,
        SupIsCA               = kTestCertLoadFlag_SuppressIsCA,
        SupKeyUsage           = kTestCertLoadFlag_SuppressKeyUsage,
        SupKeyCertSign        = kTestCertLoadFlag_SuppressKeyCertSign,
        SetPathLenZero        = kTestCertLoadFlag_SetPathLenConstZero,
        SetAppDefinedCertType = kTestCertLoadFlag_SetAppDefinedCertType,
        CTNS                  = kCertType_NotSpecified,
        CTGen                 = kCertType_General,
        CTDev                 = kCertType_Device,
        CTSE                  = kCertType_ServiceEndpoint,
        CTAT                  = kCertType_AccessToken,
        CTCA                  = kCertType_CA,
        CTAD                  = kCertType_AppDefinedBase,
    };

    static const ValidationTestCase sValidationTestCases[] =
    {
        //                       Reqd
        // Subject   Valid       Cert                                    Expected    Expected                  Input
        // Index     Flags       Type    Expected Result                 Cert Index  TA Index   Input Certs    Cert Flags
        // ==============================================================================================================

        // Basic validation of leaf certificate with different load orders.
        {  2,        0,          CTNS,   WEAVE_NO_ERROR,                 2,          0,       { Root          | IsTrusted,
                                                                                                CA            | GenTBSHash,
                                                                                                Dev           | GenTBSHash } },
        {  1,        0,          CTNS,   WEAVE_NO_ERROR,                 1,          0,       { Root          | IsTrusted,
                                                                                                Dev           | GenTBSHash,
                                                                                                CA            | GenTBSHash } },
        {  0,        0,          CTNS,   WEAVE_NO_ERROR,                 0,          2,       { Dev           | GenTBSHash,
                                                                                                CA            | GenTBSHash,
                                                                                                Root          | IsTrusted } },

        // Validation of leaf certificate with root key only.
        {  1,        0,          CTNS,   WEAVE_NO_ERROR,                 1,          0,       { RootKey,
                                                                                                Dev           | GenTBSHash,
                                                                                                CA            | GenTBSHash } },

        // Validation of trusted self-signed certificate.
        {  0,        0,          CTNS,   WEAVE_NO_ERROR,                 0,          0,       { SelfSigned    | IsTrusted | GenTBSHash } },

        // Validation of trusted self-signed certificate in presence of trusted root and CA.
        {  2,        0,          CTNS,   WEAVE_NO_ERROR,                 2,          2,       { Root          | IsTrusted,
                                                                                                CA            | GenTBSHash,
                                                                                                SelfSigned    | IsTrusted | GenTBSHash } },

        // Validation of self-signed certificate in presence of trusted copy of same certificate and
        // an unrelated trusted root certificate.
        {  1,        0,          CTNS,   WEAVE_NO_ERROR,                 2,          2,       { Root          | IsTrusted,
                                                                                                SelfSigned    | GenTBSHash,
                                                                                                SelfSigned    | IsTrusted | GenTBSHash } },

        // Validation with two copies of root certificate, one trusted, one untrusted.
        {  2,        0,          CTNS,   WEAVE_NO_ERROR,                 2,          1,       { Root,
                                                                                                Root          | IsTrusted,
                                                                                                Dev           | GenTBSHash,
                                                                                                CA            | GenTBSHash } },

        // Validation with trusted root key and trusted root certificate.
        {  2,        0,          CTNS,   WEAVE_NO_ERROR,                 2,          0,       { RootKey,
                                                                                                Root          | IsTrusted,
                                                                                                Dev           | GenTBSHash,
                                                                                                CA            | GenTBSHash } },

        // Validation with trusted root key and untrusted root certificate.
        {  3,        0,          CTNS,   WEAVE_NO_ERROR,                 3,          1,       { Root,
                                                                                                RootKey,
                                                                                                CA            | GenTBSHash,
                                                                                                Dev           | GenTBSHash } },

        // Failure due to missing CA certificate.
        {  1,        0,          CTNS,   WEAVE_ERROR_CA_CERT_NOT_FOUND,  -1,         -1,      { Root          | IsTrusted,
                                                                                                Dev           | GenTBSHash } },

        // Failure due to missing root certificate.
        {  1,        0,          CTNS,   WEAVE_ERROR_CA_CERT_NOT_FOUND,  -1,         -1,      { CA            | GenTBSHash,
                                                                                                Dev           | GenTBSHash } },

        // Failure due to lack of TBS hash.
        {  1,        0,          CTNS,   WEAVE_ERROR_INVALID_ARGUMENT,   -1,         -1,      { Root          | IsTrusted,
                                                                                                Dev,
                                                                                                CA            | GenTBSHash } },

        // Failure due to untrusted root.
        {  1,        0,          CTNS,   WEAVE_ERROR_CA_CERT_NOT_FOUND,  -1,         -1,      { Root,
                                                                                                Dev           | GenTBSHash,
                                                                                                CA            | GenTBSHash } },

        // Failure of untrusted self-signed certificate.
        {  0,        0,          CTNS,   WEAVE_ERROR_CERT_NOT_TRUSTED,   -1,         -1,      { SelfSigned    | GenTBSHash } },

        // Failure of untrusted self-signed certificate in presence of trusted root and CA.
        {  2,        0,          CTNS,   WEAVE_ERROR_CERT_NOT_TRUSTED,   -1,         -1,      { Root          | IsTrusted,
                                                                                                CA            | GenTBSHash,
                                                                                                SelfSigned    | GenTBSHash } },

        // Failure due to intermediate cert with isCA flag = false
        {  2,        0,          CTNS,   WEAVE_ERROR_CA_CERT_NOT_FOUND,  -1,         -1,      { Root          | IsTrusted,
                                                                                                CA            | GenTBSHash | SupIsCA,
                                                                                                Dev           | GenTBSHash } },

        // Failure due to CA cert with no key usage.
        {  2,        0,          CTNS,   WEAVE_ERROR_CA_CERT_NOT_FOUND,  -1,         -1,      { Root          | IsTrusted,
                                                                                                CA            | GenTBSHash | SupKeyUsage,
                                                                                                Dev           | GenTBSHash } },

        // Failure due to CA cert with no cert sign key usage.
        {  2,        0,          CTNS,   WEAVE_ERROR_CA_CERT_NOT_FOUND,  -1,         -1,      { Root          | IsTrusted,
                                                                                                CA            | GenTBSHash | SupKeyCertSign,
                                                                                                Dev           | GenTBSHash } },

        // Failure due to 3-level deep cert chain and root cert with path constraint == 0
        {  2,        0,          CTNS,   WEAVE_ERROR_CA_CERT_NOT_FOUND,  -1,         -1,      { Root          | IsTrusted | SetPathLenZero,
                                                                                                CA            | GenTBSHash,
                                                                                                Dev           | GenTBSHash } },

        // Basic validation of SHA-256 certificates.
        {  2,        0,          CTNS,   WEAVE_NO_ERROR,                 2,          0,       { RootSHA256    | IsTrusted,
                                                                                                CASHA256      | GenTBSHash,
                                                                                                DevSHA256     | GenTBSHash } },
        {  1,        0,          CTNS,   WEAVE_NO_ERROR,                 1,          0,       { RootSHA256    | IsTrusted,
                                                                                                DevSHA256     | GenTBSHash,
                                                                                                CASHA256      | GenTBSHash } },
        {  0,        0,          CTNS,   WEAVE_NO_ERROR,                 0,          2,       { DevSHA256     | GenTBSHash,
                                                                                                CASHA256      | GenTBSHash,
                                                                                                RootSHA256    | IsTrusted } },

        // Validation of trusted self-signed SHA-256 certificate.
        {  0,        0,          CTNS,   WEAVE_NO_ERROR,                 0,          0,       { SelfSigned256 | IsTrusted | GenTBSHash } },

        // Validation of SHA-256 certificates with root key only.
        {  1,        0,          CTNS,   WEAVE_NO_ERROR,                 1,          0,       { RootKey,
                                                                                                DevSHA256     | GenTBSHash,
                                                                                                CASHA256      | GenTBSHash } },

        // Validation of SHA-256 CA and leaf certificates with SHA-1 root.
        {  2,        0,          CTNS,   WEAVE_NO_ERROR,                 2,          0,       { Root          | IsTrusted,
                                                                                                CASHA256      | GenTBSHash,
                                                                                                DevSHA256     | GenTBSHash } },

        // Validation of SHA-1 leaf certificate with SHA-256 CA and root.
        {  2,        0,          CTNS,   WEAVE_NO_ERROR,                 2,          0,       { RootSHA256    | IsTrusted,
                                                                                                CASHA256      | GenTBSHash,
                                                                                                Dev           | GenTBSHash } },

        // Failure due to lack of SHA-256 CA certificate with SHA-256 leaf certificate.
        {  2,        0,          CTNS,   WEAVE_ERROR_CA_CERT_NOT_FOUND,  -1,         -1,      { RootSHA256    | IsTrusted,
                                                                                                CA            | GenTBSHash,
                                                                                                DevSHA256     | GenTBSHash } },

        // Validation of SHA-256 leaf certificate in presence of SHA-1 and SHA-256 CA certificates.
        {  1,        0,          CTNS,   WEAVE_NO_ERROR,                 1,          0,       { RootSHA256    | IsTrusted,
                                                                                                DevSHA256     | GenTBSHash,
                                                                                                CASHA256      | GenTBSHash,
                                                                                                CA            | GenTBSHash } },

        // Validation of SHA-1 leaf certificate in presence of SHA-1 and SHA-256 CA certificates.
        {  0,        0,          CTNS,   WEAVE_NO_ERROR,                 0,          1,       { Dev           | GenTBSHash,
                                                                                                RootSHA256    | IsTrusted,
                                                                                                CASHA256      | GenTBSHash,
                                                                                                CA            | GenTBSHash } },

        // Validation of self-signed SHA-256 certificate in presence of trusted copy of SHA-1 version of
        // the same certificate and an unrelated trusted root certificate.
        {  1,        0,          CTNS,   WEAVE_NO_ERROR,                 2,          2,       { Root          | IsTrusted,
                                                                                                SelfSigned256 | GenTBSHash,
                                                                                                SelfSigned    | IsTrusted | GenTBSHash } },

        // Failure due to RequireSHA256 flag set and only SHA-1 leaf certificate present.
        {  2,        ReqSHA256,  CTNS,   WEAVE_ERROR_WRONG_CERT_SIGNATURE_ALGORITHM,
                                                                         -1,         -1,      { RootSHA256    | IsTrusted,
                                                                                                CASHA256      | GenTBSHash,
                                                                                                Dev           | GenTBSHash } },

        // Require a specific certificate type.
        {  2,        0,          CTDev,  WEAVE_NO_ERROR,                 2,          0,       { Root          | IsTrusted,
                                                                                                CA            | GenTBSHash,
                                                                                                Dev           | GenTBSHash } },

        // Require a certificate with an application-defined type.
        {  2,        0,          CTAD,   WEAVE_NO_ERROR,                 2,          0,       { Root          | IsTrusted,
                                                                                                CA            | GenTBSHash,
                                                                                                Dev           | GenTBSHash | SetAppDefinedCertType } },

        // Select between two identical certificates with different types.
        {  2,        0,          CTAD,   WEAVE_NO_ERROR,                 3,          0,       { Root          | IsTrusted,
                                                                                                CA            | GenTBSHash,
                                                                                                Dev           | GenTBSHash,
                                                                                                Dev           | GenTBSHash | SetAppDefinedCertType } },

        // Failure due to required certificate type not found.
        {  2,        0,          CTSE,   WEAVE_ERROR_WRONG_CERT_TYPE,    -1,         -1,      { Root          | IsTrusted,
                                                                                                CA            | GenTBSHash,
                                                                                                Dev           | GenTBSHash } },

        // Failure due to CA certificate having wrong type.
        {  2,        0,          CTDev,  WEAVE_ERROR_CA_CERT_NOT_FOUND,  -1,         -1,      { Root          | IsTrusted,
                                                                                                CA            | GenTBSHash | SetAppDefinedCertType,
                                                                                                Dev           | GenTBSHash } },

        // Failure due to root certificate having wrong type.
        {  2,        0,          CTDev,  WEAVE_ERROR_CA_CERT_NOT_FOUND,  -1,         -1,      { Root          | IsTrusted | SetAppDefinedCertType,
                                                                                                CA            | GenTBSHash,
                                                                                                Dev           | GenTBSHash } },
    };
    static const size_t sNumValidationTestCases = sizeof(sValidationTestCases) / sizeof(sValidationTestCases[0]);

    for (unsigned i = 0; i < sNumValidationTestCases; i++)
    {
        WeaveCertificateData *resultCert = NULL;
        const ValidationTestCase& testCase = sValidationTestCases[i];

        // Initialize the certificate set and load the specified test certificates.
        certSet.Init(kMaxCertsPerTestCase, kTestCertBufSize);
        for (size_t i2 = 0; i2 < kMaxCertsPerTestCase; i2++)
            if (testCase.InputCerts[i2] != 0)
                LoadTestCert(certSet, testCase.InputCerts[i2]);

        // Make sure the test case is valid.
        VerifyOrFail(testCase.SubjectCertIndex >= 0 && testCase.SubjectCertIndex < certSet.CertCount,
                     "INVALID TEST CASE: SubjectCertIndex value out of range in test case %u", i);
        if (testCase.ExpectedResult == WEAVE_NO_ERROR)
        {
            VerifyOrFail(testCase.ExpectedCertIndex >= 0 && testCase.ExpectedCertIndex < certSet.CertCount,
                         "INVALID TEST CASE: ExpectedCertIndex value out of range in test case %u", i);
            VerifyOrFail(testCase.ExpectedTrustAnchorIndex >= 0 && testCase.ExpectedTrustAnchorIndex < certSet.CertCount,
                         "INVALID TEST CASE: ExpectedTrustAnchorIndex value out of range in test case %u", i);
        }

        // Initialize the validation context.
        memset(&validContext, 0, sizeof(validContext));
        SetEffectiveTime(validContext, 2016, 5, 3);
        validContext.ValidateFlags = testCase.ValidateFlags;
        validContext.RequiredKeyUsages = kKeyUsageFlag_DigitalSignature;
        validContext.RequiredKeyPurposes = kKeyPurposeFlag_ServerAuth;
        validContext.RequiredCertType = testCase.RequiredCertType;

        // Locate the subject DN and key id that will be used as input the FindValidCert() method.
        const WeaveDN& subjectDN = certSet.Certs[testCase.SubjectCertIndex].SubjectDN;
        const CertificateKeyId& subjectKeyId = certSet.Certs[testCase.SubjectCertIndex].SubjectKeyId;

        // Invoke the FindValidCert() method (the method being tested).
        err = certSet.FindValidCert(subjectDN, subjectKeyId, validContext, resultCert);
        VerifyOrFail(err == testCase.ExpectedResult, "Test Case %u: Unexpected return value from FindValidCert(): err = %d", i, err);

        // If the test case is expected to be successful...
        if (err == WEAVE_NO_ERROR)
        {
            // Verify that the method found the correct certificate.
            VerifyOrFail(resultCert == &certSet.Certs[testCase.ExpectedCertIndex], "Test Case %u: Unexpected certificate returned from FindValidCert()", i);

            // Verify that the method selected the correct trust anchor.
            VerifyOrFail(validContext.TrustAnchor == &certSet.Certs[testCase.ExpectedTrustAnchorIndex], "Test Case %u: Unexpected TrustAnchor returned from FindValidCert()", i);
        }

        // Clear the certificate set.
        certSet.Release();
    }

    printf("%s passed\n", __FUNCTION__);
}

void WeaveCertTest_CertValidTime()
{
    WEAVE_ERROR err;
    WeaveCertificateSet certSet;
    ValidationContext validContext;

    certSet.Init(kStandardCertsCount, kTestCertBufSize);

    LoadStandardCerts(certSet);

    memset(&validContext, 0, sizeof(validContext));
    validContext.RequiredKeyUsages = kKeyUsageFlag_DigitalSignature;
    validContext.RequiredKeyPurposes = kKeyPurposeFlag_ServerAuth;

    // Before certificate validity period.
    SetEffectiveTime(validContext, 2010, 1, 3);
    err = certSet.ValidateCert(certSet.Certs[certSet.CertCount - 1], validContext);
    VerifyOrFail(err == WEAVE_ERROR_CERT_NOT_VALID_YET, "Unexpected result from ValidateCert()");

    // 1 second before validity period.
    SetEffectiveTime(validContext, 2016, 4, 23, 23, 59, 59);
    err = certSet.ValidateCert(certSet.Certs[certSet.CertCount - 1], validContext);
    VerifyOrFail(err == WEAVE_ERROR_CERT_NOT_VALID_YET, "Unexpected result from ValidateCert()");

    // 1st second of 1st day of validity period.
    // NOTE: the given time is technically outside the stated certificate validity period, which starts mid-day.
    // However for simplicity's sake, the Weave cert validation algorithm rounds the validity period to whole days.
    SetEffectiveTime(validContext, 2016, 4, 24, 0, 0, 0);
    err = certSet.ValidateCert(certSet.Certs[certSet.CertCount - 1], validContext);
    VerifyOrFail(err == WEAVE_NO_ERROR, "Unexpected result from ValidateCert()");

    // Last second of last day of validity period.
    // As above, this time is considered valid because of rounding to whole days.
    SetEffectiveTime(validContext, 2016, 5, 24, 23, 59, 59);
    err = certSet.ValidateCert(certSet.Certs[certSet.CertCount - 1], validContext);
    VerifyOrFail(err == WEAVE_NO_ERROR, "Unexpected result from ValidateCert()");

    // 1 second after end of certificate validity period.
    SetEffectiveTime(validContext, 2016, 5, 25, 0, 0, 0);
    err = certSet.ValidateCert(certSet.Certs[certSet.CertCount - 1], validContext);
    VerifyOrFail(err == WEAVE_ERROR_CERT_EXPIRED, "Unexpected result from ValidateCert()");

    // After end of certificate validity period.
    SetEffectiveTime(validContext, 2018, 4, 25, 0, 0, 0);
    err = certSet.ValidateCert(certSet.Certs[certSet.CertCount - 1], validContext);
    VerifyOrFail(err == WEAVE_ERROR_CERT_EXPIRED, "Unexpected result from ValidateCert()");

    // Ignore 'not before' time.
    validContext.ValidateFlags = kValidateFlag_IgnoreNotBefore;
    SetEffectiveTime(validContext, 2016, 4, 23, 23, 59, 59);
    err = certSet.ValidateCert(certSet.Certs[certSet.CertCount - 1], validContext);
    VerifyOrFail(err == WEAVE_NO_ERROR, "Unexpected result from ValidateCert()");

    // Ignore 'not after' time.
    validContext.ValidateFlags = kValidateFlag_IgnoreNotAfter;
    SetEffectiveTime(validContext, 2016, 5, 25, 0, 0, 0);
    err = certSet.ValidateCert(certSet.Certs[certSet.CertCount - 1], validContext);
    VerifyOrFail(err == WEAVE_NO_ERROR, "Unexpected result from ValidateCert()");

    certSet.Release();

    printf("%s passed\n", __FUNCTION__);
}

void WeaveCertTest_CertUsage()
{
    WEAVE_ERROR err;
    WeaveCertificateSet certSet;
    ValidationContext validContext;

    struct UsageTestCase
    {
        uint8_t CertIndex;
        uint16_t RequiredKeyUsages;
        uint16_t RequiredKeyPurposes;
        WEAVE_ERROR ExpectedResult;
    };

    enum
    {
        // Short-hand names to make the test cases table more concise.
        SA = kKeyPurposeFlag_ServerAuth,
        CA = kKeyPurposeFlag_ClientAuth,
        CS = kKeyPurposeFlag_CodeSigning,
        EP = kKeyPurposeFlag_EmailProtection,
        TS = kKeyPurposeFlag_TimeStamping,
        OS = kKeyPurposeFlag_OCSPSigning,
        DS = kKeyUsageFlag_DigitalSignature,
        NR = kKeyUsageFlag_NonRepudiation,
        KE = kKeyUsageFlag_KeyEncipherment,
        DE = kKeyUsageFlag_DataEncipherment,
        KA = kKeyUsageFlag_KeyAgreement,
        KC = kKeyUsageFlag_KeyCertSign,
        CR = kKeyUsageFlag_CRLSign,
        EO = kKeyUsageFlag_EncipherOnly,
        DO = kKeyUsageFlag_DecipherOnly,
    };

    static UsageTestCase sUsageTestCases[] =
    {
        // CertIndex    KeyUsages   KeyPurposes     ExpectedResult
        // =================================================================================

        // ----- Key Usages for leaf Certificate -----
        {  2,           DS,         0,              WEAVE_NO_ERROR                        },
        {  2,           NR,         0,              WEAVE_ERROR_CERT_USAGE_NOT_ALLOWED    },
        {  2,           KE,         0,              WEAVE_NO_ERROR                        },
        {  2,           DE,         0,              WEAVE_ERROR_CERT_USAGE_NOT_ALLOWED    },
        {  2,           KA,         0,              WEAVE_ERROR_CERT_USAGE_NOT_ALLOWED    },
        {  2,           KC,         0,              WEAVE_ERROR_CERT_USAGE_NOT_ALLOWED    },
        {  2,           CR,         0,              WEAVE_ERROR_CERT_USAGE_NOT_ALLOWED    },
        {  2,           EO,         0,              WEAVE_ERROR_CERT_USAGE_NOT_ALLOWED    },
        {  2,           DO,         0,              WEAVE_ERROR_CERT_USAGE_NOT_ALLOWED    },
        {  2,           DS|NR,      0,              WEAVE_ERROR_CERT_USAGE_NOT_ALLOWED    },
        {  2,           DS|KE,      0,              WEAVE_NO_ERROR                        },
        {  2,           DS|DE,      0,              WEAVE_ERROR_CERT_USAGE_NOT_ALLOWED    },
        {  2,           DS|KA,      0,              WEAVE_ERROR_CERT_USAGE_NOT_ALLOWED    },
        {  2,           DS|KC,      0,              WEAVE_ERROR_CERT_USAGE_NOT_ALLOWED    },
        {  2,           DS|CR,      0,              WEAVE_ERROR_CERT_USAGE_NOT_ALLOWED    },
        {  2,           DS|EO,      0,              WEAVE_ERROR_CERT_USAGE_NOT_ALLOWED    },
        {  2,           DS|DO,      0,              WEAVE_ERROR_CERT_USAGE_NOT_ALLOWED    },

        // ----- Key Usages for CA Certificate -----
        {  1,           DS,         0,              WEAVE_ERROR_CERT_USAGE_NOT_ALLOWED    },
        {  1,           NR,         0,              WEAVE_ERROR_CERT_USAGE_NOT_ALLOWED    },
        {  1,           KE,         0,              WEAVE_ERROR_CERT_USAGE_NOT_ALLOWED    },
        {  1,           DE,         0,              WEAVE_ERROR_CERT_USAGE_NOT_ALLOWED    },
        {  1,           KA,         0,              WEAVE_ERROR_CERT_USAGE_NOT_ALLOWED    },
        {  1,           KC,         0,              WEAVE_NO_ERROR                        },
        {  1,           CR,         0,              WEAVE_NO_ERROR                        },
        {  1,           EO,         0,              WEAVE_ERROR_CERT_USAGE_NOT_ALLOWED    },
        {  1,           DO,         0,              WEAVE_ERROR_CERT_USAGE_NOT_ALLOWED    },
        {  1,           KC|DS,      0,              WEAVE_ERROR_CERT_USAGE_NOT_ALLOWED    },
        {  1,           KC|NR,      0,              WEAVE_ERROR_CERT_USAGE_NOT_ALLOWED    },
        {  1,           KC|KE,      0,              WEAVE_ERROR_CERT_USAGE_NOT_ALLOWED    },
        {  1,           KC|DE,      0,              WEAVE_ERROR_CERT_USAGE_NOT_ALLOWED    },
        {  1,           KC|KA,      0,              WEAVE_ERROR_CERT_USAGE_NOT_ALLOWED    },
        {  1,           KC|CR,      0,              WEAVE_NO_ERROR                        },
        {  1,           KC|EO,      0,              WEAVE_ERROR_CERT_USAGE_NOT_ALLOWED    },
        {  1,           KC|DO,      0,              WEAVE_ERROR_CERT_USAGE_NOT_ALLOWED    },

        // ----- Key Purposes for leaf Certificate -----
        {  2,           0,          SA,             WEAVE_NO_ERROR                        },
        {  2,           0,          CA,             WEAVE_NO_ERROR                        },
        {  2,           0,          CS,             WEAVE_ERROR_CERT_USAGE_NOT_ALLOWED    },
        {  2,           0,          EP,             WEAVE_ERROR_CERT_USAGE_NOT_ALLOWED    },
        {  2,           0,          TS,             WEAVE_ERROR_CERT_USAGE_NOT_ALLOWED    },
        {  2,           0,          SA|CA,          WEAVE_NO_ERROR                        },
        {  2,           0,          SA|CS,          WEAVE_ERROR_CERT_USAGE_NOT_ALLOWED    },
        {  2,           0,          SA|EP,          WEAVE_ERROR_CERT_USAGE_NOT_ALLOWED    },
        {  2,           0,          SA|TS,          WEAVE_ERROR_CERT_USAGE_NOT_ALLOWED    },

        // ----- Key Purposes for CA Certificate -----
        {  1,           0,          SA,             WEAVE_ERROR_CERT_USAGE_NOT_ALLOWED    },
        {  1,           0,          CA,             WEAVE_ERROR_CERT_USAGE_NOT_ALLOWED    },
        {  1,           0,          CS,             WEAVE_ERROR_CERT_USAGE_NOT_ALLOWED    },
        {  1,           0,          EP,             WEAVE_ERROR_CERT_USAGE_NOT_ALLOWED    },
        {  1,           0,          TS,             WEAVE_ERROR_CERT_USAGE_NOT_ALLOWED    },
        {  1,           0,          SA|CA,          WEAVE_ERROR_CERT_USAGE_NOT_ALLOWED    },
        {  1,           0,          SA|CS,          WEAVE_ERROR_CERT_USAGE_NOT_ALLOWED    },
        {  1,           0,          SA|EP,          WEAVE_ERROR_CERT_USAGE_NOT_ALLOWED    },
        {  1,           0,          SA|TS,          WEAVE_ERROR_CERT_USAGE_NOT_ALLOWED    },

        // ----- Combinations -----
        {  2,           DS|NR,      SA|CA,          WEAVE_ERROR_CERT_USAGE_NOT_ALLOWED    },
        {  2,           DS|KE,      SA|CS,          WEAVE_ERROR_CERT_USAGE_NOT_ALLOWED    },
        {  2,           DS|KE,      SA|CA,          WEAVE_NO_ERROR                        },
    };
    size_t sNumUsageTestCases = sizeof(sUsageTestCases) / sizeof(sUsageTestCases[0]);

    certSet.Init(kStandardCertsCount, kTestCertBufSize);

    LoadStandardCerts(certSet);

    for (size_t i = 0; i < sNumUsageTestCases; i++)
    {
        memset(&validContext, 0, sizeof(validContext));
        SetEffectiveTime(validContext, 2016, 5, 4);
        validContext.RequiredKeyUsages = sUsageTestCases[i].RequiredKeyUsages;
        validContext.RequiredKeyPurposes = sUsageTestCases[i].RequiredKeyPurposes;
        err = certSet.ValidateCert(certSet.Certs[sUsageTestCases[i].CertIndex], validContext);
        if (err != sUsageTestCases[i].ExpectedResult)
            printf("i = %d, err = %d\n", (int)i, err);
        VerifyOrFail(err == sUsageTestCases[i].ExpectedResult, "Unexpected result from ValidateCert()");
    }

    certSet.Release();

    printf("%s passed\n", __FUNCTION__);
}

void WeaveCertTest_CertType()
{
    WeaveCertificateSet certSet;

    struct TestCase
    {
        int Cert;
        uint8_t ExpectedCertType;
    };

    enum
    {
        // Short-hand names to make the test cases table more concise.
        Root               = kTestCert_Root,
        RootKey            = kTestCert_RootKey,
        Root256            = kTestCert_Root_SHA256,
        CA                 = kTestCert_CA,
        CA256              = kTestCert_CA_SHA256,
        Dev                = kTestCert_Dev,
        Dev256             = kTestCert_Dev_SHA256,
        SelfSigned         = kTestCert_SelfSigned,
        SelfSigned256      = kTestCert_SelfSigned_SHA256,
        ServiceEndpoint    = kTestCert_ServiceEndpoint,
        ServiceEndpoint256 = kTestCert_ServiceEndpoint_SHA256,
        FirmwareSigning    = kTestCert_FirmwareSigning,
        FirmwareSigning256 = kTestCert_FirmwareSigning_SHA256,
    };

    static TestCase sTestCases[] =
    {
        // Cert                ExpectedCertType
        // ================================================
        {  Root,               kCertType_CA               },
        {  RootKey,            kCertType_CA               },
        {  Root256,            kCertType_CA               },
        {  CA,                 kCertType_CA               },
        {  CA256,              kCertType_CA               },
        {  Dev,                kCertType_Device           },
        {  Dev256,             kCertType_Device           },
        {  SelfSigned,         kCertType_General          },
        {  SelfSigned256,      kCertType_General          },
        {  ServiceEndpoint,    kCertType_ServiceEndpoint  },
        {  ServiceEndpoint256, kCertType_ServiceEndpoint  },
        {  FirmwareSigning,    kCertType_FirmwareSigning  },
        {  FirmwareSigning256, kCertType_FirmwareSigning  },
    };
    static const size_t sNumTestCases = sizeof(sTestCases) / sizeof(sTestCases[0]);

    for (unsigned i = 0; i < sNumTestCases; i++)
    {
        const TestCase& testCase = sTestCases[i];

        // Initialize the certificate set and load the test certificate.
        certSet.Init(1, kTestCertBufSize);
        LoadTestCert(certSet, testCase.Cert);

        VerifyOrFail(certSet.Certs[0].CertType == testCase.ExpectedCertType, "Test Case %u: Unexpected certificate type", i);
    }

    printf("%s passed\n", __FUNCTION__);
}

int main(int argc, char *argv[])
{
    WeaveCertTest_WeaveToX509();
    WeaveCertTest_X509ToWeave();
    WeaveCertTest_CertValidation();
    WeaveCertTest_CertValidTime();
    WeaveCertTest_CertUsage();
    WeaveCertTest_CertType();
    printf("All tests passed.\n");
}
