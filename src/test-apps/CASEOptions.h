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
 *      Implementation of CASEOptions class, which handles CASE-specific command line options
 *      and provides an implementation of the WeaveCASEAuthDelegate interface for use in
 *      test applications.
 *
 */

#ifndef CASEOPTIONS_H_
#define CASEOPTIONS_H_

#include <Weave/Core/WeaveCore.h>
#include <Weave/Profiles/security/WeaveSecurity.h>
#include <Weave/Profiles/security/WeavePASE.h>

#include "ToolCommonOptions.h"

using namespace nl::Weave::Profiles::Security;
using namespace nl::Weave::Profiles::Security::CASE;

class CASEOptions : public WeaveCASEAuthDelegate, public OptionSetBase
{
public:
    uint32_t InitiatorCASEConfig;

    uint8_t AllowedCASEConfigs;

    const uint8_t *NodeCert;
    uint16_t NodeCertLength;

    const uint8_t *NodePrivateKey;
    uint16_t NodePrivateKeyLength;

    const uint8_t *NodeIntermediateCert;
    uint16_t NodeIntermediateCertLength;

    const uint8_t *ServiceConfig;
    uint16_t ServiceConfigLength;

    const uint8_t *NodePayload;
    uint16_t NodePayloadLength;

    bool Debug;

#if WEAVE_CONFIG_SECURITY_TEST_MODE
    bool UseKnownECDHKey;
#endif

    CASEOptions();

private:

    WEAVE_ERROR GetNodeCert(const uint8_t *& nodeCert, uint16_t & nodeCertLen);
    WEAVE_ERROR GetNodePrivateKey(const uint8_t *& weavePrivKey, uint16_t& weavePrivKeyLen);

    virtual bool HandleOption(const char *progName, OptionSet *optSet, int id, const char *name, const char *arg) __OVERRIDE;

#if !WEAVE_CONFIG_LEGACY_CASE_AUTH_DELEGATE

    // ===== Methods that implement the WeaveCASEAuthDelegate interface

    WEAVE_ERROR EncodeNodeCertInfo(const BeginSessionContext & msgCtx, TLVWriter & writer) __OVERRIDE;
    WEAVE_ERROR GenerateNodeSignature(const BeginSessionContext & msgCtx,
                const uint8_t * msgHash, uint8_t msgHashLen,
                TLVWriter & writer, uint64_t tag) __OVERRIDE;
    WEAVE_ERROR EncodeNodePayload(const BeginSessionContext & msgCtx,
            uint8_t * payloadBuf, uint16_t payloadBufSize, uint16_t & payloadLen) __OVERRIDE;
    WEAVE_ERROR BeginValidation(const BeginSessionContext & msgCtx, ValidationContext & validCtx,
            WeaveCertificateSet & certSet) __OVERRIDE;
    WEAVE_ERROR HandleValidationResult(const BeginSessionContext & msgCtx, ValidationContext & validCtx,
            WeaveCertificateSet & certSet, WEAVE_ERROR & validRes) __OVERRIDE;
    void EndValidation(const BeginSessionContext & msgCtx, ValidationContext & validCtx,
            WeaveCertificateSet & certSet) __OVERRIDE;

#else // !WEAVE_CONFIG_LEGACY_CASE_AUTH_DELEGATE

    // ===== Methods that implement the legacy WeaveCASEAuthDelegate interface

    WEAVE_ERROR GetNodeCertInfo(bool isInitiator, uint8_t *buf, uint16_t bufSize, uint16_t& certInfoLen);
    WEAVE_ERROR GetNodePrivateKey(bool isInitiator, const uint8_t *& weavePrivKey, uint16_t& weavePrivKeyLen);
    WEAVE_ERROR ReleaseNodePrivateKey(const uint8_t *weavePrivKey);

#endif // WEAVE_CONFIG_LEGACY_CASE_AUTH_DELEGATE

    WEAVE_ERROR GetNodePayload(bool isInitiator, uint8_t *buf, uint16_t bufSize, uint16_t& payloadLen);
    WEAVE_ERROR BeginCertValidation(bool isInitiator, WeaveCertificateSet& certSet, ValidationContext& validContext);
    WEAVE_ERROR HandleCertValidationResult(bool isInitiator, WEAVE_ERROR& validRes, WeaveCertificateData *peerCert,
            uint64_t peerNodeId, WeaveCertificateSet& certSet, ValidationContext& validContext);
    WEAVE_ERROR EndCertValidation(WeaveCertificateSet& certSet, ValidationContext& validContext);
};

extern CASEOptions gCASEOptions;

#endif /* CASEOPTIONS_H_ */
