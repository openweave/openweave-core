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
#include <Weave/Support/CodeUtils.h>
#include <Weave/Support/logging/WeaveLogging.h>
#include <Weave/Support/SerializationUtils.h>

namespace nl {

using namespace nl::Weave::TLV;

//
// Some notes on memory management.
//
// No dynamic memory allocation is necessary for serializing a structure,
// but it is REQUIRED for de-serializing.  If your platform needs to
// de-serialize, you can:
//
// (1) Pass TLVReaderToDeserializedData() and
//     TLVReaderToDeserializedDataHelper() a MemoryManagement
//     structure with any implementations of malloc(), free(),
//     and realloc() you choose, OR
//
// (2) Pass no MemoryManagement at all, in which case a default
//     MemoryManagement will be used.
//
// If option (2) is chosen, you must turn on
// WEAVE_CONFIG_SERIALIZATION_USE_MALLOC,
// and the stdlib versions of malloc(), free(), and realloc() will be
// used.  Otherwise the unsupported_*() functions will be used, which
// do nothing except log an error.
//
// The assumption is that de-serialization will likely be performed
// only on resource-rich platforms where dynamic memory allocation is
// supported.
//

#if WEAVE_CONFIG_SERIALIZATION_USE_MALLOC
#if HAVE_MALLOC && HAVE_FREE && HAVE_REALLOC
static MemoryManagement sDefaultMemoryManagement = { malloc, free, realloc };
#else // HAVE_MALLOC && HAVE_FREE && HAVE_REALLOC
#error "Implementations of malloc, free, and realloc must be present in order to use stdlib memory management."
#endif // HAVE_MALLOC && HAVE_FREE && HAVE_REALLOC
#else // WEAVE_CONFIG_SERIALIZATION_USE_MALLOC
static void *unsupported_malloc(size_t size)
{
    WeaveLogError(Support, "malloc() not supported");
    return NULL;
}

static void unsupported_free(void *ptr)
{
    WeaveLogError(Support, "free() not supported");
}

static void *unsupported_realloc(void *ptr, size_t size)
{
    WeaveLogError(Support, "realloc() not supported");
    return NULL;
}

static MemoryManagement sDefaultMemoryManagement = { unsupported_malloc, unsupported_free, unsupported_realloc };
#endif // WEAVE_CONFIG_SERIALIZATION_USE_MALLOC

static WEAVE_ERROR WriteArrayData(TLVWriter &aWriter,
                                  void *aStructureData,
                                  const FieldDescriptor * aFieldPtr);

static WEAVE_ERROR WriteDataForType(TLVWriter &aWriter,
                                    void *aStructureData,
                                    const FieldDescriptor *&aFieldPtr,
                                    SerializedFieldType aType,
                                    bool aInArray);

static WEAVE_ERROR ReadArrayData(TLVReader &aReader,
                                 void *aStructureData,
                                 const FieldDescriptor * aFieldPtr,
                                 SerializationContext *aContext = NULL);

static WEAVE_ERROR ReadDataForType(TLVReader &aReader,
                                   void *aStructureData,
                                   const FieldDescriptor *&aFieldPtr,
                                   SerializedFieldType aType,
                                   bool aInArray,
                                   SerializationContext *aContext = NULL);

static WEAVE_ERROR CheckForEndOfTLV(TLVReader &aReader, bool &aEndOfTLV);

static WEAVE_ERROR DeallocateDeserializedArray(void *aArrayData,
                                               const SchemaFieldDescriptor *aFieldDescriptors,
                                               SerializationContext *aContext = NULL);

#if WEAVE_CONFIG_SERIALIZATION_DEBUG_LOGGING
static int32_t sIndentationLevel = 0;
#define LogReadWrite(aFormat, ...) WeaveLogDetail(Support, "%*s" aFormat, sIndentationLevel, "", __VA_ARGS__)
#define LogReadWriteStartContainer(aFormat, ...) do { LogReadWrite(aFormat, __VA_ARGS__); sIndentationLevel += 2; } while (0);
#define LogReadWriteEndContainer(aFormat, ...) do { sIndentationLevel -= 2; LogReadWrite(aFormat, __VA_ARGS__); } while (0);
#else // WEAVE_CONFIG_SERIALIZATION_DEBUG_LOGGING
#define LogReadWrite(aFormat, ...)
#define LogReadWriteStartContainer(aFormat, ...)
#define LogReadWriteEndContainer(aFormat, ...)
#endif // WEAVE_CONFIG_SERIALIZATION_DEBUG_LOGGING

static bool IsValidFieldType(SerializedFieldType aType)
{
    return (aType <= SerializedFieldTypeArray);
}

const static uint8_t sElementSize[] =
{
    sizeof(bool),                   //SerializedFieldTypeBoolean
    sizeof(uint8_t),                //SerializedFieldTypeUInt8,
    sizeof(uint16_t),               //SerializedFieldTypeUInt16,
    sizeof(uint32_t),               //SerializedFieldTypeUInt32,
    sizeof(uint64_t),               //SerializedFieldTypeUInt64,
    sizeof(int8_t),                 //SerializedFieldTypeInt8,
    sizeof(int16_t),                //SerializedFieldTypeInt16,
    sizeof(int32_t),                //SerializedFieldTypeInt32,
    sizeof(int64_t),                //SerializedFieldTypeInt64,
    sizeof(float),                  //SerializedFieldTypeFloatingPoint32,
    sizeof(double),                 //SerializedFieldTypeFloatingPoint64,
    sizeof(char *),                 //SerializedFieldTypeUTF8String,
    sizeof(SerializedByteString),   //SerializedFieldTypeByteString,
    sizeof(void *),                 //SerializedFieldTypeStructure,
    sizeof(void *)                  //SerializedFieldTypeArray,
};

static WEAVE_ERROR GetArrayElementSize(uint32_t &aOutSize, const FieldDescriptor * aFieldPtr, SerializedFieldType aType)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    VerifyOrExit(aFieldPtr != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);

