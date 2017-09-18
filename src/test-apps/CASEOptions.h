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

    // Get the CASE Certificate Information structure for the local node.
    virtual WEAVE_ERROR GetNodeCertInfo(bool isInitiator, uint8_t *buf, uint16_t bufSize, uint16_t& certInfoLen);

    // Get the local node's private key.
    virtual WEAVE_ERROR GetNodePrivateKey(bool isInitiator, const uint8_t *& weavePrivKey, uint16_t& weavePrivKeyLen);

    // Called when the CASE engine is done with the buffer returned by GetNodePrivateKey().
    virtual WEAVE_ERROR ReleaseNodePrivateKey(const uint8_t *weavePrivKey);

    // Get payload information, if any, to be included in the message to the peer.
    virtual WEAVE_ERROR GetNodePayload(bool isInitiator, uint8_t *buf, uint16_t bufSize, uint16_t& payloadLen);

    // Prepare the supplied certificate set and validation context for use in validating the certificate of a peer.
    // This method is responsible for loading the trust anchors into the certificate set.
    virtual WEAVE_ERROR BeginCertValidation(bool isInitiator, WeaveCertificateSet& certSet, ValidationContext& validContext);

    // Called with the results of validating the peer's certificate.
    virtual WEAVE_ERROR HandleCertValidationResult(bool isInitiator, WEAVE_ERROR& validRes, WeaveCertificateData *peerCert,
            uint64_t peerNodeId, WeaveCertificateSet& certSet, ValidationContext& validContext);

    // Called when peer certificate validation is complete.
    virtual WEAVE_ERROR EndCertValidation(WeaveCertificateSet& certSet, ValidationContext& validContext);

    virtual bool HandleOption(const char *progName, OptionSet *optSet, int id, const char *name, const char *arg);

    static bool ReadCertFile(const char *fileName, uint8_t *& certBuf, uint16_t& certLen);
    static bool ReadPrivateKeyFile(const char *fileName, uint8_t *& keyBuf, uint16_t& keyLen);
};

extern CASEOptions gCASEOptions;

#endif /* CASEOPTIONS_H_ */
