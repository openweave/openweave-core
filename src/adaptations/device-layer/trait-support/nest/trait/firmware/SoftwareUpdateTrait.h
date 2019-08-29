
/**
 *    Copyright (c) 2019 Nest Labs, Inc.
 *    All rights reserved.
 *
 *    THIS FILE IS GENERATED. DO NOT MODIFY.
 *
 *    SOURCE TEMPLATE: trait.cpp.h
 *    SOURCE PROTO: nest/trait/firmware/software_update_trait.proto
 *
 */
#ifndef _NEST_TRAIT_FIRMWARE__SOFTWARE_UPDATE_TRAIT_H_
#define _NEST_TRAIT_FIRMWARE__SOFTWARE_UPDATE_TRAIT_H_

#include <Weave/Profiles/data-management/DataManagement.h>
#include <Weave/Support/SerializationUtils.h>

#include <weave/common/ProfileSpecificStatusCodeStructSchema.h>


namespace Schema {
namespace Nest {
namespace Trait {
namespace Firmware {
namespace SoftwareUpdateTrait {

extern const nl::Weave::Profiles::DataManagement::TraitSchemaEngine TraitSchema;

enum {
      kWeaveProfileId = (0x235aU << 16) | 0x505U
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
    //  last_update_time                    google.protobuf.Timestamp            uint32 seconds    YES             YES
    //
    kPropertyHandle_LastUpdateTime = 2,

    //
    // Enum for last handle
    //
    kLastSchemaHandle = 2,
};

//
// Events
//
struct SoftwareUpdateStartEvent
{
    int32_t trigger;

    static const nl::SchemaFieldDescriptor FieldSchema;

    // Statically-known Event Struct Attributes:
    enum {
            kWeaveProfileId = (0x235aU << 16) | 0x505U,
        kEventTypeId = 0x1U
    };

    static const nl::Weave::Profiles::DataManagement::EventSchema Schema;
};

struct SoftwareUpdateStartEvent_array {
    uint32_t num;
    SoftwareUpdateStartEvent *buf;
};


struct FailureEvent
{
    int32_t state;
    uint32_t platformReturnCode;
    Schema::Weave::Common::ProfileSpecificStatusCode primaryStatusCode;
    void SetPrimaryStatusCodeNull(void);
    void SetPrimaryStatusCodePresent(void);
#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
    bool IsPrimaryStatusCodePresent(void);
#endif
    Schema::Weave::Common::ProfileSpecificStatusCode remoteStatusCode;
    void SetRemoteStatusCodeNull(void);
    void SetRemoteStatusCodePresent(void);
#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
    bool IsRemoteStatusCodePresent(void);
#endif
    uint8_t __nullified_fields__[2/8 + 1];

    static const nl::SchemaFieldDescriptor FieldSchema;

    // Statically-known Event Struct Attributes:
    enum {
            kWeaveProfileId = (0x235aU << 16) | 0x505U,
        kEventTypeId = 0x2U
    };

    static const nl::Weave::Profiles::DataManagement::EventSchema Schema;
};

struct FailureEvent_array {
    uint32_t num;
    FailureEvent *buf;
};

inline void FailureEvent::SetPrimaryStatusCodeNull(void)
{
    SET_FIELD_NULLIFIED_BIT(__nullified_fields__, 0);
}

inline void FailureEvent::SetPrimaryStatusCodePresent(void)
{
    CLEAR_FIELD_NULLIFIED_BIT(__nullified_fields__, 0);
}

#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
inline bool FailureEvent::IsPrimaryStatusCodePresent(void)
{
    return (!GET_FIELD_NULLIFIED_BIT(__nullified_fields__, 0));
}
#endif
inline void FailureEvent::SetRemoteStatusCodeNull(void)
{
    SET_FIELD_NULLIFIED_BIT(__nullified_fields__, 1);
}

inline void FailureEvent::SetRemoteStatusCodePresent(void)
{
    CLEAR_FIELD_NULLIFIED_BIT(__nullified_fields__, 1);
}

#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
inline bool FailureEvent::IsRemoteStatusCodePresent(void)
{
    return (!GET_FIELD_NULLIFIED_BIT(__nullified_fields__, 1));
}
#endif

struct DownloadFailureEvent
{
    int32_t state;
    uint32_t platformReturnCode;
    Schema::Weave::Common::ProfileSpecificStatusCode primaryStatusCode;
    void SetPrimaryStatusCodeNull(void);
    void SetPrimaryStatusCodePresent(void);
#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
    bool IsPrimaryStatusCodePresent(void);
#endif
    Schema::Weave::Common::ProfileSpecificStatusCode remoteStatusCode;
    void SetRemoteStatusCodeNull(void);
    void SetRemoteStatusCodePresent(void);
#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
    bool IsRemoteStatusCodePresent(void);
#endif
    uint64_t bytesDownloaded;
    uint8_t __nullified_fields__[2/8 + 1];

