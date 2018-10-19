/*
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
 *      This file provides EAX implementations for the Weave layer.
 *
 */

#ifndef EAX_H_
#define EAX_H_

#include "AESBlockCipher.h"
#include "WeaveCrypto.h"

/*
 * Configurable option: if WEAVE_CONFIG_EAX_NO_PAD_CACHE is defined and
 * non-zero, then the internal "L1" value is not cached; this saves 16
 * bytes of RAM in the EAX class, but requires three extra AES block
 * invocations per message.
 *
#define WEAVE_CONFIG_EAX_NO_PAD_CACHE   1
 */

/*
 * Configurable option: if WEAVE_CONFIG_EAX_NO_CHUNK is defined and
 * non-zero, then the EAX class expects non-chunked input of header
 * and payload: only a single InjectHeader() call, and a single
 * Encrypt() or Decrypt() call, may be used for a given message. This
 * constrains usage, but saves 32 bytes of RAM in the EAX class. There
 * is no CPU cost penalty.
 *
#define WEAVE_CONFIG_EAX_NO_CHUNK       1
 */

namespace nl {
namespace Weave {
namespace Platform {
namespace Security {

class EAX;

/*
 * Support class for EAX optimization: an instance can be filled with
 * intermediate processing results, so that several messages that use
 * the same header and are encrypted or decrypted with the same key can
 * share some of the computational cost.
 */
class EAXSaved
{
public:
    EAXSaved(void);
    ~EAXSaved(void);
private:
    enum {
        kBlockLength = 16
    };
    uint8_t aad[kBlockLength];  // Saved OMAC^1(header)
#if !WEAVE_CONFIG_EAX_NO_CHUNK
    uint8_t om2[kBlockLength];  // Saved encryption of the OMAC^2 start block
#endif
    friend class EAX;
};

/*
 * Base class for EAX implementations; users should create an
 * EAX128 or EAX256 instance, based on key size.
 *
 * Normal usage:
 *
 *  - Use SetKey() to set the AES key. This must be done first. A new
 *    key can be set at any time, which cancels any ongoing computation.
 *
 *  - Call Start() or StartWeave() to start processing a new message.
 *    A key must have been set. These methods can be called at any
 *    time (this cancels any ongoing computation). StartWeave() uses
 *    a 12-byte nonce that encodes the sending node ID and the message ID.
 *
 *  - Inject the header with one or several calls to InjectHeader().
 *    This must follow a Start(), but must precede payload encryption
 *    or decryption. If InjectHeader() is not called, a zero-length
 *    header is assumed.
 *
 *  - Encrypt or decrypt the data, with one or several calls to
 *    Encrypt() or Decrypt(). Calls for a given message must be all
 *    encrypt or all decrypt. Encryption and decryption do not change
 *    the length of the data; chunks of arbitrary lengths can be used
 *    (even zero-length chunks).
 *
 *  - Finalize the computation of the authentication tag, and get it
 *    (with GetTag()) or check it (with CheckTag()). Encryption will
 *    typically use GetTag() (to obtain the tag value to send to the
 *    recipient) while decryption more naturally involves calling
 *    CheckTag() (to verify the tag value received from the sender).
 */
class EAX
{
public:
    enum {
        kMinTagLength = 8,   // minimum tag length, in bytes
        kMaxTagLength = 16   // maximum tag length, in bytes
    };

    /*
     * Clear this object from all secret key and data. This is
     * automatically called by the destructor.
     */
    void Reset(void);

    /*
     * Set the AES key. The key size depends on the chosen concrete class
     * (EAX128 or EAX256).
     */
    void SetKey(const uint8_t *key);

    /*
     * Process a header and fill the provided 'sav' object with the
     * result. That object can then be reused with StartSaved()
     * or StartWeaveSaved() to process messages that share the same
     * header value (and use the same key).
     */
    void SaveHeader(const uint8_t *header, size_t headerLen, EAXSaved *sav);

    /*
     * Start a new message processing, with the given nonce. This may be
     * called at any time, provided that the key has been set. Nonce
     * length is arbitrary, but the same nonce value MUST NOT be reused
     * with the same key for a different message.
     */
    void Start(const uint8_t *nonce, size_t nonceLen);

    /*
     * Variant of Start() that uses a 12-byte nonce that encodes the
     * provided sending node ID, and message ID (values use big-endian
     * encoding, the sending node ID comes first).
     */
    void StartWeave(uint64_t sendingNodeId, uint32_t msgId);

    /*
     * Start a new message processing, with the given nonce and saved
     * processed header. The 'sav' object is not modified, and may be
     * reused for other messages that use the same key and header.
     */
    void StartSaved(const uint8_t *nonce, size_t nonceLen, const EAXSaved *sav);

    /*
     * Variant of StartSaved() that uses a 12-byte nonce that encodes the
     * provided sending node ID, and message ID (values use big-endian
     * encoding, the sending node ID comes first).
     */
    void StartWeaveSaved(uint64_t sendingNodeId,
                         uint32_t msgId,
                         const EAXSaved *sav);

