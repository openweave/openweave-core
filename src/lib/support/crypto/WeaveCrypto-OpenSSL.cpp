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
 *      OpenSSL-specific crypto utility functions.
 *
 */

#include <string.h>

#include <Weave/Support/CodeUtils.h>
#include "WeaveCrypto.h"

namespace nl {
namespace Weave {
namespace Crypto {

#if WEAVE_WITH_OPENSSL

static void ReverseBytes(uint8_t *buf, size_t len)
{
    uint8_t *head = buf;
    uint8_t *tail = buf + len - 1;
    for (; head < tail; head++, tail--)
    {
        uint8_t tmp = *head;
        *head = *tail;
        *tail = tmp;
    }
}

WEAVE_ERROR EncodeBIGNUMValueLE(const BIGNUM& val, uint16_t size, uint8_t *& p)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    int bnSize = BN_num_bytes(&val);

    VerifyOrExit(!BN_is_negative(&val), err = WEAVE_ERROR_INVALID_ARGUMENT);

    VerifyOrExit(bnSize <= size, err = WEAVE_ERROR_INVALID_ARGUMENT);

    memset(p, 0, size);
    BN_bn2bin(&val, p + (size - bnSize));

    ReverseBytes(p, size);

    p += size;

exit:
    return err;
}

WEAVE_ERROR DecodeBIGNUMValueLE(BIGNUM& val, uint16_t size, const uint8_t *& p)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    uint8_t *decodeBuf = (uint8_t *)OPENSSL_malloc(size);
    VerifyOrExit(decodeBuf != NULL, err = WEAVE_ERROR_NO_MEMORY);

    memcpy(decodeBuf, p, size);

    ReverseBytes(decodeBuf, size);

    if (BN_bin2bn(decodeBuf, size, &val) == NULL)
        ExitNow(err = WEAVE_ERROR_NO_MEMORY);

    p += size;

exit:
    if (decodeBuf != NULL)
        OPENSSL_free(decodeBuf);
    return err;
}

#endif // WEAVE_WITH_OPENSSL

} // namespace Crypto
} // namespace Weave
} // namespace nl
