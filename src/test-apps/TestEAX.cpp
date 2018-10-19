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
 *      This file implements tests of the EAX implementation against
 *      known-answer test vectors from the Wycheproof project:
 *         https://github.com/google/wycheproof
 *
 */

#include <Weave/Core/WeaveConfig.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "ToolCommon.h"
#include <Weave/Support/crypto/EAX.h>

/*
 * Decode hexadecimal characters. The maximum size of the dst[] buffer is
 * dst_len; the actual length (in bytes) is returned.
 *
 * A failure (process exit) is reported in the following situations:
 *
 *  - Destination buffer is too small.
 *
 *  - One of the characters is not an hexadecimal digit (decimal digits,
 *    letters from 'A' to 'F', letters from 'a' to 'f') or an "ignored
 *    character" (ignored characters are all ASCII control characters
 *    from 0 to 31, ASCII space 0x20, ':' and '-').
 *
 *  - Source contains an odd number of hexadecimal digits.
 */
static size_t
hextobin(uint8_t *dst, size_t dst_len, const char *src)
{
    size_t u;
    int z;
    unsigned acc;

    u = 0;
    z = 0;
    acc = 0;
    while (*src != 0) {
        int c;

        c = *src ++;
        if (c >= '0' && c <= '9') {
            c -= '0';
        } else if (c >= 'A' && c <= 'F') {
            c -= ('A' - 10);
        } else if (c >= 'a' && c <= 'f') {
            c -= ('a' - 10);
        } else if (c <= 32 || c == ':' || c == '-') {
            continue;
        } else {
            fprintf(stderr, "not an hex character: 0x%02X\n", c);
            exit(EXIT_FAILURE);
        }
        if (z) {
            if (u >= dst_len) {
                fprintf(stderr, "destination too small\n");
                exit(EXIT_FAILURE);
            }
            dst[u ++] = (acc << 4) + c;
        } else {
            acc = c;
        }
        z = !z;
    }
    if (z) {
        fprintf(stderr, "odd number of hex digits\n");
        exit(EXIT_FAILURE);
    }
    return u;
}

/*
 * Check that b1[] and b2[], of common length 'len', have identical
 * contents. If not, then an error message is printed, that contains
 * the provided banner string, and hex dumps of the two buffers; then
 * the process exits.
 */
static void
check_eq(const char *banner, const void *b1, const void *b2, size_t len)
{
    size_t u;
    const uint8_t *a1, *a2;

    if (memcmp(b1, b2, len) == 0) {
        return;
    }
    fprintf(stderr, "%s: DIFF:\n", banner);
    a1 = (const uint8_t *)b1;
    a2 = (const uint8_t *)b2;
    for (u = 0; u < len; u ++) {
        fprintf(stderr, "%02x", a1[u]);
    }
    fprintf(stderr, "\n");
    for (u = 0; u < len; u ++) {
        fprintf(stderr, "%02x", a2[u]);
    }
    fprintf(stderr, "\n");
    exit(EXIT_FAILURE);
}

/*
 * Inner test function; the EAX engine has already been initialized with
 * a key. Elements (nonce, AAD, plaintext, ciphertext, and tag) are
 * provided as hexadecimal strings. If 'valid' is true, then the provided
 * tag is supposed to match the recomputed tag; otherwise, it is expected
 * _not_ to match.
 */
static void
test_EAX_inner(nl::Weave::Platform::Security::EAX *eax,
    const char *hexnonce, const char *hexaad, const char *hexplain,
    const char *hexcipher, const char *hextag, bool valid)
{
    uint8_t nonce[200];
    uint8_t aad[200];
    uint8_t plain[200];
    uint8_t cipher[200];
    uint8_t tag[16];
    uint8_t tmp[200], tmptag[16];
    size_t nonce_len, aad_len, plain_len, cipher_len, tag_len;
    nl::Weave::Platform::Security::EAXSaved sav;
#if !WEAVE_CONFIG_EAX_NO_CHUNK
    size_t u;
#endif

    /*
     * Decode elements into bytes. Plaintext and ciphertext are supposed
     * to have the same length.
     */
    nonce_len = hextobin(nonce, sizeof nonce, hexnonce);
    aad_len = hextobin(aad, sizeof aad, hexaad);
    plain_len = hextobin(plain, sizeof plain, hexplain);
    cipher_len = hextobin(cipher, sizeof cipher, hexcipher);
    tag_len = hextobin(tag, sizeof tag, hextag);
    if (plain_len != cipher_len) {
        fprintf(stderr, "inconsistent: plain_len=%zu cipher_len=%zu\n",
            plain_len, cipher_len);
        exit(EXIT_FAILURE);
    }

    /*
     * Try encryption, and verify the tag against the reference value.
     */
    eax->Start(nonce, nonce_len);
    eax->InjectHeader(aad, aad_len);
    eax->Encrypt(plain, plain_len, tmp);
    check_eq("KAT 1", cipher, tmp, cipher_len);
    eax->GetTag(tmptag, tag_len);
    if (valid) {
        check_eq("KAT 2", tag, tmptag, tag_len);
    } else {
        if (memcmp(tag, tmptag, tag_len) == 0) {
            fprintf(stderr, "KAT 2: should have obtained"
                " a different tag\n");
            exit(EXIT_FAILURE);
        }
    }

    printf(".");
    fflush(stdout);

    /*
     * Try decryption, and check the tag against the reference value.
     * This one uses CheckTag() instead of GetTag().
     */
    eax->Start(nonce, nonce_len);
    eax->InjectHeader(aad, aad_len);
    eax->Decrypt(cipher, cipher_len, tmp);
    check_eq("KAT 3", plain, tmp, plain_len);
    if (valid != eax->CheckTag(tag, tag_len)) {
        fprintf(stderr, "wrong validation status\n");
        exit(EXIT_FAILURE);
    }

    printf(".");
    fflush(stdout);

    if (!valid) {
        return;
    }

    /*
     * Try encryption again. This time:
     *
     *  - If chunked processing is supported, inject AAD and plaintext
     *    one byte at a time. If the AAD and/or the plaintext is empty,
     *    then the corresponding method is not called at all.
     *
     *  - If chunked processing is not supported, AAD and plaintext are
     *    injected with a single call each. However, if the AAD and/or
     *    the plaintext is empty, the method is skipped.
     */
    eax->Start(nonce, nonce_len);
#if WEAVE_CONFIG_EAX_NO_CHUNK
    if (aad_len > 0) {
        eax->InjectHeader(aad, aad_len);
    }
    if (plain_len > 0) {
        eax->Encrypt(plain, plain_len, tmp);
    }
#else
    for (u = 0; u < aad_len; u ++) {
        eax->InjectHeader(&aad[u], 1);
    }
    for (u = 0; u < plain_len; u ++) {
        eax->Encrypt(&plain[u], 1, &tmp[u]);
    }
#endif
    check_eq("KAT 4", cipher, tmp, cipher_len);
    eax->GetTag(tmptag, tag_len);
    check_eq("KAT 5", tag, tmptag, tag_len);

    printf(".");
    fflush(stdout);

    /*
     * Try decryption again. This time:
     *
     *  - If chunked processing is supported, inject AAD and plaintext
     *    one byte at a time. If the AAD and/or the plaintext is empty,
     *    then the corresponding method is not called at all.
     *
     *  - If chunked processing is not supported, AAD and plaintext are
     *    injected with a single call each. However, if the AAD and/or
     *    the plaintext is empty, the method is skipped.
     */
    eax->Start(nonce, nonce_len);
#if WEAVE_CONFIG_EAX_NO_CHUNK
    if (aad_len > 0) {
        eax->InjectHeader(aad, aad_len);
    }
    if (plain_len > 0) {
        eax->Decrypt(cipher, cipher_len, tmp);
    }
#else
    for (u = 0; u < aad_len; u ++) {
        eax->InjectHeader(&aad[u], 1);
    }
    for (u = 0; u < plain_len; u ++) {
        eax->Decrypt(&cipher[u], 1, &tmp[u]);
    }
#endif
    check_eq("KAT 6", plain, tmp, plain_len);
    eax->GetTag(tmptag, tag_len);
    check_eq("KAT 7", tag, tmptag, tag_len);

    printf(".");
    fflush(stdout);

    /*
     * Save the header, then use it for encryption.
     */
    eax->SaveHeader(aad, aad_len, &sav);
    eax->StartSaved(nonce, nonce_len, &sav);
    eax->Encrypt(plain, plain_len, tmp);
    check_eq("KAT 8", cipher, tmp, cipher_len);
    eax->GetTag(tmptag, tag_len);
    check_eq("KAT 9", tag, tmptag, tag_len);

    printf(".");
    fflush(stdout);

    /*
     * Also use the saved header for decryption. We also verify that
     * we can get the tag several times.
     */
    eax->StartSaved(nonce, nonce_len, &sav);
    eax->Decrypt(tmp, cipher_len);
    check_eq("KAT 10", plain, tmp, plain_len);
    eax->GetTag(tmptag, tag_len);
    check_eq("KAT 11", tag, tmptag, tag_len);
    eax->GetTag(tmptag, tag_len);
    check_eq("KAT 12", tag, tmptag, tag_len);

    printf(".");
    fflush(stdout);
}

