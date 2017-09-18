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

    // Get the key export certificate set for the local node.
    // This method is responsible for initializing certificate set and loading all certificates
    // that will be included in the signature of the message.
    virtual WEAVE_ERROR GetNodeCertSet(bool isInitiator, WeaveCertificateSet& certSet);

    // Called when the key export engine is done with the certificate set returned by GetNodeCertSet().
    virtual WEAVE_ERROR ReleaseNodeCertSet(bool isInitiator, WeaveCertificateSet& certSet);

    // Get the local node's private key.
    virtual WEAVE_ERROR GetNodePrivateKey(bool isInitiator, const uint8_t *& weavePrivKey, uint16_t& weavePrivKeyLen);

    // Called when the key export engine is done with the buffer returned by GetNodePrivateKey().
    virtual WEAVE_ERROR ReleaseNodePrivateKey(bool isInitiator, const uint8_t *& weavePrivKey);

    // Prepare the supplied certificate set and validation context for use in validating the certificate of a peer.
    // This method is responsible for loading the trust anchors into the certificate set.
    virtual WEAVE_ERROR BeginCertValidation(bool isInitiator, WeaveCertificateSet& certSet, ValidationContext& validContext);

    // Called with the results of validating the peer's certificate.
    // Responder verifies that requestor is authorized to export the specified key.
    // Requestor verifies that response came from expected node.
    virtual WEAVE_ERROR HandleCertValidationResult(bool isInitiator, WeaveCertificateSet& certSet, ValidationContext& validContext,
                                                   const IPPacketInfo *pktInfo, const WeaveMessageInfo *msgHeader, uint32_t requestedKeyId);

    // Called when peer certificate validation and request verification are complete.
    virtual WEAVE_ERROR EndCertValidation(bool isInitiator, WeaveCertificateSet& certSet, ValidationContext& validContext);

    // Called by requestor and responder to verify that received message was appropriately secured when the message isn't signed.
    virtual WEAVE_ERROR ValidateUnsignedKeyExportMessage(bool isInitiator, const IPPacketInfo *pktInfo, const WeaveMessageInfo *msgHeader, uint32_t requestedKeyId);

    virtual bool HandleOption(const char *progName, OptionSet *optSet, int id, const char *name, const char *arg);
};

extern KeyExportOptions gKeyExportOptions;

#endif /* KEYEXPORTOPTIONS_H_ */
