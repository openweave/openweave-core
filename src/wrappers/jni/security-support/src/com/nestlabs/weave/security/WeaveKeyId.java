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

package com.nestlabs.weave.security;

/**
 * Utility methods and definitions for working with WeaveKeyIds.
 */
@SuppressWarnings("unused")
public final class WeaveKeyId {
    
    // ===== Private Members =====

    private static final int kKeyFlag_UseCurrentEpochKey            = 0x80000000;
    
    private static final int kKeyTypeModifier_IncorporatesEpochKey  = 0x00001000;

    private static final int kMask_KeyFlags                         = 0xF0000000;
    private static final int kMask_AllDefinedFlags                  = kKeyFlag_UseCurrentEpochKey;
    private static final int kMask_KeyType                          = 0x0FFFF000;
    private static final int kMask_KeyNumber                        = 0x00000FFF;
    private static final int kMask_RootKeyNumber                    = 0x00000C00;
    private static final int kMask_EpochKeyNumber                   = 0x00000380;
    private static final int kMask_AppGroupMasterKeyLocalId         = 0x0000007F;

    private static final int kShift_RootKeyNumber                   = 10;
    private static final int kShift_EpochKeyNumber                  = 7;
    private static final int kShift_AppGroupMasterKeyLocalId        = 0; 


    // ===== Public Members =====

    public static int getKeyType(int keyId) {
        return keyId & kMask_KeyType;
    }

    public static int getRootKeyId(int keyId) {
        if (!incorporatesRootKey(keyId)) {
            throw new IllegalArgumentException("Supplied key id does not incorporate root key");
        }
        return kKeyType_AppRootKey | (keyId & kMask_RootKeyNumber);
    }

    public static int getEpochKeyId(int keyId) {
        if (!incorporatesEpochKey(keyId)) {
            throw new IllegalArgumentException("Supplied key id does not incorporate epoch key");
        }
        return kKeyType_AppEpochKey | (keyId & kMask_EpochKeyNumber);
    }

    public static int getAppGroupMasterKeyId(int keyId) {
        if (!incorporatesAppGroupMasterKey(keyId)) {
            throw new IllegalArgumentException("Supplied key id does not incorporate an app group master key");
        }
        return kKeyType_AppGroupMasterKey | (keyId & kMask_AppGroupMasterKeyLocalId);
    }

    public static boolean incorporatesRootKey(int keyId) {
        int keyType = getKeyType(keyId);
        return keyType == kKeyType_AppStaticKey ||
               keyType == kKeyType_AppRotatingKey ||
               keyType == kKeyType_AppRootKey ||
               keyType == kKeyType_AppIntermediateKey;
    }

    public static boolean incorporatesEpochKey(int keyId) {
        return (keyId & kKeyTypeModifier_IncorporatesEpochKey) != 0;
    }

    public static boolean incorporatesAppGroupMasterKey(int keyId) {
        int keyType = getKeyType(keyId);
        return keyType == kKeyType_AppStaticKey ||
               keyType == kKeyType_AppRotatingKey ||
               keyType == kKeyType_AppGroupMasterKey;
    }

    public static boolean usesCurrentEpochKey(int keyId) {
        return incorporatesEpochKey(keyId) && (keyId & kKeyFlag_UseCurrentEpochKey) != 0;
    }
    
    public static int makeRootKeyId(int rootKeyNumber) {
        if (rootKeyNumber < 0 || rootKeyNumber > (kMask_RootKeyNumber >> kShift_RootKeyNumber)) {
            throw new IllegalArgumentException("Invalid argument: rootKeyNumber");
        }
        return kKeyType_AppRootKey | (rootKeyNumber << kShift_EpochKeyNumber);
    }
    
