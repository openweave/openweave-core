/*
 *
 *    Copyright (c) 2017 Nest Labs, Inc.
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
 *      This file implements EAX functions for the Weave layer.
 *
 */

#include <string.h>

#include <Weave/Support/CodeUtils.h>

#include "WeaveCrypto.h"
#include "EAX.h"

namespace nl {
namespace Weave {
namespace Platform {
namespace Security {

/*
 * The 'state' variable maintains current status of the object:
 *
 *   ST_EMPTY     Created, no key set yet
 *   ST_KEYED     Key is set, ready for a new message (Start() call)
 *   ST_AAD       Start() was called, waiting for AAD
 *   ST_ENCRYPT   Payload is being encrypted
 *   ST_DECRYPT   Payload is being decrypted
 *   ST_PAYLOAD   AAD finished, ready for payload
 *   ST_TAG       Tag was computed in acc[]
 *
 * Calls and transitions:
 *   SetKey()        goes to ST_KEYED; cancels any ongoing computation
 *   Start()         requires not ST_EMPTY; goes to ST_AAD; cancels ongoing
 *   InjectHeader()  requires ST_AAD
 *   Encrypt()       requires ST_AAD, ST_ENCRYPT or ST_PAYLOAD;
 *                   goes to ST_ENCRYPT
 *   Decrypt()       requires ST_AAD, ST_DECRYPT or ST_PAYLOAD;
 *                   goes to ST_DECRYPT
 *   GetTag()        requires ST_ADD, ST_ENCRYPT, ST_DECRYPT or ST_PAYLOAD;
 *                   goes to ST_TAG
 *   CheckTag()      requires ST_ADD, ST_ENCRYPT, ST_DECRYPT or ST_PAYLOAD;
 *                   goes to ST_TAG
 *
 * The ST_PAYLOAD is a special case for when the AAD was injected from a
 * saved state object (EAXSaved).
 *
 * The VerifyOrDie() macro is used to react on violations. Such cases
 * happen only if the calling code is wrong, not because of invalid
 * data from the outside.
 *
 * When WEAVE_CONFIG_EAX_NO_CHUNK is activated, ST_ENCRYPT and ST_DECRYPT
 * are not used; instead, AAD processing leads to ST_PAYLOAD state, and
 * calling Encrypt() or Decrypt() brings to ST_TAG.
 */
#define ST_EMPTY     0
#define ST_KEYED     1
#define ST_AAD       2
#define ST_PAYLOAD   3
#define ST_ENCRYPT   4
#define ST_DECRYPT   5
#define ST_TAG       6

/*
 * Implementation Notes
 * ====================
 *
 * L1[] contains the encryption of the all-zero block. This is used when
 * processing the nonce; the pad blocks for OMAC also use it (pad blocks
 * use either L2 or L4, where L2 is the double of L1 in GF(2^128), and
 * L4 is the double of L2 in GF(2^128)).
 *
 * buf[] is an all-purpose buffer:
 *
 *  - When using OMAC only (processing of nonce and header), it contains
 *    ptr unprocessed bytes. That value ranges from 1 to 16 (inclusive).
 *
 *  - When using both OMAC and CTR (processing of payload):
 *      The first ptr bytes must still be processed with OMAC.
 *      The remaining 16-ptr bytes are CTR stream bytes to be XORed into
 *      the next 16-ptr payload bytes.
 *    Again, ptr ranges from 1 to 16.
 *
 * cbcmac[] is the OMAC buffer. It contains the current CBC-MAC value.
 *
 * ctr[] is the counter for CTR encryption/decryption. It contains the
 * counter value for the next invocation of AES/CTR.
 *
 * acc[] is the buffer that accumulates the tag value:
 *  - It first receives a copy of OMAC^0(nonce).
 *  - OMAC^1(header) is XORed into it.
 *  - OMAC^2(ciphertext) is XORed into it.
 *
 * When WEAVE_CONFIG_EAX_NO_CHUNK is not enabled, the 'ptr' field is
 * present and normally has a value between 1 and 16 (inclusive). There
 * is a special case where ptr == 0: when StartSaved() has been used.
 * In that case, the first OMAC^2 block, already encrypted, has been
 * saved in cbcmac[]. Code in this class defensively assumes that ptr
 * may be zero at all times.
 */

EAXSaved::EAXSaved(void)
{
}

EAXSaved::~EAXSaved(void)
{
    ClearSecretData(aad, sizeof aad);
#if !WEAVE_CONFIG_EAX_NO_CHUNK
    ClearSecretData(om2, sizeof om2);
#endif
}

EAX::EAX(void)
{
    state = ST_EMPTY;
}

EAX::~EAX(void)
{
    Reset();
}

void
EAX::Reset(void)
{
#if !WEAVE_CONFIG_EAX_NO_PAD_CACHE
    ClearSecretData(L1, sizeof L1);
#endif
#if !WEAVE_CONFIG_EAX_NO_CHUNK
    ClearSecretData(buf, sizeof buf);
    ClearSecretData(cbcmac, sizeof cbcmac);
#endif
    ClearSecretData(ctr, sizeof ctr);
    ClearSecretData(acc, sizeof acc);

    state = ST_EMPTY;
}

/*
 * Double a value in finite field GF(2^128), with modulus X^128+X^7+X^2+X+1.
 * Value bytes are in big-endian order: elt[15] is the byte corresponding
 * to 1, X, X^2,... X^7. Within each byte, numerical encoding is used, i.e.
 * X^7 is the most significant bit in elt[15].
 */
void
EAX::double_gf128(uint8_t *elt)
{
    unsigned cc;
    int i;

    /*
     * 'cc' is a constant-time extraction of the top bit, promoted to the
     * effect of the field modulus (0x87 is the encoding for X^7+X^2+X+1).
     */
    cc = 0x87 & -((unsigned)elt[0] >> 7);
    for (i = kBlockLength - 1; i >= 0; i --) {
        unsigned z;

        z = (elt[i] << 1) ^ cc;
        cc = z >> 8;
        elt[i] = (uint8_t)z;
    }
}

/*
 * XOR a block (16 bytes) into another.
 */
void
EAX::xor_block(const uint8_t *src, uint8_t *dst)
{
    /*
     * TODO: this is done byte-by-byte; on some architectures, it
     * could be done by full words, or even bigger values. E.g. on
     * x86 with SSE2, _mm_loadu_si128() and _mm_storeu_si128() can
     * read and write unaligned 128-bit values, and _mm_xor_si128()
     * can compute the full XOR in a single cycle.
     */
    size_t u;

    for (u = 0; u < kBlockLength; u ++) {
        dst[u] ^= src[u];
    }
}

/*
 * This method computes OMAC^val on data[], result in mac[] (16 bytes).
 * This handles non-chunked input, with no buffering.
 *
 * If val == 0 and len != 0, then the first block to be encrypted will
 * be the all-zero block. If WEAVE_CONFIG_EAX_NO_PAD_CACHE is not enabled,
 * the encryption of the all-zero block is available in the L1[] array,
 * and it is automatically reused by this function.
 */
void
EAX::omac(unsigned val, const uint8_t *data, size_t len, uint8_t *mac)
{
    /*
     * There are three situations:
     *
     *  - Data is empty; the pad block is L2, XORed into the initial
     *    OMAC^2 block (0000...0002).
     *
     *  - Data has length multiple of 16 and is not empty; pad block
     *    is L2, XORed into the last block.
     *
     *  - Data has length not multiple of 16; last partial block is
     *    padded with 0x80 then zeros, and XORed with the pad block,
     *    which is L4.
     */
    uint8_t pad[kBlockLength];
    size_t u, v;

#if WEAVE_CONFIG_EAX_NO_PAD_CACHE
    memset(pad, 0, sizeof pad);
    AESEncryptBlock(pad);
#else
    memcpy(pad, L1, sizeof L1);
#endif
    double_gf128(pad);
    if ((len & (kBlockLength - 1)) != 0) {
        /*
         * Note: we wanted to test whether len was a multiple of the
         * block length (16 bytes), but divisions are expensive. Here,
         * we used the fact that the block length is a power of two,
         * which allows for using a faster bitwise AND.
         */
        double_gf128(pad);
    }

    /*
     * The following cases may happen:
     *
     *  - If len == 0, then the output is the encryption of the XOR of
     *    the first block (all-zero except the last byte) with the pad
     *    block.
     *
     *  - If len != 0 and val == 0 and WEAVE_CONFIG_EAX_NO_PAD_CACHE is
     *    disabled, then the first block to be encrypted is the all-zero
     *    block, and it already is in L1[], so we reuse it.
     *
     *  - Otherwise, the first block to be encrypted is not the all-zero
     *    block (or we don't have L1); the first block must be assembled
     *    and used as starting point.
     */
    if (len == 0) {
        memcpy(mac, pad, sizeof pad);
        mac[kBlockLength - 1] ^= (uint8_t)val;
        AESEncryptBlock(mac);
    } else {
#if WEAVE_CONFIG_EAX_NO_PAD_CACHE
        memset(mac, 0, kBlockLength);
        mac[kBlockLength - 1] = (uint8_t)val;
        AESEncryptBlock(mac);
#else
        if (val == 0) {
            memcpy(mac, L1, sizeof L1);
        } else {
            memset(mac, 0, kBlockLength);
            mac[kBlockLength - 1] = (uint8_t)val;
            AESEncryptBlock(mac);
        }
#endif
        for (u = 0; (u + kBlockLength) < len; u += kBlockLength) {
            xor_block(data + u, mac);
            AESEncryptBlock(mac);
        }
        for (v = 0; (u + v) < len; v ++) {
            mac[v] ^= data[u + v];
        }
        if (v < kBlockLength) {
            mac[v] ^= 0x80;
        }
        xor_block(pad, mac);
        AESEncryptBlock(mac);
    }
}

#if !WEAVE_CONFIG_EAX_NO_CHUNK

/*
 * Start OMAC processing: buffer is set to the initial block whose last
 * byte has value 'val' (normally, 0 for the nonce, 1 for the header,
 * 2 for the ciphertext).
 */
void
EAX::omac_start(unsigned val)
{
    memset(cbcmac, 0, sizeof cbcmac);
    memset(buf, 0, sizeof buf);
    buf[kBlockLength - 1] = val;
    ptr = kBlockLength;
}

/*
 * Continue OMAC processing on the provided data.
 */
void
EAX::omac_process(const uint8_t *data, size_t len)
{
    if (len == 0) {
        return;
    }

    /*
     * Make sure that buf[] is full, and that there still are bytes to
     * process after that.
     */
    if (ptr != kBlockLength) {
        size_t clen;

        clen = kBlockLength - ptr;
        if (clen >= len) {
            memcpy(buf + ptr, data, len);
            ptr += len;
            return;
        }
        memcpy(buf + ptr, data, clen);
        data += clen;
        len -= clen;
    }

    /*
     * buf[] is full and there are remaining bytes to process, so we
     * can compute one block.
     */
    xor_block(buf, cbcmac);
    AESEncryptBlock(cbcmac);

    /*
     * Process full blocks, as long as at least one unprocessed byte
     * remains afterwards.
     */
    while (len > kBlockLength) {
        xor_block(data, cbcmac);
        AESEncryptBlock(cbcmac);
        data += kBlockLength;
        len -= kBlockLength;
    }

    /*
     * Buffer unprocessed bytes.
     */
    memcpy(buf, data, len);
    ptr = len;
}

/*
 * Finish OMAC. The MAC value is in cbcmac[]. The 'val' parameter is the
 * type of OMAC (1 for AAD, 2 for ciphertext): it is used if ptr == 0
 * (meaning that the first block must be rebuilt and subject to padding).
 */
void
EAX::omac_finish(unsigned val)
{
    uint8_t pad[kBlockLength];

    /*
     * If the total input size is a multiple of the block size, then
     * the last block (still in buf[] at that point) is XORed with L1;
     * otherwise, padding is applied (0x80, then 0x00 bytes up to the
     * next block boundary) and L4 is XORed in.
     */
#if WEAVE_CONFIG_EAX_NO_PAD_CACHE
    memset(pad, 0, sizeof pad);
    AESEncryptBlock(pad);
#else
    memcpy(pad, L1, sizeof L1);
#endif
    double_gf128(pad);
    if (ptr == 0) {
        memcpy(cbcmac, pad, sizeof pad);
        cbcmac[kBlockLength - 1] ^= (uint8_t)val;
    } else {
        if (ptr != kBlockLength) {
            double_gf128(pad);
            buf[ptr ++] = 0x80;
            memset(buf + ptr, 0x00, kBlockLength - ptr);
        }
        xor_block(buf, cbcmac);
        xor_block(pad, cbcmac);
    }
    AESEncryptBlock(cbcmac);
}

/*
 * Finish the OMAC^1 on AAD, and prepare things for payload processing.
 * This does NOT set the 'state' value.
 */
void
EAX::aad_finish(void)
{
    omac_finish(1);
    xor_block(cbcmac, acc);
    omac_start(2);
}

#endif  // WEAVE_CONFIG_EAX_NO_CHUNK

/*
 * Increment the CTR counter.
 */
void
EAX::incr_ctr(void)
{
    /*
     * TODO: optimize this when possible. Counter encoding is big-endian
     * with the full 128-bit width; since the starting point is the
     * OMAC^0(nonce) value, it cannot be assumed that the leftmost bytes
     * will remain untouched.
     *
     * If the current architecture supports big-endian unaligned accesses
     * for words, when we could read the counter as a sequence of four
     * 32-bit words, or even two 64-bit words, and handle the long addition
     * with fewer iterations.
     */
    unsigned cc;
    int i;

    cc = 1;
    for (i = kBlockLength - 1; i >= 0; i --) {
        unsigned z;

        z = ctr[i] + cc;
        ctr[i] = (uint8_t)z;
        cc = z >> 8;
    }
}

/*
 * Payload processing. Data is encrypted or decrypted in place. This
 * method assumes that the 'state' has already been checked.
 */
void
EAX::payload_process(bool encrypt, uint8_t *data, size_t len)
{

#if WEAVE_CONFIG_EAX_NO_CHUNK

    /*
     * In non-buffering mode, we process the whole payload in one go.
     * We have to XOR the data with the AES/CTR stream, and also
     * compute the CBC-MAC on the ciphertext. Depending on whether we
     * encrypt or decrypt, AES/CTR must happen first or not.
     */
    int i, te;

    te = encrypt ? 0 : 1;
    for (i = 0; i < 2; i ++) {
        if (i == te) {
            /*
             * AES/CTR encryption. This is straightforward, except for
             * the final block, which may be incomplete.
             */
            size_t u;

            for (u = 0; u < len; u += kBlockLength) {
                uint8_t tmp[kBlockLength];

                memcpy(tmp, ctr, sizeof ctr);
                incr_ctr();
                AESEncryptBlock(tmp);
                if ((u + kBlockLength) <= len) {
                    xor_block(tmp, data + u);
                } else {
                    size_t v;

                    for (v = u; v < len; v ++) {
                        data[v] ^= tmp[v - u];
                    }
                }
            }
        } else {
            /*
             * CBC-MAC on the ciphertext.
             */
            uint8_t mac[kBlockLength];

            omac(2, data, len, mac);
            xor_block(mac, acc);
        }
    }

#else

    uint8_t tmp[kBlockLength];

    /*
     * Complete current block, if applicable.
     * If ptr == 0, this is a special case: the previous OMAC block
     * has been encrypted, but the next CTR block has not been generated.
     */
    if (ptr < kBlockLength) {
        size_t clen, u;

        if (ptr == 0) {
            if (len == 0) {
                return;
            }
            memcpy(buf, ctr, sizeof ctr);
            incr_ctr();
            AESEncryptBlock(buf);
        }
        clen = kBlockLength - ptr;
        if (clen > len) {
            clen = len;
        }
        if (encrypt) {
            for (u = 0; u < clen; u ++) {
                data[u] ^= buf[ptr + u];
                buf[ptr + u] = data[u];
            }
        } else {
            for (u = 0; u < clen; u ++) {
                unsigned z;

                z = data[u];
                data[u] ^= buf[ptr + u];
                buf[ptr + u] = z;
            }
        }
        data += clen;
        ptr += clen;
        len -= clen;
    }

    if (len == 0) {
        return;
    }

    /*
     * At that point, the buffer is full, and some data remains afterwards.
     * Therefore, we can process the buffered block with OMAC.
     */
    xor_block(buf, cbcmac);
    AESEncryptBlock(cbcmac);

    /*
     * We now have an empty buffer; we MUST exit this function with a
     * non-empty buffer, so we process full blocks without buffering
     * only as long as there remain more than 16 bytes.
     *
     * We perform interleaved CTR and CBC-MAC under the hope that, on
     * recent enough x86 with AES-NI opcodes, the out-of-order execution
     * unit will leverage parallelism and run two AES instances at a
     * time.
     */
    if (encrypt) {
        while (len > kBlockLength) {
            memcpy(tmp, ctr, sizeof ctr);
            incr_ctr();
            AESEncryptBlock(tmp);
            xor_block(tmp, data);
            xor_block(data, cbcmac);
            AESEncryptBlock(cbcmac);
            data += kBlockLength;
            len -= kBlockLength;
        }
    } else {
        while (len > kBlockLength) {
            memcpy(tmp, ctr, sizeof ctr);
            incr_ctr();
            AESEncryptBlock(tmp);
            xor_block(data, cbcmac);
            xor_block(tmp, data);
            AESEncryptBlock(cbcmac);
            data += kBlockLength;
            len -= kBlockLength;
        }
    }

    /*
     * Now there are between 1 and 16 bytes that need to be processed.
     * We need to put the 'len' ciphertext bytes in buf[], along with
     * the remainder of the CTR stream block.
     */
    memcpy(tmp, ctr, sizeof ctr);
    incr_ctr();
    AESEncryptBlock(tmp);
    if (encrypt) {
        size_t u;

        for (u = 0; u < len; u ++) {
            data[u] ^= tmp[u];
        }
        memcpy(buf, data, len);
    } else {
        size_t u;

        memcpy(buf, data, len);
        for (u = 0; u < len; u ++) {
            data[u] ^= tmp[u];
        }
    }
    memcpy(buf + len, tmp + len, kBlockLength - len);
    ptr = len;

#endif  // WEAVE_CONFIG_EAX_NO_CHUNK
}

void
EAX::SetKey(const uint8_t *key)
{
    AESSetKey(key);

#if !WEAVE_CONFIG_EAX_NO_PAD_CACHE
    /*
     * We encrypt the all-zero block.
     */
    memset(L1, 0, sizeof L1);
    AESEncryptBlock(L1);
#endif

    state = ST_KEYED;
}

void
EAX::SaveHeader(const uint8_t *header, size_t headerLen, EAXSaved *sav)
{
    /*
     * We compute OMAC^1(header) and save it.
     */
    omac(1, header, headerLen, sav->aad);

#if !WEAVE_CONFIG_EAX_NO_CHUNK
    /*
     * We also pre-process the first block of OMAC^2 (it will be used to
     * speed up the processing of the ciphertext, except in the
     * pathological case of an empty ciphertext).
     */
    memset(sav->om2, 0, sizeof sav->om2);
    sav->om2[kBlockLength - 1] = 2;
    AESEncryptBlock(sav->om2);
#endif
}

void
EAX::Start(const uint8_t *nonce, size_t nonceLen)
{
    /*
     * A key must have been set.
     */
    VerifyOrDie(state != ST_EMPTY);

    /*
     * Process the nonce with OMAC^0.
     * Result is both one of the three values that make up the tag, but
     * also the initial counter value for CTR encryption.
     */
    omac(0, nonce, nonceLen, acc);
    memcpy(ctr, acc, sizeof acc);

#if !WEAVE_CONFIG_EAX_NO_CHUNK
    /*
     * Start OMAC^1 for the AAD (header).
     */
    omac_start(1);
#endif

    state = ST_AAD;
}

void
EAX::StartWeave(uint64_t sendingNodeId, uint32_t msgId)
{
    /*
     * We reuse ctr[] in order to avoid allocating a stack buffer
     * for the synthetic nonce. It saves a dozen bytes.
     */
    ctr[ 0] = (uint8_t)(sendingNodeId >> 56);
    ctr[ 1] = (uint8_t)(sendingNodeId >> 48);
    ctr[ 2] = (uint8_t)(sendingNodeId >> 40);
    ctr[ 3] = (uint8_t)(sendingNodeId >> 32);
    ctr[ 4] = (uint8_t)(sendingNodeId >> 24);
    ctr[ 5] = (uint8_t)(sendingNodeId >> 16);
    ctr[ 6] = (uint8_t)(sendingNodeId >>  8);
    ctr[ 7] = (uint8_t)sendingNodeId;
    ctr[ 8] = (uint8_t)(msgId >> 24);
    ctr[ 9] = (uint8_t)(msgId >> 16);
    ctr[10] = (uint8_t)(msgId >>  8);
    ctr[11] = (uint8_t)msgId;
    Start(ctr, 12);
}

void
EAX::StartSaved(const uint8_t *nonce, size_t nonceLen, const EAXSaved *sav)
{
    Start(nonce, nonceLen);
    xor_block(sav->aad, acc);
    state = ST_PAYLOAD;
#if !WEAVE_CONFIG_EAX_NO_CHUNK
    memcpy(cbcmac, sav->om2, sizeof sav->om2);
    ptr = 0;
#endif
}

void
EAX::StartWeaveSaved(uint64_t sendingNodeId,
                     uint32_t msgId,
                     const EAXSaved *sav)
{
    StartWeave(sendingNodeId, msgId);
    xor_block(sav->aad, acc);
    state = ST_PAYLOAD;
#if !WEAVE_CONFIG_EAX_NO_CHUNK
    memcpy(cbcmac, sav->om2, sizeof sav->om2);
    ptr = 0;
#endif
}

void
EAX::InjectHeader(const uint8_t *header, size_t headerLen)
{
    /*
     * We can inject AAD only in ST_AAD state.
     */
    VerifyOrDie(state == ST_AAD);

#if WEAVE_CONFIG_EAX_NO_CHUNK
    uint8_t mac[kBlockLength];

    omac(1, header, headerLen, mac);
    xor_block(mac, acc);
    state = ST_PAYLOAD;
#else
    omac_process(header, headerLen);
#endif
}

void
EAX::Encrypt(const uint8_t *input, size_t inputLen, uint8_t *output)
{
    if (input != output) {
        memmove(output, input, inputLen);
    }
    Encrypt(output, inputLen);
}

void
EAX::Encrypt(uint8_t *data, size_t dataLen)
{
#if WEAVE_CONFIG_EAX_NO_CHUNK
    if (state == ST_AAD) {
        InjectHeader(NULL, 0);
    } else {
        VerifyOrDie(state == ST_PAYLOAD);
    }
#else
    if (state == ST_AAD) {
        aad_finish();
        state = ST_ENCRYPT;
    } else if (state == ST_PAYLOAD) {
        state = ST_ENCRYPT;
    } else {
        VerifyOrDie(state == ST_ENCRYPT);
    }
#endif

    payload_process(true, data, dataLen);
#if WEAVE_CONFIG_EAX_NO_CHUNK
    state = ST_TAG;
#endif
}

void
EAX::Decrypt(const uint8_t *input, size_t inputLen, uint8_t *output)
{
    if (input != output) {
        memmove(output, input, inputLen);
    }
    Decrypt(output, inputLen);
}

void
EAX::Decrypt(uint8_t *data, size_t dataLen)
{
#if WEAVE_CONFIG_EAX_NO_CHUNK
    if (state == ST_AAD) {
        InjectHeader(NULL, 0);
    } else {
        VerifyOrDie(state == ST_PAYLOAD);
    }
#else
    if (state == ST_AAD) {
        aad_finish();
        state = ST_DECRYPT;
    } else if (state == ST_PAYLOAD) {
        state = ST_DECRYPT;
    } else {
        VerifyOrDie(state == ST_DECRYPT);
    }
#endif

    payload_process(false, data, dataLen);
#if WEAVE_CONFIG_EAX_NO_CHUNK
    state = ST_TAG;
#endif
}

void
EAX::GetTag(uint8_t *tag, size_t tagLen)
{
    /*
     * Sanity check on tag length.
     */
    VerifyOrDie(tagLen >= kMinTagLength && tagLen <= kMaxTagLength);

#if WEAVE_CONFIG_EAX_NO_CHUNK
    if (state == ST_AAD || state == ST_PAYLOAD) {
        /*
         * If we are not at ST_TAG yet, then the payload and possibly
         * the AAD have not been provided, which means they are empty.
         * Calling Encrypt() will set things right.
         */
        Encrypt(NULL, 0);
    } else {
        VerifyOrDie(state == ST_TAG);
    }
#else
    if (state == ST_AAD || state == ST_PAYLOAD) {
        /*
         * If we are still in ST_AAD, then this means that the
         * payload is empty. We temporarily claim encryption.
         */
        Encrypt(NULL, 0);
    }
    if (state == ST_ENCRYPT || state == ST_DECRYPT) {
        /*
         * We have to finish OMAC^2.
         */
        omac_finish(2);
        xor_block(cbcmac, acc);
        state = ST_TAG;
    } else {
        VerifyOrDie(state == ST_TAG);
    }
#endif

    /* At that point, the tag is in acc[] and state is ST_TAG. */
    memcpy(tag, acc, tagLen);
}

bool
EAX::CheckTag(const uint8_t *tag, size_t tagLen)
{
    uint8_t tmp[kMaxTagLength];
    unsigned z;
    size_t u;

    /*
     * Invalid tag lengths are reported with false, since that might be
     * triggered with crafted incoming data.
     */
    if (tagLen < kMinTagLength || tagLen > kMaxTagLength) {
        return false;
    }

    /*
     * Get the tag and compare it with the provided value. The loop
     * below performs a constant-time equality comparison.
     */
    GetTag(tmp, tagLen);
    z = 0;
    for (u = 0; u < tagLen; u ++) {
        z |= tag[u] ^ tmp[u];
    }
    return z == 0;
}

/*
 * Key-size specific functions.
 */

void
EAX128::AESReset(void)
{
    aesCtx.Reset();
}

void
EAX128::AESSetKey(const uint8_t *key)
{
    aesCtx.SetKey(key);
}

void
EAX128::AESEncryptBlock(uint8_t *data)
{
    aesCtx.EncryptBlock(data, data);
}

void
EAX256::AESReset(void)
{
    aesCtx.Reset();
}

void
EAX256::AESSetKey(const uint8_t *key)
{
    aesCtx.SetKey(key);
}

void
EAX256::AESEncryptBlock(uint8_t *data)
{
    aesCtx.EncryptBlock(data, data);
}

} // namespace Security
} // namespace Platform
} // namespace Weave
} // namespace nl
