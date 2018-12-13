/*
 *
 *    Copyright (c) 2018 Nest Labs, Inc.
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
 *          Provides implementations for the Weave entropy sourcing functions
 *          on the Nordic nRF5* platforms.
 */

#include <Weave/DeviceLayer/internal/WeaveDeviceLayerInternal.h>
#include <Weave/Support/crypto/WeaveRNG.h>

#include "nrf_crypto.h"

using namespace ::nl;
using namespace ::nl::Weave;

#if WEAVE_CONFIG_RNG_IMPLEMENTATION_NESTDRBG

#if NRF_CRYPTO_BACKEND_CC310_RNG_ENABLED
#error "Nest DRBG implementation not required when using Nordic CC310 RNG source"
#endif

#if NRF_CRYPTO_BACKEND_NRF_HW_RNG_ENABLED && NRF_CRYPTO_BACKEND_NRF_HW_RNG_MBEDTLS_CTR_DRBG_ENABLED
#error "Nest DRBG implementation not required when use Nordic HW RNG source with mbed TLS CTR-DRBG"
#endif

#else // WEAVE_CONFIG_RNG_IMPLEMENTATION_NESTDRBG

#if NRF_CRYPTO_BACKEND_NRF_HW_RNG_ENABLED
#error "Nest DRBG implementation must be enabled when using Nordic HW RNG source"
#endif

#endif // !WEAVE_CONFIG_RNG_IMPLEMENTATION_NESTDRBG


namespace nl {
namespace Weave {
namespace DeviceLayer {
namespace Internal {

namespace {

#if WEAVE_CONFIG_RNG_IMPLEMENTATION_NESTDRBG

/**
 * Retrieve entropy from the underlying RNG source.
 *
 * This function is called by the Nest DRBG to acquire entropy.  Is is only
 * used when the Nest DRBG is enabled, which on the nRF5* platforms, is only
 * when the Nordic nRF HW RNG source is used *without* the mbed TLS CTR-DRBG.
 */
int GetEntropy_nRF5(uint8_t * buf, size_t bufSize)
{
    return (nrf_crypto_rng_vector_generate(buf, bufSize) == NRF_SUCCESS) ? 0 : 1;
}

#endif // WEAVE_CONFIG_RNG_IMPLEMENTATION_NESTDRBG

#if NRF_CRYPTO_RNG_STATIC_MEMORY_BUFFERS_ENABLED

nrf_crypto_rng_context_t sRNGContext;

#endif // NRF_CRYPTO_RNG_STATIC_MEMORY_BUFFERS_ENABLED

} // unnamed namespace

WEAVE_ERROR InitEntropy()
{
    WEAVE_ERROR err;

    // Initialize the nrf_crypto RNG source, if not done automatically.
#if !NRF_CRYPTO_RNG_AUTO_INIT_ENABLED
    err = nrf_crypto_rng_init(
#if !NRF_CRYPTO_RNG_STATIC_MEMORY_BUFFERS_ENABLED
            &sRNGContext,
#else
            NULL,
#endif
            NULL);
    SuccessOrExit(err);
#endif // ! NRF_CRYPTO_RNG_AUTO_INIT_ENABLED

    // If enabled, initialize the Nest DRBG.
#if WEAVE_CONFIG_RNG_IMPLEMENTATION_NESTDRBG
    err = ::nl::Weave::Platform::Security::InitSecureRandomDataSource(GetEntropy_nRF5, 64, NULL, 0);
    SuccessOrExit(err);
#endif // WEAVE_CONFIG_RNG_IMPLEMENTATION_NESTDRBG

    // Seed the standard rand() pseudo-random generator with data from the secure random source.
    {
        unsigned int seed;
        err = ::nl::Weave::Platform::Security::GetSecureRandomData((uint8_t *)&seed, sizeof(seed));
        SuccessOrExit(err);
        srand(seed);
    }

exit:
    if (err != WEAVE_NO_ERROR)
    {
        WeaveLogError(Crypto, "InitEntropy() failed: 0x%08" PRIX32, err);
    }
    return err;
}

} // namespace Internal
} // namespace DeviceLayer
} // namespace Weave
} // namespace nl


namespace nl {
namespace Weave {
namespace Platform {
namespace Security {

#if !WEAVE_CONFIG_RNG_IMPLEMENTATION_NESTDRBG

/**
 * Get random data suitable for cryptographic use.
 *
 * This function is only used in cases where the Nest DRBG is *not* enabled.  On the nRF5*
 * platforms, this is when the CC310 RNG source is enabled, or when the nRF HW RNG source
 * is enabled using the mbed TLS CTR-DRBG.
 */
WEAVE_ERROR GetSecureRandomData(uint8_t *buf, uint16_t len)
{
    return nrf_crypto_rng_vector_generate(buf, len);
}

#endif // !WEAVE_CONFIG_RNG_IMPLEMENTATION_NESTDRBG

} // namespace Platform
} // namespace Security
} // namespace Weave
} // namespace nl
