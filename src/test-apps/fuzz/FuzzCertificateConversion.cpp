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
 *      This file implements a process to fuzz the certificate
 *      parser for weave.
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>
#include <unistd.h>

#include "FuzzUtils.h"
#include "../tools/weave/weave-tool.h"

using namespace nl::Weave::Profiles::Security;

bool gOpenSSLSet = false;

//Assume the user compiled with clang > 6.0
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    static uint8_t inCert[MAX_CERT_SIZE];
    static uint8_t outCert[MAX_CERT_SIZE];
    CertFormat outputFormats[4] =
    {
        kCertFormat_X509_PEM,
        kCertFormat_X509_DER,
        kCertFormat_Weave_Base64,
        kCertFormat_Weave_Raw
    };

    bool res = true;
    WEAVE_ERROR err;
    uint32_t inCertLen = 0;
    CertFormat inCertFormat;
    CertFormat outCertFormat;
    uint32_t outCertLen = 0;

    if (size > sizeof(inCert))
    {
        fprintf(stderr, "weave: Input certificate too big\n");
        ExitNow();
    }
    memcpy(inCert, data, size);
    inCertLen = size;

    if (!gOpenSSLSet)
    {
        InitOpenSSL();
        gOpenSSLSet = true;
    }

    inCertFormat = DetectCertFormat(inCert, inCertLen);
    for (int i = 0; i < 4; i++)
    {
        outCertFormat = outputFormats[i];
        if (inCertFormat == outCertFormat)
        {
            memcpy(outCert, inCert, inCertLen);
            outCertLen = inCertLen;
        }

        else
        {
            if (inCertFormat == kCertFormat_X509_PEM)
            {
                if (!X509PEMToDER(inCert, inCertLen))
                    ExitNow(res = false);
                inCertFormat = kCertFormat_X509_DER;
            }
            else if (inCertFormat == kCertFormat_Weave_Base64)
            {
                if (!Base64Decode(inCert, inCertLen, inCert, sizeof(inCert), inCertLen))
                    ExitNow(res = false);
                inCertFormat = kCertFormat_Weave_Raw;
            }

            if (inCertFormat == kCertFormat_X509_DER && (outCertFormat == kCertFormat_Weave_Raw || outCertFormat == kCertFormat_Weave_Base64))
            {
                err = ConvertX509CertToWeaveCert(inCert, inCertLen, outCert, sizeof(outCert), outCertLen);
                if (err != WEAVE_NO_ERROR)
                {
                    //fprintf(stderr, "weave: x509->Weave Error converting certificate: %s\n", nl::ErrorStr(err));
                    ExitNow(res = false);
                }
            }
            else if (inCertFormat == kCertFormat_Weave_Raw && (outCertFormat == kCertFormat_X509_DER || outCertFormat == kCertFormat_X509_PEM))
            {
                err = ConvertWeaveCertToX509Cert(inCert, inCertLen, outCert, sizeof(outCert), outCertLen);
                if (err != WEAVE_NO_ERROR)
                {
                    //fprintf(stderr, "weave: Weave -> X509 Error converting certificate: %s\n", nl::ErrorStr(err));
                    ExitNow(res = false);
                }
            }
            else
            {
                memcpy(outCert, inCert, inCertLen);
                outCertLen = inCertLen;
            }

            if (outCertFormat == kCertFormat_X509_PEM)
            {
                if (!X509DERToPEM(outCert, outCertLen, sizeof(outCert)))
                    ExitNow(res = false);
            }
            else if (outCertFormat == kCertFormat_Weave_Base64)
            {
                if (!Base64Encode(outCert, outCertLen, outCert, sizeof(outCert), outCertLen))
                    ExitNow(res = false);
            }
        }
    }

exit:
    return res;
}

// When NOT building for fuzzing using libFuzzer, supply a main() function to satisfy
// the linker.  Even though the resultant application does nothing, being able to link
// it confirms that the fuzzing tests can be built successfully.
#ifndef WEAVE_FUZZING_ENABLED
int main(int argc, char *argv[])
{
    return 0;
}
#endif
