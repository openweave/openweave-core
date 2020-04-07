
/*
 *    Copyright (c) 2019-2020 Google LLC.
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
 *    SOURCE PROTO: nest/test/trait/test_e_trait.proto
 *
 */
#ifndef _NEST_TEST_TRAIT__TEST_E_TRAIT_H_
#define _NEST_TEST_TRAIT__TEST_E_TRAIT_H_

#include <Weave/Profiles/data-management/DataManagement.h>
#include <Weave/Support/SerializationUtils.h>

#include <nest/test/trait/TestCommon.h>


namespace Schema {
namespace Nest {
namespace Test {
namespace Trait {
namespace TestETrait {

extern const nl::Weave::Profiles::DataManagement::TraitSchemaEngine TraitSchema;

enum {
      kWeaveProfileId = (0x235aU << 16) | 0xfe06U
};

//
// Event Structs
//

struct StructE
{
    uint32_t seA;
    bool seB;
    int32_t seC;

    static const nl::SchemaFieldDescriptor FieldSchema;

};

struct StructE_array {
    uint32_t num;
    StructE *buf;
};

struct NullableE
{
    uint32_t neA;
    void SetNeANull(void);
    void SetNeAPresent(void);
#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
    bool IsNeAPresent(void);
#endif
    bool neB;
    void SetNeBNull(void);
    void SetNeBPresent(void);
#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
    bool IsNeBPresent(void);
#endif
    uint8_t __nullified_fields__[2/8 + 1];

    static const nl::SchemaFieldDescriptor FieldSchema;

};

struct NullableE_array {
    uint32_t num;
    NullableE *buf;
};

inline void NullableE::SetNeANull(void)
{
    SET_FIELD_NULLIFIED_BIT(__nullified_fields__, 0);
}

inline void NullableE::SetNeAPresent(void)
{
    CLEAR_FIELD_NULLIFIED_BIT(__nullified_fields__, 0);
}

#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
inline bool NullableE::IsNeAPresent(void)
{
    return (!GET_FIELD_NULLIFIED_BIT(__nullified_fields__, 0));
}
#endif
inline void NullableE::SetNeBNull(void)
{
    SET_FIELD_NULLIFIED_BIT(__nullified_fields__, 1);
}

inline void NullableE::SetNeBPresent(void)
{
    CLEAR_FIELD_NULLIFIED_BIT(__nullified_fields__, 1);
}

#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
inline bool NullableE::IsNeBPresent(void)
{
    return (!GET_FIELD_NULLIFIED_BIT(__nullified_fields__, 1));
}
#endif
struct StructELarge
{
    uint32_t selA;
    uint32_t selB;
    uint32_t selC;
    uint32_t selD;
    uint32_t selF;
    bool selG;

    static const nl::SchemaFieldDescriptor FieldSchema;

};

struct StructELarge_array {
    uint32_t num;
    StructELarge *buf;
};

//
// Events
//
struct TestEEvent
{
    uint32_t teA;
    int32_t teB;
    bool teC;
    int32_t teD;
    Schema::Nest::Test::Trait::TestETrait::StructE teE;
    int32_t teF;
    Schema::Nest::Test::Trait::TestCommon::CommonStructE teG;
    nl::SerializedFieldTypeUInt32_array  teH;
    Schema::Nest::Test::Trait::TestCommon::CommonStructE_array teI;
    int16_t teJ;
    void SetTeJNull(void);
    void SetTeJPresent(void);
#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
    bool IsTeJPresent(void);
#endif
    nl::SerializedByteString teK;
    uint32_t teL;
    uint64_t teM;
    void SetTeMNull(void);
    void SetTeMPresent(void);
#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
    bool IsTeMPresent(void);
#endif
    nl::SerializedByteString teN;
    void SetTeNNull(void);
    void SetTeNPresent(void);
#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
    bool IsTeNPresent(void);
#endif
    uint32_t teO;
    int64_t teP;
    void SetTePNull(void);
    void SetTePPresent(void);
#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
    bool IsTePPresent(void);
#endif
    int64_t teQ;
    uint32_t teR;
    uint32_t teS;
    void SetTeSNull(void);
    void SetTeSPresent(void);
#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
    bool IsTeSPresent(void);
#endif
    uint32_t teT;
    uint8_t __nullified_fields__[5/8 + 1];

    static const nl::SchemaFieldDescriptor FieldSchema;

