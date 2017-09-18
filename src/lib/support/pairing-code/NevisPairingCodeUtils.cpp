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
 *      Utility functions for working with Nest Nevis pairing codes.
 *
 */

#include <string.h>

#include <Weave/Support/CodeUtils.h>
#include <Weave/Support/pairing-code/PairingCodeUtils.h>

namespace nl {
namespace PairingCode {

enum
{
    kNevisDeviceIdBase = 0x18B4300400000000ULL,
    kNevisDeviceIdMax  = 0x18B4300401FFFFFFULL,
};

/** Returns the device ID encoded in Nevis pairing code.
 *
 * @param[in]  pairingCode                  A NULL-terminated string containing a Nevis pairing code.
 * @param[out] deviceId                     A reference to an integer that receives the decoded Nevis device id.
 *
 * @retval #WEAVE_NO_ERROR                  If the method succeeded.
 * @retval #WEAVE_ERROR_INVALID_ARGUMENT    If the length of the supplied pairing code is incorrect, or if the
 *                                          pairing code contains invalid characters, or if the initial characters
 *                                          of the pairing code are not consistent with the check character.
 */
WEAVE_ERROR NevisPairingCodeToDeviceId(const char *pairingCode, uint64_t& deviceId)
{
    WEAVE_ERROR err;
    size_t pairingCodeLen = strlen(pairingCode);

    // Make sure the pairing code is the correct length.
    VerifyOrExit(pairingCodeLen == kStandardPairingCodeLength, err = WEAVE_ERROR_INVALID_ARGUMENT);

    // Verify the check character.
    err = VerifyPairingCode(pairingCode, pairingCodeLen);
    SuccessOrExit(err);

    // Convert the initial characters of the pairing code to an integer.
    err = PairingCodeToInt(pairingCode, pairingCodeLen, deviceId);
    SuccessOrExit(err);

    // Convert the integer to a device id.
    deviceId += kNevisDeviceIdBase;

exit:
    return err;
}

/** Generates a Nevis pairing code string given a Nevis device id.
 *
 * @param[in]  deviceId                     A Nevis device id.
 * @param[out] pairingCodeBuf               A pointer to a buffer that will receive the Nevis pairing code,
 *                                          a NULL termination character.  The supplied buffer should be 7
 *                                          characters or more in size.
 * @param[in]  pairingCodeBufSize           The size of the buffer pointed at by pairingCodeBuf.
 *
 * @retval #WEAVE_NO_ERROR                  If the method succeeded.
 * @retval #WEAVE_ERROR_INVALID_ARGUMENT    If the supplied device id is out of range, or if the supplied buffer
 *                                          is too small.
 *
 */
WEAVE_ERROR NevisDeviceIdToPairingCode(uint64_t deviceId, char *pairingCodeBuf, size_t pairingCodeBufSize)
{
    WEAVE_ERROR err;

    // Verify the device id is in range.
    VerifyOrExit(deviceId >= kNevisDeviceIdBase && deviceId <= kNevisDeviceIdMax, err = WEAVE_ERROR_INVALID_ARGUMENT);

    // Verify that the supplied buffer is the correct size.
    VerifyOrExit(pairingCodeBufSize >= kStandardPairingCodeLength + 1, err = WEAVE_ERROR_BUFFER_TOO_SMALL);

    // Convert the device id to a 25-bit integer.
    deviceId -= kNevisDeviceIdBase;

    // Encode the integer value into a pairing code that includes a trailing check character.
    err = IntToPairingCode(deviceId, kStandardPairingCodeLength, pairingCodeBuf);
    SuccessOrExit(err);

exit:
    return err;
}

} // namespace PairingCode
} // namespace nl
