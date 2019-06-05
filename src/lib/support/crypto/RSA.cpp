/*
 *
 *    Copyright (c) 2019 Google LLC.
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
 *      General RSA utility functions.
 *
 */

#include "RSA.h"
#include <Weave/Core/WeaveEncoding.h>
#include <Weave/Support/CodeUtils.h>

#if WEAVE_WITH_OPENSSL
#include <openssl/bio.h>
#include <openssl/x509.h>
#include <openssl/rsa.h>
#endif

namespace nl {
namespace Weave {
namespace Crypto {

using namespace nl::Weave::TLV;

/**
 * Compares with another RSA key.
 *
 * @param[in] other             The EncodedRSAKey object with which key should be compared.
 *
 * @retval true                 The keys are equal.
 * @retval false                The keys are not equal.
 */
bool EncodedRSAKey::IsEqual(const EncodedRSAKey& other) const
{
    return (Key != NULL &&
            other.Key != NULL &&
            Len == other.Len &&
            memcmp(Key, other.Key, Len) == 0);
}

/**
 * Compares with another RSA signature.
 *
 * @param[in] other             The EncodedRSASignature object with which signature should be compared.
 *
 * @retval true                 The signatures are equal.
 * @retval false                The signatures are not equal.
 */
bool EncodedRSASignature::IsEqual(const EncodedRSASignature& other) const
{
    return (Sig != NULL &&
            other.Sig != NULL &&
            Len == other.Len &&
            memcmp(Sig, other.Sig, Len) == 0);
}

/**
 * Reads the signature as a Weave RSASignature structure from the specified TLV reader.
 *
 * @param[in] reader            The TLVReader object from which the encoded signature should
 *                              be read.
 *
 * @retval #WEAVE_NO_ERROR      If the operation succeeded.
 * @retval other                Other Weave error codes related to signature reading.
 */
WEAVE_ERROR EncodedRSASignature::ReadSignature(TLVReader& reader)
{
    WEAVE_ERROR err;

    VerifyOrExit(reader.GetType() == kTLVType_ByteString, err = WEAVE_ERROR_WRONG_TLV_TYPE);

    err = reader.GetDataPtr(const_cast<const uint8_t *&>(Sig));
    SuccessOrExit(err);

    Len = reader.GetLength();

exit:
    return err;
}

#if WEAVE_WITH_OPENSSL

static int ShaNIDFromSigAlgoOID(OID sigAlgoOID)
{
    int nid;

    // Current implementation only supports SHA256WithRSAEncryption signature algorithm.
    if (sigAlgoOID == ASN1::kOID_SigAlgo_SHA256WithRSAEncryption)
        nid = NID_sha256;
    else
        nid = NID_undef;

    return nid;
}

/**
 * Generate and encode a Weave RSA signature
 *
 * Computes an RSA signature using a given X509 encoded RSA private key and message hash
 * and writes the signature as a Weave RSASignature structure to the specified TLV writer
 * with the given tag.
 *
 * @param[in] sigAlgoOID        Algorithm OID to be used to generate RSA signature.
 * @param[in] writer            The TLVWriter object to which the encoded signature should
 *                              be written.
 * @param[in] tag               TLV tag to be associated with the encoded signature structure.
 * @param[in] hash              A buffer containing the hash of the data to be signed.
 * @param[in] hashLen           The length in bytes of the data hash.
 * @param[in] keyDER            A buffer containing the private key to be used to generate
 *                              the signature.  The private key is expected to be encoded as
 *                              an X509 RSA private key structure.
 * @param[in] keyDERLen         The length in bytes of the encoded private key.
 *
 * @retval #WEAVE_NO_ERROR      If the operation succeeded.
 * @retval other                Other Weave error codes related to decoding the private key,
 *                              generating the signature or encoding the signature.
 *
 */
WEAVE_ERROR GenerateAndEncodeWeaveRSASignature(OID sigAlgoOID,
                                               TLVWriter& writer, uint64_t tag,
                                               const uint8_t * hash, uint8_t hashLen,
                                               const uint8_t * keyDER, uint16_t keyDERLen)
{
    WEAVE_ERROR err;
    RSA *rsa = NULL;
    uint8_t sigBuf[EncodedRSASignature::kMaxValueLength];
    uint32_t sigLen;
    EncodedRSASignature sig;
    int shaNID;

    shaNID = ShaNIDFromSigAlgoOID(sigAlgoOID);
    VerifyOrExit(shaNID != NID_undef, err = WEAVE_ERROR_UNSUPPORTED_SIGNATURE_TYPE);

    rsa = d2i_RSAPrivateKey(NULL, &keyDER, keyDERLen);
    VerifyOrExit(rsa != NULL, err = WEAVE_ERROR_NO_MEMORY);

    if (RSA_sign(shaNID, hash, hashLen, sigBuf, &sigLen, rsa) == 0)
        ExitNow(err = WEAVE_ERROR_NO_MEMORY);

    sig.Sig = sigBuf;
    sig.Len = sigLen;

    // Encode an RSASignature value into the supplied writer.
    err = sig.WriteSignature(writer, tag);
    SuccessOrExit(err);

exit:
    if (NULL != rsa) RSA_free(rsa);

    return err;
}

/**
 * Verify a Weave RSA signature.
 *
 * Verifies an RSA signature using given data hash and an X509 encoded RSA certificate containing
 * the public key to be used to verify the signature.
 *
 * @param[in] sigAlgoOID        Algorithm OID to be used to generate RSA signature.
 * @param[in] hash              A buffer containing the hash of the data to be signed.
 * @param[in] hashLen           The length in bytes of the data hash.
 * @param[in] sig               Encoded RSA signature to be verified.
 * @param[in] certDER           A buffer containing the certificate with public key to be used
 *                              to verify the signature. The certificate is expected to be DER
 *                              encoded an X509 RSA structure.
 * @param[in] certDERLen        The length in bytes of the encoded certificate.
 *
 * @retval #WEAVE_NO_ERROR      If the operation succeeded.
 */
WEAVE_ERROR VerifyRSASignature(OID sigAlgoOID,
                               const uint8_t * hash, uint8_t hashLen,
                               const EncodedRSASignature& sig,
                               const uint8_t * certDER, uint16_t certDERLen)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    BIO *certBuf = NULL;
    X509 *cert = NULL;
    EVP_PKEY *pubKey = NULL;
    int shaNID;
    int res;

