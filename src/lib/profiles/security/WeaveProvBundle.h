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
 *      This file defines an object for decoding, verifying, and
 *      accessing a Weave device provisioning bundle, consisting of a
 *      certificate, private key, and pairing (entry) code.
 *
 */

#ifndef WEAVEPROVBUNDLE_H_
#define WEAVEPROVBUNDLE_H_

namespace nl {
namespace Weave {
namespace Profiles {
namespace Security {

// WeaveProvisioningBundle -- Represents the decoded form of a Weave provisioning bundle.
//
class WeaveProvisioningBundle
{
public:
    uint64_t WeaveDeviceId;

    const uint8_t *Certificate;
    uint16_t CertificateLen;

    const uint8_t *PrivateKey;
    uint16_t PrivateKeyLen;

    const char *PairingCode;
    uint16_t PairingCodeLen;

    void Clear(void);

    WEAVE_ERROR Verify(uint64_t expectedDeviceId);

    static WEAVE_ERROR Decode(uint8_t *provBundleBuf, uint32_t provBundleLen, const char *masterKey, uint32_t masterKeyLen, WeaveProvisioningBundle &provBundle);
};


} // namespace Security
} // namespace Profiles
} // namespace Weave
} // namespace nl


#endif /* WEAVEPROVBUNDLE_H_ */
