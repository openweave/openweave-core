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
 *      This file defines a template object for a Counter Mode
 *      Deterministic Random Bit Generator (CTR-DRBG) and a
 *      specialized object for CTR-DRBG with AES-128 CTR Mode.
 *
 */

#ifndef CTR_DRBG_H_
#define CTR_DRBG_H_

#include <stddef.h>

#include "WeaveCrypto.h"

// Periodic reseeding reduces risks of a compromise of the data
// that is protected by cryptographic mechanisms that use the DRBG.
// DRBG reseeds automatically after WEAVE_CONFIG_DRBG_RESEED_INTERVAL
// Generate requests.
// When WEAVE_CONFIG_DRBG_RESEED_INTERVAL = 0, DRBG reseeds every Generate
// request, which is equivalent to Prediction Resistance mode of DRBG.
#ifndef WEAVE_CONFIG_DRBG_RESEED_INTERVAL
    #define WEAVE_CONFIG_DRBG_RESEED_INTERVAL        128
#endif

#ifndef WEAVE_CONFIG_DRBG_MAX_ENTROPY_LENGTH
    #define WEAVE_CONFIG_DRBG_MAX_ENTROPY_LENGTH     64
#endif

namespace nl {
namespace Weave {
namespace Platform {
namespace Security {
class AES128BlockCipherEnc;
}
}
}
}

namespace nl {
namespace Weave {
namespace Crypto {

template <class BlockCipher>
class NL_DLL_EXPORT CTR_DRBG
{
public:
    enum
    {
        kKeyLength             = BlockCipher::kKeyLength,
        kBlockLength           = BlockCipher::kBlockLength,
        kSeedLength            = kKeyLength + kBlockLength,
        kRoundedSeedLength     = (kSeedLength + kBlockLength - 1) / kBlockLength * kBlockLength,
        kSecurityStrength      = kKeyLength,
    };

    CTR_DRBG(void);
    ~CTR_DRBG(void);

    WEAVE_ERROR Instantiate(EntropyFunct entropyFunct, uint16_t entropyLen,
                            const uint8_t *personalizationData, uint16_t perDataLen);
    WEAVE_ERROR Reseed(const uint8_t *addData = NULL, uint16_t addDataLen = 0);
    WEAVE_ERROR Generate(uint8_t *outData, uint16_t outDataLen,
                         const uint8_t *addData = NULL, uint16_t addDataLen = 0);
    void Uninstantiate(void);

    WEAVE_ERROR SelfTest(int verbose);

private:
    EntropyFunct mEntropyFunct;
    BlockCipher mBlockCipher;
    uint32_t mReseedCounter;
    uint16_t mEntropyLen;
    uint8_t mCounter[kBlockLength];

    void Update(const uint8_t *data);
    void IncrementCounter(void);
    void DerivationFunction(uint8_t *seed, const uint8_t *data2, uint16_t data2Len,
                            const uint8_t *data1 = NULL, uint16_t data1Len = 0);
    WEAVE_ERROR GenerateInternal(uint8_t *outData, uint16_t outDataLen,
                                 const uint8_t *addData, uint16_t addDataLen);
};

typedef CTR_DRBG<Platform::Security::AES128BlockCipherEnc> AES128CTRDRBG;

} /* namespace Crypto */
} /* namespace Weave */
} /* namespace nl */

#endif /* CTR_DRBG_H_ */