    static const nl::SchemaFieldDescriptor FieldSchema;

    // Statically-known Event Struct Attributes:
    enum {
            kWeaveProfileId = (0x235aU << 16) | 0x505U,
        kEventTypeId = 0x3U
    };

    static const nl::Weave::Profiles::DataManagement::EventSchema Schema;
};

struct DownloadFailureEvent_array {
    uint32_t num;
    DownloadFailureEvent *buf;
};

inline void DownloadFailureEvent::SetPrimaryStatusCodeNull(void)
{
    SET_FIELD_NULLIFIED_BIT(__nullified_fields__, 0);
}

inline void DownloadFailureEvent::SetPrimaryStatusCodePresent(void)
{
    CLEAR_FIELD_NULLIFIED_BIT(__nullified_fields__, 0);
}

#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
inline bool DownloadFailureEvent::IsPrimaryStatusCodePresent(void)
{
    return (!GET_FIELD_NULLIFIED_BIT(__nullified_fields__, 0));
}
#endif
inline void DownloadFailureEvent::SetRemoteStatusCodeNull(void)
{
    SET_FIELD_NULLIFIED_BIT(__nullified_fields__, 1);
}

inline void DownloadFailureEvent::SetRemoteStatusCodePresent(void)
{
    CLEAR_FIELD_NULLIFIED_BIT(__nullified_fields__, 1);
}

#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
inline bool DownloadFailureEvent::IsRemoteStatusCodePresent(void)
{
    return (!GET_FIELD_NULLIFIED_BIT(__nullified_fields__, 1));
}
#endif

struct QueryBeginEvent
{
    const char * currentSwVersion;
    void SetCurrentSwVersionNull(void);
    void SetCurrentSwVersionPresent(void);
#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
    bool IsCurrentSwVersionPresent(void);
#endif
    uint16_t vendorId;
    void SetVendorIdNull(void);
    void SetVendorIdPresent(void);
#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
    bool IsVendorIdPresent(void);
#endif
    uint16_t vendorProductId;
    void SetVendorProductIdNull(void);
    void SetVendorProductIdPresent(void);
#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
    bool IsVendorProductIdPresent(void);
#endif
    uint16_t productRevision;
    void SetProductRevisionNull(void);
    void SetProductRevisionPresent(void);
#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
    bool IsProductRevisionPresent(void);
#endif
    const char * locale;
    void SetLocaleNull(void);
    void SetLocalePresent(void);
#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
    bool IsLocalePresent(void);
#endif
    const char * queryServerAddr;
    void SetQueryServerAddrNull(void);
    void SetQueryServerAddrPresent(void);
#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
    bool IsQueryServerAddrPresent(void);
#endif
    nl::SerializedByteString queryServerId;
    void SetQueryServerIdNull(void);
    void SetQueryServerIdPresent(void);
#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
    bool IsQueryServerIdPresent(void);
#endif
    uint8_t __nullified_fields__[7/8 + 1];

    static const nl::SchemaFieldDescriptor FieldSchema;

    // Statically-known Event Struct Attributes:
    enum {
            kWeaveProfileId = (0x235aU << 16) | 0x505U,
        kEventTypeId = 0x4U
    };

