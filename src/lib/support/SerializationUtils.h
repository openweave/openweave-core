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
 *   Functions and structures to use for:
 *   (1) serializing a C-structure to a TLVWriter and
 *   (2) deserializing a C-structure from a TLVReader
 */

#ifndef SERIALIZATION_UTILS_H
#define SERIALIZATION_UTILS_H

#include <Weave/Core/WeaveCore.h>
#include <Weave/Core/WeaveTLV.h>
#include <Weave/Profiles/data-management/DataManagement.h>

namespace nl {

struct FieldDescriptor;
struct SchemaFieldDescriptor;
struct StructureSchemaPointerPair;

/**
 * @brief
 *   A list of TLV types to write with a TLV field.
 */
enum SerializedFieldType
{
    SerializedFieldTypeBoolean = 0x00,  //!< Boolean type
    SerializedFieldTypeUInt8,           //!< Unsigned 8-bit type
    SerializedFieldTypeUInt16,          //!< Unsigned 16-bit type
    SerializedFieldTypeUInt32,          //!< Unsigned 32-bit type
    SerializedFieldTypeUInt64,          //!< Unsigned 64-bit type
    SerializedFieldTypeInt8,            //!< Signed 8-bit type
    SerializedFieldTypeInt16,           //!< Signed 16-bit type
    SerializedFieldTypeInt32,           //!< Signed 32-bit type
    SerializedFieldTypeInt64,           //!< Signed 64-bit type
    SerializedFieldTypeFloatingPoint32, //!< 32-bit float type
    SerializedFieldTypeFloatingPoint64, //!< 64-bit float type
    SerializedFieldTypeUTF8String,      //!< UTF-8 string type
    SerializedFieldTypeByteString,      //!< Byte string type
    SerializedFieldTypeStructure,       //!< User-defined structure type
    SerializedFieldTypeArray,           //!< Array type
};

/**
 * @brief
 *   Masks for accessing bits of SerializedFieldType.
 */
enum SerializedFieldTypeMasks
{
    kMask_Type = 0x7f,
    kMask_NullableFlag = 0x80,
};

/**
 * @brief
 *   Bitfield of SerializedFieldType.
 */
enum SerializedFieldTypeBits
{
    kBit_Nullable = 7,
};

/**
 * @brief
 *   Utility function to create FieldDescriptor.mTypeAndFlags
 */
#define SET_TYPE_AND_FLAGS(type, nullable)     ( static_cast<uint8_t>((type & nl::kMask_Type) | ((nullable << nl::kBit_Nullable) & nl::kMask_NullableFlag)) )

/**
 * @brief
 *   Utility functions to access nullified field array. Used by codegen.
 */
#define GET_FIELD_NULLIFIED_BIT(nullifiedFieldsPtr, aBit)      (nullifiedFieldsPtr[aBit/8] & (1 << (aBit % 8)))
#define SET_FIELD_NULLIFIED_BIT(nullifiedFieldsPtr, aBit)      (nullifiedFieldsPtr[aBit/8] |= (1 << (aBit % 8)))
#define CLEAR_FIELD_NULLIFIED_BIT(nullifiedFieldsPtr, aBit)    (nullifiedFieldsPtr[aBit/8] &= ~(1 << (aBit % 8)))


/**
 * @brief
 *   Structure that describes a TLV field in a schema structure and connects it to data
 *   in a c-struct
 */
struct FieldDescriptor
{
    const SchemaFieldDescriptor *mNestedFieldDescriptors; //!< Pointer to another group of field descriptors, if we have structs, etc.
    uint16_t mOffset;                                     //!< Where to look in the c-struct for the data to write into the TLV field.
    uint8_t mTypeAndFlags;                                //!< Data type of the TLV field.
    uint8_t mTVDContextTag;                               //!< Context tag of the TLV field.