    // Statically-known Event Struct Attributes:
    enum {
            kWeaveProfileId = (0x235aU << 16) | 0xfe06U,
        kEventTypeId = 0x1U
    };

    static const nl::Weave::Profiles::DataManagement::EventSchema Schema;
};

struct TestEEvent_array {
    uint32_t num;
    TestEEvent *buf;
};

inline void TestEEvent::SetTeJNull(void)
{
    SET_FIELD_NULLIFIED_BIT(__nullified_fields__, 0);
}

inline void TestEEvent::SetTeJPresent(void)
{
    CLEAR_FIELD_NULLIFIED_BIT(__nullified_fields__, 0);
}

#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
inline bool TestEEvent::IsTeJPresent(void)
{
    return (!GET_FIELD_NULLIFIED_BIT(__nullified_fields__, 0));
}
#endif
inline void TestEEvent::SetTeMNull(void)
{
    SET_FIELD_NULLIFIED_BIT(__nullified_fields__, 1);
}

inline void TestEEvent::SetTeMPresent(void)
{
    CLEAR_FIELD_NULLIFIED_BIT(__nullified_fields__, 1);
}

#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
inline bool TestEEvent::IsTeMPresent(void)
{
    return (!GET_FIELD_NULLIFIED_BIT(__nullified_fields__, 1));
}
#endif
inline void TestEEvent::SetTeNNull(void)
{
    SET_FIELD_NULLIFIED_BIT(__nullified_fields__, 2);
}

inline void TestEEvent::SetTeNPresent(void)
{
    CLEAR_FIELD_NULLIFIED_BIT(__nullified_fields__, 2);
}

#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
inline bool TestEEvent::IsTeNPresent(void)
{
    return (!GET_FIELD_NULLIFIED_BIT(__nullified_fields__, 2));
}
#endif
inline void TestEEvent::SetTePNull(void)
{
    SET_FIELD_NULLIFIED_BIT(__nullified_fields__, 3);
}

inline void TestEEvent::SetTePPresent(void)
{
    CLEAR_FIELD_NULLIFIED_BIT(__nullified_fields__, 3);
}

#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
inline bool TestEEvent::IsTePPresent(void)
{
    return (!GET_FIELD_NULLIFIED_BIT(__nullified_fields__, 3));
}
#endif
inline void TestEEvent::SetTeSNull(void)
{
    SET_FIELD_NULLIFIED_BIT(__nullified_fields__, 4);
}

inline void TestEEvent::SetTeSPresent(void)
{
    CLEAR_FIELD_NULLIFIED_BIT(__nullified_fields__, 4);
}

#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
inline bool TestEEvent::IsTeSPresent(void)
{
    return (!GET_FIELD_NULLIFIED_BIT(__nullified_fields__, 4));
}
#endif

struct TestENullableEvent
{
    uint32_t neA;
    void SetNeANull(void);
    void SetNeAPresent(void);
#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
    bool IsNeAPresent(void);
#endif
    int32_t neB;
    void SetNeBNull(void);
    void SetNeBPresent(void);
#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
    bool IsNeBPresent(void);
#endif
    bool neC;
    void SetNeCNull(void);
    void SetNeCPresent(void);
#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
    bool IsNeCPresent(void);
#endif
    const char * neD;
    void SetNeDNull(void);
    void SetNeDPresent(void);
#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
    bool IsNeDPresent(void);
#endif
    int16_t neE;
    void SetNeENull(void);
    void SetNeEPresent(void);
#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
    bool IsNeEPresent(void);
#endif
    uint32_t neF;
    void SetNeFNull(void);
    void SetNeFPresent(void);
#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
    bool IsNeFPresent(void);
#endif
    int32_t neG;
    void SetNeGNull(void);
    void SetNeGPresent(void);
#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
    bool IsNeGPresent(void);
#endif
    bool neH;
    void SetNeHNull(void);
    void SetNeHPresent(void);
#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
    bool IsNeHPresent(void);
#endif
    const char * neI;
    void SetNeINull(void);
    void SetNeIPresent(void);
#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
    bool IsNeIPresent(void);
#endif
    Schema::Nest::Test::Trait::TestETrait::NullableE neJ;
    void SetNeJNull(void);
    void SetNeJPresent(void);
#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
    bool IsNeJPresent(void);
#endif
    uint8_t __nullified_fields__[10/8 + 1];

    static const nl::SchemaFieldDescriptor FieldSchema;

    // Statically-known Event Struct Attributes:
    enum {
            kWeaveProfileId = (0x235aU << 16) | 0xfe06U,
        kEventTypeId = 0x2U
    };