    static const nl::Weave::Profiles::DataManagement::EventSchema Schema;
};

struct QueryBeginEvent_array {
    uint32_t num;
    QueryBeginEvent *buf;
};

inline void QueryBeginEvent::SetCurrentSwVersionNull(void)
{
    SET_FIELD_NULLIFIED_BIT(__nullified_fields__, 0);
}

inline void QueryBeginEvent::SetCurrentSwVersionPresent(void)
{
    CLEAR_FIELD_NULLIFIED_BIT(__nullified_fields__, 0);
}

#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
inline bool QueryBeginEvent::IsCurrentSwVersionPresent(void)
{
    return (!GET_FIELD_NULLIFIED_BIT(__nullified_fields__, 0));
}
#endif
inline void QueryBeginEvent::SetVendorIdNull(void)
{
    SET_FIELD_NULLIFIED_BIT(__nullified_fields__, 1);
}

inline void QueryBeginEvent::SetVendorIdPresent(void)
{
    CLEAR_FIELD_NULLIFIED_BIT(__nullified_fields__, 1);
}

#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
inline bool QueryBeginEvent::IsVendorIdPresent(void)
{
    return (!GET_FIELD_NULLIFIED_BIT(__nullified_fields__, 1));
}
#endif
inline void QueryBeginEvent::SetVendorProductIdNull(void)
{
    SET_FIELD_NULLIFIED_BIT(__nullified_fields__, 2);
}

inline void QueryBeginEvent::SetVendorProductIdPresent(void)
{
    CLEAR_FIELD_NULLIFIED_BIT(__nullified_fields__, 2);
}

#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
inline bool QueryBeginEvent::IsVendorProductIdPresent(void)
{
    return (!GET_FIELD_NULLIFIED_BIT(__nullified_fields__, 2));
}
#endif
inline void QueryBeginEvent::SetProductRevisionNull(void)
{
    SET_FIELD_NULLIFIED_BIT(__nullified_fields__, 3);
}

inline void QueryBeginEvent::SetProductRevisionPresent(void)
{
    CLEAR_FIELD_NULLIFIED_BIT(__nullified_fields__, 3);
}

#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
inline bool QueryBeginEvent::IsProductRevisionPresent(void)
{
    return (!GET_FIELD_NULLIFIED_BIT(__nullified_fields__, 3));
}
#endif
inline void QueryBeginEvent::SetLocaleNull(void)
{
    SET_FIELD_NULLIFIED_BIT(__nullified_fields__, 4);
}

inline void QueryBeginEvent::SetLocalePresent(void)
{
    CLEAR_FIELD_NULLIFIED_BIT(__nullified_fields__, 4);
}

#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
inline bool QueryBeginEvent::IsLocalePresent(void)
{
    return (!GET_FIELD_NULLIFIED_BIT(__nullified_fields__, 4));
}
#endif
inline void QueryBeginEvent::SetQueryServerAddrNull(void)
{
    SET_FIELD_NULLIFIED_BIT(__nullified_fields__, 5);
}

inline void QueryBeginEvent::SetQueryServerAddrPresent(void)
{
    CLEAR_FIELD_NULLIFIED_BIT(__nullified_fields__, 5);
}

#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
inline bool QueryBeginEvent::IsQueryServerAddrPresent(void)
{
    return (!GET_FIELD_NULLIFIED_BIT(__nullified_fields__, 5));
}
#endif
inline void QueryBeginEvent::SetQueryServerIdNull(void)
{
    SET_FIELD_NULLIFIED_BIT(__nullified_fields__, 6);
}

inline void QueryBeginEvent::SetQueryServerIdPresent(void)
{
    CLEAR_FIELD_NULLIFIED_BIT(__nullified_fields__, 6);
}

#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
inline bool QueryBeginEvent::IsQueryServerIdPresent(void)
{
    return (!GET_FIELD_NULLIFIED_BIT(__nullified_fields__, 6));
}
#endif

struct QueryFinishEvent
{
    const char * imageUrl;
    void SetImageUrlNull(void);
    void SetImageUrlPresent(void);
#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
    bool IsImageUrlPresent(void);
#endif
    const char * imageVersion;
    void SetImageVersionNull(void);
    void SetImageVersionPresent(void);
#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
    bool IsImageVersionPresent(void);
#endif
    uint8_t __nullified_fields__[2/8 + 1];

    static const nl::SchemaFieldDescriptor FieldSchema;

    // Statically-known Event Struct Attributes:
    enum {
            kWeaveProfileId = (0x235aU << 16) | 0x505U,
        kEventTypeId = 0x5U
    };

