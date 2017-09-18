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
#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif // __STDC_FORMAT_MACROS

#include <inttypes.h>

#include <Weave/Core/WeaveCore.h>
#include <Weave/Support/CodeUtils.h>
#include <Weave/Support/crypto/HashAlgos.h>
#include <Weave/Support/Base64.h>
#include <Weave/Profiles/security/WeaveProvHash.h>

namespace nl {
namespace Weave {
namespace Profiles {
namespace Security {

using namespace nl::Weave::Platform::Security;

/**
 * Generate a verification hash (in base-64 format) for a given set of Weave provisioning information.
 *
 * @param[in]      nodeId               The device's Weave node id.
 * @param[in]      weaveCert            A pointer to a buffer containing the Weave device certificate
 *                                      in base-64 format.
 * @param[in]      weaveCertLen         The length of the certificate value pointed at by weaveCert.
 * @param[in]      weavePrivKey         A pointer to a buffer containing the Weave device private key
 *                                      in base-64 format.
 * @param[in]      weavePrivKeyLen      The length of the private key value pointed at by weavePrivKey.
 * @param[in]      pairingCode          A pointer to a buffer containing the device pairing code.
 * @param[in]      weavePairingCodeLen  The length of the pairing code value pointed at by pairingCode.
 * @param[in,out]  hashBuf              A pointer to a buffer that will receive the verification hash
 *                                      value, in base-64 format.  The output string will be null
 *                                      terminated.  This buffer should be at least as big as
 *                                      kWeaveProvisioningHashLength + 1.
 * @param[in]      hashBufSize          The size in bytes of the buffer pointed at by hashBuf.
 *
 * @retval #WEAVE_NO_ERROR              If the method succeeded.
 * @retval #WEAVE_ERROR_INVALID_STRING_LENGTH
 *                                      If one of the input values is too long (> 65535).
 * @retval #WEAVE_ERROR_BUFFER_TOO_SMALL
 *                                      If the supplied buffer is too small to hold the generated hash
 *                                      value.
 */
NL_DLL_EXPORT WEAVE_ERROR MakeWeaveProvisioningHash(uint64_t nodeId,
                                                    const char *weaveCert, size_t weaveCertLen,
                                                    const char *weavePrivKey, size_t weavePrivKeyLen,
                                                    const char *pairingCode, size_t pairingCodeLen,
                                                    char *hashBuf, size_t hashBufSize)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    SHA256 sha256;
    char tmpBuf[21]; // 20 hex digits plus null terminator
    uint8_t tmpHashBuf[SHA256::kHashLength];
    size_t l;

    VerifyOrExit(hashBufSize >= kWeaveProvisioningHashLength + 1, err = WEAVE_ERROR_BUFFER_TOO_SMALL);
    VerifyOrExit(weaveCertLen <= 65535, err = WEAVE_ERROR_INVALID_STRING_LENGTH);
    VerifyOrExit(weavePrivKeyLen <= 65535, err = WEAVE_ERROR_INVALID_STRING_LENGTH);
    VerifyOrExit(pairingCodeLen <= 65535, err = WEAVE_ERROR_INVALID_STRING_LENGTH);

    // Begin hash.
    sha256.Begin();

    // Add length of node id (always 16 characters) and actual node id value in hex.
    l = snprintf(tmpBuf, sizeof(tmpBuf), "0010%016" PRIX64, nodeId);
    VerifyOrDie(l < sizeof(tmpBuf));
    sha256.AddData((const uint8_t *)tmpBuf, (uint16_t)l);

    // Add length of Weave certificate.
    l = snprintf(tmpBuf, sizeof(tmpBuf), "%04" PRIX32, (uint32_t)weaveCertLen);
    VerifyOrDie(l < sizeof(tmpBuf));
    sha256.AddData((const uint8_t *)tmpBuf, (uint16_t)l);

    // Add Weave certificate.
    sha256.AddData((const uint8_t *)weaveCert, (uint16_t)weaveCertLen);

    // Add length of Weave private key.
    l = snprintf(tmpBuf, sizeof(tmpBuf), "%04" PRIX32, (uint32_t)weavePrivKeyLen);
    VerifyOrDie(l < sizeof(tmpBuf));
    sha256.AddData((const uint8_t *)tmpBuf, (uint16_t)l);

    // Add Weave private key.
    sha256.AddData((const uint8_t *)weavePrivKey, (uint16_t)weavePrivKeyLen);

    // Add length of pairing code.
    l = snprintf(tmpBuf, sizeof(tmpBuf), "%04" PRIX32, (uint32_t)pairingCodeLen);
    VerifyOrDie(l < sizeof(tmpBuf));
    sha256.AddData((const uint8_t *)tmpBuf, (uint16_t)l);

