/*
 *
 *    Copyright (c) 2019 Google LLC.
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
 *      This file implements a template object for doing keyed-hash
 *      message authentication code (HMAC) and specialized objects
 *      for HMAC with SHA-1 and SHA-256.
 *
 */

#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif
#include <stdint.h>
#include <string.h>

#include "WeaveCrypto.h"
#include "HMAC.h"
#include <Weave/Support/CodeUtils.h>

namespace nl {
namespace Weave {
namespace Crypto {

using namespace nl::Weave::TLV;

template <class H>
HMAC<H>::HMAC()
{
    Reset();
}

template <class H>
HMAC<H>::~HMAC()
{
    Reset();
}

template <class H>
void HMAC<H>::Begin(const uint8_t *key, uint16_t keyLen)
{
    uint8_t pad[kBlockLength];

    Reset();

    // Copy the key. If the key is larger than a block, hash it an use the result as the key.
    if (keyLen > kBlockLength)
    {
        mHash.Begin();
        mHash.AddData(key, keyLen);
        mHash.Finish(mKey);
        mKeyLen = keyLen = kDigestLength;
    }
    else
    {
        memcpy(mKey, key, keyLen);
        mKeyLen = keyLen;
    }

    // Form the pad for the inner hash.
    memcpy(pad, mKey, mKeyLen);
    if (keyLen < kBlockLength)
        memset(pad + mKeyLen, 0, kBlockLength - mKeyLen);
    for (size_t i = 0; i < kBlockLength; i++)
        pad[i] = pad[i] ^ 0x36;

    // Begin generating the inner hash starting with the pad.
    mHash.Begin();
    mHash.AddData(pad, kBlockLength);

    ClearSecretData(pad, sizeof(kBlockLength));
}

template <class H>
void HMAC<H>::AddData(const uint8_t *msgData, uint16_t dataLen)
{
    // Add a chunk of data to the inner hash.
    mHash.AddData(msgData, dataLen);
}

#if WEAVE_WITH_OPENSSL
template <class H>
void HMAC<H>::AddData(const BIGNUM& num)
{
    // Add a chunk of data to the inner hash.
    mHash.AddData(num);
}
#endif

template <class H>
void HMAC<H>::Finish(uint8_t *hashBuf)
{
    uint8_t pad[kBlockLength];
    uint8_t innerHash[kDigestLength];

    // Finalize the inner hash.
    mHash.Finish(innerHash);

    // Form the pad for the outer hash.
    memcpy(pad, mKey, mKeyLen);
    if (mKeyLen < kBlockLength)
        memset(pad + mKeyLen, 0, kBlockLength - mKeyLen);
    for (size_t i = 0; i < kBlockLength; i++)
        pad[i] = pad[i] ^ 0x5c;

    // Generate the outer hash from the pad and the inner hash.
    mHash.Begin();
    mHash.AddData(pad, kBlockLength);
    mHash.AddData(innerHash, kDigestLength);
    mHash.Finish(hashBuf);

    // Clear state.
    Reset();
    ClearSecretData(pad, sizeof(kBlockLength));
    ClearSecretData(innerHash, sizeof(kDigestLength));
}

template <class H>
void HMAC<H>::Reset()
{
    mHash.Reset();
    ClearSecretData(mKey, sizeof(mKey));
    mKeyLen = 0;
}

template class HMAC<Platform::Security::SHA1>;
template class HMAC<Platform::Security::SHA256>;


/**
 * Compares with another HMAC signature.
 *
 * @param[in] other             The EncodedHMACSignature object with which signature should be compared.
 *
 * @retval true                 The signatures are equal.
 * @retval false                The signatures are not equal.
 */
bool EncodedHMACSignature::IsEqual(const EncodedHMACSignature& other) const
{
    return (Sig != NULL &&
            other.Sig != NULL &&
            Len == other.Len &&
            memcmp(Sig, other.Sig, Len) == 0);
}

/**
 * Reads the signature as a Weave HMACSignature structure from the specified TLV reader.
 *
 * @param[in] reader            The TLVReader object from which the encoded signature should
 *                              be read.
 *
 * @retval #WEAVE_NO_ERROR      If the operation succeeded.
 * @retval other                Other Weave error codes related to signature reading.
 */
WEAVE_ERROR EncodedHMACSignature::ReadSignature(TLVReader& reader)
{
    WEAVE_ERROR err;

    VerifyOrExit(reader.GetType() == kTLVType_ByteString, err = WEAVE_ERROR_WRONG_TLV_TYPE);

    err = reader.GetDataPtr(const_cast<const uint8_t *&>(Sig));
    SuccessOrExit(err);

    Len = reader.GetLength();

exit:
    return err;
}

/**
 * Generate and encode a Weave HMAC signature
 *
 * Computes an HMAC signature for a given data using secret key and writes the signature
 * as a Weave HMACSignature structure to the specified TLV writer with the given tag.
 *
 * @param[in] sigAlgoOID        Algorithm OID to be used to generate HMAC signature.
 * @param[in] writer            The TLVWriter object to which the encoded signature should
 *                              be written.
 * @param[in] tag               TLV tag to be associated with the encoded signature structure.
 * @param[in] data              A buffer containing the data to be signed.
 * @param[in] dataLen           The length in bytes of the data.
 * @param[in] key               A buffer containing the secret key to be used to generate HMAC
 *                              signature.
 * @param[in] keyLen            The length in bytes of the secret key.
 *
 * @retval #WEAVE_NO_ERROR      If the operation succeeded.
 * @retval other                Other Weave error codes related to the signature encoding.
 */
WEAVE_ERROR GenerateAndEncodeWeaveHMACSignature(OID sigAlgoOID,
                                                TLVWriter& writer, uint64_t tag,
                                                const uint8_t * data, uint16_t dataLen,
                                                const uint8_t * key, uint16_t keyLen)
{
    WEAVE_ERROR err;
    HMACSHA256 hmac;
    uint8_t hmacSig[HMACSHA256::kDigestLength];
    EncodedHMACSignature sig;

    // Current implementation only supports HMACWithSHA256 signature algorithm.
    VerifyOrExit(sigAlgoOID == ASN1::kOID_SigAlgo_HMACWithSHA256, err = WEAVE_ERROR_UNSUPPORTED_SIGNATURE_TYPE);

    // Generate the MAC.
    hmac.Begin(key, keyLen);
    hmac.AddData(data, dataLen);
    hmac.Finish(hmacSig);

    sig.Sig = hmacSig;
    sig.Len = HMACSHA256::kDigestLength;

    // Encode an HMACSignature value into the supplied writer.
    err = sig.WriteSignature(writer, tag);
    SuccessOrExit(err);

exit:
    return err;
}

/**
 * Verify a Weave HMAC signature.
 *
 * Verifies an HMAC signature using given data and a secret key to be used to verify the signature.
 *
 * @param[in] sigAlgoOID        Algorithm OID to be used to generate HMAC signature.
 * @param[in] data              A buffer containing the data to be signed.
 * @param[in] dataLen           The length in bytes of the data.
 * @param[in] sig               Encoded HMAC signature to be verified.
 * @param[in] key               A buffer containing the secret key to be used to generate HMAC
 *                              signature.
 * @param[in] keyLen            The length in bytes of the secret key.
 *
 * @retval #WEAVE_NO_ERROR      If HMAC signature verification succeeded.
 * @retval #WEAVE_ERROR_INVALID_SIGNATURE
 *                              If HMAC signature verification failed.
 */
WEAVE_ERROR VerifyHMACSignature(OID sigAlgoOID,
                                const uint8_t * data, uint16_t dataLen,
                                const EncodedHMACSignature& sig,
                                const uint8_t * key, uint16_t keyLen)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    HMACSHA256 hmac;
    uint8_t hmacSig[HMACSHA256::kDigestLength];
    EncodedHMACSignature lSig;

