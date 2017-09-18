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

/**
 *    @file
 *      This file implements interface to Weave Crypto Tests library.
 *
 */

#ifndef WEAVE_CRYPTO_TESTS_H_
#define WEAVE_CRYPTO_TESTS_H_

/*
 * Top-level function that runs all crypto tests, unless
 * a specific crypto name is passed as an argument.
 * Use this function to call from a shell command line interface.
 */
int WeaveCryptoTests(int argc, char *argv[]);

/*
 * Test function for SHA cryptography.
 */
int WeaveCryptoSHATests(void);

/*
 * Test function for HMAC cryptography.
 */
int WeaveCryptoHMACTests(void);

/*
 * Test function for HKDF cryptography.
 */
int WeaveCryptoHKDFTests(void);

/*
 * Test function for AES cryptography.
 */
int WeaveCryptoAESTests(void);

#endif /* WEAVE_CRYPTO_TESTS_H_ */
