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

/**
 * @file
 *
 * @brief
 *   Sample Schema Definitions - Open Close for Pinna and a hypothetical SampleTrait
 *
 */

#ifndef _PROFILE_WEAVE_EVENT_LOGGING_SCHEMA_DEFINITIONS_H
#define _PROFILE_WEAVE_EVENT_LOGGING_SCHEMA_DEFINITIONS_H

#include <Weave/Support/SerializationUtils.h>
#include <Weave/Profiles/data-management/DataManagement.h>

namespace nl {
namespace Weave {
namespace Profiles {
namespace WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current) {

// GENERATED
/**********************************/
/*       START SAMPLE TRAIT       */
/**********************************/
// Trait we're trying to log:
// SampleTrait {
//  profileId = 0x200;
//  messageId = 0x1;
//  Event {
//   uint32_t state = 1;
//   uint32_t timestamp = 2;
//   eventStruct structure = 3;
//   array-of-uint32_t samples = 4;
//  }
//
//  eventStruct {
//   bool a = 1;
//   eventStats b = 2;
//  }
//
//  eventStats  {
//   char *str = 1;
//  }
// }

/*********************/
/*     C-STRUCTS     */
/*********************/
struct eventStats
{
    const char *str;
};

struct eventStruct
{
    bool a;
    eventStats b;
};

namespace SampleTrait
{
    struct samplesArray
    {
        uint32_t num_samples;
        uint32_t *samples_buf;
    };

    struct Event
    {
        uint32_t state;
        uint32_t timestamp;

        eventStruct structure;

        // Array of elements
        samplesArray samples;
    };

};

/************************/
/*  SCHEMA DESCRIPTORS  */
/************************/
nl::FieldDescriptor eventStatsFieldDescriptors[] =
{
    // STR
    {
        NULL, offsetof(eventStats, str), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUTF8String, 0), 1
    }
};

// Schema Descriptors
const nl::SchemaFieldDescriptor eventStatsSchema =
{
    .mNumFieldDescriptorElements = sizeof(eventStatsFieldDescriptors)/sizeof(eventStatsFieldDescriptors[0]),

    .mFields = eventStatsFieldDescriptors,

    .mSize = sizeof(eventStats)
};

nl::FieldDescriptor eventStructFieldDescriptors[] =
{
    // A
    {
        NULL, offsetof(eventStruct, a), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeBoolean, 0), 1
    },
    // STRUCTURE (eventStats)
    {
        &eventStatsSchema, offsetof(eventStruct, b), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeStructure, 0), 2
    }
};

const nl::SchemaFieldDescriptor eventStructSchema =
{
    .mNumFieldDescriptorElements = sizeof(eventStructFieldDescriptors)/sizeof(eventStructFieldDescriptors[0]),

    .mFields = eventStructFieldDescriptors,

    .mSize = sizeof(eventStruct)
};

nl::FieldDescriptor sampleEventFieldDescriptors[] =
{
    // STATE
    {
        NULL, offsetof(SampleTrait::Event, state), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt32, 0), 1
    },
    // TIMESTAMP
    {
        NULL, offsetof(SampleTrait::Event, timestamp), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt32, 0), 2
    },
    // STRUCTURE (eventStruct)
    {
        &eventStructSchema, offsetof(SampleTrait::Event, structure), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeStructure, 0), 3
    },
    // ARRAY (samples)
    {
        NULL, offsetof(SampleTrait::Event, samples) + offsetof(SampleTrait::samplesArray, num_samples), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeArray, 0), 4
    },
    // SAMPLES TYPE
    // This is a little weird, we need to somehow convey that it is an array of ____
    {
        NULL, offsetof(SampleTrait::Event, samples) + offsetof(SampleTrait::samplesArray, samples_buf), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt32, 0), 4
    }
};

const nl::SchemaFieldDescriptor sampleEventSchema =
{
    .mNumFieldDescriptorElements = sizeof(sampleEventFieldDescriptors)/sizeof(sampleEventFieldDescriptors[0]),

    .mFields = sampleEventFieldDescriptors,

    .mSize = sizeof(SampleTrait::Event)
};

const nl::Weave::Profiles::DataManagement::EventSchema sampleSchema =
{
    .mProfileId = 0x200,
    .mStructureType = 0x1,
    .mImportance = nl::Weave::Profiles::DataManagement::Production,
};

inline event_id_t LogSampleEvent(SampleTrait::Event *aEvent, ImportanceType aImportance)
{
    nl::StructureSchemaPointerPair eventSchemaPair = { (void *)aEvent, &sampleEventSchema };

    return LogEvent(sampleSchema, SerializedDataToTLVWriterHelper, (void *)&eventSchemaPair);
}

inline WEAVE_ERROR DeserializeSampleEvent(nl::Weave::TLV::TLVReader &aReader, SampleTrait::Event *aEvent, nl::SerializationContext *aContext = NULL)
{
    nl::StructureSchemaPointerPair eventSchemaPair = { (void *)aEvent, &sampleEventSchema };

    return nl::TLVReaderToDeserializedDataHelper(aReader, kTag_EventData, (void *)&eventSchemaPair, aContext);
}

/**********************************/
/*        END SAMPLE TRAIT        */
/**********************************/



/**********************************/
/*      START OPENCLOSE TRAIT     */
/**********************************/
// Trait we're trying to log:
// https://stash.nestlabs.com/projects/PHX/repos/phoenix-schema/browse/schema/src/maldives_prototype/trait/open_close_trait.proto
// OpenCloseTrait {
//  profileId = 0x0208;
//  eventType = 0x1;
//  Event {
//   OpenCloseState open_close_state = 1;
//  }
// }