    // Add pairing code.
    sha256.AddData((const uint8_t *)pairingCode, (uint16_t)pairingCodeLen);

    // Complete hash.
    sha256.Finish(tmpHashBuf);

    // Convert binary hash value to null terminated base-64 string.
    l = Base64Encode(tmpHashBuf, SHA256::kHashLength, hashBuf);
    hashBuf[l] = 0;

exit:
    return err;
}

/**
 * Generate a verification hash (in base-64 format) for a given set of Thermostat device credentials.
 *
 * @param[in]      serialNum            A pointer to a buffer containing the device's serial number.
 * @param[in]      serialNumLen         The length of the serial number string.
 * @param[in]      deviceId             A pointer to a buffer containing the device's id.
 * @param[in]      deviceIdLen          The length of the device's id.
 * @param[in]      deviceSecret         A pointer to a buffer containing the device's secret.
 * @param[in]      deviceSecretLen      The length of the device's secret.
 * @param[in,out]  hashBuf              A pointer to a buffer that will receive the verification hash
 *                                      value, in base-64 format.  The output string will be null
 *                                      terminated.  This buffer should be at least as big as
 *                                      kDeviceCredentialHashLength + 1.
 * @param[in]      hashBufSize          The size in bytes of the buffer pointed at by hashBuf.
 *
 * @retval #WEAVE_NO_ERROR              If the method succeeded.
 * @retval #WEAVE_ERROR_INVALID_STRING_LENGTH
 *                                      If one of the input values is too long (> 65535).
 * @retval #WEAVE_ERROR_BUFFER_TOO_SMALL
 *                                      If the supplied buffer is too small to hold the generated hash
 *                                      value.
 */
NL_DLL_EXPORT WEAVE_ERROR MakeDeviceCredentialHash(const char *serialNum, size_t serialNumLen,
                                                   const char *deviceId, size_t deviceIdLen,
                                                   const char *deviceSecret, size_t deviceSecretLen,
                                                   char *hashBuf, size_t hashBufSize)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    SHA256 sha256;
    char tmpBuf[5]; // 4 hex digits plus null terminator
    uint8_t tmpHashBuf[SHA256::kHashLength];
    size_t l;

    VerifyOrExit(hashBufSize >= kDeviceCredentialHashLength + 1, err = WEAVE_ERROR_BUFFER_TOO_SMALL);
    VerifyOrExit(serialNumLen <= 65535, err = WEAVE_ERROR_INVALID_STRING_LENGTH);
    VerifyOrExit(deviceIdLen <= 65535, err = WEAVE_ERROR_INVALID_STRING_LENGTH);
    VerifyOrExit(deviceSecretLen <= 65535, err = WEAVE_ERROR_INVALID_STRING_LENGTH);

    // Begin hash.
    sha256.Begin();

    // Add length of serial number.
    l = snprintf(tmpBuf, sizeof(tmpBuf), "%04" PRIX32, (uint32_t)serialNumLen);
    VerifyOrDie(l < sizeof(tmpBuf));
    sha256.AddData((const uint8_t *)tmpBuf, (uint16_t)l);

    // Add serial number.
    sha256.AddData((const uint8_t *)serialNum, (uint16_t)serialNumLen);

    // Add length of device id.
    l = snprintf(tmpBuf, sizeof(tmpBuf), "%04" PRIX32, (uint32_t)deviceIdLen);
    VerifyOrDie(l < sizeof(tmpBuf));
    sha256.AddData((const uint8_t *)tmpBuf, (uint16_t)l);

    // Add device id.
    sha256.AddData((const uint8_t *)deviceId, (uint16_t)deviceIdLen);

    // Add length of device secret.
    l = snprintf(tmpBuf, sizeof(tmpBuf), "%04" PRIX32, (uint32_t)deviceSecretLen);
    VerifyOrDie(l < sizeof(tmpBuf));
    sha256.AddData((const uint8_t *)tmpBuf, (uint16_t)l);

    // Add device secret.
    sha256.AddData((const uint8_t *)deviceSecret, (uint16_t)deviceSecretLen);

    // Complete hash.
    sha256.Finish(tmpHashBuf);

    // Convert binary hash value to null terminated base-64 string.
    l = Base64Encode(tmpHashBuf, SHA256::kHashLength, hashBuf);
    hashBuf[l] = 0;

exit:
    return err;
}

} // namespace Security
} // namespace Profiles
} // namespace Weave
} // namespace nl
