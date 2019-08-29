
/**
 *    Copyright (c) 2019 Nest Labs, Inc.
 *    All rights reserved.
 *
 *    THIS FILE IS GENERATED. DO NOT MODIFY.
 *
 *    SOURCE TEMPLATE: trait.cpp
 *    SOURCE PROTO: nest/trait/firmware/software_update_trait.proto
 *
 */

#include <nest/trait/firmware/SoftwareUpdateTrait.h>

namespace Schema {
namespace Nest {
namespace Trait {
namespace Firmware {
namespace SoftwareUpdateTrait {

using namespace ::nl::Weave::Profiles::DataManagement;

//
// Property Table
//

const TraitSchemaEngine::PropertyInfo PropertyMap[] = {
    { kPropertyHandle_Root, 1 }, // last_update_time
};

//
// IsOptional Table
//

uint8_t IsOptionalHandleBitfield[] = {
        0x1
};

//
// IsNullable Table
//

uint8_t IsNullableHandleBitfield[] = {
        0x1
};

//
// Supported version
//
const ConstSchemaVersionRange traitVersion = { .mMinVersion = 1, .mMaxVersion = 2 };

//
// Schema
//

const TraitSchemaEngine TraitSchema = {
    {
        kWeaveProfileId,
        PropertyMap,
        sizeof(PropertyMap) / sizeof(PropertyMap[0]),
        1,
#if (TDM_EXTENSION_SUPPORT) || (TDM_VERSIONING_SUPPORT)
        2,
#endif
        NULL,
        &IsOptionalHandleBitfield[0],
        NULL,
        &IsNullableHandleBitfield[0],
        NULL,
#if (TDM_EXTENSION_SUPPORT)
        NULL,
#endif
#if (TDM_VERSIONING_SUPPORT)
        &traitVersion,
#endif
    }
};

    //
    // Events
    //

const nl::FieldDescriptor SoftwareUpdateStartEventFieldDescriptors[] =
{
    {
        NULL, offsetof(SoftwareUpdateStartEvent, trigger), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeInt32, 0), 1
    },

};

const nl::SchemaFieldDescriptor SoftwareUpdateStartEvent::FieldSchema =
{
    .mNumFieldDescriptorElements = sizeof(SoftwareUpdateStartEventFieldDescriptors)/sizeof(SoftwareUpdateStartEventFieldDescriptors[0]),
    .mFields = SoftwareUpdateStartEventFieldDescriptors,
    .mSize = sizeof(SoftwareUpdateStartEvent)
};
const nl::Weave::Profiles::DataManagement::EventSchema SoftwareUpdateStartEvent::Schema =
{
    .mProfileId = kWeaveProfileId,
    .mStructureType = 0x1,
    .mImportance = nl::Weave::Profiles::DataManagement::Production,
    .mDataSchemaVersion = 2,
    .mMinCompatibleDataSchemaVersion = 1,
};

const nl::FieldDescriptor FailureEventFieldDescriptors[] =
{
    {
        NULL, offsetof(FailureEvent, state), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeInt32, 0), 1
    },

    {
        NULL, offsetof(FailureEvent, platformReturnCode), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt32, 0), 2
    },

    {
        &Schema::Weave::Common::ProfileSpecificStatusCode::FieldSchema, offsetof(FailureEvent, primaryStatusCode), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeStructure, 1), 3
    },

    {
        &Schema::Weave::Common::ProfileSpecificStatusCode::FieldSchema, offsetof(FailureEvent, remoteStatusCode), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeStructure, 1), 4
    },

};

const nl::SchemaFieldDescriptor FailureEvent::FieldSchema =
{
    .mNumFieldDescriptorElements = sizeof(FailureEventFieldDescriptors)/sizeof(FailureEventFieldDescriptors[0]),
    .mFields = FailureEventFieldDescriptors,
    .mSize = sizeof(FailureEvent)
};
const nl::Weave::Profiles::DataManagement::EventSchema FailureEvent::Schema =
{
    .mProfileId = kWeaveProfileId,
    .mStructureType = 0x2,
    .mImportance = nl::Weave::Profiles::DataManagement::Production,
    .mDataSchemaVersion = 2,
    .mMinCompatibleDataSchemaVersion = 1,
};

const nl::FieldDescriptor DownloadFailureEventFieldDescriptors[] =
{
    {
        NULL, offsetof(DownloadFailureEvent, state), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeInt32, 0), 1
    },

    {
        NULL, offsetof(DownloadFailureEvent, platformReturnCode), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt32, 0), 2
    },

    {
        &Schema::Weave::Common::ProfileSpecificStatusCode::FieldSchema, offsetof(DownloadFailureEvent, primaryStatusCode), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeStructure, 1), 3
    },

    {
        &Schema::Weave::Common::ProfileSpecificStatusCode::FieldSchema, offsetof(DownloadFailureEvent, remoteStatusCode), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeStructure, 1), 4
    },

    {
        NULL, offsetof(DownloadFailureEvent, bytesDownloaded), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt64, 0), 32
    },

};