    shaNID = ShaNIDFromSigAlgoOID(sigAlgoOID);
    VerifyOrExit(shaNID != NID_undef, err = WEAVE_ERROR_UNSUPPORTED_SIGNATURE_TYPE);

    certBuf = BIO_new_mem_buf(certDER, certDERLen);
    VerifyOrExit(certBuf != NULL, err = WEAVE_ERROR_NO_MEMORY);

    cert = d2i_X509_bio(certBuf, NULL);
    VerifyOrExit(cert != NULL, err = WEAVE_ERROR_NO_MEMORY);

    pubKey = X509_get_pubkey(cert);
    VerifyOrExit(pubKey != NULL, err = WEAVE_ERROR_NO_MEMORY);

    // Verify that the public key from the certificate is an RSA key.
    VerifyOrExit(EVP_PKEY_RSA == EVP_PKEY_base_id(pubKey), err = WEAVE_ERROR_WRONG_KEY_TYPE);

    res = RSA_verify(shaNID, hash, hashLen, sig.Sig, sig.Len, EVP_PKEY_get1_RSA(pubKey));
    VerifyOrExit(res == 1, err = WEAVE_ERROR_INVALID_SIGNATURE);

exit:
    if (NULL != pubKey) EVP_PKEY_free(pubKey);
    if (NULL != cert) X509_free(cert);
    if (NULL != certBuf) BIO_free(certBuf);

    return err;
}

#endif // WEAVE_WITH_OPENSSL

// ==================== Documentation for Inline Public Members ====================

/**
 * @fn WEAVE_ERROR EncodedRSASignature::WriteSignature(TLVWriter& writer, uint64_t tag) const
 *
 * Writes the signature as a Weave RSASignature structure to the specified TLV writer
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