    static const nl::Weave::Profiles::DataManagement::EventSchema Schema;
};

struct QueryFinishEvent_array {
    uint32_t num;
    QueryFinishEvent *buf;
};

inline void QueryFinishEvent::SetImageUrlNull(void)
{
    SET_FIELD_NULLIFIED_BIT(__nullified_fields__, 0);
}

inline void QueryFinishEvent::SetImageUrlPresent(void)
{
    CLEAR_FIELD_NULLIFIED_BIT(__nullified_fields__, 0);
}

#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
inline bool QueryFinishEvent::IsImageUrlPresent(void)
{
    return (!GET_FIELD_NULLIFIED_BIT(__nullified_fields__, 0));
}
#endif
inline void QueryFinishEvent::SetImageVersionNull(void)
{
    SET_FIELD_NULLIFIED_BIT(__nullified_fields__, 1);
}

inline void QueryFinishEvent::SetImageVersionPresent(void)
{
    CLEAR_FIELD_NULLIFIED_BIT(__nullified_fields__, 1);
}

#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
inline bool QueryFinishEvent::IsImageVersionPresent(void)
{
    return (!GET_FIELD_NULLIFIED_BIT(__nullified_fields__, 1));
}
#endif

struct DownloadStartEvent
{
    const char * imageUrl;
    void SetImageUrlNull(void);
    void SetImageUrlPresent(void);
#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
    bool IsImageUrlPresent(void);
#endif
    const char * imageVersion;
    void SetImageVersionNull(void);
    void SetImageVersionPresent(void);
#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
    bool IsImageVersionPresent(void);
#endif
    const char * subImageName;
    void SetSubImageNameNull(void);
    void SetSubImageNamePresent(void);
#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
    bool IsSubImageNamePresent(void);
#endif
    uint64_t offset;
    void SetOffsetNull(void);
    void SetOffsetPresent(void);
#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
    bool IsOffsetPresent(void);
#endif
    const char * destination;
    void SetDestinationNull(void);
    void SetDestinationPresent(void);
#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
    bool IsDestinationPresent(void);
#endif
    uint8_t __nullified_fields__[5/8 + 1];

    static const nl::SchemaFieldDescriptor FieldSchema;

    // Statically-known Event Struct Attributes:
    enum {
            kWeaveProfileId = (0x235aU << 16) | 0x505U,
        kEventTypeId = 0x6U
    };

    static const nl::Weave::Profiles::DataManagement::EventSchema Schema;
};

struct DownloadStartEvent_array {
    uint32_t num;
    DownloadStartEvent *buf;
};

inline void DownloadStartEvent::SetImageUrlNull(void)
{
    SET_FIELD_NULLIFIED_BIT(__nullified_fields__, 0);
}

inline void DownloadStartEvent::SetImageUrlPresent(void)
{
    CLEAR_FIELD_NULLIFIED_BIT(__nullified_fields__, 0);
}

#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
inline bool DownloadStartEvent::IsImageUrlPresent(void)
{
    return (!GET_FIELD_NULLIFIED_BIT(__nullified_fields__, 0));
}
#endif
inline void DownloadStartEvent::SetImageVersionNull(void)
{
    SET_FIELD_NULLIFIED_BIT(__nullified_fields__, 1);
}

inline void DownloadStartEvent::SetImageVersionPresent(void)
{
    CLEAR_FIELD_NULLIFIED_BIT(__nullified_fields__, 1);
}

#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
inline bool DownloadStartEvent::IsImageVersionPresent(void)
{
    return (!GET_FIELD_NULLIFIED_BIT(__nullified_fields__, 1));
}
#endif
inline void DownloadStartEvent::SetSubImageNameNull(void)
{
    SET_FIELD_NULLIFIED_BIT(__nullified_fields__, 2);
}

inline void DownloadStartEvent::SetSubImageNamePresent(void)
{
    CLEAR_FIELD_NULLIFIED_BIT(__nullified_fields__, 2);
}

#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
inline bool DownloadStartEvent::IsSubImageNamePresent(void)
{
    return (!GET_FIELD_NULLIFIED_BIT(__nullified_fields__, 2));
}
#endif
inline void DownloadStartEvent::SetOffsetNull(void)
{
    SET_FIELD_NULLIFIED_BIT(__nullified_fields__, 3);
}

inline void DownloadStartEvent::SetOffsetPresent(void)
{
    CLEAR_FIELD_NULLIFIED_BIT(__nullified_fields__, 3);
}

#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
inline bool DownloadStartEvent::IsOffsetPresent(void)
{
    return (!GET_FIELD_NULLIFIED_BIT(__nullified_fields__, 3));
}
#endif
inline void DownloadStartEvent::SetDestinationNull(void)
{
    SET_FIELD_NULLIFIED_BIT(__nullified_fields__, 4);
}

inline void DownloadStartEvent::SetDestinationPresent(void)
{
    CLEAR_FIELD_NULLIFIED_BIT(__nullified_fields__, 4);
}

#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
inline bool DownloadStartEvent::IsDestinationPresent(void)
{
    return (!GET_FIELD_NULLIFIED_BIT(__nullified_fields__, 4));
}
#endif

struct DownloadFinishEvent
{
    const char * imageUrl;
    void SetImageUrlNull(void);
    void SetImageUrlPresent(void);
#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
    bool IsImageUrlPresent(void);
#endif
    const char * imageVersion;
    void SetImageVersionNull(void);
    void SetImageVersionPresent(void);
#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
    bool IsImageVersionPresent(void);
#endif
    const char * subImageName;
    void SetSubImageNameNull(void);
    void SetSubImageNamePresent(void);
#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
    bool IsSubImageNamePresent(void);
#endif
    const char * destination;
    void SetDestinationNull(void);
    void SetDestinationPresent(void);
#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
    bool IsDestinationPresent(void);
#endif
    uint8_t __nullified_fields__[4/8 + 1];

