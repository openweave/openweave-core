
/*
 *    Copyright (c) 2019 Google LLC.
 *    Copyright (c) 2016-2018 Nest Labs, Inc.
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

/*
 *    THIS FILE IS GENERATED. DO NOT MODIFY.
 *
 *    SOURCE TEMPLATE: trait.cpp.h
 *    SOURCE PROTO: weave/trait/security/pincode_input_trait.proto
 *
 */
#ifndef _WEAVE_TRAIT_SECURITY__PINCODE_INPUT_TRAIT_H_
#define _WEAVE_TRAIT_SECURITY__PINCODE_INPUT_TRAIT_H_

#include <Weave/Profiles/data-management/DataManagement.h>
#include <Weave/Support/SerializationUtils.h>



namespace Schema {
namespace Weave {
namespace Trait {
namespace Security {
namespace PincodeInputTrait {

extern const nl::Weave::Profiles::DataManagement::TraitSchemaEngine TraitSchema;

enum {
      kWeaveProfileId = (0x0U << 16) | 0xe05U
};

//
// Properties
//

enum {
    kPropertyHandle_Root = 1,

    //---------------------------------------------------------------------------------------------------------------------------//
    //  Name                                IDL Type                            TLV Type           Optional?       Nullable?     //
    //---------------------------------------------------------------------------------------------------------------------------//

    //
    //  pincode_input_state                 PincodeInputState                    int               NO              NO
    //
    kPropertyHandle_PincodeInputState = 2,

    //
    // Enum for last handle
    //
    kLastSchemaHandle = 2,
};

//
// Events
//
struct KeypadEntryEvent
{
    bool pincodeCredentialEnabled;
    void SetPincodeCredentialEnabledNull(void);
    void SetPincodeCredentialEnabledPresent(void);
#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
    bool IsPincodeCredentialEnabledPresent(void);
#endif
    nl::SerializedByteString userId;
    void SetUserIdNull(void);
    void SetUserIdPresent(void);
#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
    bool IsUserIdPresent(void);
#endif
    uint32_t invalidEntryCount;
    int32_t pincodeEntryResult;
    uint8_t __nullified_fields__[2/8 + 1];

    static const nl::SchemaFieldDescriptor FieldSchema;

    // Statically-known Event Struct Attributes:
    enum {
            kWeaveProfileId = (0x0U << 16) | 0xe05U,
        kEventTypeId = 0x1U
    };

    static const nl::Weave::Profiles::DataManagement::EventSchema Schema;
};

struct KeypadEntryEvent_array {
    uint32_t num;
    KeypadEntryEvent *buf;
};

inline void KeypadEntryEvent::SetPincodeCredentialEnabledNull(void)
{
    SET_FIELD_NULLIFIED_BIT(__nullified_fields__, 0);
}

inline void KeypadEntryEvent::SetPincodeCredentialEnabledPresent(void)
{
    CLEAR_FIELD_NULLIFIED_BIT(__nullified_fields__, 0);
}

#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
inline bool KeypadEntryEvent::IsPincodeCredentialEnabledPresent(void)
{
    return (!GET_FIELD_NULLIFIED_BIT(__nullified_fields__, 0));
}
#endif
inline void KeypadEntryEvent::SetUserIdNull(void)
{
    SET_FIELD_NULLIFIED_BIT(__nullified_fields__, 1);
}

inline void KeypadEntryEvent::SetUserIdPresent(void)
{
    CLEAR_FIELD_NULLIFIED_BIT(__nullified_fields__, 1);
}

#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
inline bool KeypadEntryEvent::IsUserIdPresent(void)
{
    return (!GET_FIELD_NULLIFIED_BIT(__nullified_fields__, 1));
}
#endif

struct PincodeInputStateChangeEvent
{
    int32_t pincodeInputState;
    uint64_t userId;
    void SetUserIdNull(void);
    void SetUserIdPresent(void);
#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
    bool IsUserIdPresent(void);
#endif
    uint8_t __nullified_fields__[1/8 + 1];

    static const nl::SchemaFieldDescriptor FieldSchema;

    // Statically-known Event Struct Attributes:
    enum {
            kWeaveProfileId = (0x0U << 16) | 0xe05U,
        kEventTypeId = 0x2U
    };

    static const nl::Weave::Profiles::DataManagement::EventSchema Schema;
};

struct PincodeInputStateChangeEvent_array {
    uint32_t num;
    PincodeInputStateChangeEvent *buf;
};

inline void PincodeInputStateChangeEvent::SetUserIdNull(void)
{
    SET_FIELD_NULLIFIED_BIT(__nullified_fields__, 0);
}

inline void PincodeInputStateChangeEvent::SetUserIdPresent(void)
{
    CLEAR_FIELD_NULLIFIED_BIT(__nullified_fields__, 0);
}

#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
inline bool PincodeInputStateChangeEvent::IsUserIdPresent(void)
{
    return (!GET_FIELD_NULLIFIED_BIT(__nullified_fields__, 0));
}
#endif

//
// Enums
//

enum PincodeEntryResult {
    PINCODE_ENTRY_RESULT_FAILURE_INVALID_PINCODE = 1,
    PINCODE_ENTRY_RESULT_FAILURE_OUT_OF_SCHEDULE = 2,
    PINCODE_ENTRY_RESULT_FAILURE_PINCODE_DISABLED = 3,
    PINCODE_ENTRY_RESULT_SUCCESS = 4,
};

enum PincodeInputState {
    PINCODE_INPUT_STATE_ENABLED = 1,
    PINCODE_INPUT_STATE_DISABLED = 2,
};

} // namespace PincodeInputTrait
} // namespace Security
} // namespace Trait
} // namespace Weave
} // namespace Schema
#endif // _WEAVE_TRAIT_SECURITY__PINCODE_INPUT_TRAIT_H_
