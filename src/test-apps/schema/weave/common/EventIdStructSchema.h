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
 *    SOURCE TEMPLATE: struct.cpp.h
 *    SOURCE PROTO: weave/common/identifiers.proto
 *
 */
#ifndef _WEAVE_COMMON__EVENT_ID_STRUCT_SCHEMA_H_
#define _WEAVE_COMMON__EVENT_ID_STRUCT_SCHEMA_H_

#include <Weave/Support/SerializationUtils.h>
#include <Weave/Profiles/data-management/DataManagement.h>



namespace Schema {
namespace Weave {
namespace Common {

struct EventId
{
    nl::SerializedByteString resourceId;
    void SetResourceIdNull(void);
    void SetResourceIdPresent(void);
#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
    bool IsResourceIdPresent(void);
#endif
    int32_t importance;
    uint64_t id;
    uint8_t __nullified_fields__[1/8 + 1];

    static const nl::SchemaFieldDescriptor FieldSchema;

};

struct EventId_array {
    uint32_t num;
    EventId *buf;
};

inline void EventId::SetResourceIdNull(void)
{
    SET_FIELD_NULLIFIED_BIT(__nullified_fields__, 0);
}

inline void EventId::SetResourceIdPresent(void)
{
    CLEAR_FIELD_NULLIFIED_BIT(__nullified_fields__, 0);
}

#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
inline bool EventId::IsResourceIdPresent(void)
{
    return (!GET_FIELD_NULLIFIED_BIT(__nullified_fields__, 0));
}
#endif


} // namespace Common
} // namespace Weave
} // namespace Schema
#endif // _WEAVE_COMMON__EVENT_ID_STRUCT_SCHEMA_H_
