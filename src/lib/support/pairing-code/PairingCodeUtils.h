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
 *      Utility functions for working with Nest pairing codes.
 *
 */

namespace nl {
namespace PairingCode {

enum
{
    /** Pairing code length for most Nest products. */
    kStandardPairingCodeLength = 6,

    /** Pairing code length for Kryptonite. */
    kKryptonitePairingCodeLength = 9,

    /** Minimum length of a pairing code. */
    kPairingCodeLenMin = 2, // 1 value character plus 1 check character

    /** Number of bits encoded in a single pairing code character. */
    kBitsPerCharacter = 5,
};

extern WEAVE_ERROR VerifyPairingCode(const char *pairingCode, size_t pairingCodeLen);
extern void NormalizePairingCode(char *pairingCode, size_t& pairingCodeLen);

extern WEAVE_ERROR IntToPairingCode(uint64_t val, uint8_t pairingCodeLen, char *outBuf);
extern WEAVE_ERROR PairingCodeToInt(const char *pairingCode, size_t pairingCodeLen, uint64_t& val);

extern WEAVE_ERROR NevisPairingCodeToDeviceId(const char *pairingCode, uint64_t& deviceId);
extern WEAVE_ERROR NevisDeviceIdToPairingCode(uint64_t deviceId, char *pairingCodeBuf, size_t pairingCodeBufSize);

extern WEAVE_ERROR KryptonitePairingCodeToDeviceId(const char *pairingCode, uint64_t& deviceId);
extern WEAVE_ERROR KryptoniteDeviceIdToPairingCode(uint64_t deviceId, char *pairingCodeBuf, size_t pairingCodeBufSize);

extern bool IsValidPairingCodeChar(char ch);
extern int PairingCodeCharToInt(char ch);
extern char IntToPairingCodeChar(int val);

} // namespace PairingCode
} // namespace nl