    // Current implementation only supports HMACWithSHA256 signature algorithm.
    VerifyOrExit(sigAlgoOID == ASN1::kOID_SigAlgo_HMACWithSHA256, err = WEAVE_ERROR_UNSUPPORTED_SIGNATURE_TYPE);

    // Generate the MAC.
    hmac.Begin(key, keyLen);
    hmac.AddData(data, dataLen);
    hmac.Finish(hmacSig);

    lSig.Sig = hmacSig;
    lSig.Len = HMACSHA256::kDigestLength;

    VerifyOrExit(lSig.IsEqual(sig), err = WEAVE_ERROR_INVALID_SIGNATURE);

exit:
    return err;
}

// ==================== Documentation for Inline Public Members ====================

/**
 * @fn WEAVE_ERROR EncodedHMACSignature::WriteSignature(TLVWriter& writer, uint64_t tag) const
 *
 * Writes the signature as a Weave HMACSignature structure to the specified TLV writer
 * with the given tag.
 *
 * @param[in] writer            The TLVWriter object to which the encoded signature should
 *                              be written.
 * @param[in] tag               TLV tag to be associated with the encoded signature structure.

 * @retval #WEAVE_NO_ERROR      If the operation succeeded.
 * @retval other                Other Weave error codes related to signature writing.
 */

} // namespace Crypto
} // namespace Weave
} // namespace nl
