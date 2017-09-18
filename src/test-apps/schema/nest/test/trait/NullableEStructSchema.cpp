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
#include <nest/test/trait/NullableEStructSchema.h>

namespace Schema {
namespace Nest {
namespace Test {
namespace Trait {
namespace TestETrait {

    const nl::FieldDescriptor NullableEFieldDescriptors[] =
    {
        {
            NULL, offsetof(NullableE, neA), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt32, 1), 1
        },
        {
            NULL, offsetof(NullableE, neB), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeBoolean, 1), 2
        },
    };

    const nl::SchemaFieldDescriptor NullableE::FieldSchema =
    {
        .mNumFieldDescriptorElements = sizeof(NullableEFieldDescriptors)/sizeof(NullableEFieldDescriptors[0]),
        .mFields = NullableEFieldDescriptors,
        .mSize = sizeof(NullableE)
    };
}
}
}
}
}
