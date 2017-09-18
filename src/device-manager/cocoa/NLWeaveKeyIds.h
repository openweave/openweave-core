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
 *      This file defines a Wrapper for C++ implementation of WeaveKeyId operations
 *      for pin encryption.
 *
 */

#import <Foundation/Foundation.h>


NS_ASSUME_NONNULL_BEGIN

extern NSUInteger const NLWeaveKeyIds_KeyTypeNone;
extern NSUInteger const NLWeaveKeyIds_KeyTypeGeneral;
extern NSUInteger const NLWeaveKeyIds_KeyTypeSession;
extern NSUInteger const NLWeaveKeyIds_KeyTypeAppStaticKey;
extern NSUInteger const NLWeaveKeyIds_KeyTypeAppRotatingKey;
extern NSUInteger const NLWeaveKeyIds_KeyTypeAppRootKey;
extern NSUInteger const NLWeaveKeyIds_KeyTypeAppEpochKey;
extern NSUInteger const NLWeaveKeyIds_KeyTypeAppGroupMasterKey;
extern NSUInteger const NLWeaveKeyIds_KeyTypeAppIntermediateKey;
extern NSUInteger const NLWeaveKeyIds_None;
extern NSUInteger const NLWeaveKeyIds_FabricSecret;
extern NSUInteger const NLWeaveKeyIds_FabricRootKey;
extern NSUInteger const NLWeaveKeyIds_ClientRootKey;
extern NSUInteger const NLWeaveKeyIds_ServiceRootKey;

/**
 * @class Wrapper for C++ implementation of WeaveKeyId functionality
 */
@interface NLWeaveKeyIds : NSObject

/**
 *  Get Weave key type of the specified key ID.
 *
 *  @param[in]   keyId     Weave key identifier.
 *  @return                type of the key ID.
 *
 */
+ (UInt32) getType: (UInt32) keyId;

/**
 *  Determine whether the specified key ID is of a general type.
 *
 *  @param[in]   keyId     Weave key identifier.
 *  @return      true      if the keyId has General type.
 *
 */
+ (BOOL) isGeneralKey: (UInt32) keyId;

/**
 *  Determine whether the specified key ID is of a session type.
 *
 *  @param[in]   keyId     Weave key identifier.
 *  @return      true      if the keyId of a session type.
 *
 */
+ (BOOL) isSessionKey: (UInt32) keyId;

/**
 *  Determine whether the specified key ID is of an application static type.
 *
 *  @param[in]   keyId     Weave key identifier.
 *  @return      true      if the keyId of an application static type.
 *
 */
+ (BOOL) isAppStaticKey: (UInt32) keyId;

/**
 *  Determine whether the specified key ID is of an application rotating type.
 *
 *  @param[in]   keyId     Weave key identifier.
 *  @return      true      if the keyId of an application rotating type.
 *
 */
+ (BOOL) isAppRotatingKey: (UInt32) keyId;

/**
 *  Determine whether the specified key ID is of an application root key type.
 *
 *  @param[in]   keyId     Weave key identifier.
 *  @return      true      if the keyId of an application root key type.
 *
 */
+ (BOOL) isAppRootKey: (UInt32) keyId;

/**
 *  Determine whether the specified key ID is of an application epoch key type.
 *
 *  @param[in]   keyId     Weave key identifier.
 *  @return      true      if the keyId of an application epoch key type.
 *
 */
+ (BOOL) isAppEpochKey: (UInt32) keyId;

/**
 *  Determine whether the specified key ID is of an application group master key type.
 *
 *  @param[in]       keyId     Weave key identifier.
 *  @return  true      if the keyId of an application group master key type.
 *
 */
+ (BOOL) isAppGroupMasterKey: (UInt32) keyId;

/**
 *  Construct session key ID given session key number.
 *
 *  @param[in]   sessionKeyNumber      Session key number.
 *  @return      session key ID.
 *
 */
+ (UInt16) makeSessionKeyId: (UInt16) sessionKeyNumber;

/**
 *  Construct fabric key ID given fabric key number.
 *
 *  @param[in]   fabricKeyNumber       Fabric key number.
 *  @return      fabric key ID.
 *
 */
+ (UInt16) makeGeneralKeyId: (UInt16) generalKeyNumber;

/**
 *  Get application group root key ID that was used to derive specified application key.
 *
 *  @param[in]   keyId     Weave application group key identifier.
 *  @return      root key ID.
 *
 */