    if (aFieldPtr->mNestedFieldDescriptors)
    {
        VerifyOrExit(aType == SerializedFieldTypeStructure, err = WEAVE_ERROR_INCORRECT_STATE);

        LogReadWrite("%s element is a structure", __FUNCTION__);
        aOutSize = aFieldPtr->mNestedFieldDescriptors->mSize;
    }
    else
    {
        LogReadWrite("%s element is a primitive size", __FUNCTION__);
        aOutSize = sElementSize[aType];
    }

exit:
    return err;
}

/**
 * @brief
 *   A writer function that writes an array structure.
 *
 * @param aWriter[in]           The writer to use for writing out the structure
 *
 * @param aStructureData[in]    A pointer to the c-structure data to write based on the FieldDescriptor
 *
 * @param aFieldPtr[in]         FieldDescriptor to describe the array c struct + TLV
 *
 * @retval #WEAVE_NO_ERROR On success.
 *
 * @retval other          Other errors that mey be returned from the aWriter.
 *
 */
WEAVE_ERROR WriteArrayData(TLVWriter &aWriter, void *aStructureData, const FieldDescriptor * aFieldPtr)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    const bool writingInArray = true;
    SerializedFieldType type;
    ArrayLengthAndBuffer *array;
    const FieldDescriptor *fieldPtr = aFieldPtr;
    uint32_t elementSize = 0;

    // aStructureData should be pointing to the wrapped length and buffer structure
    array = static_cast<ArrayLengthAndBuffer *>(aStructureData);

    // the type of the array is next in the FieldDescriptors list
    fieldPtr++;

    // Error check to see that this returns a valid type
    type = fieldPtr->GetType();
    VerifyOrExit(IsValidFieldType(type), err = WEAVE_ERROR_INVALID_ARGUMENT);

    err = GetArrayElementSize(elementSize, fieldPtr, type);
    LogReadWrite("%s elementSize %d", "W", elementSize);

    for (uint32_t idx = 0; idx < array->mNumElements; idx++)
    {
        // Increment based on type
        aFieldPtr = fieldPtr;

        LogReadWrite("%s aStructureData 0x%x array 0x%x array->mNumElements %d array->mElementBuffer 0x%x idx %d idx * elementSize 0x%x", "W", aStructureData, array, array->mNumElements, array->mElementBuffer, idx, idx * elementSize);

        err = WriteDataForType(aWriter, static_cast<char *>(array->mElementBuffer) + idx * elementSize, aFieldPtr, type, writingInArray);
        SuccessOrExit(err);
    }

exit:
    return err;
}

/**
 * @brief
 *   A writer function to check whether data is nullable/nullified before
 *   writing to the TLV.
 *
 * @param aWriter[in]           The writer to use for writing out the structure
 *
 * @param aStructureData[in]    A pointer to the c-structure data to read
 *
 * @param aFieldPtr[inout]      FieldDescriptor to describe the fields and
 *                              TLV tag.  The function will increment
 *                              the pointer s.t. it will point to the
 *                              next element in the FieldDescriptor array
 *
 * @param aType[in]             The SerializedFieldType of the field
 *
 * @param aIsNullified[in]      The TLV tag will be nullified if this is true.
 *
 * @retval #WEAVE_NO_ERROR On success.
 *
 * @retval other          TLV errors while writing.
 *
 */
WEAVE_ERROR WriteNullableDataForType(TLVWriter &aWriter, void *aStructureData, const FieldDescriptor *&aFieldPtr, SerializedFieldType aType, bool aIsNullified)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    if (aIsNullified)
    {
        LogReadWrite("%s nullified", "W");

        err = aWriter.PutNull(ContextTag(aFieldPtr->mTVDContextTag));
        SuccessOrExit(err);

        aFieldPtr++;
    }
    else
    {
        err = WriteDataForType(aWriter, aStructureData, aFieldPtr, aType, false);
    }

exit:
    return err;
}

/**
 * @brief
 *   A writer function write a specific entry into the TLV based on structure data.
 *
 * @param aWriter[in]           The writer to use for writing out the structure
 *
 * @param aStructureData[in]    A pointer to the c-structure data to write
 *
 * @param aFieldPtr[inout]      FieldDescriptor to describe the fields and
 *                              TLV tag.  The function will increment
 *                              the pointer s.t. it will point to the
 *                              next element in the FieldDescritor array
 *
 * @param aType[in]             The SerializedFieldType of the field
 *
 * @param aInArray[in]          True if we're writing an array (use anonymous tag)
 *
 * @retval #WEAVE_NO_ERROR On success.
 *
 * @retval other          Other errors that mey be returned from the aWriter.
 *
 */
