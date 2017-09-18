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
#include <nest/test/trait/StructEAStructSchema.h>

namespace Schema {
namespace Nest {
namespace Test {
namespace Trait {
namespace TestBTrait {

    const nl::FieldDescriptor StructEAFieldDescriptors[] =
    {
        {
            NULL, offsetof(StructEA, saA), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt32, 1), 1
        },
        {
            NULL, offsetof(StructEA, saB), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeBoolean, 0), 2
        },
        {
            NULL, offsetof(StructEA, seaC), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUTF8String, 0), 32
        },
    };

    const nl::SchemaFieldDescriptor StructEA::FieldSchema =
    {
        .mNumFieldDescriptorElements = sizeof(StructEAFieldDescriptors)/sizeof(StructEAFieldDescriptors[0]),
        .mFields = StructEAFieldDescriptors,
        .mSize = sizeof(StructEA)
    };
}
}
}
}
}