    public static int makeEpochKeyId(int epochKeyNumber) {
        if (epochKeyNumber < 0 || epochKeyNumber > (kMask_EpochKeyNumber >> kShift_EpochKeyNumber)) {
            throw new IllegalArgumentException("Invalid argument: epochKeyNumber");
        }
        return kKeyType_AppEpochKey | (epochKeyNumber << kShift_EpochKeyNumber);
    }
    
    public static int makeAppGroupMasterKeyId(int appGroupMasterKeyLocalId) {
        if (appGroupMasterKeyLocalId < 0 || appGroupMasterKeyLocalId > (kMask_AppGroupMasterKeyLocalId >> kShift_AppGroupMasterKeyLocalId)) {
            throw new IllegalArgumentException("Invalid argument: appGroupMasterKeyLocalId");
        }
        return kKeyType_AppGroupMasterKey | (appGroupMasterKeyLocalId << kShift_AppGroupMasterKeyLocalId);
    }
    
    public static int makeAppKeyId(int keyType, int rootKeyId, int epochKeyId, int appGroupMasterKeyId, boolean useCurrentEpochKey) {
        if (keyType != kKeyType_AppStaticKey && keyType != kKeyType_AppRotatingKey) {
            throw new IllegalArgumentException("Invalid argument: keyType");
        }
        if (rootKeyId != FabricRootKey && rootKeyId != ClientRootKey && rootKeyId != ServiceRootKey) {
            throw new IllegalArgumentException("Invalid argument: rootKeyId");
        }
        if ((keyType == kKeyType_AppStaticKey && epochKeyId != None) ||
            (keyType == kKeyType_AppRotatingKey && (( useCurrentEpochKey && epochKeyId != None) ||
                                                    (!useCurrentEpochKey && getKeyType(epochKeyId) != kKeyType_AppEpochKey)))) {
            throw new IllegalArgumentException("Invalid argument: epochKeyId");
        }
        if (getKeyType(appGroupMasterKeyId) != kKeyType_AppGroupMasterKey) {
            throw new IllegalArgumentException("Invalid argument: appGroupMasterKeyId");
        }
        return (keyType |
                (rootKeyId & kMask_RootKeyNumber) |
                (epochKeyId & kMask_EpochKeyNumber) |
                (appGroupMasterKeyId & kMask_AppGroupMasterKeyLocalId) |
                (useCurrentEpochKey ? kKeyFlag_UseCurrentEpochKey : 0));
    }
    
    public static int makeAppRotatingKeyId(int rootKeyId, int epochKeyId, int appGroupMasterKeyId, boolean useCurrentEpochKey) {
        return makeAppKeyId(kKeyType_AppRotatingKey, rootKeyId, epochKeyId, appGroupMasterKeyId, useCurrentEpochKey);
    }
    
    public static int makeAppStaticKeyId(int rootKeyId, int appGroupMasterKeyId) {
        return makeAppKeyId(kKeyType_AppStaticKey, rootKeyId, None, appGroupMasterKeyId, false);
    }
    
    public static int convertToStaticAppKeyId(int keyId) {
        if (getKeyType(keyId) == kKeyType_AppRotatingKey) {
            keyId = makeAppStaticKeyId(getRootKeyId(keyId), getAppGroupMasterKeyId(keyId));
        }
        return keyId;
    }
    
    public static int updateEpochKey(int keyId, int epochKeyId) {
        if (!incorporatesEpochKey(keyId)) {
            throw new IllegalArgumentException("Supplied keyId does not incorporate epoch key");
        }
        if (getKeyType(epochKeyId) != kKeyType_AppEpochKey) {
            throw new IllegalArgumentException("Supplied epochKeyId is wrong type");
        }
        return (keyId & ~(kKeyFlag_UseCurrentEpochKey|kMask_EpochKeyNumber)) | (epochKeyId & kMask_EpochKeyNumber);
    }
    