    static const nl::Weave::Profiles::DataManagement::EventSchema Schema;
};

struct TestENullableEvent_array {
    uint32_t num;
    TestENullableEvent *buf;
};

inline void TestENullableEvent::SetNeANull(void)
{
    SET_FIELD_NULLIFIED_BIT(__nullified_fields__, 0);
}

inline void TestENullableEvent::SetNeAPresent(void)
{
    CLEAR_FIELD_NULLIFIED_BIT(__nullified_fields__, 0);
}

#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
inline bool TestENullableEvent::IsNeAPresent(void)
{
    return (!GET_FIELD_NULLIFIED_BIT(__nullified_fields__, 0));
}
#endif
inline void TestENullableEvent::SetNeBNull(void)
{
    SET_FIELD_NULLIFIED_BIT(__nullified_fields__, 1);
}

inline void TestENullableEvent::SetNeBPresent(void)
{
    CLEAR_FIELD_NULLIFIED_BIT(__nullified_fields__, 1);
}

#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
inline bool TestENullableEvent::IsNeBPresent(void)
{
    return (!GET_FIELD_NULLIFIED_BIT(__nullified_fields__, 1));
}
#endif
inline void TestENullableEvent::SetNeCNull(void)
{
    SET_FIELD_NULLIFIED_BIT(__nullified_fields__, 2);
}

inline void TestENullableEvent::SetNeCPresent(void)
{
    CLEAR_FIELD_NULLIFIED_BIT(__nullified_fields__, 2);
}

#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
inline bool TestENullableEvent::IsNeCPresent(void)
{
    return (!GET_FIELD_NULLIFIED_BIT(__nullified_fields__, 2));
}
#endif
inline void TestENullableEvent::SetNeDNull(void)
{
    SET_FIELD_NULLIFIED_BIT(__nullified_fields__, 3);
}

inline void TestENullableEvent::SetNeDPresent(void)
{
    CLEAR_FIELD_NULLIFIED_BIT(__nullified_fields__, 3);
}

#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
inline bool TestENullableEvent::IsNeDPresent(void)
{
    return (!GET_FIELD_NULLIFIED_BIT(__nullified_fields__, 3));
}
#endif
inline void TestENullableEvent::SetNeENull(void)
{
    SET_FIELD_NULLIFIED_BIT(__nullified_fields__, 4);
}

inline void TestENullableEvent::SetNeEPresent(void)
{
    CLEAR_FIELD_NULLIFIED_BIT(__nullified_fields__, 4);
}

#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
inline bool TestENullableEvent::IsNeEPresent(void)
{
    return (!GET_FIELD_NULLIFIED_BIT(__nullified_fields__, 4));
}
#endif
inline void TestENullableEvent::SetNeFNull(void)
{
    SET_FIELD_NULLIFIED_BIT(__nullified_fields__, 5);
}

inline void TestENullableEvent::SetNeFPresent(void)
{
    CLEAR_FIELD_NULLIFIED_BIT(__nullified_fields__, 5);
}

#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
inline bool TestENullableEvent::IsNeFPresent(void)
{
    return (!GET_FIELD_NULLIFIED_BIT(__nullified_fields__, 5));
}
#endif
inline void TestENullableEvent::SetNeGNull(void)
{
    SET_FIELD_NULLIFIED_BIT(__nullified_fields__, 6);
}

inline void TestENullableEvent::SetNeGPresent(void)
{
    CLEAR_FIELD_NULLIFIED_BIT(__nullified_fields__, 6);
}

#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
inline bool TestENullableEvent::IsNeGPresent(void)
{
    return (!GET_FIELD_NULLIFIED_BIT(__nullified_fields__, 6));
}
#endif
inline void TestENullableEvent::SetNeHNull(void)
{
    SET_FIELD_NULLIFIED_BIT(__nullified_fields__, 7);
}

inline void TestENullableEvent::SetNeHPresent(void)
{
    CLEAR_FIELD_NULLIFIED_BIT(__nullified_fields__, 7);
}

#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
inline bool TestENullableEvent::IsNeHPresent(void)
{
    return (!GET_FIELD_NULLIFIED_BIT(__nullified_fields__, 7));
}
#endif
inline void TestENullableEvent::SetNeINull(void)
{
    SET_FIELD_NULLIFIED_BIT(__nullified_fields__, 8);
}

inline void TestENullableEvent::SetNeIPresent(void)
{
    CLEAR_FIELD_NULLIFIED_BIT(__nullified_fields__, 8);
}

#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
inline bool TestENullableEvent::IsNeIPresent(void)
{
    return (!GET_FIELD_NULLIFIED_BIT(__nullified_fields__, 8));
}
#endif
inline void TestENullableEvent::SetNeJNull(void)
{
    SET_FIELD_NULLIFIED_BIT(__nullified_fields__, 9);
}

inline void TestENullableEvent::SetNeJPresent(void)
{
    CLEAR_FIELD_NULLIFIED_BIT(__nullified_fields__, 9);
}

#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
inline bool TestENullableEvent::IsNeJPresent(void)
{
    return (!GET_FIELD_NULLIFIED_BIT(__nullified_fields__, 9));
}
#endif

struct TestEEmptyEvent
{