namespace OpenCloseTrait
{
    struct Event
    {
        uint32_t state;
    };
};

FieldDescriptor openCloseEventFieldDescriptors[] =
{
    // STATE
    {
        NULL, offsetof(OpenCloseTrait::Event, state), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt32, 0), 1
    },
};

const nl::SchemaFieldDescriptor openCloseEventSchema =
{
    .mNumFieldDescriptorElements = sizeof(openCloseEventFieldDescriptors)/sizeof(openCloseEventFieldDescriptors[0]),

    .mFields = openCloseEventFieldDescriptors,

    .mSize = sizeof(OpenCloseTrait::Event)
};

const nl::Weave::Profiles::DataManagement::EventSchema openCloseSchema =
{
    .mProfileId = 0x208,
    .mStructureType = 0x1,
    .mImportance = nl::Weave::Profiles::DataManagement::Production,
};

inline event_id_t LogOpenCloseEvent(OpenCloseTrait::Event *aEvent, ImportanceType aImportance)
{
    nl::StructureSchemaPointerPair eventSchemaPair = { (void *)aEvent, &openCloseEventSchema };

    return LogEvent(openCloseSchema, SerializedDataToTLVWriterHelper, (void *)&eventSchemaPair);
}

/**********************************/
/*       END OPENCLOSE TRAIT      */
/**********************************/
// END GENERATED

namespace ByteStringTestTrait
{
    struct Event
    {
        nl::SerializedByteString byte_string;
    };
};

FieldDescriptor ByteStringTestEventFieldDescriptors[] =
{
    {
        NULL, offsetof(ByteStringTestTrait::Event, byte_string), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeByteString, 0), 1
    },
};

const nl::SchemaFieldDescriptor ByteStringTestEventSchema =
{
    .mNumFieldDescriptorElements = sizeof(ByteStringTestEventFieldDescriptors)/sizeof(ByteStringTestEventFieldDescriptors[0]),

    .mFields = ByteStringTestEventFieldDescriptors,

    .mSize = sizeof(ByteStringTestTrait::Event)
};

const nl::Weave::Profiles::DataManagement::EventSchema ByteStringTestSchema =
{
    .mProfileId = 0x209,
    .mStructureType = 0x1,
    .mImportance = nl::Weave::Profiles::DataManagement::Production,
};

inline event_id_t LogByteStringTestEvent(ByteStringTestTrait::Event *aEvent)
{
    nl::StructureSchemaPointerPair eventSchemaPair = { (void *)aEvent, &ByteStringTestEventSchema };

    return LogEvent(ByteStringTestSchema, SerializedDataToTLVWriterHelper, (void *)&eventSchemaPair);
}

inline WEAVE_ERROR DeserializeByteStringTestEvent(nl::Weave::TLV::TLVReader &aReader, ByteStringTestTrait::Event *aEvent, nl::SerializationContext *aContext = NULL)
{
    nl::StructureSchemaPointerPair eventSchemaPair = { (void *)aEvent, &ByteStringTestEventSchema };

    return nl::TLVReaderToDeserializedDataHelper(aReader, kTag_EventData, (void *)&eventSchemaPair, aContext);
}

namespace ByteStringArrayTestTrait
{
    struct byteString_array {
        uint32_t num;
        nl::SerializedByteString *buf;
    };

    struct Event
    {
        byteString_array testArray;
    };
};

FieldDescriptor ByteStringArrayTestEventFieldDescriptors[] =
{
    {
        NULL, offsetof(ByteStringArrayTestTrait::Event, testArray) + offsetof(ByteStringArrayTestTrait::byteString_array, num), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeArray, 0), 1
    },
    {
        NULL, offsetof(ByteStringArrayTestTrait::Event, testArray) + offsetof(ByteStringArrayTestTrait::byteString_array, buf), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeByteString, 0), 1
    },
};

const nl::SchemaFieldDescriptor ByteStringArrayTestEventSchema =
{
    .mNumFieldDescriptorElements = sizeof(ByteStringArrayTestEventFieldDescriptors)/sizeof(ByteStringArrayTestEventFieldDescriptors[0]),

    .mFields = ByteStringArrayTestEventFieldDescriptors,

    .mSize = sizeof(ByteStringArrayTestTrait::Event)
};

const nl::Weave::Profiles::DataManagement::EventSchema ByteStringArrayTestSchema =
{
    .mProfileId = 0x209,
    .mStructureType = 0x1,
    .mImportance = nl::Weave::Profiles::DataManagement::Production,
};

inline event_id_t LogByteStringArrayTestEvent(ByteStringArrayTestTrait::Event *aEvent)
{
    nl::StructureSchemaPointerPair eventSchemaPair = { (void *)aEvent, &ByteStringArrayTestEventSchema };

    return LogEvent(ByteStringArrayTestSchema, SerializedDataToTLVWriterHelper, (void *)&eventSchemaPair);
}

inline WEAVE_ERROR DeserializeByteStringArrayTestEvent(nl::Weave::TLV::TLVReader &aReader, ByteStringArrayTestTrait::Event *aEvent, nl::SerializationContext *aContext = NULL)
{
    nl::StructureSchemaPointerPair eventSchemaPair = { (void *)aEvent, &ByteStringArrayTestEventSchema };

    return nl::TLVReaderToDeserializedDataHelper(aReader, kTag_EventData, (void *)&eventSchemaPair, aContext);
}

} // WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current)
} // Profiles
} // Weave
} // nl

#endif //_PROFILE_WEAVE_EVENT_LOGGING_SCHEMA_DEFINITIONS_H
