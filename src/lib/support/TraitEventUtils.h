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
 *   Templated functions for type-safe usage of TraitEvents
 */

#ifndef TRAITEVENT_UTILS_H
#define TRAITEVENT_UTILS_H

#include <Weave/Support/SerializationUtils.h>
#include <Weave/Profiles/data-management/DataManagement.h>

namespace nl {

template < class TEvent >
nl::Weave::Profiles::DataManagement::event_id_t LogEvent(TEvent* aEvent)
{
    nl::StructureSchemaPointerPair structureSchemaPair = {
        (void *)aEvent,
        &TEvent::FieldSchema
    };

    return nl::Weave::Profiles::DataManagement::LogEvent(TEvent::Schema,
        nl::SerializedDataToTLVWriterHelper,
        (void *)&structureSchemaPair);
}

/**
 *  @def NullifyAllEventFields
 *
 *  @brief
 *    Convenience setter function to set all nullable fields within an event to NULL
 *    It does so by setting the '__nullified_fields' member within the code-generated
 *    structure to 0xFFs (bit set = null, bit cleared = not-null).
 */
template < class TEvent >
void NullifyAllEventFields(TEvent *aEvent)
{
    memset(aEvent->__nullified_fields__, 0xff, sizeof(aEvent->__nullified_fields__));
}

template < class TEvent >
nl::Weave::Profiles::DataManagement::event_id_t LogEvent(TEvent* aEvent,
    const nl::Weave::Profiles::DataManagement::EventOptions& aOptions)
{
    nl::StructureSchemaPointerPair structureSchemaPair = {
        (void *)aEvent,
        &TEvent::FieldSchema
    };

    return nl::Weave::Profiles::DataManagement::LogEvent(TEvent::Schema,
        nl::SerializedDataToTLVWriterHelper,
        (void *)&structureSchemaPair,
        &aOptions);
}

#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
template < class TEvent >
WEAVE_ERROR DeserializeEvent(nl::Weave::TLV::TLVReader &aReader,
    TEvent *aEvent,
    nl::SerializationContext *aContext)
{
    nl::StructureSchemaPointerPair structureSchemaPair = {
        (void *)aEvent,
        &TEvent::FieldSchema
    };

    return nl::TLVReaderToDeserializedDataHelper(aReader,
        nl::Weave::Profiles::DataManagement::kTag_EventData,
        (void *)&structureSchemaPair,
        aContext);
}

template < class TEvent >
WEAVE_ERROR DeallocateEvent(TEvent *aEvent,
    nl::SerializationContext *aContext)
{
    return nl::DeallocateDeserializedStructure(aEvent,
        &TEvent::FieldSchema,
        aContext);
}
#endif // WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION

} // nl
#endif // TRAITEVENT_UTILS_H