/*
 * Outer test function. The test name is provided; it will be printed.
 * Elements (key, nonce, AAD, plaintext, ciphertext, and tag) are
 * provided as hexadecimal strings. If 'valid' is true, then the provided
 * tag is supposed to match the recomputed tag; otherwise, it is expected
 * _not_ to match.
 */
static void
test_EAX(const char *name,
    const char *hexkey, const char *hexnonce, const char *hexaad,
    const char *hexplain, const char *hexcipher, const char *hextag,
    bool valid)
{
    unsigned char key[32];
    size_t key_len;

    printf("Test %s: ", name);
    fflush(stdout);
    key_len = hextobin(key, sizeof key, hexkey);
    if (key_len == 16) {
        nl::Weave::Platform::Security::EAX128 eax;

        eax.SetKey(key);
        test_EAX_inner(&eax, hexnonce, hexaad, hexplain,
            hexcipher, hextag, valid);
    } else if (key_len == 32) {
        nl::Weave::Platform::Security::EAX256 eax;

        eax.SetKey(key);
        test_EAX_inner(&eax, hexnonce, hexaad, hexplain,
            hexcipher, hextag, valid);
    } else {
        fprintf(stderr, "unsupported key length: %zu bytes\n", key_len);
        exit(EXIT_FAILURE);
    }
    printf("OK\n");
    fflush(stdout);
}