const nl::SchemaFieldDescriptor DownloadFailureEvent::FieldSchema =
{
    .mNumFieldDescriptorElements = sizeof(DownloadFailureEventFieldDescriptors)/sizeof(DownloadFailureEventFieldDescriptors[0]),
    .mFields = DownloadFailureEventFieldDescriptors,
    .mSize = sizeof(DownloadFailureEvent)
};
const nl::Weave::Profiles::DataManagement::EventSchema DownloadFailureEvent::Schema =
{
    .mProfileId = kWeaveProfileId,
    .mStructureType = 0x3,
    .mImportance = nl::Weave::Profiles::DataManagement::Production,
    .mDataSchemaVersion = 2,
    .mMinCompatibleDataSchemaVersion = 1,
};

const nl::FieldDescriptor QueryBeginEventFieldDescriptors[] =
{
    {
        NULL, offsetof(QueryBeginEvent, currentSwVersion), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUTF8String, 1), 2
    },

    {
        NULL, offsetof(QueryBeginEvent, vendorId), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt16, 1), 3
    },

    {
        NULL, offsetof(QueryBeginEvent, vendorProductId), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt16, 1), 4
    },

    {
        NULL, offsetof(QueryBeginEvent, productRevision), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt16, 1), 5
    },

    {
        NULL, offsetof(QueryBeginEvent, locale), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUTF8String, 1), 6
    },

    {
        NULL, offsetof(QueryBeginEvent, queryServerAddr), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUTF8String, 1), 7
    },

    {
        NULL, offsetof(QueryBeginEvent, queryServerId), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeByteString, 1), 8
    },

};

const nl::SchemaFieldDescriptor QueryBeginEvent::FieldSchema =
{
    .mNumFieldDescriptorElements = sizeof(QueryBeginEventFieldDescriptors)/sizeof(QueryBeginEventFieldDescriptors[0]),
    .mFields = QueryBeginEventFieldDescriptors,
    .mSize = sizeof(QueryBeginEvent)
};
const nl::Weave::Profiles::DataManagement::EventSchema QueryBeginEvent::Schema =
{
    .mProfileId = kWeaveProfileId,
    .mStructureType = 0x4,
    .mImportance = nl::Weave::Profiles::DataManagement::Info,
    .mDataSchemaVersion = 2,
    .mMinCompatibleDataSchemaVersion = 1,
};

const nl::FieldDescriptor QueryFinishEventFieldDescriptors[] =
{
    {
        NULL, offsetof(QueryFinishEvent, imageUrl), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUTF8String, 1), 2
    },

    {
        NULL, offsetof(QueryFinishEvent, imageVersion), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUTF8String, 1), 1
    },

};

const nl::SchemaFieldDescriptor QueryFinishEvent::FieldSchema =
{
    .mNumFieldDescriptorElements = sizeof(QueryFinishEventFieldDescriptors)/sizeof(QueryFinishEventFieldDescriptors[0]),
    .mFields = QueryFinishEventFieldDescriptors,
    .mSize = sizeof(QueryFinishEvent)
};
const nl::Weave::Profiles::DataManagement::EventSchema QueryFinishEvent::Schema =
{
    .mProfileId = kWeaveProfileId,
    .mStructureType = 0x5,
    .mImportance = nl::Weave::Profiles::DataManagement::Info,
    .mDataSchemaVersion = 2,
    .mMinCompatibleDataSchemaVersion = 1,
};

const nl::FieldDescriptor DownloadStartEventFieldDescriptors[] =
{
    {
        NULL, offsetof(DownloadStartEvent, imageUrl), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUTF8String, 1), 2
    },

    {
        NULL, offsetof(DownloadStartEvent, imageVersion), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUTF8String, 1), 1
    },

    {
        NULL, offsetof(DownloadStartEvent, subImageName), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUTF8String, 1), 3
    },

    {
        NULL, offsetof(DownloadStartEvent, offset), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUInt64, 1), 4
    },

    {
        NULL, offsetof(DownloadStartEvent, destination), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUTF8String, 1), 5
    },

};

const nl::SchemaFieldDescriptor DownloadStartEvent::FieldSchema =
{
    .mNumFieldDescriptorElements = sizeof(DownloadStartEventFieldDescriptors)/sizeof(DownloadStartEventFieldDescriptors[0]),
    .mFields = DownloadStartEventFieldDescriptors,
    .mSize = sizeof(DownloadStartEvent)
};
const nl::Weave::Profiles::DataManagement::EventSchema DownloadStartEvent::Schema =
{
    .mProfileId = kWeaveProfileId,
    .mStructureType = 0x6,
    .mImportance = nl::Weave::Profiles::DataManagement::Info,
    .mDataSchemaVersion = 2,
    .mMinCompatibleDataSchemaVersion = 1,
};

