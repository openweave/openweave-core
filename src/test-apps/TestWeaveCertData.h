/*
 *
 *    Copyright (c) 2019 Google LLC.
 *    Copyright (c) 2016-2017 Nest Labs, Inc.
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
 *      This file contains definitions of test certificates used by various unit tests.
 *
 */

#ifndef TESTWEAVECERTDATA_H_
#define TESTWEAVECERTDATA_H_

namespace nl {
namespace TestCerts {

using namespace nl::Weave::Profiles::Security;

enum
{
    kTestCert_Mask                                  = 0x000FF000,
    kTestCertLoadFlag_Mask                          = 0x0FF00000,
    kDecodeFlag_Mask                          		= 0x0000000F, // values defined in WeaveCert.h


	// Available test cert types.
    kTestCert_None                                  = 0x00000000,
    kTestCert_Root                                  = 0x00001000,
    kTestCert_RootKey                               = 0x00002000,
    kTestCert_Root_SHA256                           = 0x00003000,
    kTestCert_CA                                    = 0x00004000,
    kTestCert_CA_SHA256                             = 0x00005000,
    kTestCert_Dev                                   = 0x00006000,
    kTestCert_Dev_SHA256                            = 0x00007000,
    kTestCert_SelfSigned                            = 0x00008000,
    kTestCert_SelfSigned_SHA256                     = 0x00009000,
    kTestCert_ServiceEndpoint                       = 0x0000A000,
    kTestCert_ServiceEndpoint_SHA256                = 0x0000B000,
    kTestCert_FirmwareSigning                       = 0x0000C000,
    kTestCert_FirmwareSigning_SHA256                = 0x0000D000,

    // Special flags to alter how certificates are fetched/loaded.
    kTestCertLoadFlag_DERForm	                    = 0x00100000,
    kTestCertLoadFlag_SuppressIsCA                  = 0x00200000,
    kTestCertLoadFlag_SuppressKeyUsage              = 0x00400000,
    kTestCertLoadFlag_SuppressKeyCertSign           = 0x00800000,
    kTestCertLoadFlag_SetPathLenConstZero           = 0x01000000,
    kTestCertLoadFlag_SetAppDefinedCertType         = 0x02000000,

    kTestCertBufSize                                = 1024,      // Size of buffer needed to hold any of the test certificates
                                                                 // (in either Weave or DER form), or to decode the certificates.
};

extern void GetTestCert(int selector, const uint8_t *& certData, size_t& certDataLen);
extern const char *GetTestCertName(int selector);
extern void LoadTestCert(WeaveCertificateSet& certSet, int selector);

extern const int gTestCerts[];
extern const size_t gNumTestCerts;

extern const uint8_t sTestCert_Root_Weave[];
extern const size_t sTestCertLength_Root_Weave;
extern const uint8_t sTestCert_Root_DER[];
extern const size_t sTestCertLength_Root_DER;

extern const uint8_t sTestCert_Root_PublicKey[];
extern const size_t sTestCertLength_Root_PublicKey;
extern const uint8_t sTestCert_Root_SubjectKeyId[];
extern const size_t sTestCertLength_Root_SubjectKeyId;
extern const uint32_t sTestCert_Root_CurveId;
extern const uint64_t sTestCert_Root_Id;

extern const uint8_t sTestCert_Root_SHA256_Weave[];
extern const size_t sTestCertLength_Root_SHA256_Weave;
extern const uint8_t sTestCert_Root_SHA256_DER[];
extern const size_t sTestCertLength_Root_SHA256_DER;

extern const uint8_t sTestCert_CA_Weave[];
extern const size_t sTestCertLength_CA_Weave;
extern const uint8_t sTestCert_CA_DER[];
extern const size_t sTestCertLength_CA_DER;

extern const uint8_t sTestCert_CA_PublicKey[];
extern const size_t sTestCertLength_CA_PublicKey;
extern const uint8_t sTestCert_CA_PrivateKey[];
extern const size_t sTestCertLength_CA_PrivateKey;
extern const uint8_t sTestCert_CA_SubjectKeyId[];
extern const size_t sTestCertLength_CA_SubjectKeyId;
extern const uint32_t sTestCert_CA_CurveId;
extern const uint64_t sTestCert_CA_Id;

extern const uint8_t sTestCert_CA_SHA256_Weave[];
extern const size_t sTestCertLength_CA_SHA256_Weave;
extern const uint8_t sTestCert_CA_SHA256_DER[];
extern const size_t sTestCertLength_CA_SHA256_DER;

extern const uint8_t sTestCert_Dev_Weave[];
extern const size_t sTestCertLength_Dev_Weave;
extern const uint8_t sTestCert_Dev_DER[];
extern const size_t sTestCertLength_Dev_DER;

extern const uint8_t sTestCert_Dev_SHA256_Weave[];
extern const size_t sTestCertLength_Dev_SHA256_Weave;
extern const uint8_t sTestCert_Dev_SHA256_DER[];
extern const size_t sTestCertLength_Dev_SHA256_DER;

extern const uint8_t sTestCert_SelfSigned_Weave[];
extern const size_t sTestCertLength_SelfSigned_Weave;
extern const uint8_t sTestCert_SelfSigned_DER[];
extern const size_t sTestCertLength_SelfSigned_DER;

extern const uint8_t sTestCert_SelfSigned_SHA256_Weave[];
extern const size_t sTestCertLength_SelfSigned_SHA256_Weave;
extern const uint8_t sTestCert_SelfSigned_SHA256_DER[];
extern const size_t sTestCertLength_SelfSigned_SHA256_DER;

extern const uint8_t sTestCert_ServiceEndpoint_Weave[];
extern const size_t sTestCertLength_ServiceEndpoint_Weave;
extern const uint8_t sTestCert_ServiceEndpoint_DER[];
extern const size_t sTestCertLength_ServiceEndpoint_DER;

extern const uint8_t sTestCert_ServiceEndpoint_SHA256_Weave[];
extern const size_t sTestCertLength_ServiceEndpoint_SHA256_Weave;
extern const uint8_t sTestCert_ServiceEndpoint_SHA256_DER[];
extern const size_t sTestCertLength_ServiceEndpoint_SHA256_DER;

extern const uint8_t sTestCert_FirmwareSigning_Weave[];
extern const size_t sTestCertLength_FirmwareSigning_Weave;
extern const uint8_t sTestCert_FirmwareSigning_DER[];
extern const size_t sTestCertLength_FirmwareSigning_DER;

extern const uint8_t sTestCert_FirmwareSigning_SHA256_Weave[];
extern const size_t sTestCertLength_FirmwareSigning_SHA256_Weave;
extern const uint8_t sTestCert_FirmwareSigning_SHA256_DER[];
extern const size_t sTestCertLength_FirmwareSigning_SHA256_DER;


} // namespace TestCerts
} // namespace nl


#endif /* TESTWEAVECERTDATA_H_ */