    static const nl::SchemaFieldDescriptor FieldSchema;

    // Statically-known Event Struct Attributes:
    enum {
            kWeaveProfileId = (0x235aU << 16) | 0x505U,
        kEventTypeId = 0x7U
    };

    static const nl::Weave::Profiles::DataManagement::EventSchema Schema;
};

struct DownloadFinishEvent_array {
    uint32_t num;
    DownloadFinishEvent *buf;
};

inline void DownloadFinishEvent::SetImageUrlNull(void)
{
    SET_FIELD_NULLIFIED_BIT(__nullified_fields__, 0);
}

inline void DownloadFinishEvent::SetImageUrlPresent(void)
{
    CLEAR_FIELD_NULLIFIED_BIT(__nullified_fields__, 0);
}

#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
inline bool DownloadFinishEvent::IsImageUrlPresent(void)
{
    return (!GET_FIELD_NULLIFIED_BIT(__nullified_fields__, 0));
}
#endif
inline void DownloadFinishEvent::SetImageVersionNull(void)
{
    SET_FIELD_NULLIFIED_BIT(__nullified_fields__, 1);
}

inline void DownloadFinishEvent::SetImageVersionPresent(void)
{
    CLEAR_FIELD_NULLIFIED_BIT(__nullified_fields__, 1);
}

#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
inline bool DownloadFinishEvent::IsImageVersionPresent(void)
{
    return (!GET_FIELD_NULLIFIED_BIT(__nullified_fields__, 1));
}
#endif
inline void DownloadFinishEvent::SetSubImageNameNull(void)
{
    SET_FIELD_NULLIFIED_BIT(__nullified_fields__, 2);
}

inline void DownloadFinishEvent::SetSubImageNamePresent(void)
{
    CLEAR_FIELD_NULLIFIED_BIT(__nullified_fields__, 2);
}

#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
inline bool DownloadFinishEvent::IsSubImageNamePresent(void)
{
    return (!GET_FIELD_NULLIFIED_BIT(__nullified_fields__, 2));
}
#endif
inline void DownloadFinishEvent::SetDestinationNull(void)
{
    SET_FIELD_NULLIFIED_BIT(__nullified_fields__, 3);
}

inline void DownloadFinishEvent::SetDestinationPresent(void)
{
    CLEAR_FIELD_NULLIFIED_BIT(__nullified_fields__, 3);
}

#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
inline bool DownloadFinishEvent::IsDestinationPresent(void)
{
    return (!GET_FIELD_NULLIFIED_BIT(__nullified_fields__, 3));
}
#endif

struct InstallStartEvent
{
    const char * imageVersion;
    void SetImageVersionNull(void);
    void SetImageVersionPresent(void);
#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
    bool IsImageVersionPresent(void);
#endif
    const char * subImageName;
    void SetSubImageNameNull(void);
    void SetSubImageNamePresent(void);
#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
    bool IsSubImageNamePresent(void);
#endif
    const char * localSource;
    void SetLocalSourceNull(void);
    void SetLocalSourcePresent(void);
#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
    bool IsLocalSourcePresent(void);
#endif
    const char * destination;
    void SetDestinationNull(void);
    void SetDestinationPresent(void);
#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
    bool IsDestinationPresent(void);
#endif
    uint8_t __nullified_fields__[4/8 + 1];