    static const nl::SchemaFieldDescriptor FieldSchema;

    // Statically-known Event Struct Attributes:
    enum {
            kWeaveProfileId = (0x235aU << 16) | 0xfe06U,
        kEventTypeId = 0x3U
    };

    static const nl::Weave::Profiles::DataManagement::EventSchema Schema;
};

struct TestEEmptyEvent_array {
    uint32_t num;
    TestEEmptyEvent *buf;
};


struct TestELargeArrayNullableEvent
{
    uint32_t telaneA;
    void SetTelaneANull(void);
    void SetTelaneAPresent(void);
#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
    bool IsTelaneAPresent(void);
#endif
    Schema::Nest::Test::Trait::TestETrait::StructELarge_array telaneB;
    uint8_t __nullified_fields__[1/8 + 1];

    static const nl::SchemaFieldDescriptor FieldSchema;

    // Statically-known Event Struct Attributes:
    enum {
            kWeaveProfileId = (0x235aU << 16) | 0xfe06U,
        kEventTypeId = 0x4U
    };

    static const nl::Weave::Profiles::DataManagement::EventSchema Schema;
};

struct TestELargeArrayNullableEvent_array {
    uint32_t num;
    TestELargeArrayNullableEvent *buf;
};

inline void TestELargeArrayNullableEvent::SetTelaneANull(void)
{
    SET_FIELD_NULLIFIED_BIT(__nullified_fields__, 0);
}

inline void TestELargeArrayNullableEvent::SetTelaneAPresent(void)
{
    CLEAR_FIELD_NULLIFIED_BIT(__nullified_fields__, 0);
}

#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
inline bool TestELargeArrayNullableEvent::IsTelaneAPresent(void)
{
    return (!GET_FIELD_NULLIFIED_BIT(__nullified_fields__, 0));
}
#endif

struct TestESmallArrayNullableEvent
{
    uint32_t tesaneA;
    void SetTesaneANull(void);
    void SetTesaneAPresent(void);
#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
    bool IsTesaneAPresent(void);
#endif
    nl::SerializedFieldTypeBoolean_array  tesaneB;
    uint8_t __nullified_fields__[1/8 + 1];

    static const nl::SchemaFieldDescriptor FieldSchema;

    // Statically-known Event Struct Attributes:
    enum {
            kWeaveProfileId = (0x235aU << 16) | 0xfe06U,
        kEventTypeId = 0x5U
    };

    static const nl::Weave::Profiles::DataManagement::EventSchema Schema;
};

struct TestESmallArrayNullableEvent_array {
    uint32_t num;
    TestESmallArrayNullableEvent *buf;
};

inline void TestESmallArrayNullableEvent::SetTesaneANull(void)
{
    SET_FIELD_NULLIFIED_BIT(__nullified_fields__, 0);
}

inline void TestESmallArrayNullableEvent::SetTesaneAPresent(void)
{
    CLEAR_FIELD_NULLIFIED_BIT(__nullified_fields__, 0);
}

#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
inline bool TestESmallArrayNullableEvent::IsTesaneAPresent(void)
{
    return (!GET_FIELD_NULLIFIED_BIT(__nullified_fields__, 0));
}
#endif

//
// Enums
//

enum EnumE {
    ENUM_E_VALUE_1 = 1,
    ENUM_E_VALUE_2 = 2,
    ENUM_E_VALUE_3 = 3,
    ENUM_E_VALUE_4 = 4,
};

} // namespace TestETrait
} // namespace Trait
} // namespace Test
} // namespace Nest
} // namespace Schema
#endif // _NEST_TEST_TRAIT__TEST_E_TRAIT_H_
