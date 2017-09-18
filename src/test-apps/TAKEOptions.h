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
 *      Definition of TAKEConfig class, which provides an implementation of the WeaveTAKEAuthDelegate
 *      interface for use in test applications.
 *
 */

#ifndef TAKEOPTIONS_H_
#define TAKEOPTIONS_H_

#include <Weave/Core/WeaveCore.h>
#include <Weave/Profiles/security/WeaveSecurity.h>
#include <Weave/Profiles/security/WeaveTAKE.h>

#include "ToolCommonOptions.h"

using namespace nl::Weave::Profiles::Security;
using namespace nl::Weave::Profiles::Security::TAKE;

class MockTAKEChallengerDelegate : public WeaveTAKEChallengerAuthDelegate
{
protected:
    bool AuthenticationKeySet;
    bool Rewinded;

public:
    MockTAKEChallengerDelegate();

    // Rewind Identification Key Iterator.
    // Called to prepare for a new Identification Key search.
    virtual WEAVE_ERROR RewindIdentificationKeyIterator();

    // Get next {tokenId, IK} pair.
    // returns tokenId = kNodeIdNotSpecified if no more IKs are available.
    virtual WEAVE_ERROR GetNextIdentificationKey(uint64_t & tokenId, uint8_t *identificationKey, uint16_t & identificationKeyLen);

    //Get Token Authentication Data.
    // Function returns {takeConfig = 0x00, authKey = NULL, encAuthBlob = NULL} if Authentication Data associated with a specified Token
    // is not stored on the device.
    // On the function call authKeyLen and encAuthBlobLen inputs specify sizes of the authKey and encAuthBlob buffers, respectively.
    // Function should update these parameters to reflect actual sizes.
    virtual WEAVE_ERROR GetTokenAuthData(uint64_t tokenId, uint8_t &takeConfig, uint8_t *authKey, uint16_t &authKeyLen, uint8_t *encAuthBlob, uint16_t &encAuthBlobLen);

    // Store Token Authentication Data.
    // This function should clear Authentication Data that was previously stored on the device for the specified Token (if any).
    virtual  WEAVE_ERROR StoreTokenAuthData(uint64_t tokenId, uint8_t takeConfig, const uint8_t *authKey, uint16_t authKeyLen, const uint8_t *encAuthBlob, uint16_t encAuthBlobLen);

    // Clear Token Authentication Data.
    // This function should be called if ReAuthentication phase with the Token Authentication Data stored on the device failed.
    virtual WEAVE_ERROR ClearTokenAuthData(uint64_t tokenId);

    // Get Token public key.
    // On the function call tokenPubKeyLen input specifies size of the tokenPubKey buffer. Function should update this parameter to reflect actual sizes.
    virtual WEAVE_ERROR GetTokenPublicKey(uint64_t tokenId, OID& curveOID, EncodedECPublicKey& tokenPubKey);

    // Get the challenger ID.
    virtual WEAVE_ERROR GetChallengerID(uint8_t *challengerID, uint8_t &challengerIDLen) const;
};


class MockTAKETokenDelegate : public WeaveTAKETokenAuthDelegate
{
public:
    // Get the token Master key. size: kTokenMasterKeySize
    virtual WEAVE_ERROR GetTokenMasterKey(uint8_t *tokenMasterKey) const;

    // Get the Identification Root Key. size: kIdentificationRootKeySize
    virtual WEAVE_ERROR GetIdentificationRootKey(uint8_t *identificationRootKey) const;

    // Get the token Private Key.
    // On the function call tokenPrivKeyLen input specifies size of the tokenPrivKey buffer.
    // Function should update this parameter to reflect actual sizes of the private key.
    virtual WEAVE_ERROR GetTokenPrivateKey(OID& curveOID, EncodedECPrivateKey& tokenPrivKey) const;

    // Get TAKE Time.
    // Function returns takeTime, which is Unix time rounded with 24 hour granularity
    // i.e. number of days elapsed after 1 January 1970.
    virtual WEAVE_ERROR GetTAKETime(uint32_t &takeTime) const;
};

class TAKEOptions : public OptionSetBase
{
public:
    TAKEOptions();

    const uint8_t *IK;
    const uint8_t *ChallengerId;
    uint8_t ChallengerIdLen;
    const uint8_t *TPub;
    uint16_t TPubLen;
    const uint8_t *TPriv;
    uint16_t TPrivLen;
    const uint8_t *IRK;
    const uint8_t *MasterKey;
    const uint8_t *AK;
    const uint8_t *EncAK;
    bool ForceReauth;

    virtual bool HandleOption(const char *progName, OptionSet *optSet, int id, const char *name, const char *arg);

    WEAVE_ERROR PrepopulateTokenData();
};

extern TAKEOptions gTAKEOptions;
extern MockTAKEChallengerDelegate gMockTAKEChallengerDelegate;
extern MockTAKETokenDelegate gMockTAKETokenDelegate;


#endif /* TAKEOPTIONS_H_ */