    static const nl::SchemaFieldDescriptor FieldSchema;

    // Statically-known Event Struct Attributes:
    enum {
            kWeaveProfileId = (0x235aU << 16) | 0x505U,
        kEventTypeId = 0x8U
    };

    static const nl::Weave::Profiles::DataManagement::EventSchema Schema;
};

struct InstallStartEvent_array {
    uint32_t num;
    InstallStartEvent *buf;
};

inline void InstallStartEvent::SetImageVersionNull(void)
{
    SET_FIELD_NULLIFIED_BIT(__nullified_fields__, 0);
}

inline void InstallStartEvent::SetImageVersionPresent(void)
{
    CLEAR_FIELD_NULLIFIED_BIT(__nullified_fields__, 0);
}

#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
inline bool InstallStartEvent::IsImageVersionPresent(void)
{
    return (!GET_FIELD_NULLIFIED_BIT(__nullified_fields__, 0));
}
#endif
inline void InstallStartEvent::SetSubImageNameNull(void)
{
    SET_FIELD_NULLIFIED_BIT(__nullified_fields__, 1);
}

inline void InstallStartEvent::SetSubImageNamePresent(void)
{
    CLEAR_FIELD_NULLIFIED_BIT(__nullified_fields__, 1);
}

#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
inline bool InstallStartEvent::IsSubImageNamePresent(void)
{
    return (!GET_FIELD_NULLIFIED_BIT(__nullified_fields__, 1));
}
#endif
inline void InstallStartEvent::SetLocalSourceNull(void)
{
    SET_FIELD_NULLIFIED_BIT(__nullified_fields__, 2);
}

inline void InstallStartEvent::SetLocalSourcePresent(void)
{
    CLEAR_FIELD_NULLIFIED_BIT(__nullified_fields__, 2);
}

#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
inline bool InstallStartEvent::IsLocalSourcePresent(void)
{
    return (!GET_FIELD_NULLIFIED_BIT(__nullified_fields__, 2));
}
#endif
inline void InstallStartEvent::SetDestinationNull(void)
{
    SET_FIELD_NULLIFIED_BIT(__nullified_fields__, 3);
}

inline void InstallStartEvent::SetDestinationPresent(void)
{
    CLEAR_FIELD_NULLIFIED_BIT(__nullified_fields__, 3);
}

#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
inline bool InstallStartEvent::IsDestinationPresent(void)
{
    return (!GET_FIELD_NULLIFIED_BIT(__nullified_fields__, 3));
}
#endif

struct InstallFinishEvent
{
    const char * imageVersion;
    void SetImageVersionNull(void);
    void SetImageVersionPresent(void);
#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
    bool IsImageVersionPresent(void);
#endif
    const char * subImageName;
    void SetSubImageNameNull(void);
    void SetSubImageNamePresent(void);
#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
    bool IsSubImageNamePresent(void);
#endif
    uint8_t __nullified_fields__[2/8 + 1];

    static const nl::SchemaFieldDescriptor FieldSchema;

    // Statically-known Event Struct Attributes:
    enum {
            kWeaveProfileId = (0x235aU << 16) | 0x505U,
        kEventTypeId = 0x9U
    };

    static const nl::Weave::Profiles::DataManagement::EventSchema Schema;
};

struct InstallFinishEvent_array {
    uint32_t num;
    InstallFinishEvent *buf;
};

inline void InstallFinishEvent::SetImageVersionNull(void)
{
    SET_FIELD_NULLIFIED_BIT(__nullified_fields__, 0);
}

inline void InstallFinishEvent::SetImageVersionPresent(void)
{
    CLEAR_FIELD_NULLIFIED_BIT(__nullified_fields__, 0);
}

#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
inline bool InstallFinishEvent::IsImageVersionPresent(void)
{
    return (!GET_FIELD_NULLIFIED_BIT(__nullified_fields__, 0));
}
#endif
inline void InstallFinishEvent::SetSubImageNameNull(void)
{
    SET_FIELD_NULLIFIED_BIT(__nullified_fields__, 1);
}

inline void InstallFinishEvent::SetSubImageNamePresent(void)
{
    CLEAR_FIELD_NULLIFIED_BIT(__nullified_fields__, 1);
}

#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
inline bool InstallFinishEvent::IsSubImageNamePresent(void)
{
    return (!GET_FIELD_NULLIFIED_BIT(__nullified_fields__, 1));
}
#endif

struct ImageRollbackEvent
{
    const char * imageVersion;
    void SetImageVersionNull(void);
    void SetImageVersionPresent(void);
#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
    bool IsImageVersionPresent(void);
#endif
    const char * subImageName;
    void SetSubImageNameNull(void);
    void SetSubImageNamePresent(void);
#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
    bool IsSubImageNamePresent(void);
#endif
    const char * rollbackFrom;
    void SetRollbackFromNull(void);
    void SetRollbackFromPresent(void);
#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
    bool IsRollbackFromPresent(void);
#endif
    const char * rollbackTo;
    void SetRollbackToNull(void);
    void SetRollbackToPresent(void);
#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
    bool IsRollbackToPresent(void);
#endif
    uint8_t __nullified_fields__[4/8 + 1];