WEAVE_ERROR WriteDataForType(TLVWriter &aWriter, void *aStructureData, const FieldDescriptor *&aFieldPtr, SerializedFieldType aType, bool aInArray)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    TLVType containerType;

    uint64_t tag = aInArray ? AnonymousTag : ContextTag(aFieldPtr->mTVDContextTag);

    LogReadWrite("%s aStructureData 0x%x", "W", aStructureData);

    switch (aType)
    {
        case SerializedFieldTypeBoolean:
        {
            bool v = *static_cast<bool *>(aStructureData);

            LogReadWrite("%s boolean '%s'", "W", v ? "true" : "false");

            err = aWriter.PutBoolean(tag, v);
            SuccessOrExit(err);
            break;
        }

        case SerializedFieldTypeUInt8:
        {
            uint8_t v = *static_cast<uint8_t *>(aStructureData);

            LogReadWrite("%s uint8 %u", "W", v);

            err = aWriter.Put(tag, v);
            SuccessOrExit(err);
            break;
        }

        case SerializedFieldTypeUInt16:
        {
            uint16_t v = *static_cast<uint16_t *>(aStructureData);

            LogReadWrite("%s uint16 %u", "W", v);

            err = aWriter.Put(tag, v);
            SuccessOrExit(err);
            break;
        }

        case SerializedFieldTypeUInt32:
        {
            uint32_t v = *static_cast<uint32_t *>(aStructureData);

            LogReadWrite("%s uint32 %u", "W", v);

            err = aWriter.Put(tag, v);
            SuccessOrExit(err);
            break;
        }

        case SerializedFieldTypeUInt64:
        {
            uint64_t v = *static_cast<uint64_t *>(aStructureData);

            LogReadWrite("%s uint64 %llu", "W", v);

            err = aWriter.Put(tag, v);
            SuccessOrExit(err);
            break;
        }

        case SerializedFieldTypeInt8:
        {
            int8_t v = *static_cast<int8_t *>(aStructureData);

            LogReadWrite("%s int8 %d", "W", v);

            err = aWriter.Put(tag, v);
            SuccessOrExit(err);
            break;
        }

        case SerializedFieldTypeInt16:
        {
            int16_t v = *static_cast<int16_t *>(aStructureData);

            LogReadWrite("%s int16 %d", "W", v);

            err = aWriter.Put(tag, v);
            SuccessOrExit(err);
            break;
        }

        case SerializedFieldTypeInt32:
        {
            int32_t v = *static_cast<int32_t *>(aStructureData);

            LogReadWrite("%s int32 %d", "W", v);

            err = aWriter.Put(tag, v);
            SuccessOrExit(err);
            break;
        }

        case SerializedFieldTypeInt64:
        {
            int64_t v = *static_cast<int64_t *>(aStructureData);

            LogReadWrite("%s int64 %d", "W", v);

            err = aWriter.Put(tag, v);
            SuccessOrExit(err);
            break;
        }

        case SerializedFieldTypeFloatingPoint32:
        {
            float v = *static_cast<float *>(aStructureData);

#if WEAVE_CONFIG_SERIALIZATION_LOG_FLOATS
            LogReadWrite("%s float %f", "W", v);
#endif

            err = aWriter.Put(tag, v);
            SuccessOrExit(err);
            break;
        }

        case SerializedFieldTypeFloatingPoint64:
        {
            double v = *static_cast<double *>(aStructureData);

#if WEAVE_CONFIG_SERIALIZATION_LOG_FLOATS
            LogReadWrite("%s double %f", "W", v);
#endif

            err = aWriter.Put(tag, v);
            SuccessOrExit(err);
            break;
        }

        case SerializedFieldTypeUTF8String:
        {
            const char *v = *static_cast<const char**>(aStructureData);

            LogReadWrite("%s utf8string '%s'", "W", v);

            err = aWriter.PutString(tag, v);
            SuccessOrExit(err);
            break;
        }

        case SerializedFieldTypeByteString:
        {
            SerializedByteString v = *static_cast<SerializedByteString *>(aStructureData);

            LogReadWrite("%s bytestring len: %u", "W", v.mLen);

            err = aWriter.PutBytes(tag, v.mBuf, v.mLen);
            SuccessOrExit(err);
            break;
        }

        // We can hit this case when we have an array of structures.
        case SerializedFieldTypeStructure:
        {
            err = aWriter.StartContainer(tag,
                                         kTLVType_Structure,
                                         containerType);
            SuccessOrExit(err);

            LogReadWriteStartContainer("%s Structure Start", "W");

            err = SerializedDataToTLVWriter(aWriter, aStructureData, aFieldPtr->mNestedFieldDescriptors);
            SuccessOrExit(err);

            LogReadWriteEndContainer("%s Structure End", "W");

            err = aWriter.EndContainer(containerType);
            SuccessOrExit(err);
            break;
        }

        case SerializedFieldTypeArray:
        {
            err = aWriter.StartContainer(tag,
                                         kTLVType_Array,
                                         containerType);
            SuccessOrExit(err);

            LogReadWriteStartContainer("%s Array Start", "W");

            err = WriteArrayData(aWriter, aStructureData, aFieldPtr);
            if (err == WEAVE_END_OF_TLV)
            {
                err = WEAVE_NO_ERROR;
            }
            SuccessOrExit(err);

            LogReadWriteEndContainer("%s Array End", "W");

            err = aWriter.EndContainer(containerType);
            SuccessOrExit(err);

            // Skip over the elements.
            aFieldPtr++;

            break;
        }

        default:
        {
            err = WEAVE_ERROR_INVALID_ARGUMENT;
            break;
        }
    }
    aFieldPtr++;
exit:
    return err;
}

/**
 * @brief
 *   A reader function that reads an array structure.
 *
 * @param aReader[in]           The reader to use for reading in the structure
 *
 * @param aStructureData[in]    A pointer to the c-structure data to read based on the FieldDescriptor
 *
 * @param aFieldPtr[in]         FieldDescriptor to describe the array c struct + TLV
 *
 * @retval #WEAVE_NO_ERROR On success.
 *
 * @retval other          Other errors that mey be returned from the aWriter.
 *
 */