const nl::FieldDescriptor DownloadFinishEventFieldDescriptors[] =
{
    {
        NULL, offsetof(DownloadFinishEvent, imageUrl), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUTF8String, 1), 2
    },

    {
        NULL, offsetof(DownloadFinishEvent, imageVersion), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUTF8String, 1), 1
    },

    {
        NULL, offsetof(DownloadFinishEvent, subImageName), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUTF8String, 1), 3
    },

    {
        NULL, offsetof(DownloadFinishEvent, destination), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUTF8String, 1), 4
    },

};

const nl::SchemaFieldDescriptor DownloadFinishEvent::FieldSchema =
{
    .mNumFieldDescriptorElements = sizeof(DownloadFinishEventFieldDescriptors)/sizeof(DownloadFinishEventFieldDescriptors[0]),
    .mFields = DownloadFinishEventFieldDescriptors,
    .mSize = sizeof(DownloadFinishEvent)
};
const nl::Weave::Profiles::DataManagement::EventSchema DownloadFinishEvent::Schema =
{
    .mProfileId = kWeaveProfileId,
    .mStructureType = 0x7,
    .mImportance = nl::Weave::Profiles::DataManagement::Info,
    .mDataSchemaVersion = 2,
    .mMinCompatibleDataSchemaVersion = 1,
};

const nl::FieldDescriptor InstallStartEventFieldDescriptors[] =
{
    {
        NULL, offsetof(InstallStartEvent, imageVersion), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUTF8String, 1), 2
    },

    {
        NULL, offsetof(InstallStartEvent, subImageName), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUTF8String, 1), 3
    },

    {
        NULL, offsetof(InstallStartEvent, localSource), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUTF8String, 1), 4
    },

    {
        NULL, offsetof(InstallStartEvent, destination), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUTF8String, 1), 5
    },

};

const nl::SchemaFieldDescriptor InstallStartEvent::FieldSchema =
{
    .mNumFieldDescriptorElements = sizeof(InstallStartEventFieldDescriptors)/sizeof(InstallStartEventFieldDescriptors[0]),
    .mFields = InstallStartEventFieldDescriptors,
    .mSize = sizeof(InstallStartEvent)
};
const nl::Weave::Profiles::DataManagement::EventSchema InstallStartEvent::Schema =
{
    .mProfileId = kWeaveProfileId,
    .mStructureType = 0x8,
    .mImportance = nl::Weave::Profiles::DataManagement::Info,
    .mDataSchemaVersion = 2,
    .mMinCompatibleDataSchemaVersion = 1,
};

const nl::FieldDescriptor InstallFinishEventFieldDescriptors[] =
{
    {
        NULL, offsetof(InstallFinishEvent, imageVersion), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUTF8String, 1), 2
    },

    {
        NULL, offsetof(InstallFinishEvent, subImageName), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUTF8String, 1), 3
    },

};

const nl::SchemaFieldDescriptor InstallFinishEvent::FieldSchema =
{
    .mNumFieldDescriptorElements = sizeof(InstallFinishEventFieldDescriptors)/sizeof(InstallFinishEventFieldDescriptors[0]),
    .mFields = InstallFinishEventFieldDescriptors,
    .mSize = sizeof(InstallFinishEvent)
};
const nl::Weave::Profiles::DataManagement::EventSchema InstallFinishEvent::Schema =
{
    .mProfileId = kWeaveProfileId,
    .mStructureType = 0x9,
    .mImportance = nl::Weave::Profiles::DataManagement::Production,
    .mDataSchemaVersion = 2,
    .mMinCompatibleDataSchemaVersion = 1,
};

const nl::FieldDescriptor ImageRollbackEventFieldDescriptors[] =
{
    {
        NULL, offsetof(ImageRollbackEvent, imageVersion), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUTF8String, 1), 2
    },

    {
        NULL, offsetof(ImageRollbackEvent, subImageName), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUTF8String, 1), 3
    },

    {
        NULL, offsetof(ImageRollbackEvent, rollbackFrom), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUTF8String, 1), 4
    },

    {
        NULL, offsetof(ImageRollbackEvent, rollbackTo), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeUTF8String, 1), 5
    },

};

const nl::SchemaFieldDescriptor ImageRollbackEvent::FieldSchema =
{
    .mNumFieldDescriptorElements = sizeof(ImageRollbackEventFieldDescriptors)/sizeof(ImageRollbackEventFieldDescriptors[0]),
    .mFields = ImageRollbackEventFieldDescriptors,
    .mSize = sizeof(ImageRollbackEvent)
};
const nl::Weave::Profiles::DataManagement::EventSchema ImageRollbackEvent::Schema =
{
    .mProfileId = kWeaveProfileId,
    .mStructureType = 0xa,
    .mImportance = nl::Weave::Profiles::DataManagement::Info,
    .mDataSchemaVersion = 2,
    .mMinCompatibleDataSchemaVersion = 1,
};

} // namespace SoftwareUpdateTrait
} // namespace Firmware
} // namespace Trait
} // namespace Nest
} // namespace Schema
