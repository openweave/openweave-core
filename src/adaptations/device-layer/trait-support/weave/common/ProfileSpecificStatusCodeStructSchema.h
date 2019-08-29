/**
 *    Copyright (c) 2019 Nest Labs, Inc.
 *    All rights reserved.
 *
 *    THIS FILE IS GENERATED. DO NOT MODIFY.
 *
 *    SOURCE TEMPLATE: struct.cpp.h
 *    SOURCE PROTO: weave/common/identifiers.proto
 *
 */
#ifndef _WEAVE_COMMON__PROFILE_SPECIFIC_STATUS_CODE_STRUCT_SCHEMA_H_
#define _WEAVE_COMMON__PROFILE_SPECIFIC_STATUS_CODE_STRUCT_SCHEMA_H_

#include <Weave/Support/SerializationUtils.h>
#include <Weave/Profiles/data-management/DataManagement.h>



namespace Schema {
namespace Weave {
namespace Common {

struct ProfileSpecificStatusCode
{
    uint32_t profileId;
    uint16_t statusCode;

    static const nl::SchemaFieldDescriptor FieldSchema;

};

struct ProfileSpecificStatusCode_array {
    uint32_t num;
    ProfileSpecificStatusCode *buf;
};



} // namespace Common
} // namespace Weave
} // namespace Schema
#endif // _WEAVE_COMMON__PROFILE_SPECIFIC_STATUS_CODE_STRUCT_SCHEMA_H_