WEAVE_ERROR ReadArrayData(TLVReader &aReader, void *aStructureData, const FieldDescriptor * aFieldPtr, SerializationContext *aContext)
{
#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    const bool readingOutArray = true;
    SerializedFieldType type;
    ArrayLengthAndBuffer *array;
    const FieldDescriptor *fieldPtr = aFieldPtr;
    size_t count = 0;
    char *outputBuffer = NULL;
    size_t outputBufferNumItems = 0;
    size_t outputBufferNumResizes = 0;
    MemoryManagement *memMgmt = NULL;
    uint32_t elementSize = 0;
    bool endOfTLV = false;

    if ((aContext == NULL) || !(aContext->memMgmt.mem_alloc && aContext->memMgmt.mem_free && aContext->memMgmt.mem_realloc))
    {
        memMgmt = &sDefaultMemoryManagement;
    }
    else
    {
        memMgmt = &aContext->memMgmt;
    }

    // aStructureData should be pointing to the wrapped length and buffer structure
    array = static_cast<ArrayLengthAndBuffer *>(aStructureData);

    // Initialize.
    array->mNumElements = 0;
    array->mElementBuffer = NULL;

    // the type of the array is next in the FieldDescriptors list
    fieldPtr++;

    // Error check to see that this returns a valid type
    type = fieldPtr->GetType();
    VerifyOrExit(IsValidFieldType(type), err = WEAVE_ERROR_INVALID_ARGUMENT);

    err = GetArrayElementSize(elementSize, fieldPtr, type);
    LogReadWrite("%s elementSize %d", "R", elementSize);

    VerifyOrExit(array && (elementSize > 0), err = WEAVE_ERROR_INCORRECT_STATE);

    // Check to see whether there are any elements to read in.
    err = CheckForEndOfTLV(aReader, endOfTLV);
    SuccessOrExit(err);
    VerifyOrExit(endOfTLV == false, LogReadWrite("%s array contains no elements", "R"));

    // Read in each element.
    while (err == WEAVE_NO_ERROR)
    {
        // See whether the output buffer needs to be resized.
        if (count >= outputBufferNumItems)
        {
            outputBufferNumItems = (1 << ++outputBufferNumResizes);
            outputBuffer = (char *)memMgmt->mem_realloc((void *)outputBuffer, outputBufferNumItems*elementSize);
            VerifyOrExit(outputBuffer != NULL, err = WEAVE_ERROR_NO_MEMORY);

            LogReadWrite("%s allocating array memory at 0x%x", "R", outputBuffer);
        }

        // Increment based on type
        aFieldPtr = fieldPtr;

        LogReadWrite("%s aStructureData 0x%x array 0x%x count %d outputBuffer 0x%x count*elementSize 0x%x", "R", aStructureData, array, count, outputBuffer, count, count*elementSize);

        err = ReadDataForType(aReader, outputBuffer + count*elementSize, aFieldPtr, type, readingOutArray, aContext);
        if (err == WEAVE_END_OF_TLV)
        {
            count++;

            array->mNumElements = count;
            array->mElementBuffer = outputBuffer;
            err = WEAVE_NO_ERROR;
            break;
        }

        count++;
    }

exit:
    if (err != WEAVE_NO_ERROR)
    {
        if (outputBuffer != NULL)
        {
            memMgmt->mem_free(outputBuffer);
        }
    }

    return err;
#else // WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
    return WEAVE_ERROR_NOT_IMPLEMENTED;
#endif // WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
}

/**
 * @brief
 *   A reader function to see whether the next element in a TLVReader is the end of TLV.
 *
 * @param aReader[in]           The reader
 *
 * @param aEndOfTLV[inout]      A bool set to true if the next element in the reader is
 *                              the end of TLV, false otherwise
 *
 * @retval #WEAVE_NO_ERROR On success.
 *
 * @retval other          Other errors that may be returned from the aReader.
 *
 */
static WEAVE_ERROR CheckForEndOfTLV(TLVReader &aReader, bool &aEndOfTLV)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    TLVReader reader;

    // Initialize.
    aEndOfTLV = false;

    // Initialize a new reader, at aReader's position.
    reader.Init(aReader);

    // Position it the next element.
    err = reader.Next();
    VerifyOrExit(err == WEAVE_NO_ERROR || err == WEAVE_END_OF_TLV, );

    // If err was WEAVE_END_OF_TLV, we've the end of TLV and we can
    // safely suppress the error.
    if (err == WEAVE_END_OF_TLV)
    {
        aEndOfTLV = true;
        err = WEAVE_NO_ERROR;
    }

exit:
    return err;
}

/**
 * @brief
 *   A reader function to check whether data is nullable/nullified before
 *   reading from the TLV.
 *
 * @param aReader[in]           The reader to use for reading in the structure
 *
 * @param aStructureData[in]    A pointer to the c-structure data to read
 *
 * @param aFieldPtr[inout]      FieldDescriptor to describe the fields and
 *                              TLV tag.  The function will increment
 *                              the pointer s.t. it will point to the
 *                              next element in the FieldDescriptor array
 *
 * @param aType[in]             The SerializedFieldType of the field
 *
 * @param aIsNullified[out]     Set to indicate that a field is nullified.
 *
 * @retval #WEAVE_NO_ERROR On success.
 *
 * @retval other          TLV errors while writing.
 *
 */
WEAVE_ERROR ReadNullableDataForType(TLVReader &aReader, void *aStructureData, const FieldDescriptor *&aFieldPtr, SerializedFieldType aType, bool &aIsNullified, SerializationContext *aContext)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    if ((aFieldPtr->IsNullable()) && (aReader.GetType() == kTLVType_Null))
    {
        aIsNullified = true;
        aFieldPtr++;
        err = aReader.Next();
        LogReadWrite("%s nullified", "R");
    }
    else
    {
        aIsNullified = false;
        err = ReadDataForType(aReader, aStructureData, aFieldPtr, aType, false, aContext);
    }

    return err;
}