    /*
     * Inject the header. The header data is not encrypted, but participates
     * to the authentication tag. Header injection must occur after the
     * call to Start(), but before processing the payload. If no header
     * is injected, then a zero-length header is used.
     *
     * Unless WEAVE_CONFIG_EAX_NO_CHUNK is enabled, the header may be
     * injected in several chunks (several calls to InjectHeader() with
     * arbitrary chunk lengths).
     */
    void InjectHeader(const uint8_t *header, size_t headerLen);

    /*
     * Encrypt the provided payload. Input data (plaintext) is read from
     * 'input' and has size 'inputLen' bytes; the corresponding data has
     * the same length and is written in 'output'. The 'input' and 'output'
     * buffers may overlap partially or totally.
     *
     * Unless WEAVE_CONFIG_EAX_NO_CHUNK is enabled, the payload may be
     * processed in several chunks (several calls to Encrypt() with
     * arbitrary chunk lengths).
     */
    void Encrypt(const uint8_t *input, size_t inputLen, uint8_t *output);

    /*
     * Variant of Encrypt() for in-place processing: the encrypted data
     * replaces the plaintext data in the 'data' buffer.
     */
    void Encrypt(uint8_t *data, size_t dataLen);

    /*
     * Identical to Encrypt(), except for decryption instead of encryption.
     * Note that, for a given message, all chunks must be encrypted, or
     * all chunks must be decrypted; mixing encryption and decryption for
     * a single message is not permitted.
     */
    void Decrypt(const uint8_t *input, size_t inputLen, uint8_t *output);

    /*
     * Variant of Decrypt() for in-place processing: the decrypted data
     * replaces the ciphertext data in the 'data' buffer.
     */
    void Decrypt(uint8_t *data, size_t dataLen);

    /*
     * Finalize encryption or decryption, and get the authentication tag.
     * This may be called only once per message; after the tag has been
     * obtained, only SetKey() and Start() may be called again on the
     * instance.
     *
     * Tag length must be between 8 and 16 bytes. Normal EAX tag length is
     * 16 bytes.
     */
    void GetTag(uint8_t *tag, size_t tagLen);

    /*
     * Variant of GetTag() that does not return the tag, but compares it
     * with the provided tag value. This is meant to be used by recipient,
     * to verify the tag on an incoming message. Returned value is true
     * if the tags match, false otherwise. Comparison is constant-time.
     */
    bool CheckTag(const uint8_t *tag, size_t tagLen);

protected:
    EAX(void);
    ~EAX(void);

private:
    enum {
        kBlockLength = 16
    };
    virtual void AESReset(void) = 0;
    virtual void AESSetKey(const uint8_t *key) = 0;
    virtual void AESEncryptBlock(uint8_t *data) = 0;

#if !WEAVE_CONFIG_EAX_NO_PAD_CACHE
    uint8_t L1[kBlockLength];
#endif
#if !WEAVE_CONFIG_EAX_NO_CHUNK
    uint8_t buf[kBlockLength];
    uint8_t cbcmac[kBlockLength];
#endif
    uint8_t ctr[kBlockLength];
    uint8_t acc[kBlockLength];
#if !WEAVE_CONFIG_EAX_NO_CHUNK
    uint8_t ptr;
#endif
    uint8_t state;

    void double_gf128(uint8_t *elt);
    void xor_block(const uint8_t *src, uint8_t *dst);
    void omac(unsigned val, const uint8_t *data, size_t len, uint8_t *mac);
#if !WEAVE_CONFIG_EAX_NO_CHUNK
    void omac_start(unsigned val);
    void omac_process(const uint8_t *data, size_t len);
    void omac_finish(unsigned val);
    void aad_finish(void);
#endif
    void incr_ctr(void);
    void payload_process(bool encrypt, uint8_t *data, size_t len);
};

/*
 * Concrete class for EAX with a 128-bit key (16 bytes).
 */
class NL_DLL_EXPORT EAX128 : public EAX
{
public:
    enum
    {
        kKeyLength      = 16,
        kKeyLengthBits  = kKeyLength * CHAR_BIT
    };
private:
    AES128BlockCipherEnc aesCtx;
    void AESReset(void);
    void AESSetKey(const uint8_t *key);
    void AESEncryptBlock(uint8_t *data);
};

/*
 * Concrete class for EAX with a 256-bit key (32 bytes).
 */
class NL_DLL_EXPORT EAX256 : public EAX
{
public:
    enum
    {
        kKeyLength      = 32,
        kKeyLengthBits  = kKeyLength * CHAR_BIT
    };
private:
    AES256BlockCipherEnc aesCtx;
    void AESReset(void);
    void AESSetKey(const uint8_t *key);
    void AESEncryptBlock(uint8_t *data);
};

} // namespace Security
} // namespace Platform
} // namespace Weave
} // namespace nl

#endif /* AES_H_ */
