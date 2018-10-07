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
 *      Definition of KeyExportOptions class, which handles KeyExport-specific command line options
 *      and provides an implementation of the WeaveKeyExportDelegate interface for use in
 *      test applications.
 *
 */

#ifndef KEYEXPORTOPTIONS_H_
#define KEYEXPORTOPTIONS_H_

#include <Weave/Core/WeaveCore.h>
#include <Weave/Profiles/security/WeaveSecurity.h>
#include <Weave/Profiles/security/WeaveKeyExport.h>

#include "ToolCommonOptions.h"

using nl::Inet::IPPacketInfo;
using nl::Weave::WeaveMessageInfo;
using namespace nl::Weave::Profiles::Security;
using namespace nl::Weave::Profiles::Security::KeyExport;

class KeyExportOptions : public WeaveKeyExportDelegate, public OptionSetBase
{
public:
    uint8_t AllowedKeyExportConfigs;

    const uint8_t *mAccessToken;
    uint16_t mAccessTokenLength;

    KeyExportOptions();

    virtual bool HandleOption(const char *progName, OptionSet *optSet, int id, const char *name, const char *arg);

private:

#if !WEAVE_CONFIG_LEGACY_KEY_EXPORT_DELEGATE

    WEAVE_ERROR GetNodeCertSet(WeaveKeyExport * keyExport, WeaveCertificateSet & certSet) __OVERRIDE;
    WEAVE_ERROR ReleaseNodeCertSet(WeaveKeyExport * keyExport, WeaveCertificateSet & certSet) __OVERRIDE;
    WEAVE_ERROR GenerateNodeSignature(WeaveKeyExport * keyExport, const uint8_t * msgHash, uint8_t msgHashLen,
        TLVWriter & writer) __OVERRIDE;
    WEAVE_ERROR BeginCertValidation(WeaveKeyExport * keyExport, ValidationContext & validCtx,
            WeaveCertificateSet & certSet) __OVERRIDE;
    WEAVE_ERROR HandleCertValidationResult(WeaveKeyExport * keyExport, ValidationContext & validCtx,
            WeaveCertificateSet & certSet, uint32_t requestedKeyId) __OVERRIDE;
    WEAVE_ERROR EndCertValidation(WeaveKeyExport * keyExport, ValidationContext & validCtx,
            WeaveCertificateSet & certSet) __OVERRIDE;
    WEAVE_ERROR ValidateUnsignedKeyExportMessage(WeaveKeyExport * keyExport, uint32_t requestedKeyId) __OVERRIDE;

#endif // !WEAVE_CONFIG_LEGACY_KEY_EXPORT_DELEGATE

    WEAVE_ERROR GetNodeCertSet(bool isInitiator, WeaveCertificateSet& certSet);
    WEAVE_ERROR ReleaseNodeCertSet(bool isInitiator, WeaveCertificateSet& certSet);
    WEAVE_ERROR GetNodePrivateKey(bool isInitiator, const uint8_t *& weavePrivKey, uint16_t& weavePrivKeyLen);
    WEAVE_ERROR ReleaseNodePrivateKey(bool isInitiator, const uint8_t *& weavePrivKey);
    WEAVE_ERROR BeginCertValidation(bool isInitiator, WeaveCertificateSet& certSet, ValidationContext& validContext);
    WEAVE_ERROR HandleCertValidationResult(bool isInitiator, WeaveCertificateSet& certSet, ValidationContext& validContext,
            const IPPacketInfo *pktInfo, const WeaveMessageInfo *msgHeader, uint32_t requestedKeyId);
    WEAVE_ERROR EndCertValidation(bool isInitiator, WeaveCertificateSet& certSet, ValidationContext& validContext);
    WEAVE_ERROR ValidateUnsignedKeyExportMessage(bool isInitiator, const IPPacketInfo *pktInfo, const WeaveMessageInfo *msgHeader, uint32_t requestedKeyId);
};

extern KeyExportOptions gKeyExportOptions;

#endif /* KEYEXPORTOPTIONS_H_ */