/**
 * @brief
 *   A reader function to read a specific entry from the TLV based on structure data.
 *
 * @param aReader[in]           The reader to use for reading in the structure
 *
 * @param aStructureData[in]    A pointer to the c-structure data to read
 *
 * @param aFieldPtr[inout]      FieldDescriptor to describe the fields and
 *                              TLV tag.  The function will increment
 *                              the pointer s.t. it will point to the
 *                              next element in the FieldDescritor array
 *
 * @param aType[in]             The SerializedFieldType of the field
 *
 * @param aInArray[in]          True if we're reading an array (use anonymous tag)
 *
 * @retval #WEAVE_NO_ERROR On success.
 *
 * @retval other          Other errors that mey be returned from the aReader.
 *
 */
WEAVE_ERROR ReadDataForType(TLVReader &aReader, void *aStructureData, const FieldDescriptor *&aFieldPtr, SerializedFieldType aType, bool aInArray, SerializationContext *aContext)
{
#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    TLVType containerType;
    MemoryManagement *memMgmt = NULL;

    if ((aContext == NULL) || !(aContext->memMgmt.mem_alloc && aContext->memMgmt.mem_free && aContext->memMgmt.mem_realloc))
    {
        memMgmt = &sDefaultMemoryManagement;
    }
    else
    {
        memMgmt = &aContext->memMgmt;
    }

    LogReadWrite("%s aStructureData 0x%x", "R", aStructureData);

    switch (aType)
    {
        case SerializedFieldTypeBoolean:
        {
            bool v = false;
            err = aReader.Get(v);
            SuccessOrExit(err);

            LogReadWrite("%s boolean '%s'", "R", v ? "true" : "false");

            *static_cast<bool *>(aStructureData) = v;
            break;
        }

        case SerializedFieldTypeUInt8:
        {
            uint8_t v = 0;
            err = aReader.Get(v);
            SuccessOrExit(err);

            LogReadWrite("%s uint8 %d", "R", v);

            *static_cast<uint8_t *>(aStructureData) = v;
            break;
        }

        case SerializedFieldTypeUInt16:
        {
            uint16_t v = 0;
            err = aReader.Get(v);
            SuccessOrExit(err);

            LogReadWrite("%s unt16 %u", "R", v);

            *static_cast<uint16_t *>(aStructureData) = v;
            break;
        }

        case SerializedFieldTypeUInt32:
        {
            uint32_t v = 0;
            err = aReader.Get(v);
            SuccessOrExit(err);

            LogReadWrite("%s uint32 %u", "R", v);

            *static_cast<uint32_t *>(aStructureData) = v;
            break;
        }

        case SerializedFieldTypeUInt64:
        {
            uint64_t v = 0;
            err = aReader.Get(v);
            SuccessOrExit(err);

            LogReadWrite("%s uint64 %llu", "R", v);

            *static_cast<uint64_t *>(aStructureData) = v;
            break;
        }

        case SerializedFieldTypeInt8:
        {
            int8_t v = 0;
            err = aReader.Get(v);
            SuccessOrExit(err);

            LogReadWrite("%s int8 %d", "R", v);

            *static_cast<int8_t *>(aStructureData) = v;
            break;
        }

        case SerializedFieldTypeInt16:
        {
            int16_t v = 0;
            err = aReader.Get(v);
            SuccessOrExit(err);

            LogReadWrite("%s int16 %d", "R", v);

            *static_cast<int16_t *>(aStructureData) = v;
            break;
        }

        case SerializedFieldTypeInt32:
        {
            int32_t v = 0;
            err = aReader.Get(v);
            SuccessOrExit(err);

            LogReadWrite("%s int32 %d", "R", v);

            *static_cast<int32_t *>(aStructureData) = v;
            break;
        }

        case SerializedFieldTypeInt64:
        {
            int64_t v = 0;
            err = aReader.Get(v);
            SuccessOrExit(err);

            LogReadWrite("%s int64 %d", "R", v);

            *static_cast<int8_t *>(aStructureData) = v;
            break;
        }

        case SerializedFieldTypeFloatingPoint32:
        {
            double v = 0;
            err = aReader.Get(v);
            SuccessOrExit(err);

#if WEAVE_CONFIG_SERIALIZATION_LOG_FLOATS
            LogReadWrite("%s float %f", "R", v);
#endif

            *static_cast<float *>(aStructureData) = (float)v;
            break;
        }

        case SerializedFieldTypeFloatingPoint64:
        {
            double v = 0;
            err = aReader.Get(v);
            SuccessOrExit(err);

#if WEAVE_CONFIG_SERIALIZATION_LOG_FLOATS
            LogReadWrite("%s double %f", "R", v);
#endif

            *static_cast<double *>(aStructureData) = v;
            break;
        }

        case SerializedFieldTypeUTF8String:
        {
            char *dst = NULL;
            // TLV Strings are not null terminated
            uint32_t length = aReader.GetLength() + 1;

            dst = (char *)memMgmt->mem_alloc(length);
            VerifyOrExit(dst != NULL, err = WEAVE_ERROR_NO_MEMORY);

            err = aReader.GetString(dst, length);
            SuccessOrExit(err);

            LogReadWrite("%s utf8string '%s' allocating %d bytes at %p", "R", dst, length, dst);
            *static_cast<char**>(aStructureData) = dst;
            break;
        }

        case SerializedFieldTypeByteString:
        {
            SerializedByteString byteString;
            byteString.mLen = aReader.GetLength();

            byteString.mBuf = static_cast<uint8_t *>(memMgmt->mem_alloc(byteString.mLen));
            VerifyOrExit(byteString.mBuf != NULL, err = WEAVE_ERROR_NO_MEMORY);
            aReader.GetBytes(byteString.mBuf, byteString.mLen);

            LogReadWrite("%s bytestring allocated %d bytes at %p", "R", byteString.mLen, byteString.mBuf);
            *static_cast<SerializedByteString *>(aStructureData) = byteString;
            break;
        }

        // We can hit this case when we have an array of structures.
        case SerializedFieldTypeStructure:
            err = aReader.EnterContainer(containerType);
            SuccessOrExit(err);
            VerifyOrExit(aReader.GetContainerType() == nl::Weave::TLV::kTLVType_Structure, err = WEAVE_ERROR_WRONG_TLV_TYPE);

            err = aReader.Next();
            if (err == WEAVE_END_OF_TLV)
                err = WEAVE_NO_ERROR;
            SuccessOrExit(err);

            LogReadWriteStartContainer("%s Structure Start", "R");

            err = TLVReaderToDeserializedData(aReader, aStructureData, aFieldPtr->mNestedFieldDescriptors, aContext);
            if (err == WEAVE_END_OF_TLV)
            {
                err = WEAVE_NO_ERROR;
            }
            SuccessOrExit(err);

            LogReadWriteEndContainer("%s Structure End", "R");

            err = aReader.ExitContainer(containerType);
            break;
        case SerializedFieldTypeArray:
        {
            err = aReader.EnterContainer(containerType);
            SuccessOrExit(err);
            VerifyOrExit(aReader.GetContainerType() == nl::Weave::TLV::kTLVType_Array, err = WEAVE_ERROR_WRONG_TLV_TYPE);

            err = aReader.Next();
            if (err == WEAVE_END_OF_TLV)
                err = WEAVE_NO_ERROR;
            SuccessOrExit(err);

            LogReadWriteStartContainer("%s Array Start", "R");

            err = ReadArrayData(aReader, aStructureData, aFieldPtr, aContext);
            SuccessOrExit(err);

            LogReadWriteEndContainer("%s Array End", "R");

            err = aReader.ExitContainer(containerType);

            // Skip over elements.
            aFieldPtr++;

            break;
        }
        default:
            err = WEAVE_ERROR_INVALID_ARGUMENT;
            break;
    }
    aFieldPtr++;
    err = aReader.Next();
exit:
    return err;
#else // WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
    return WEAVE_ERROR_NOT_IMPLEMENTED;
#endif // WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
}

