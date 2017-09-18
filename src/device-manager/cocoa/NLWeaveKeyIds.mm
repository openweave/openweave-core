/*
 *
 *    Copyright (c) 2016-2017 Nest Labs, Inc.
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
 *      This file implements a wrapper for C++ implementation of WeaveKeyId operations
 *      for pin encryption.
 *
 */

#import "NLWeaveKeyIds.h"
#include <Weave/Core/WeaveKeyIds.h>

NSUInteger const NLWeaveKeyIds_KeyTypeNone = nl::Weave::WeaveKeyId::kType_None;
NSUInteger const NLWeaveKeyIds_KeyTypeGeneral = nl::Weave::WeaveKeyId::kType_General;
NSUInteger const NLWeaveKeyIds_KeyTypeSession = nl::Weave::WeaveKeyId::kType_Session;
NSUInteger const NLWeaveKeyIds_KeyTypeAppStaticKey = nl::Weave::WeaveKeyId::kType_AppStaticKey;
NSUInteger const NLWeaveKeyIds_KeyTypeAppRotatingKey = nl::Weave::WeaveKeyId::kType_AppRotatingKey;
NSUInteger const NLWeaveKeyIds_KeyTypeAppRootKey = nl::Weave::WeaveKeyId::kType_AppRootKey;
NSUInteger const NLWeaveKeyIds_KeyTypeAppEpochKey = nl::Weave::WeaveKeyId::kType_AppEpochKey;
NSUInteger const NLWeaveKeyIds_KeyTypeAppGroupMasterKey = nl::Weave::WeaveKeyId::kType_AppGroupMasterKey;
NSUInteger const NLWeaveKeyIds_KeyTypeAppIntermediateKey = nl::Weave::WeaveKeyId::kType_AppIntermediateKey;
NSUInteger const NLWeaveKeyIds_None = nl::Weave::WeaveKeyId::kNone;
NSUInteger const NLWeaveKeyIds_FabricSecret = nl::Weave::WeaveKeyId::kFabricSecret;
NSUInteger const NLWeaveKeyIds_FabricRootKey = nl::Weave::WeaveKeyId::kFabricRootKey;
NSUInteger const NLWeaveKeyIds_ClientRootKey = nl::Weave::WeaveKeyId::kClientRootKey;
NSUInteger const NLWeaveKeyIds_ServiceRootKey = nl::Weave::WeaveKeyId::kServiceRootKey;


@implementation NLWeaveKeyIds

+ (UInt32) getType: (UInt32) keyId {
    return nl::Weave::WeaveKeyId::GetType(keyId);
}

+ (BOOL) isGeneralKey: (UInt32) keyId {
    return nl::Weave::WeaveKeyId::IsGeneralKey(keyId);
}

+ (BOOL) isSessionKey: (UInt32) keyId {
    return nl::Weave::WeaveKeyId::IsSessionKey(keyId);
}

+ (BOOL) isAppStaticKey: (UInt32) keyId {
    return nl::Weave::WeaveKeyId::IsAppStaticKey(keyId);
}

+ (BOOL) isAppRotatingKey: (UInt32) keyId {
    return nl::Weave::WeaveKeyId::IsAppRotatingKey(keyId);
}

+ (BOOL) isAppRootKey: (UInt32) keyId {
    return nl::Weave::WeaveKeyId::IsAppRootKey(keyId);
}

+ (BOOL) isAppEpochKey: (UInt32) keyId {
    return nl::Weave::WeaveKeyId::IsAppEpochKey(keyId);
}

+ (BOOL) isAppGroupMasterKey: (UInt32) keyId {
    return nl::Weave::WeaveKeyId::IsAppGroupMasterKey(keyId);
}

+ (UInt16) makeSessionKeyId: (UInt16) sessionKeyNumber {
    return nl::Weave::WeaveKeyId::MakeSessionKeyId(sessionKeyNumber);
}

+ (UInt16) makeGeneralKeyId: (UInt16) generalKeyNumber {
    return nl::Weave::WeaveKeyId::MakeGeneralKeyId(generalKeyNumber);
}

+ (UInt32) getRootKeyId: (UInt32) keyId {
    return nl::Weave::WeaveKeyId::GetRootKeyId(keyId);
}

+ (UInt32) getEpochKeyId: (UInt32) keyId {
    return nl::Weave::WeaveKeyId::GetEpochKeyId(keyId);
}

+ (UInt32) getAppGroupMasterKeyId: (UInt32) keyId {
    return nl::Weave::WeaveKeyId::GetAppGroupMasterKeyId(keyId);
}