+ (UInt32) getRootKeyId: (UInt32) keyId;

/**
 *  Get application group epoch key ID that was used to derive specified application key.
 *
 *  @param[in]   keyId     Weave application group key identifier.
 *  @return      epoch key ID.
 *
 */
+ (UInt32) getEpochKeyId: (UInt32) keyId;

/**
 *  Get application group master key ID that was used to derive specified application key.
 *
 *  @param[in]   keyId     Weave application group key identifier.
 *  @return      application group master key ID.
 *
 */
+ (UInt32) getAppGroupMasterKeyId: (UInt32) keyId;

/**
 *  Get application group root key number that was used to derive specified application key.
 *
 *  @param[in]   keyId     Weave application group key identifier.
 *  @return      root key number.
 *
 */
+ (UInt8) getRootKeyNumber: (UInt32) keyId;

/**
 *  Get application group epoch key number that was used to derive specified application key.
 *
 *  @param[in]   keyId     Weave application group key identifier.
 *  @return      epoch key number.
 *
 */
+ (UInt8) getEpochKeyNumber: (UInt32) keyId;

/**
 *  Get application group local number that was used to derive specified application key.
 *
 *  @param[in]   keyId     Weave application group key identifier.
 *  @return      application group local number.
 *
 */
+ (UInt8) getAppGroupLocalNumber: (UInt32) keyId;

/**
 *  Construct application group root key ID given root key number.
 *
 *  @param[in]   rootKeyNumber         Root key number.
 *  @return      root key ID.
 *
 */
+ (UInt32) makeRootKeyId: (UInt8) epochKeyNumber;

/**
 *  Construct application group root key ID given epoch key number.
 *
 *  @param[in]   epochKeyNumber        Epoch key number.
 *  @return      epoch key ID.
 *
 */
+ (UInt32) makeEpochKeyId: (UInt8) epochKeyNumber;

/**
 *  Construct application group master key ID given application group local number.
 *
 *  @param[in]   appGroupLocalNumber   Application group local number.
 *  @return      application group master key ID.
 *
 */
+ (UInt32) makeAppGroupMasterKeyId: (UInt32) appGroupMasterKeyLocalId;

/**
 *  Convert application group key ID to application current key ID.
 *
 *  @param[in]   keyId                 Application key ID.
 *  @return      application current key ID.
 *
 */
+ (UInt32) convertToCurrentAppKeyId: (UInt32) keyId;

/**
 *  Determine whether the specified application group key ID incorporates epoch key.
 *
 *  @param[in]   keyId     Weave application group key identifier.
 *  @return      true      if the keyId incorporates epoch key.
 *
 */
+ (BOOL) incorporatesEpochKey: (UInt32) keyId;

+ (BOOL) usesCurrentEpochKey: (UInt32) keyId;

+ (BOOL) incorporatesRootKey: (UInt32) keyId;

+ (BOOL) incorporatesAppGroupMasterKey: (UInt32) keyId;

+ (UInt32) makeAppKeyId: (UInt32) keyType
              rootKeyId: (UInt32) rootKeyId
             epochKeyId: (UInt32) epochKeyId
    appGroupMasterKeyId: (UInt32) appGroupMasterKeyId
     useCurrentEpochKey: (BOOL) useCurrentEpochKey;

+ (UInt32) makeAppIntermediateKeyId: (UInt32) rootKeyId
                         epochKeyId: (UInt32) epochKeyId
                 useCurrentEpochKey: (BOOL) useCurrentEpochKey;

+ (UInt32) makeAppRotatingKeyId: (UInt32) rootKeyId
                     epochKeyId: (UInt32) epochKeyId
            appGroupMasterKeyId: (UInt32) appGroupMasterKeyId
             useCurrentEpochKey: (BOOL) useCurrentEpochKey;

+ (UInt32) makeAppStaticKeyId: (UInt32) rootKeyId
          appGroupMasterKeyId: (UInt32) appGroupMasterKeyId;

+ (UInt32) convertToStaticAppKeyId: (UInt32) keyId;

+ (UInt32) updateEpochKeyId: (UInt32) keyId
                 epochKeyId: (UInt32) epochKeyId;

+ (BOOL) isValidKeyId: (UInt32) keyId;

+ (NSString *) describeKey: (UInt32) keyId;

@end
NS_ASSUME_NONNULL_END