/**
 * @brief
 *   A helper function to find the location of the nullified fields array
 *   located at the end of the C struct.
 *
 * The __nullified_fields__ member of the C struct is expected to be located
 * directly after the last member described by the array of FieldDescriptors.
 * It is not in the list of field descriptors, as it's meant to be a hidden
 * utility for creators and consumers of nullable events. This struct member
 * does not exist for events with no nullable fields, however by construction
 * of setters and getters, no out of bounds accesses should occur.
 *
 * @param aStructureData[in]    A pointer to the c-struct
 *
 * @param aFieldDescriptors[in] SchemaFieldDescriptors to describe the c struct
 *
 * @param aNullifiedFields[out] A pointer to __nullified_fields__ member of
 *                              the c struct
 *
 * @retval #WEAVE_NO_ERROR               On success.
 *
 * @retval #WEAVE_ERROR_INVALID_ARGUMENT If the field descriptor pointer is NULL.
 *
 * @retval #WEAVE_ERROR_INCORRECT_STATE  If the format of the field descriptors
 *                                      doesn't match expectation.
 *
 *
 */
static WEAVE_ERROR FindNullifiedFieldsArray(void *aStructureData, const SchemaFieldDescriptor *aSchemaDescriptor, uint8_t *&aNullifiedFields)
{
    uint32_t offset;
    WEAVE_ERROR err;
    const FieldDescriptor *lastFieldDescriptor = &(aSchemaDescriptor->mFields[aSchemaDescriptor->mNumFieldDescriptorElements - 1]);

    err = GetArrayElementSize(offset, lastFieldDescriptor, lastFieldDescriptor->GetType());
    SuccessOrExit(err);

    aNullifiedFields = static_cast<uint8_t *>(aStructureData) + lastFieldDescriptor->mOffset + offset;
exit:
    return err;
}

/**
 * @brief
 *   A writer function to convert a data structure into a TLV structure. Uses
 *   a SchemaFieldDescriptor to interpret the data structure and write to the TLV.
 *
 * @param aWriter[in]           The writer to use for writing out the structure
 *
 * @param aStructureData[in]    A pointer to the c-structure data to write based on the SchemaFieldDescriptor
 *
 * @param aFieldDescriptors[in] SchemaFieldDescriptors to describe the c struct + TLV
 *
 * @retval #WEAVE_NO_ERROR On success.
 *
 * @retval other          Other errors that mey be returned from the aWriter.
 *
 */
WEAVE_ERROR SerializedDataToTLVWriter(TLVWriter &aWriter, void *aStructureData, const SchemaFieldDescriptor *aFieldDescriptors)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    const FieldDescriptor *fieldPtr = aFieldDescriptors->mFields;
    const FieldDescriptor *endFieldPtr = &(aFieldDescriptors->mFields[aFieldDescriptors->mNumFieldDescriptorElements]);
    int nullifiedBitIdx = 0;
    uint8_t *nullifiedFields = NULL;

    err = FindNullifiedFieldsArray(aStructureData, aFieldDescriptors, nullifiedFields);
    SuccessOrExit(err);

    while (fieldPtr < endFieldPtr)
    {
        bool aIsNullified = fieldPtr->IsNullable() &&
            GET_FIELD_NULLIFIED_BIT(nullifiedFields, nullifiedBitIdx);

        if (fieldPtr->IsNullable())
        {
            nullifiedBitIdx++;
        }

        err = WriteNullableDataForType(aWriter,
                               static_cast<char *>(aStructureData) + fieldPtr->mOffset,
                               fieldPtr,
                               fieldPtr->GetType(),
                               aIsNullified);
        SuccessOrExit(err);
    }