+ (UInt8) getRootKeyNumber: (UInt32) keyId {
    return nl::Weave::WeaveKeyId::GetRootKeyNumber(keyId);
}

+ (UInt8) getEpochKeyNumber: (UInt32) keyId {
    return nl::Weave::WeaveKeyId::GetEpochKeyNumber(keyId);
}

+ (UInt8) getAppGroupLocalNumber: (UInt32) keyId {
    return nl::Weave::WeaveKeyId::GetAppGroupLocalNumber(keyId);
}

+ (UInt32) makeRootKeyId: (UInt8) epochKeyNumber {
    return nl::Weave::WeaveKeyId::MakeRootKeyId(epochKeyNumber);
}

+ (UInt32) makeEpochKeyId: (UInt8) epochKeyNumber {
    return nl::Weave::WeaveKeyId::MakeEpochKeyId(epochKeyNumber);
}

+ (UInt32) makeAppGroupMasterKeyId: (UInt32) appGroupMasterKeyLocalId{
    return nl::Weave::WeaveKeyId::MakeAppGroupMasterKeyId(appGroupMasterKeyLocalId);
}

+ (UInt32) convertToCurrentAppKeyId: (UInt32) keyId{
    return nl::Weave::WeaveKeyId::ConvertToCurrentAppKeyId(keyId);
}

+ (BOOL) incorporatesEpochKey: (UInt32) keyId {
    return nl::Weave::WeaveKeyId::IncorporatesEpochKey(keyId);
}


+ (BOOL) usesCurrentEpochKey: (UInt32) keyId {
    return nl::Weave::WeaveKeyId::UsesCurrentEpochKey(keyId);
}
                               
+ (BOOL) incorporatesRootKey: (UInt32) keyId {
    return nl::Weave::WeaveKeyId::IncorporatesRootKey(keyId);
}
                               
+ (BOOL) incorporatesAppGroupMasterKey: (UInt32) keyId {
    return nl::Weave::WeaveKeyId::IncorporatesAppGroupMasterKey(keyId);
}

+ (UInt32) makeAppKeyId: (UInt32) keyType
              rootKeyId: (UInt32) rootKeyId
             epochKeyId: (UInt32) epochKeyId
    appGroupMasterKeyId: (UInt32) appGroupMasterKeyId
     useCurrentEpochKey: (BOOL) useCurrentEpochKey {
    return nl::Weave::WeaveKeyId::MakeAppKeyId(keyType, rootKeyId,
                epochKeyId, appGroupMasterKeyId, useCurrentEpochKey);
}

+ (UInt32) makeAppIntermediateKeyId: (UInt32) rootKeyId
                         epochKeyId: (UInt32) epochKeyId
                 useCurrentEpochKey: (BOOL) useCurrentEpochKey {
    return nl::Weave::WeaveKeyId::MakeAppIntermediateKeyId(rootKeyId,
                                        epochKeyId, useCurrentEpochKey);
}

+ (UInt32) makeAppRotatingKeyId: (UInt32) rootKeyId
                     epochKeyId: (UInt32) epochKeyId
            appGroupMasterKeyId: (UInt32) appGroupMasterKeyId
             useCurrentEpochKey: (BOOL) useCurrentEpochKey {
    return nl::Weave::WeaveKeyId::MakeAppRotatingKeyId(rootKeyId,
            epochKeyId, appGroupMasterKeyId, useCurrentEpochKey);
}

+ (UInt32) makeAppStaticKeyId: (UInt32) rootKeyId
          appGroupMasterKeyId: (UInt32) appGroupMasterKeyId {
    return nl::Weave::WeaveKeyId::MakeAppStaticKeyId(rootKeyId, appGroupMasterKeyId);
}

+ (UInt32) convertToStaticAppKeyId: (UInt32) keyId {
    return nl::Weave::WeaveKeyId::ConvertToStaticAppKeyId(keyId);
}

+ (UInt32) updateEpochKeyId: (UInt32) keyId
                 epochKeyId: (UInt32) epochKeyId {
    return nl::Weave::WeaveKeyId::UpdateEpochKeyId(keyId, epochKeyId);
}

+ (BOOL) isValidKeyId: (UInt32) keyId {
    return nl::Weave::WeaveKeyId::IsValidKeyId(keyId);
}

+ (NSString *) describeKey: (UInt32) keyId {
    return @(nl::Weave::WeaveKeyId::DescribeKey(keyId));
}


@end