    static const nl::SchemaFieldDescriptor FieldSchema;

    // Statically-known Event Struct Attributes:
    enum {
            kWeaveProfileId = (0x235aU << 16) | 0x505U,
        kEventTypeId = 0xaU
    };

    static const nl::Weave::Profiles::DataManagement::EventSchema Schema;
};

struct ImageRollbackEvent_array {
    uint32_t num;
    ImageRollbackEvent *buf;
};

inline void ImageRollbackEvent::SetImageVersionNull(void)
{
    SET_FIELD_NULLIFIED_BIT(__nullified_fields__, 0);
}

inline void ImageRollbackEvent::SetImageVersionPresent(void)
{
    CLEAR_FIELD_NULLIFIED_BIT(__nullified_fields__, 0);
}

#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
inline bool ImageRollbackEvent::IsImageVersionPresent(void)
{
    return (!GET_FIELD_NULLIFIED_BIT(__nullified_fields__, 0));
}
#endif
inline void ImageRollbackEvent::SetSubImageNameNull(void)
{
    SET_FIELD_NULLIFIED_BIT(__nullified_fields__, 1);
}

inline void ImageRollbackEvent::SetSubImageNamePresent(void)
{
    CLEAR_FIELD_NULLIFIED_BIT(__nullified_fields__, 1);
}

#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
inline bool ImageRollbackEvent::IsSubImageNamePresent(void)
{
    return (!GET_FIELD_NULLIFIED_BIT(__nullified_fields__, 1));
}
#endif
inline void ImageRollbackEvent::SetRollbackFromNull(void)
{
    SET_FIELD_NULLIFIED_BIT(__nullified_fields__, 2);
}

inline void ImageRollbackEvent::SetRollbackFromPresent(void)
{
    CLEAR_FIELD_NULLIFIED_BIT(__nullified_fields__, 2);
}

#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
inline bool ImageRollbackEvent::IsRollbackFromPresent(void)
{
    return (!GET_FIELD_NULLIFIED_BIT(__nullified_fields__, 2));
}
#endif
inline void ImageRollbackEvent::SetRollbackToNull(void)
{
    SET_FIELD_NULLIFIED_BIT(__nullified_fields__, 3);
}

inline void ImageRollbackEvent::SetRollbackToPresent(void)
{
    CLEAR_FIELD_NULLIFIED_BIT(__nullified_fields__, 3);
}

#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
inline bool ImageRollbackEvent::IsRollbackToPresent(void)
{
    return (!GET_FIELD_NULLIFIED_BIT(__nullified_fields__, 3));
}
#endif

//
// Enums
//

enum State {
    STATE_IDLE = 1,
    STATE_QUERYING = 2,
    STATE_DOWNLOADING = 3,
    STATE_INSTALLING = 4,
    STATE_ROLLING_BACK = 5,
};

enum StartTrigger {
    START_TRIGGER_USER_INITIATED = 1,
    START_TRIGGER_SCHEDULED = 2,
    START_TRIGGER_USB = 3,
    START_TRIGGER_FROM_DFU = 4,
    START_TRIGGER_BLE = 5,
    START_TRIGGER_REMOTE_AGENT = 6,
    START_TRIGGER_OTHER = 7,
};

} // namespace SoftwareUpdateTrait
} // namespace Firmware
} // namespace Trait
} // namespace Nest
} // namespace Schema
#endif // _NEST_TRAIT_FIRMWARE__SOFTWARE_UPDATE_TRAIT_H_