exit:
    return err;
}


/**
 * @brief
 *   A wrapper writer function that surrounds the SerializedDataToTLVWriter with a container.
 *   Also splits up an StructureSchemaPointerPair into structure data and descriptors to pass through.
 *
 * @param aWriter[in]           The writer to use for writing out the structure
 *
 * @param aAppData[in]          StructureSchemaPointerPair that contains a pointer to
 *                              structure data and field descriptors. void* due to prototype
 *
 * @retval #WEAVE_NO_ERROR On success.
 *
 * @retval other          Other errors that mey be returned from the aWriter.
 *
 */
WEAVE_ERROR SerializedDataToTLVWriterHelper(TLVWriter &aWriter, uint8_t aDataTag, void *aAppData)
{
    StructureSchemaPointerPair *structureSchemaPair = static_cast<StructureSchemaPointerPair *>(aAppData);
    const bool inArray = false;
    FieldDescriptor descriptor = { structureSchemaPair->mFieldSchema,
                                   0,
                                   static_cast<uint8_t>(SerializedFieldTypeStructure),
                                   aDataTag };
    const FieldDescriptor * pDescriptor = & descriptor;

#if WEAVE_CONFIG_SERIALIZATION_DEBUG_LOGGING
    sIndentationLevel = 0;
#endif

    return WriteDataForType(aWriter, structureSchemaPair->mStructureData, pDescriptor, SerializedFieldTypeStructure, inArray);
}

/**
 * @brief
 *   A reader function to convert TLV into a C-struct. Uses
 *   a SchemaFieldDescriptor to interpret the data structure.
 *
 *   It must be robust both to encoutering unknown fields and to not
 *   encountering an expected field.
 *
 * @param aReader[in]           The reader to use for reading in the data
 *
 * @param aStructureData[in]        A pointer to the destination c-structure data into which we'll read
 *                              based on the SchemaFieldDescriptor
 *
 * @param aFieldDescriptors[in] SchemaFieldDescriptors to describe the c struct + TLV
 *
 * @retval #WEAVE_NO_ERROR On success.
 *
 * @retval other          Other errors that may be returned from the aReader.
 *
 */
WEAVE_ERROR TLVReaderToDeserializedData(nl::Weave::TLV::TLVReader &aReader,
                                        void *aStructureData,
                                        const SchemaFieldDescriptor *aFieldDescriptors,
                                        SerializationContext *aContext)
{
#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    const FieldDescriptor *fieldPtr = aFieldDescriptors->mFields;
    const FieldDescriptor *endFieldPtr = &(aFieldDescriptors->mFields[aFieldDescriptors->mNumFieldDescriptorElements]);
    int nullifiedBitIdx = 0;
    uint8_t *nullifiedFields = NULL;

    err = FindNullifiedFieldsArray(aStructureData, aFieldDescriptors, nullifiedFields);
    SuccessOrExit(err);

    // while there are remaining fields to be parsed
    while (fieldPtr < endFieldPtr)
    {
        // Tentatively searches for the next schema field matching the TLV tag in head.
        const FieldDescriptor *searchFieldPtr = fieldPtr;
        int searchNullifiedBitIdx = nullifiedBitIdx;
        while (searchFieldPtr < endFieldPtr
            && TagNumFromTag(aReader.GetTag()) != searchFieldPtr->mTVDContextTag)
        {
            // Sets any nullable skipped fields to NULL
            if (searchFieldPtr->IsNullable())
            {
                SET_FIELD_NULLIFIED_BIT(nullifiedFields, searchNullifiedBitIdx);
                searchNullifiedBitIdx++;
            }

            searchFieldPtr++;
        }

        // If schema found for TLV tag
        if (searchFieldPtr != endFieldPtr)
        {
            // Commit to found schema
            fieldPtr = searchFieldPtr;
            nullifiedBitIdx = searchNullifiedBitIdx;

            bool isNullified = false;
            bool isNullable = fieldPtr->IsNullable();
            err = ReadNullableDataForType(aReader,
                                  static_cast<char *>(aStructureData) + fieldPtr->mOffset,
                                  fieldPtr,
                                  fieldPtr->GetType(),
                                  isNullified,
                                  aContext);

            if (isNullable)
            {
                if (isNullified)
                {
                    SET_FIELD_NULLIFIED_BIT(nullifiedFields, nullifiedBitIdx);
                }
                else
                {
                    CLEAR_FIELD_NULLIFIED_BIT(nullifiedFields, nullifiedBitIdx);
                }

                nullifiedBitIdx++;
            }

            if (err == WEAVE_END_OF_TLV)
            {
                err = WEAVE_NO_ERROR;
                break;
            }
            SuccessOrExit(err);
        }
        else
        {
            // If schema not found, move TLV forward to skip unknown field.
            err = aReader.Next();
            if (err == WEAVE_END_OF_TLV)
            {
                err = WEAVE_NO_ERROR;
                break;
            }
        }
    }

    // Sets any unparsed field (TLV ended prematurely) to NULL
    while (fieldPtr < endFieldPtr)
    {
        if (fieldPtr->IsNullable())
        {
            SET_FIELD_NULLIFIED_BIT(nullifiedFields, nullifiedBitIdx);
            nullifiedBitIdx++;
        }
        fieldPtr++;
    }

exit:
    return err;
#else // WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
    return WEAVE_ERROR_NOT_IMPLEMENTED;
#endif // WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
}