    bool IsNullable(void) const { return (mTypeAndFlags & kMask_NullableFlag) != 0; }
    SerializedFieldType GetType(void) const { return static_cast<SerializedFieldType>(mTypeAndFlags & kMask_Type); }
};

/**
 * @brief
 *   Wrapper around an array of FieldDescriptors to describe a schema structure/structure
 */
struct SchemaFieldDescriptor
{
    uint16_t mNumFieldDescriptorElements;                 //!< Number of elements in our FieldDescriptor array
    const FieldDescriptor *mFields;                       //!< Pointer to array of FieldDescriptors
    const uint32_t mSize;                                 //!< Size (in bytes) of the structure
};

struct SerializedByteString
{
    uint32_t mLen;                                        //!< Number of bytes in byte string
    uint8_t *mBuf;                                        //!< Pointer to byte string
};

struct SerializedFieldTypeBoolean_array
{
    uint32_t num;
    bool *buf;
};

struct SerializedFieldTypeUInt8_array
{
    uint32_t num;
    uint8_t *buf;
};

struct SerializedFieldTypeUInt16_array
{
    uint32_t num;
    uint16_t *buf;
};

struct SerializedFieldTypeUInt32_array
{
    uint32_t num;
    uint32_t *buf;
};

struct SerializedFieldTypeUInt64_array
{
    uint32_t num;
    uint64_t *buf;
};

struct SerializedFieldTypeInt8_array
{
    uint32_t num;
    int8_t *buf;
};

struct SerializedFieldTypeInt16_array
{
    uint32_t num;
    int16_t *buf;
};

struct SerializedFieldTypeInt32_array
{
    uint32_t num;
    int32_t *buf;
};

struct SerializedFieldTypeInt64_array
{
    uint32_t num;
    int64_t *buf;
};

struct SerializedFieldTypeFloatingPoint32_array
{
    uint32_t num;
    float *buf;
};

struct SerializedFieldTypeFloatingPoint64_array
{
    uint32_t num;
    double *buf;
};

struct SerializedFieldTypeUTF8String_array
{
    uint32_t num;
    char * *buf;
};

struct SerializedFieldTypeByteString_array
{
    uint32_t num;
    SerializedByteString *buf;
};

/**
 * @brief
 *   A helper for wrapping an array with a length
 */
struct ArrayLengthAndBuffer
{
    uint32_t mNumElements;                                //!< Number of elements in the array
    void *mElementBuffer;                                 //!< Actual array definition
};

/**
 * @brief
 *   Pair of data with a c-struct of data and the StructureSchemaDescriptor to write a TLV structure based on that data
 */
struct StructureSchemaPointerPair
{
    void *mStructureData;                                     //!< Pointer to a c-struct of data for the structure
    const SchemaFieldDescriptor *mFieldSchema;                //!< SchemaFieldDescriptor to describe how to process the data into TLV
};

/**
 * @brief
 *   Memory allocate/free function pointers.
 */
typedef void *(*MemoryAllocate)(size_t size);
typedef void (*MemoryFree)(void *ptr);
typedef void *(*MemoryReallocate)(void *ptr, size_t size);

/**
 * @brief
 *   A c-struct of memory allocate/free functions.

 */
struct MemoryManagement
{
    MemoryAllocate mem_alloc;
    MemoryFree mem_free;
    MemoryReallocate mem_realloc;
};

/**
 * @brief
 *   A c-struct containing any context or state we need for serializing or deserializing.
     For now, just memory management.
 */
struct SerializationContext
{
    MemoryManagement memMgmt;
};

WEAVE_ERROR SerializedDataToTLVWriter(nl::Weave::TLV::TLVWriter &aWriter,
                                      void *aStructureData,
                                      const SchemaFieldDescriptor *aFieldDescriptors);

WEAVE_ERROR SerializedDataToTLVWriterHelper(nl::Weave::TLV::TLVWriter &aWriter,
                                            uint8_t aDataTag,
                                            void *aAppData);

/**
 * @brief
 *   When this function returns, aStructureData may contain dynamically-allocated memory
 *   for which the caller is responsible for de-allocating when finished with it.
 */
WEAVE_ERROR TLVReaderToDeserializedData(nl::Weave::TLV::TLVReader &aReader,
                                        void *aStructureData,
                                        const SchemaFieldDescriptor *aFieldDescriptors,
                                        SerializationContext *aContext = NULL);

/**
 * @brief
 *   When this function returns, aStructureData may contain dynamically-allocated memory
 *   for which the caller is responsible for de-allocating when finished with it.
 */
WEAVE_ERROR TLVReaderToDeserializedDataHelper(nl::Weave::TLV::TLVReader &aReader,
                                              uint8_t aDataTag,
                                              void *aAppData,
                                              SerializationContext *aContext = NULL);

WEAVE_ERROR DeallocateDeserializedStructure(void *aStructureData,
                                            const SchemaFieldDescriptor *aFieldDescriptors,
                                            SerializationContext *aContext = NULL);

} // nl
#endif // SERIALIZATION_UTILS_H
