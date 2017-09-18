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
#ifndef _NEST_TEST_TRAIT__TEST_E_TRAIT__NULLABLE_E_STRUCT_SCHEMA_H_
#define _NEST_TEST_TRAIT__TEST_E_TRAIT__NULLABLE_E_STRUCT_SCHEMA_H_

#include <Weave/Support/SerializationUtils.h>
#include <Weave/Profiles/data-management/DataManagement.h>

namespace Schema {
namespace Nest {
namespace Test {
namespace Trait {
namespace TestETrait {

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

}
}
}
}

}
#endif // _NEST_TEST_TRAIT__TEST_E_TRAIT__NULLABLE_E_STRUCT_SCHEMA_H_