    public static boolean isValid(int keyId) {
        int usedBits = kMask_KeyType;
        
        switch (getKeyType(keyId)) {
        case kKeyType_None:
            return false;
        case kKeyType_General:
        case kKeyType_Session:
            usedBits |= kMask_KeyNumber;
            break;
        case kKeyType_AppStaticKey:
            usedBits |= kMask_RootKeyNumber|kMask_AppGroupMasterKeyLocalId;
            break;
        case kKeyType_AppRotatingKey:
            usedBits |= kKeyFlag_UseCurrentEpochKey|kMask_RootKeyNumber|kMask_AppGroupMasterKeyLocalId;
            if (!usesCurrentEpochKey(keyId)) {
                usedBits |= kMask_EpochKeyNumber;
            }
            break;
        case kKeyType_AppRootKey:
            usedBits |= kMask_RootKeyNumber;
            break;
        case kKeyType_AppIntermediateKey:
            usedBits |= kKeyFlag_UseCurrentEpochKey|kMask_RootKeyNumber;
            if (!usesCurrentEpochKey(keyId)) {
                usedBits |= kMask_EpochKeyNumber;
            }
            break;
        case kKeyType_AppEpochKey:
            usedBits |= kKeyFlag_UseCurrentEpochKey;
            if (!usesCurrentEpochKey(keyId)) {
                usedBits |= kMask_EpochKeyNumber;
            }
            break;
        case kKeyType_AppGroupMasterKey:
            usedBits |= kMask_AppGroupMasterKeyLocalId;
            break;
        default:
            return false;
        }
        
        if (incorporatesRootKey(keyId)) {
            int rootKeyId = getRootKeyId(keyId);
            if (rootKeyId != FabricRootKey && rootKeyId != ClientRootKey && rootKeyId == ServiceRootKey) {
                return false;
            }
        }
        
        return (keyId & ~usedBits) == 0;
    }
    
    public static String describeKey(int keyId) {
        switch (getKeyType(keyId)) {
        case kKeyType_None:
            return "No Key";
        case kKeyType_General:
            return "General Key";
        case kKeyType_Session:
            return "Session Key";
        case kKeyType_AppStaticKey:
            return "Application Static Key";
        case kKeyType_AppRotatingKey:
            return "Application Rotating Key";
        case kKeyType_AppRootKey:
            if (keyId == FabricRootKey) {
                return "Fabric Root Key";
            }
            if (keyId == ClientRootKey) {
                return "Client Root Key";
            }
            if (keyId == ServiceRootKey) {
                return "Service Root Key";
            }
            return "Other Root Key";
        case kKeyType_AppIntermediateKey:
            return "Application Intermediate Key";
        case kKeyType_AppEpochKey:
            return "Application Epoch Key";
        case kKeyType_AppGroupMasterKey:
            return "Application Group Master Key";
        default:
            return String.format("Key Type 0x08X", getKeyType(keyId));
        }
    }

    public static final int kKeyType_None                           = 0x00000000;
    public static final int kKeyType_General                        = 0x00001000; 
    public static final int kKeyType_Session                        = 0x00002000; 
    public static final int kKeyType_AppStaticKey                   = 0x00004000; 
    public static final int kKeyType_AppRotatingKey                 = 0x00004000 | kKeyTypeModifier_IncorporatesEpochKey; 
    public static final int kKeyType_AppRootKey                     = 0x00010000;
    public static final int kKeyType_AppIntermediateKey             = 0x00010000 | kKeyTypeModifier_IncorporatesEpochKey;
    public static final int kKeyType_AppEpochKey                    = 0x00020000 | kKeyTypeModifier_IncorporatesEpochKey;
    public static final int kKeyType_AppGroupMasterKey              = 0x00030000;

    public static final int None                                    = kKeyType_None;
    public static final int FabricRootKey                           = kKeyType_AppRootKey | 0x000;
    public static final int ClientRootKey                           = kKeyType_AppRootKey | 0x400;
    public static final int ServiceRootKey                          = kKeyType_AppRootKey | 0x800;
    
    public static final int FabricSecret                            = kKeyType_General | 0x0001;
}