/**
 * @brief
 *   A wrapper reader function that surrounds the TLVReaderToDeserializedData with a container.
 *   Also splits up an StructureSchemaPointerPair into structure data and descriptors to pass through.
 *
 * @param aReader[in]           The reader to use for writing out the structure
 *
 * @param aProfileId[in]        Unused for the moment
 *
 * @param aStructureType[in]    Unused for the moment
 *
 * @param aAppData[in]          StructureSchemaPointerPair that contains a pointer to
 *                              structure data and field descriptors. void* due to prototype
 *
 * @retval #WEAVE_NO_ERROR On success.
 *
 * @retval other          Other errors that mey be returned from the aReader.
 *
 */
WEAVE_ERROR TLVReaderToDeserializedDataHelper(nl::Weave::TLV::TLVReader &aReader,
                                              uint8_t aDataTag,
                                              void *aAppData,
                                              SerializationContext *aContext)
{
#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    StructureSchemaPointerPair *structureSchemaPair = static_cast<StructureSchemaPointerPair *>(aAppData);
    const bool inArray = false;
    FieldDescriptor descriptor = { structureSchemaPair->mFieldSchema,
                                   0,
                                   static_cast<uint8_t>(SerializedFieldTypeStructure),
                                   aDataTag };
    const FieldDescriptor * pDescriptor = & descriptor;

#if WEAVE_CONFIG_SERIALIZATION_DEBUG_LOGGING
    sIndentationLevel = 0;
#endif

    err = ReadDataForType(aReader, structureSchemaPair->mStructureData, pDescriptor, SerializedFieldTypeStructure, inArray, aContext);
    if (err == WEAVE_END_OF_TLV)
    {
        err = WEAVE_NO_ERROR;
    }

    return err;
#else // WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
    return WEAVE_ERROR_NOT_IMPLEMENTED;
#endif // WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
}

WEAVE_ERROR DeallocateDeserializedArray(void *aArrayData,
                                        const SchemaFieldDescriptor *aFieldDescriptors,
                                        SerializationContext *aContext/* = NULL*/)
{
#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    MemoryManagement *memMgmt = NULL;
    ArrayLengthAndBuffer *array = NULL;

    // aStructureData should be pointing to the wrapped length and buffer structure
    array = static_cast<ArrayLengthAndBuffer *>(aArrayData);

    if ((aContext == NULL) || !(aContext->memMgmt.mem_alloc && aContext->memMgmt.mem_free && aContext->memMgmt.mem_realloc))
    {
        memMgmt = &sDefaultMemoryManagement;
    }
    else
    {
        memMgmt = &aContext->memMgmt;
    }

    if (aFieldDescriptors == NULL)
    {
        LogReadWrite("%s Freeing array of primitive type at 0x%x", "R", array->mElementBuffer);

        // The elements are of a primitive type, we can free the array now.
        memMgmt->mem_free(array->mElementBuffer);
    }
    else
    {
        // The elements are structures, and we need to deallocate each one.
        for (uint32_t i = 0; i < array->mNumElements; i++)
        {
            char *element = static_cast<char *>(array->mElementBuffer) + i*aFieldDescriptors->mSize;
            err = DeallocateDeserializedStructure(element, aFieldDescriptors, aContext);
            SuccessOrExit(err);
        }

        LogReadWrite("%s Freeing array of structures at 0x%x", "R", array->mElementBuffer);

        // Free the array now.
        memMgmt->mem_free(array->mElementBuffer);
    }

exit:
    return err;
#else // WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
    return WEAVE_ERROR_NOT_IMPLEMENTED;
#endif // WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
}

WEAVE_ERROR DeallocateDeserializedStructure(void *aStructureData,
                                            const SchemaFieldDescriptor *aFieldDescriptors,
                                            SerializationContext *aContext/* = NULL*/)
{
#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    const FieldDescriptor *fieldPtr = aFieldDescriptors->mFields;
    const FieldDescriptor *endFieldPtr = &(aFieldDescriptors->mFields[aFieldDescriptors->mNumFieldDescriptorElements]);
    MemoryManagement *memMgmt = NULL;

    if ((aContext == NULL) || !(aContext->memMgmt.mem_alloc && aContext->memMgmt.mem_free && aContext->memMgmt.mem_realloc))
    {
        memMgmt = &sDefaultMemoryManagement;
    }
    else
    {
        memMgmt = &aContext->memMgmt;
    }

    while (fieldPtr < endFieldPtr)
    {
        char *currentFieldData = static_cast<char *>(aStructureData) + fieldPtr->mOffset;

        switch (fieldPtr->GetType())
        {
            case SerializedFieldTypeStructure:
            {
                err = DeallocateDeserializedStructure(currentFieldData,
                                                      fieldPtr->mNestedFieldDescriptors,
                                                      aContext);
                SuccessOrExit(err);
                break;
            }

            case SerializedFieldTypeArray:
            {
                // fieldPtr is telling us we have an array, but the
                // elements reside at the next field.  Increment
                // fieldPtr to get us to the elements, but leave
                // currentFieldData where it is.
                fieldPtr++;

                err = DeallocateDeserializedArray(currentFieldData,
                                                  fieldPtr->mNestedFieldDescriptors,
                                                  aContext);
                SuccessOrExit(err);
                break;
            }

            case SerializedFieldTypeUTF8String:
            {
                char *str = *((char **)currentFieldData);

                LogReadWrite("%s Freeing UTF8String '%s' at 0x%x", "R", str, str);

                // Go ahead and free it here.
                memMgmt->mem_free(str);
                break;
            }

            default:
            {
                // A primitive type, doesn't need to be deallocated.
                break;
            }
        }

        fieldPtr++;
    }

exit:
    if (err == WEAVE_END_OF_TLV)
    {
        err = WEAVE_NO_ERROR;
    }
    return err;
#else // WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
    return WEAVE_ERROR_NOT_IMPLEMENTED;
#endif // WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION
}

} // nl
