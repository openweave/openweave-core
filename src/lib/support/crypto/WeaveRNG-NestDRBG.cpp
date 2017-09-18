/*
 *
 *    Copyright (c) 2015-2017 Nest Labs, Inc.
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
 *      This file implements secure random data initialization and generation functions
 *      for the Weave layer. The implementation is based on the Nest implementation of the
 *      Deterministic Random Bit Generator (DRBG) algorithms as specified in NIST SP800-90A.
 *      This implementation is used when #WEAVE_CONFIG_RNG_IMPLEMENTATION_NESTDRBG
 *      is enabled (1).
 *
 */

#include "WeaveCrypto.h"
#include "DRBG.h"
#include "AESBlockCipher.h"
#include <Weave/Support/CodeUtils.h>

#if WEAVE_CONFIG_DEV_RANDOM_DRBG_SEED
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#endif

namespace nl {
namespace Weave {
namespace Platform {
namespace Security {

using namespace nl::Weave::Crypto;

#if WEAVE_CONFIG_DEV_RANDOM_DRBG_SEED

/**
 * Get DRBG seed data from the system /dev/(u)random device.
 *
 * @param[in]  buf      Pointer to a memory buffer in which the requested random seed data
 *                      should be stored.
 *
 * @param[in]  bufSize  The size of the buffer pointed at by buf.
 *
 * @retval  0           Requested data was successfully read.
 * @retval  1           An error occurred.
 *
 */
int GetDRBGSeedDevRandom(uint8_t *buf, size_t bufSize)
{
    int res = 0;
    int devFD = -1;
    int readRes = 0;

    devFD = ::open(WEAVE_CONFIG_DEV_RANDOM_DEVICE_NAME, O_RDONLY);
    if (devFD < 0)
    {
        WeaveLogError(Crypto, "Failed to open %s: %s", WEAVE_CONFIG_DEV_RANDOM_DEVICE_NAME, strerror(errno));
        ExitNow(res = 1);
    }

    readRes = ::read(devFD, buf, bufSize);
    if (readRes < 0)
    {
        WeaveLogError(Crypto, "Error reading %s: %s", WEAVE_CONFIG_DEV_RANDOM_DEVICE_NAME, strerror(errno));
        ExitNow(res = 1);
    }
    if ((size_t)readRes != bufSize)
    {
        WeaveLogError(Crypto, "Unable to read %lu bytes from %s", (uint32_t)bufSize, WEAVE_CONFIG_DEV_RANDOM_DEVICE_NAME);
        ExitNow(res = 1);
    }

    WeaveLogProgress(Crypto, "Seeding DRBG with %lu bytes from %s", (uint32_t)bufSize, WEAVE_CONFIG_DEV_RANDOM_DEVICE_NAME);

exit:
    if (devFD >= 0)
        close(devFD);
    return res;
}

#endif // WEAVE_CONFIG_DEV_RANDOM_DRBG_SEED

#if WEAVE_CONFIG_RNG_IMPLEMENTATION_NESTDRBG

AES128CTRDRBG CtrDRBG;

WEAVE_ERROR InitSecureRandomDataSource(EntropyFunct entropyFunct, uint16_t entropyLen, const uint8_t *personalizationData, uint16_t perDataLen)
{
#if WEAVE_CONFIG_DEV_RANDOM_DRBG_SEED
    if (entropyFunct == NULL)
        entropyFunct = GetDRBGSeedDevRandom;
#endif

    return CtrDRBG.Instantiate(entropyFunct, entropyLen, personalizationData, perDataLen);
}

WEAVE_ERROR GetSecureRandomData(uint8_t *buf, uint16_t len)
{
    return CtrDRBG.Generate(buf, len);
}

#endif // WEAVE_CONFIG_RNG_IMPLEMENTATION_NESTDRBG

} // namespace Platform
} // namespace Security
} // namespace Weave
} // namespace nl