int
main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    /*
     * Following test vectors are from the Wycheproof project. Source file is:
     *  https://github.com/google/wycheproof/blob/master/testvectors/aes_eax_test.json
     * (retrieved on 2018-10-17.)
     *
     * Test vectors with a key size of 192 bits have not been retained,
     * since we only support keys of 128 and 256 bits.
     */

    /* ivSize=128 keySize=128 */
    test_EAX("wycheproof-1",
        "233952dee4d5ed5f9b9c6d6ff80ff478",
        "62ec67f9c3a4a407fcb2a8c49031a8b3",
        "6bfb914fd07eae6b",
        "",
        "",
        "e037830e8389f27b025a2d6527e79d01",
        true);
    test_EAX("wycheproof-2",
        "91945d3f4dcbee0bf45ef52255f095a4",
        "becaf043b0a23d843194ba972c66debd",
        "fa3bfd4806eb53fa",
        "f7fb",
        "19dd",
        "5c4c9331049d0bdab0277408f67967e5",
        true);
    test_EAX("wycheproof-3",
        "01f74ad64077f2e704c0f60ada3dd523",
        "70c3db4f0d26368400a10ed05d2bff5e",
        "234a3463c1264ac6",
        "1a47cb4933",
        "d851d5bae0",
        "3a59f238a23e39199dc9266626c40f80",
        true);
    test_EAX("wycheproof-4",
        "d07cf6cbb7f313bdde66b727afd3c5e8",
        "8408dfff3c1a2b1292dc199e46b7d617",
        "33cce2eabff5a79d",
        "481c9e39b1",
        "632a9d131a",
        "d4c168a4225d8e1ff755939974a7bede",
        true);
    test_EAX("wycheproof-5",
        "35b6d0580005bbc12b0587124557d2c2",
        "fdb6b06676eedc5c61d74276e1f8e816",
        "aeb96eaebe2970e9",
        "40d0c07da5e4",
        "071dfe16c675",
        "cb0677e536f73afe6a14b74ee49844dd",
        true);
    test_EAX("wycheproof-6",
        "bd8e6e11475e60b268784c38c62feb22",
        "6eac5c93072d8e8513f750935e46da1b",
        "d4482d1ca78dce0f",
        "4de3b35c3fc039245bd1fb7d",
        "835bb4f15d743e350e728414",
        "abb8644fd6ccb86947c5e10590210a4f",
        true);
    test_EAX("wycheproof-7",
        "7c77d6e813bed5ac98baa417477a2e7d",
        "1a8c98dcd73d38393b2bf1569deefc19",
        "65d2017990d62528",
        "8b0a79306c9ce7ed99dae4f87f8dd61636",
        "02083e3979da014812f59f11d52630da30",
        "137327d10649b0aa6e1c181db617d7f2",
        true);
    test_EAX("wycheproof-8",
        "5fff20cafab119ca2fc73549e20f5b0d",
        "dde59b97d722156d4d9aff2bc7559826",
        "54b9f04e6a09189a",
        "1bda122bce8a8dbaf1877d962b8592dd2d56",
        "2ec47b2c4954a489afc7ba4897edcdae8cc3",
        "3b60450599bd02c96382902aef7f832a",
        true);
    test_EAX("wycheproof-9",
        "a4a4782bcffd3ec5e7ef6d8c34a56123",
        "b781fcf2f75fa5a8de97a9ca48e522ec",
        "899a175897561d7e",
        "6cf36720872b8513f6eab1a8a44438d5ef11",
        "0de18fd0fdd91e7af19f1d8ee8733938b1e8",
        "e7f6d2231618102fdb7fe55ff1991700",
        true);
    test_EAX("wycheproof-10",
        "8395fcf1e95bebd697bd010bc766aac3",
        "22e7add93cfc6393c57ec0b3c17d6b44",
        "126735fcc320d25a",
        "ca40d7446e545ffaed3bd12a740a659ffbbb3ceab7",
        "cb8920f87a6c75cff39627b56e3ed197c552d295a7",
        "cfc46afc253b4652b1af3795b124ab6e",
        true);
    test_EAX("wycheproof-11",
        "000102030405060708090a0b0c0d0e0f",
        "3c8cc2970a008f75cc5beae2847258c2",
        "",
        "0000000000000000000000000000000011111111111111111111111111111111",
        "3c441f32ce07822364d7a2990e50bb13d7b02a26969e4a937e5e9073b0d9c968",
        "db90bdb3da3d00afd0fc6a83551da95e",
        true);
    test_EAX("wycheproof-12",
        "000102030405060708090a0b0c0d0e0f",
        "aef03d00598494e9fb03cd7d8b590866",
        "",
        "0000000000000000000000000000000011111111111111111111111111111111",
        "d19ac59849026a91aa1b9aec29b11a202a4d739fd86c28e3ae3d588ea21d70c6",
        "c30f6cd9202074ed6e2a2a360eac8c47",
        true);
    test_EAX("wycheproof-13",
        "000102030405060708090a0b0c0d0e0f",
        "55d12511c696a80d0514d1ffba49cada",
        "",
        "0000000000000000000000000000000011111111111111111111111111111111",
        "2108558ac4b2c2d5cc66cea51d6210e046177a67631cd2dd8f09469733acb517",
        "fc355e87a267be3ae3e44c0bf3f99b2b",
        true);
    test_EAX("wycheproof-14",
        "000102030405060708090a0b0c0d0e0f",
        "79422ddd91c4eee2deaef1f968305304",
        "",
        "0000000000000000000000000000000011111111111111111111111111111111",
        "4d2c1524ca4baa4eefcce6b91b227ee83abaff8105dcafa2ab191f5df2575035",
        "e2c865ce2d7abdac024c6f991a848390",
        true);
    test_EAX("wycheproof-15",
        "000102030405060708090a0b0c0d0e0f",
        "0af5aa7a7676e28306306bcd9bf2003a",
        "",
        "0000000000000000000000000000000011111111111111111111111111111111",
        "8eb01e62185d782eb9287a341a6862ac5257d6f9adc99ee0a24d9c22b3e9b38a",
        "39c339bc8a74c75e2c65c6119544d61e",
        true);
    test_EAX("wycheproof-16",
        "000102030405060708090a0b0c0d0e0f",
        "af5a03ae7edd73471bdcdfac5e194a60",
        "",
        "0000000000000000000000000000000011111111111111111111111111111111",
        "94c5d2aca6dbbce8c24513a25e095c0e54a942860d327a222a815cc713b163b4",
        "f50b30304e45c9d411e8df4508a98612",
        true);
    test_EAX("wycheproof-17",
        "000102030405060708090a0b0c0d0e0f",
        "b37087680f0edd5a52228b8c7aaea664",
        "",
        "00000000000000000000000000000000111111111111111111111111111111112222222222222222222222222222222233333333333333333333333333333333",
        "3bb6173e3772d4b62eef37f9ef0781f360b6c74be3bf6b371067bc1b090d9d6622a1fbec6ac471b3349cd4277a101d40890fbf27dfdcd0b4e3781f9806daabb6",
        "a0498745e59999ddc32d5b140241124e",
        true);
    test_EAX("wycheproof-18",
        "000102030405060708090a0b0c0d0e0f",
        "4f802da62a384555a19bc2b382eb25af",
        "",
        "0000000000000000000000000000000011111111111111111111111111111111222222222222222222222222222222223333333333333333333333333333333344444444444444444444444444444444",
        "e9b0bb8857818ce3201c3690d21daa7f264fb8ee93cc7a4674ea2fc32bf182fb2a7e8ad51507ad4f31cefc2356fe7936a7f6e19f95e88fdbf17620916d3a6f3d01fc17d358672f777fd4099246e436e1",
        "67910be744b8315ae0eb6124590c5d8b",
        true);
    test_EAX("wycheproof-19",
        "b67b1a6efdd40d37080fbe8f8047aeb9",
        "fa294b129972f7fc5bbd5b96bba837c9",
        "",
        "",
        "",
        "b14b64fb589899699570cc9160e39896",
        true);
    test_EAX("wycheproof-20",
        "209e6dbf2ad26a105445fc0207cd9e9a",
        "9477849d6ccdfca112d92e53fae4a7ca",
        "",
        "01",
        "1d",
        "52a5f600fe5338026a7cb09c11640082",
        true);
    test_EAX("wycheproof-21",
        "a549442e35154032d07c8666006aa6a2",
        "5171524568e81d97e8c4de4ba56c10a0",
        "",
        "1182e93596cac5608946400bc73f3a",
        "d7b8a6b43d2e9f98c2b44ce5e3cfdb",
        "1bdd52fc987daf0ee19234c905ea645f",
        true);
    test_EAX("wycheproof-22",
        "958bcdb66a3952b53701582a68a0e474",
        "0e6ec879b02c6f516976e35898428da7",
        "",
        "140415823ecc8932a058384b738ea6ea6d4dfe3bbeee",
        "73e5c6f0e703a52d02f7f7faeb1b77fd4fd0cb421eaf",
        "6c154a85968edd74776575a4450bd897",
        true);
    test_EAX("wycheproof-23",
        "965b757ba5018a8d66edc78e0ceee86b",
        "2e35901ae7d491eecc8838fedd631405",
        "df10d0d212242450",
        "36e57a763958b02cea9d6a676ebce81f",
        "936b69b6c955adfd15539b9be4989cb6",
        "ee15a1454e88faad8e48a8df2983b425",
        true);
    test_EAX("wycheproof-24",
        "88d02033781c7b4164711a05420f256e",
        "7f2985296315507aa4c0a93d5c12bd77",
        "7c571d2fbb5f62523c0eb338bef9a9",
        "d98adc03d9d582732eb07df23d7b9f74",
        "67caac35443a3138d2cb811f0ce04dd2",
        "b7968e0b5640e3b236569653208b9deb",
        true);
    test_EAX("wycheproof-25",
        "515840cf67d2e40eb65e54a24c72cbf2",
        "bf47afdfd492137a24236bc36797a88e",
        "16843c091d43b0a191d0c73d15601be9",
        "c834588cb6daf9f06dd23519f4be9f56",
        "200ac451fbeb0f6151d61583a43b7343",
        "2ad43e4caa51983a9d4d24481bf4c839",
        true);
    test_EAX("wycheproof-26",
        "2e4492d444e5b6f4cec8c2d3615ac858",
        "d02bf0763a9fefbf70c33aee1e9da1d6",
        "904d86f133cec15a0c3caf14d7e029c82a07705a23f0d080",
        "9e62d6511b0bda7dd7740b614d97bae0",
        "27c6e9a653c5253ca1c5673f97b9b33e",
        "2d581271e1fa9e3686136caa8f4d6c8e",
        true);
    test_EAX("wycheproof-27",
        "000102030405060708090a0b0c0d0e0f",
        "505152535455565758595a5b5c5d5e5f",
        "",
        "202122232425262728292a2b2c2d2e2f",
        "29a0914fec4bef54babf6613a9f9cd70",
        "e70e7c5013a6dbf25298b1929bc356a7",
        false);
    test_EAX("wycheproof-28",
        "000102030405060708090a0b0c0d0e0f",
        "505152535455565758595a5b5c5d5e5f",
        "",
        "202122232425262728292a2b2c2d2e2f",
        "29a0914fec4bef54babf6613a9f9cd70",
        "e40e7c5013a6dbf25298b1929bc356a7",
        false);
    test_EAX("wycheproof-29",
        "000102030405060708090a0b0c0d0e0f",
        "505152535455565758595a5b5c5d5e5f",
        "",
        "202122232425262728292a2b2c2d2e2f",
        "29a0914fec4bef54babf6613a9f9cd70",
        "660e7c5013a6dbf25298b1929bc356a7",
        false);
    test_EAX("wycheproof-30",
        "000102030405060708090a0b0c0d0e0f",
        "505152535455565758595a5b5c5d5e5f",
        "",
        "202122232425262728292a2b2c2d2e2f",
        "29a0914fec4bef54babf6613a9f9cd70",
        "e60f7c5013a6dbf25298b1929bc356a7",
        false);
    test_EAX("wycheproof-31",
        "000102030405060708090a0b0c0d0e0f",
        "505152535455565758595a5b5c5d5e5f",
        "",
        "202122232425262728292a2b2c2d2e2f",
        "29a0914fec4bef54babf6613a9f9cd70",
        "e60e7cd013a6dbf25298b1929bc356a7",
        false);
    test_EAX("wycheproof-32",
        "000102030405060708090a0b0c0d0e0f",
        "505152535455565758595a5b5c5d5e5f",
        "",
        "202122232425262728292a2b2c2d2e2f",
        "29a0914fec4bef54babf6613a9f9cd70",
        "e60e7c5012a6dbf25298b1929bc356a7",
        false);
    test_EAX("wycheproof-33",
        "000102030405060708090a0b0c0d0e0f",
        "505152535455565758595a5b5c5d5e5f",
        "",
        "202122232425262728292a2b2c2d2e2f",
        "29a0914fec4bef54babf6613a9f9cd70",
        "e60e7c5011a6dbf25298b1929bc356a7",
        false);
    test_EAX("wycheproof-34",
        "000102030405060708090a0b0c0d0e0f",
        "505152535455565758595a5b5c5d5e5f",
        "",
        "202122232425262728292a2b2c2d2e2f",
        "29a0914fec4bef54babf6613a9f9cd70",
        "e60e7c5013a6db725298b1929bc356a7",
        false);
    test_EAX("wycheproof-35",
        "000102030405060708090a0b0c0d0e0f",
        "505152535455565758595a5b5c5d5e5f",
        "",
        "202122232425262728292a2b2c2d2e2f",
        "29a0914fec4bef54babf6613a9f9cd70",
        "e60e7c5013a6dbf25398b1929bc356a7",
        false);
    test_EAX("wycheproof-36",
        "000102030405060708090a0b0c0d0e0f",
        "505152535455565758595a5b5c5d5e5f",
        "",
        "202122232425262728292a2b2c2d2e2f",
        "29a0914fec4bef54babf6613a9f9cd70",
        "e60e7c5013a6dbf2d298b1929bc356a7",
        false);
    test_EAX("wycheproof-37",
        "000102030405060708090a0b0c0d0e0f",
        "505152535455565758595a5b5c5d5e5f",
        "",
        "202122232425262728292a2b2c2d2e2f",
        "29a0914fec4bef54babf6613a9f9cd70",
        "e60e7c5013a6dbf252b8b1929bc356a7",
        false);
    test_EAX("wycheproof-38",
        "000102030405060708090a0b0c0d0e0f",
        "505152535455565758595a5b5c5d5e5f",
        "",
        "202122232425262728292a2b2c2d2e2f",
        "29a0914fec4bef54babf6613a9f9cd70",
        "e60e7c5013a6dbf25298b0929bc356a7",
        false);
    test_EAX("wycheproof-39",
        "000102030405060708090a0b0c0d0e0f",
        "505152535455565758595a5b5c5d5e5f",
        "",
        "202122232425262728292a2b2c2d2e2f",
        "29a0914fec4bef54babf6613a9f9cd70",
        "e60e7c5013a6dbf25298b1929ac356a7",
        false);
    test_EAX("wycheproof-40",
        "000102030405060708090a0b0c0d0e0f",
        "505152535455565758595a5b5c5d5e5f",
        "",
        "202122232425262728292a2b2c2d2e2f",
        "29a0914fec4bef54babf6613a9f9cd70",
        "e60e7c5013a6dbf25298b19299c356a7",
        false);
    test_EAX("wycheproof-41",
        "000102030405060708090a0b0c0d0e0f",
        "505152535455565758595a5b5c5d5e5f",
        "",
        "202122232425262728292a2b2c2d2e2f",
        "29a0914fec4bef54babf6613a9f9cd70",
        "e60e7c5013a6dbf25298b1921bc356a7",
        false);
    test_EAX("wycheproof-42",
        "000102030405060708090a0b0c0d0e0f",
        "505152535455565758595a5b5c5d5e5f",
        "",
        "202122232425262728292a2b2c2d2e2f",
        "29a0914fec4bef54babf6613a9f9cd70",
        "e60e7c5013a6dbf25298b1929bc356a6",
        false);
    test_EAX("wycheproof-43",
        "000102030405060708090a0b0c0d0e0f",
        "505152535455565758595a5b5c5d5e5f",
        "",
        "202122232425262728292a2b2c2d2e2f",
        "29a0914fec4bef54babf6613a9f9cd70",
        "e60e7c5013a6dbf25298b1929bc356a5",
        false);
    test_EAX("wycheproof-44",
        "000102030405060708090a0b0c0d0e0f",
        "505152535455565758595a5b5c5d5e5f",
        "",
        "202122232425262728292a2b2c2d2e2f",
        "29a0914fec4bef54babf6613a9f9cd70",
        "e60e7c5013a6dbf25298b1929bc356e7",
        false);
    test_EAX("wycheproof-45",
        "000102030405060708090a0b0c0d0e0f",
        "505152535455565758595a5b5c5d5e5f",
        "",
        "202122232425262728292a2b2c2d2e2f",
        "29a0914fec4bef54babf6613a9f9cd70",
        "e60e7c5013a6dbf25298b1929bc35627",
        false);
    test_EAX("wycheproof-46",
        "000102030405060708090a0b0c0d0e0f",
        "505152535455565758595a5b5c5d5e5f",
        "",
        "202122232425262728292a2b2c2d2e2f",
        "29a0914fec4bef54babf6613a9f9cd70",
        "e70e7c5013a6dbf25398b1929bc356a7",
        false);
    test_EAX("wycheproof-47",
        "000102030405060708090a0b0c0d0e0f",
        "505152535455565758595a5b5c5d5e5f",
        "",
        "202122232425262728292a2b2c2d2e2f",
        "29a0914fec4bef54babf6613a9f9cd70",
        "e60e7cd013a6db725298b1929bc356a7",
        false);
    test_EAX("wycheproof-48",
        "000102030405060708090a0b0c0d0e0f",
        "505152535455565758595a5b5c5d5e5f",
        "",
        "202122232425262728292a2b2c2d2e2f",
        "29a0914fec4bef54babf6613a9f9cd70",
        "e60e7c5013a6db725298b1929bc35627",
        false);
    test_EAX("wycheproof-49",
        "000102030405060708090a0b0c0d0e0f",
        "505152535455565758595a5b5c5d5e5f",
        "",
        "202122232425262728292a2b2c2d2e2f",
        "29a0914fec4bef54babf6613a9f9cd70",
        "19f183afec59240dad674e6d643ca958",
        false);
    test_EAX("wycheproof-50",
        "000102030405060708090a0b0c0d0e0f",
        "505152535455565758595a5b5c5d5e5f",
        "",
        "202122232425262728292a2b2c2d2e2f",
        "29a0914fec4bef54babf6613a9f9cd70",
        "00000000000000000000000000000000",
        false);
    test_EAX("wycheproof-51",
        "000102030405060708090a0b0c0d0e0f",
        "505152535455565758595a5b5c5d5e5f",
        "",
        "202122232425262728292a2b2c2d2e2f",
        "29a0914fec4bef54babf6613a9f9cd70",
        "ffffffffffffffffffffffffffffffff",
        false);
    test_EAX("wycheproof-52",
        "000102030405060708090a0b0c0d0e0f",
        "505152535455565758595a5b5c5d5e5f",
        "",
        "202122232425262728292a2b2c2d2e2f",
        "29a0914fec4bef54babf6613a9f9cd70",
        "668efcd093265b72d21831121b43d627",
        false);
    test_EAX("wycheproof-53",
        "000102030405060708090a0b0c0d0e0f",
        "505152535455565758595a5b5c5d5e5f",
        "",
        "202122232425262728292a2b2c2d2e2f",
        "29a0914fec4bef54babf6613a9f9cd70",
        "e70f7d5112a7daf35399b0939ac257a6",
        false);

    /* ivSize=96 keySize=128 */
    test_EAX("wycheproof-54",
        "bedcfb5a011ebc84600fcb296c15af0d",
        "438a547a94ea88dce46c6c85",
        "",
        "",
        "",
        "9607977cd7556b1dfedf0c73a35a5197",
        true);
    test_EAX("wycheproof-55",
        "384ea416ac3c2f51a76e7d8226346d4e",
        "b30c084727ad1c592ac21d12",
        "",
        "35",
        "98",
        "f5d7930952e275beecb998d804c241f0",
        true);
    test_EAX("wycheproof-56",
        "cae31cd9f55526eb038241fc44cac1e5",
        "b5e006ded553110e6dc56529",
        "",
        "d10989f2c52e94ad",
        "7fd2878318ab0f2b",
        "ab184ffde523565529a9be111b0c2d6d",
        true);
    test_EAX("wycheproof-57",
        "ffdf4228361ea1f8165852136b3480f7",
        "0e1666f2dc652f7708fb8f0d",
        "",
        "25b12e28ac0ef6ead0226a3b2288c800",
        "e928622d1e6e798d8665ae732c4c1e5f",
        "33ab476757ffa42c0f6c276391a46eac",
        true);
    test_EAX("wycheproof-58",
        "a8ee11b26d7ceb7f17eaa1e4b83a2cf6",
        "fbbc04fd6e025b7193eb57f6",
        "",
        "c08f085e6a9e0ef3636280c11ecfadf0c1e72919ffc17eaf",
        "efd299a43b25ce8cc31b80e5489ef9ce7356ececa91bc7bd",
        "3c33fc0bcd256b0a8a34ecc8b01e52a6",
        true);
    test_EAX("wycheproof-59",
        "1655bf662f7ee685615701fd3779d628",
        "42b51388f6f9047a2a994575",
        "",
        "857b2f6cd608c9cea0246c740caa4ca19c5f1c7d71cb9273f0d8c8bb65b70a",
        "356bca9cddd39efd393278e43b4e80266071608036e81d6e924d4e4800fb27",
        "71f02ba7c6cf3a579e56245025420071",
        true);
    test_EAX("wycheproof-60",
        "42e38abef2dd7573248c5aefb3ecca54",
        "064b3cfbe04d94d4d5c19b30",
        "",
        "2c763b9ec84903bcbb8aec15e678a3a955e4870edbf62d9d3c81c4f9ed6154877875779ca33cce8f73a55ca7af1d8d817fc6baac00ef962c5a0da339ce81427a3d59",
        "9d911b934a68ce7db322410028bd31bd81bcbdadf26f15676be472bc3821fb68e4728db76930bc0958aeed6faf3e333da7af3d48c480b424ff3d6600cc56a507c8ad",
        "d679eb9e5d744b62d91dcf6fb6284f41",
        true);

    /* ivSize=96 keySize=256 */
    test_EAX("wycheproof-68",
        "80ba3192c803ce965ea371d5ff073cf0f43b6a2ab576b208426e11409c09b9b0",
        "4da5bf8dfd5852c1ea12379d",
        "",
        "",
        "",
        "4d293af9a8fe4ac034f14b14334c16ae",
        true);
    test_EAX("wycheproof-69",
        "cc56b680552eb75008f5484b4cb803fa5063ebd6eab91f6ab6aef4916a766273",
        "99e23ec48985bccdeeab60f1",
        "",
        "2a",
        "8c",
        "c460d5ff45235c3c2491c7e6a32491d6",
        true);
    test_EAX("wycheproof-70",
        "51e4bf2bad92b7aff1a4bc05550ba81df4b96fabf41c12c7b00e60e48db7e152",
        "4f07afedfdc3b6c2361823d3",
        "",
        "be3308f72a2c6aed",
        "6038296421fb5007",
        "0a91c72219c0b9ad716accd910e04e13",
        true);
    test_EAX("wycheproof-71",
        "59d4eafb4de0cfc7d3db99a8f54b15d7b39f0acc8da69763b019c1699f87674a",
        "2fcb1b38a99e71b84740ad9b",
        "",
        "549b365af913f3b081131ccb6b825588",
        "c4066e265a948f40e05e37fa400fde1b",
        "611de27128955c54edd7a4d6d23e78ee",
        true);
    test_EAX("wycheproof-72",
        "0212a8de5007ed87b33f1a7090b6114f9e08cefd9607f2c276bdcfdbc5ce9cd7",
        "e6b1adf2fd58a8762c65f31b",
        "",
        "10f1ecf9c60584665d9ae5efe279e7f7377eea6916d2b111",
        "f64ffe52cd838cea89dd500662a2ee4b4b450eee68218e84",
        "ae1e2eda96bed82182240aae08f9fe9c",
        true);
    test_EAX("wycheproof-73",
        "2eb51c469aa8eb9e6c54a8349bae50a20f0e382711bba1152c424f03b6671d71",
        "04a9be03508a5f31371a6fd2",
        "",
        "b053999286a2824f42cc8c203ab24e2c97a685adcc2ad32662558e55a5c729",
        "01f09a6a136909c158e13502ee5488f592ee24059d6da734acba8c11e9815f",
        "79e57b518fa6dabe94e0e89cae89976b",
        true);
    test_EAX("wycheproof-74",
        "95e87eda64d0dc2d4e851030c3e1b27cca2265b3464c2c572bd8fc8cfb282d1b",
        "ce03bbb56778f25d4528350b",
        "",
        "2e5acc19acb9940bb74d414b45e71386a409b641490b139493d7d632cbf1674fdf2511c3fad6c27359e6137b4cd52efc4bf871e6623451517d6a3c68240f2a79916a",
        "72356ce9f1822e30809817a3b91ea13700ab3275b6f3718a845ad0b132bf4bbbb61ee466c1b0a1cb5a26424dbcc8d1b649f22785907a9c0164a2a41a9fc477d6c4dd",
        "872861d71412e15732f60a83d4b47ee1",
        true);

    /* ivSize=128 keySize=256 */
    test_EAX("wycheproof-110",
        "b4cd11db0b3e0b9b34eafd9fe027746976379155e76116afde1b96d21298e34f",
        "00c49f4ebb07393f07ebc3825f7b0830",
        "",
        "",
        "",
        "80d821cde2d6c523b718597b11dd0fa8",
        true);
    test_EAX("wycheproof-111",
        "b7797eb0c1a6089ad5452d81fdb14828c040ddc4589c32b565aad8cb4de3e4a0",
        "0ad570d8863918fe89124e09d125a271",
        "",
        "ed",
        "25",
        "4fef9ec45255dbba5631105d00a55767",
        true);
    test_EAX("wycheproof-112",
        "4c010d9561c7234c308c01cea3040c925a9f324dc958ff904ae39b37e60e1e03",
        "2a55caa137c5b0b66cf3809eb8f730c4",
        "",
        "2a093c9ed72b8ff4994201e9f9e010",
        "cbfcaa3634d6cff5656bc6bda6ab5f",
        "0144be0643b036a8147e19f4ea9e7af2",
        true);
    test_EAX("wycheproof-113",
        "2f6cfb7a215a7bafb607c273f7e66f9a6d51d57f9c29422ec64699bad0c6f33b",
        "21cbeff0b123799da74f4daff2e279c5",
        "",
        "39dbc71f6838ed6c6e582137436e1c61bbbfb80531f4",
        "f531097aa1bb35d9f401d459340afbd27f9bdf72c537",
        "e4e18170dce4e1af90b15eae64355331",
        true);
    test_EAX("wycheproof-114",
        "7517c973a9de3614431e3198f4ddc0f8dc33862654649e9ff7838635bb278231",
        "42f82085c08afd5b19a9491a79cd8119",
        "e9ee894ad5b0781d",
        "d17fbed25ad5f72477580b9e82a7b883",
        "0b70b24253b2e1c3ef1165925b5c5e57",
        "45009a2a101877ed70e58f2e5910004f",
        true);
    test_EAX("wycheproof-115",
        "9f5c60fb5df5cf2b1b39254c3fa80e51d30d64e344b3aba59574305b4d2212ad",
        "d4df79c69f73b26a13598af07eed6a77",
        "813399ff1e1ef0b58bb2be130ce5d4",
        "a3ca2ef9bd1fdbaa83db4c7eae6de94e",
        "65019212ccbbd4cd2f995cc59d46fd27",
        "4026c486430a1ae2a5fc4081cd665468",
        true);
    test_EAX("wycheproof-116",
        "38f3d880ed6cd605f2eab88027c9a1c21d13e3de1af50ac884723bcf2b70f495",
        "7078c9239650b8a1a8cf031d460e51c1",
        "d1544013b885a7083abece9e31d98ebc",
        "52609620d7f572aa9267565e459ae419",
        "91b9f4424b68b4af839ce553d10b7dbc",
        "0541b1a518f4bb585a594f3eab5535c3",
        true);
    test_EAX("wycheproof-117",
        "ec88cec13d8ebae7d62f60197e5486d61c33ee5a50b19f197c1348fbc9e27e8e",
        "1ec1d18c96ca6cad66690e60b91cf222",
        "d28d5811d4168a08da54b97831b59200041adb0e2891ea91",
        "658c6c7d8ea64a48375d69d9a405095a",
        "e42b53912ce21a3ee7a1fb51194d6fe3",
        "2bc8cc7f42cac1a121fd9ddff4f2073c",
        true);
    test_EAX("wycheproof-118",
        "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f",
        "505152535455565758595a5b5c5d5e5f",
        "",
        "202122232425262728292a2b2c2d2e2f",
        "059e01599f94b38f2435b47a0c7b5c59",
        "e976fdd461c0a0a49971db8d9678acb8",
        false);
    test_EAX("wycheproof-119",
        "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f",
        "505152535455565758595a5b5c5d5e5f",
        "",
        "202122232425262728292a2b2c2d2e2f",
        "059e01599f94b38f2435b47a0c7b5c59",
        "ea76fdd461c0a0a49971db8d9678acb8",
        false);
    test_EAX("wycheproof-120",
        "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f",
        "505152535455565758595a5b5c5d5e5f",
        "",
        "202122232425262728292a2b2c2d2e2f",
        "059e01599f94b38f2435b47a0c7b5c59",
        "6876fdd461c0a0a49971db8d9678acb8",
        false);
    test_EAX("wycheproof-121",
        "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f",
        "505152535455565758595a5b5c5d5e5f",
        "",
        "202122232425262728292a2b2c2d2e2f",
        "059e01599f94b38f2435b47a0c7b5c59",
        "e877fdd461c0a0a49971db8d9678acb8",
        false);
    test_EAX("wycheproof-122",
        "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f",
        "505152535455565758595a5b5c5d5e5f",
        "",
        "202122232425262728292a2b2c2d2e2f",
        "059e01599f94b38f2435b47a0c7b5c59",
        "e876fd5461c0a0a49971db8d9678acb8",
        false);
    test_EAX("wycheproof-123",
        "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f",
        "505152535455565758595a5b5c5d5e5f",
        "",
        "202122232425262728292a2b2c2d2e2f",
        "059e01599f94b38f2435b47a0c7b5c59",
        "e876fdd460c0a0a49971db8d9678acb8",
        false);
    test_EAX("wycheproof-124",
        "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f",
        "505152535455565758595a5b5c5d5e5f",
        "",
        "202122232425262728292a2b2c2d2e2f",
        "059e01599f94b38f2435b47a0c7b5c59",
        "e876fdd463c0a0a49971db8d9678acb8",
        false);
    test_EAX("wycheproof-125",
        "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f",
        "505152535455565758595a5b5c5d5e5f",
        "",
        "202122232425262728292a2b2c2d2e2f",
        "059e01599f94b38f2435b47a0c7b5c59",
        "e876fdd461c0a0249971db8d9678acb8",
        false);
    test_EAX("wycheproof-126",
        "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f",
        "505152535455565758595a5b5c5d5e5f",
        "",
        "202122232425262728292a2b2c2d2e2f",
        "059e01599f94b38f2435b47a0c7b5c59",
        "e876fdd461c0a0a49871db8d9678acb8",
        false);
    test_EAX("wycheproof-127",
        "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f",
        "505152535455565758595a5b5c5d5e5f",
        "",
        "202122232425262728292a2b2c2d2e2f",
        "059e01599f94b38f2435b47a0c7b5c59",
        "e876fdd461c0a0a41971db8d9678acb8",
        false);
    test_EAX("wycheproof-128",
        "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f",
        "505152535455565758595a5b5c5d5e5f",
        "",
        "202122232425262728292a2b2c2d2e2f",
        "059e01599f94b38f2435b47a0c7b5c59",
        "e876fdd461c0a0a49951db8d9678acb8",
        false);
    test_EAX("wycheproof-129",
        "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f",
        "505152535455565758595a5b5c5d5e5f",
        "",
        "202122232425262728292a2b2c2d2e2f",
        "059e01599f94b38f2435b47a0c7b5c59",
        "e876fdd461c0a0a49971da8d9678acb8",
        false);
    test_EAX("wycheproof-130",
        "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f",
        "505152535455565758595a5b5c5d5e5f",
        "",
        "202122232425262728292a2b2c2d2e2f",
        "059e01599f94b38f2435b47a0c7b5c59",
        "e876fdd461c0a0a49971db8d9778acb8",
        false);
    test_EAX("wycheproof-131",
        "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f",
        "505152535455565758595a5b5c5d5e5f",
        "",
        "202122232425262728292a2b2c2d2e2f",
        "059e01599f94b38f2435b47a0c7b5c59",
        "e876fdd461c0a0a49971db8d9478acb8",
        false);
    test_EAX("wycheproof-132",
        "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f",
        "505152535455565758595a5b5c5d5e5f",
        "",
        "202122232425262728292a2b2c2d2e2f",
        "059e01599f94b38f2435b47a0c7b5c59",
        "e876fdd461c0a0a49971db8d1678acb8",
        false);
    test_EAX("wycheproof-133",
        "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f",
        "505152535455565758595a5b5c5d5e5f",
        "",
        "202122232425262728292a2b2c2d2e2f",
        "059e01599f94b38f2435b47a0c7b5c59",
        "e876fdd461c0a0a49971db8d9678acb9",
        false);
    test_EAX("wycheproof-134",
        "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f",
        "505152535455565758595a5b5c5d5e5f",
        "",
        "202122232425262728292a2b2c2d2e2f",
        "059e01599f94b38f2435b47a0c7b5c59",
        "e876fdd461c0a0a49971db8d9678acba",
        false);
    test_EAX("wycheproof-135",
        "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f",
        "505152535455565758595a5b5c5d5e5f",
        "",
        "202122232425262728292a2b2c2d2e2f",
        "059e01599f94b38f2435b47a0c7b5c59",
        "e876fdd461c0a0a49971db8d9678acf8",
        false);
    test_EAX("wycheproof-136",
        "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f",
        "505152535455565758595a5b5c5d5e5f",
        "",
        "202122232425262728292a2b2c2d2e2f",
        "059e01599f94b38f2435b47a0c7b5c59",
        "e876fdd461c0a0a49971db8d9678ac38",
        false);
    test_EAX("wycheproof-137",
        "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f",
        "505152535455565758595a5b5c5d5e5f",
        "",
        "202122232425262728292a2b2c2d2e2f",
        "059e01599f94b38f2435b47a0c7b5c59",
        "e976fdd461c0a0a49871db8d9678acb8",
        false);
    test_EAX("wycheproof-138",
        "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f",
        "505152535455565758595a5b5c5d5e5f",
        "",
        "202122232425262728292a2b2c2d2e2f",
        "059e01599f94b38f2435b47a0c7b5c59",
        "e876fd5461c0a0249971db8d9678acb8",
        false);
    test_EAX("wycheproof-139",
        "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f",
        "505152535455565758595a5b5c5d5e5f",
        "",
        "202122232425262728292a2b2c2d2e2f",
        "059e01599f94b38f2435b47a0c7b5c59",
        "e876fdd461c0a0249971db8d9678ac38",
        false);
    test_EAX("wycheproof-140",
        "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f",
        "505152535455565758595a5b5c5d5e5f",
        "",
        "202122232425262728292a2b2c2d2e2f",
        "059e01599f94b38f2435b47a0c7b5c59",
        "1789022b9e3f5f5b668e247269875347",
        false);
    test_EAX("wycheproof-141",
        "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f",
        "505152535455565758595a5b5c5d5e5f",
        "",
        "202122232425262728292a2b2c2d2e2f",
        "059e01599f94b38f2435b47a0c7b5c59",
        "00000000000000000000000000000000",
        false);
    test_EAX("wycheproof-142",
        "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f",
        "505152535455565758595a5b5c5d5e5f",
        "",
        "202122232425262728292a2b2c2d2e2f",
        "059e01599f94b38f2435b47a0c7b5c59",
        "ffffffffffffffffffffffffffffffff",
        false);
    test_EAX("wycheproof-143",
        "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f",
        "505152535455565758595a5b5c5d5e5f",
        "",
        "202122232425262728292a2b2c2d2e2f",
        "059e01599f94b38f2435b47a0c7b5c59",
        "68f67d54e140202419f15b0d16f82c38",
        false);
    test_EAX("wycheproof-144",
        "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f",
        "505152535455565758595a5b5c5d5e5f",
        "",
        "202122232425262728292a2b2c2d2e2f",
        "059e01599f94b38f2435b47a0c7b5c59",
        "e977fcd560c1a1a59870da8c9779adb9",
        false);

    /* ivSize=160 keySize=128 */
    test_EAX("wycheproof-145",
        "7edabee31897bf9b29394aeca84c4dcc",
        "ef4886c4fe8b26f045e09ac925ccbbad42d70347",
        "",
        "52583c7b11de051c2e5c2114ee20527b",
        "298e86436ead703a38f869690f020d4c",
        "f20d2f2d170ebbe1d0ec718eefe632e4",
        true);

    /* ivSize=256 keySize=128 */
    test_EAX("wycheproof-146",
        "e071a62bcde9ee648118ed3b1c629c20",
        "f23a924d75c57fee8e75defd97be48e8cf3202cd658add0a4f50b24b5af9f013",
        "",
        "a0650c4299cf63ec5e28104e9064247f",
        "487e94228d338acee8e9f5c07e22fb06",
        "72c99b644664378c88fd1f4ecfd80f76",
        true);

    /* ivSize=160 keySize=256 */
    test_EAX("wycheproof-149",
        "1739fd2876258457e3e4c323dbabd85edda8ecad83a7496d8feb0b88aeab2e74",
        "989f015e6ab79d5e43eca8364a38c9f6b381dda1",
        "",
        "d1b13ceacedad362851dc876d8b1dd20",
        "5cceb0253bcbd6800d3b316af3a56937",
        "15186910a0f2a2bc41d32e7fe687f17c",
        true);

    /* ivSize=256 keySize=256 */
    test_EAX("wycheproof-150",
        "aa5429fd3f178b3885f2c696975e88890102455b5d9e42766429e80d4889672a",
        "e1ed38af5753851b79175e4ae11fd6cf80033f81aec484ecd0448c5e7cc0a27e",
        "",
        "7aa8919ebb950f34690acb98651854cb",
        "1a288496f909036b35f3604b3ecd3493",
        "62eb49550cbee8c3cf88302c826690a2",
        true);

    /* ivSize=32 keySize=128 */
    test_EAX("wycheproof-151",
        "d83c1d7a97c43f182409a4aa5609c1b1",
        "7b5faeb2",
        "",
        "c8f07ba1d65554a9bd40390c30c5529c",
        "d324ca1530c68ed86c775ed9bb1d8490",
        "30062eb9cedbaddf36f93e4219620afa",
        true);

    /* ivSize=64 keySize=128 */
    test_EAX("wycheproof-152",
        "deb62233559b57476602b5adac57c77f",
        "d084547de55bbc15",
        "",
        "d8986df0241ed3297582c0c239c724cb",
        "3064cf4883703f170bf01e6c2d67259f",
        "09471c09f897d46216fbb52436e3c4fc",
        true);

    /* ivSize=32 keySize=256 */
    test_EAX("wycheproof-155",
        "093eb12343537ee8e91c1f715b862603f8daf9d4e1d7d67212a9d68e5aac9358",
        "5110604c",
        "",
        "33efb58c91e8c70271870ec00fe2e202",
        "5aca28621e2bd92d7f182ff653b1e8eb",
        "8a89a0db74a55f907f8ba115e2e15853",
        true);

    /* ivSize=64 keySize=256 */
    test_EAX("wycheproof-156",
        "115884f693b155563e9bfb3b07cacb2f7f7caa9bfe51f89e23feb5a9468bfdd0",
        "04102199ef21e1df",
        "",
        "82e3e604d2be8fcab74f638d1e70f24c",
        "df32c13a2278326a3c966dee321a42f6",
        "b1798b8e4b95df6c620a5cbcbe1238d1",
        true);

    /* ivSize=0 keySize=128 */
    test_EAX("wycheproof-157",
        "8f3f52e3c75c58f5cb261f518f4ad30a",
        "",
        "",
        "",
        "",
        "5adbeefc8fa9cae2b9a6db3f5f6c82e9",
        true);
    test_EAX("wycheproof-158",
        "2a4bf90e56b70fdd8649d775c089de3b",
        "",
        "",
        "324ced6cd15ecc5b3741541e22c18ad9",
        "73b4716f7e44f3bb22a2648069ebbc1e",
        "3f6ac9672db499324ead0c234b544054",
        true);

    /* ivSize=0 keySize=256 */
    test_EAX("wycheproof-161",
        "3f8ca47b9a940582644e8ecf9c2d44e8138377a8379c5c11aafe7fec19856cf1",
        "",
        "",
        "",
        "",
        "b17f6100882e6b419d9fed0c8b7c8d9a",
        true);
    test_EAX("wycheproof-162",
        "7660d10966c6503903a552dde2a809ede9da490e5e5cc3e349da999671809883",
        "",
        "",
        "c314235341debfafa1526bb61044a7f1",
        "8187621069d3c07b7861bb40e8a56b3a",
        "c1f0897558300e979ba29b36336a0d06",
        true);

    printf("All tests succeeded\n");
    return 0;
}
