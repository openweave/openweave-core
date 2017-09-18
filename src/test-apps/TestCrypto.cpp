/*
 *
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

#include <stdio.h>
#include <string.h>
#include <WeaveCryptoTests.h>
//#include "WeaveCryptoTests.h"

int main(int argc, char *argv[])
{
    int err = 0;

    if (argc == 2)
    {
        if (!strcmp(argv[1], "sha"))
        {
            WeaveCryptoSHATests();
        }
        else if (!strcmp(argv[1], "hmac"))
        {
            WeaveCryptoHMACTests();
        }
        else if (!strcmp(argv[1], "hkdf"))
        {
            WeaveCryptoHKDFTests();
        }
        else if (!strcmp(argv[1], "aes"))
        {
            WeaveCryptoAESTests();
        }
        else
        {
            printf("%s: unknown parameter %s.\n", argv[0], argv[1]);
            err = 1;
        }
    }
    else
    {
        /* By default, run all crypto tests */
        WeaveCryptoSHATests();
        WeaveCryptoHMACTests();
        WeaveCryptoHKDFTests();
        WeaveCryptoAESTests();
    }

    return err;
}
