/*
 *
 *    Copyright (c) 2018 Nest Labs, Inc.
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
 *    @file
 *          Utilities for interacting with the Nordic Flash Data Storage (FDS) API.
 */

#include <Weave/DeviceLayer/internal/WeaveDeviceLayerInternal.h>
#include <Weave/DeviceLayer/nRF5/NRF5Config.h>
#include <Weave/Core/WeaveEncoding.h>

#include "FreeRTOS.h"
#include "semphr.h"

#include "fds.h"
#include "mem_manager.h"

namespace nl {
namespace Weave {
namespace DeviceLayer {
namespace Internal {

namespace {

struct FDSAsyncOp
{
    enum
    {
        kNone                           = 0,
        kAddRecord,
        kAddOrUpdateRecord,
        kDeleteRecord,
        kDeleteFile,
        kGC,
        kInit,
        kWaitQueueSpaceAvailable
    };

    ret_code_t Result;
    uint16_t FileId;
    uint16_t RecordKey;
    uint8_t OpType;
};

volatile FDSAsyncOp sActiveAsyncOp;
SemaphoreHandle_t sAsyncOpCompletionSem;

constexpr uint16_t kFDSWordSize = 4;

inline uint16_t FDSWords(size_t s)
{
    return (s + (kFDSWordSize - 1)) / kFDSWordSize;
}

inline WEAVE_ERROR MapFDSError(ret_code_t fdsRes)
{
    // TODO: implement this
    return fdsRes;
}

WEAVE_ERROR OpenRecord(NRF5Config::Key key, fds_record_desc_t & recDesc, fds_flash_record_t & rec)
{
    WEAVE_ERROR err;
    ret_code_t fdsRes;
    fds_find_token_t findToken;

    // Search for the requested record.  Return a "Config Not Found" error if it doesn't exist.
    memset(&findToken, 0, sizeof(findToken));
    fdsRes = fds_record_find(NRF5Config::GetFileId(key), NRF5Config::GetRecordKey(key), &recDesc, &findToken);
    err = (fdsRes == FDS_ERR_NOT_FOUND) ? WEAVE_DEVICE_ERROR_CONFIG_NOT_FOUND : MapFDSError(fdsRes);
    SuccessOrExit(err);

    // Open the record for reading.
    fdsRes = fds_record_open(&recDesc, &rec);
    err = MapFDSError(fdsRes);
    SuccessOrExit(err);

exit:
    return err;
}

WEAVE_ERROR DoAsyncFDSOp(uint8_t opType, fds_record_t & rec)
{
    WEAVE_ERROR err;
    ret_code_t fdsRes;
    fds_record_desc_t recDesc;
    bool gcPerformed = false;
    bool updateNeeded = false;

    VerifyOrExit(sActiveAsyncOp.OpType == FDSAsyncOp::kNone, err = WEAVE_ERROR_INCORRECT_STATE);

    while (true)
    {
        // If performing an AddOrUpdateRecord or a DeleteRecord, search for an existing record with the same key.
        if (opType == FDSAsyncOp::kAddOrUpdateRecord || opType == FDSAsyncOp::kDeleteRecord)
        {
            fds_find_token_t findToken;
            memset(&findToken, 0, sizeof(findToken));
            fdsRes = fds_record_find(rec.file_id, rec.key, &recDesc, &findToken);
            VerifyOrExit(fdsRes == FDS_SUCCESS || fdsRes == FDS_ERR_NOT_FOUND, err = MapFDSError(fdsRes));

            // If performing a DeleteRecord and no record was found, simply return success.
            if (opType == FDSAsyncOp::kDeleteRecord && fdsRes == FDS_ERR_NOT_FOUND)
            {
                ExitNow(err = WEAVE_NO_ERROR);
            }

            updateNeeded = (fdsRes != FDS_ERR_NOT_FOUND);
        }

        // Initiate the requested operation.
        sActiveAsyncOp.FileId = rec.file_id;
        sActiveAsyncOp.RecordKey = rec.key;
        sActiveAsyncOp.Result = FDS_SUCCESS;
        sActiveAsyncOp.OpType = opType;
        switch (opType)
        {
        case FDSAsyncOp::kAddOrUpdateRecord:
            if (updateNeeded)
            {
                fdsRes = fds_record_update(&recDesc, &rec);
                break;
            }
            // fall through
        case FDSAsyncOp::kAddRecord:
            fdsRes = fds_record_write(NULL, &rec);
            break;
        case FDSAsyncOp::kDeleteRecord:
            fdsRes = fds_record_delete(&recDesc);
            break;
        case FDSAsyncOp::kDeleteFile:
            fdsRes = fds_file_delete(rec.file_id);
            break;
        case FDSAsyncOp::kGC:
            fdsRes = fds_gc();
            break;
        case FDSAsyncOp::kWaitQueueSpaceAvailable:
            // In this case, arrange to wait for any operation to complete, which coincides with
            // space on the operation queue being available.
            fdsRes = FDS_SUCCESS;
            break;
        }

        // If the operation was queued successfully, wait for it to complete and retrieve the result.
        if (fdsRes == FDS_SUCCESS)
        {
            xSemaphoreTake(sAsyncOpCompletionSem, portMAX_DELAY);
            fdsRes = sActiveAsyncOp.Result;
        }

        // Clear the active operation in case it wasn't done by the event handler.
        sActiveAsyncOp.OpType = FDSAsyncOp::kNone;

        // Return immediately if the operation completed successfully.
        if (fdsRes == FDS_SUCCESS)
        {
            ExitNow(err = WEAVE_NO_ERROR);
        }

        // If the operation failed for lack of flash space...
        if (fdsRes == FDS_ERR_NO_SPACE_IN_FLASH)
        {
            // If we've already performed a garbage collection, fail immediately.
            if (gcPerformed)
            {
                ExitNow(err = MapFDSError(fdsRes));
            }

            // Request a garbage collection cycle and wait for it to complete.
            err = DoAsyncFDSOp(FDSAsyncOp::kGC, rec);
            SuccessOrExit(err);

            // Repeat the requested operation.
            continue;
        }

        // If the write/update failed because the operation queue is full, wait for
        // space to become available and repeat the requested operation.
        if (fdsRes == FDS_ERR_NO_SPACE_IN_QUEUES)
        {
            err = DoAsyncFDSOp(FDSAsyncOp::kWaitQueueSpaceAvailable, rec);
            SuccessOrExit(err);
            continue;
        }

        // If the operation timed out, simply try it again.
        if (fdsRes == FDS_ERR_OPERATION_TIMEOUT)
        {
            continue;
        }

        // Otherwise fail with an unrecoverable error.
        ExitNow(err = MapFDSError(fdsRes));
    }

exit:
    return err;
}

WEAVE_ERROR DoAsyncFDSOp(uint8_t opType)
{
    fds_record_t rec;
    memset(&rec, 0, sizeof(rec));
    return DoAsyncFDSOp(opType, rec);
}

void HandleFDSEvent(const fds_evt_t * fdsEvent)
{
    switch (sActiveAsyncOp.OpType)
    {
    case FDSAsyncOp::kNone:
        // Ignore the event if there's no outstanding async operation.
        return;
    case FDSAsyncOp::kAddRecord:
    case FDSAsyncOp::kAddOrUpdateRecord:
        // Ignore the event if its not a WRITE or UPDATE, or if its for a different file/record.
        if ((fdsEvent->id != FDS_EVT_WRITE && fdsEvent->id != FDS_EVT_UPDATE) ||
            fdsEvent->write.file_id != sActiveAsyncOp.FileId ||
            fdsEvent->write.record_key != sActiveAsyncOp.RecordKey)
        {
            return;
        }
        break;
    case FDSAsyncOp::kDeleteRecord:
        // Ignore the event if its not a DEL_RECORD, or if its for a different file/record.
        if (fdsEvent->id != FDS_EVT_DEL_RECORD ||
            fdsEvent->write.file_id != sActiveAsyncOp.FileId ||
            fdsEvent->write.record_key != sActiveAsyncOp.RecordKey)
        {
            return;
        }
        break;
    case FDSAsyncOp::kDeleteFile:
        // Ignore the event if its not a DEL_FILE or its for a different file.
        if (fdsEvent->id != FDS_EVT_DEL_FILE || fdsEvent->write.file_id != sActiveAsyncOp.FileId)
        {
            return;
        }
        break;
    case FDSAsyncOp::kGC:
        // Ignore the event if its not a GC.
        if (fdsEvent->id != FDS_EVT_GC)
        {
            return;
        }
        break;
    case FDSAsyncOp::kWaitQueueSpaceAvailable:
        break;
    }

    // Capture the result and mark the operation as complete.
    sActiveAsyncOp.Result = fdsEvent->result;
    sActiveAsyncOp.OpType = FDSAsyncOp::kNone;

    // Signal the Weave thread that the operation has completed.
#if defined(SOFTDEVICE_PRESENT) && SOFTDEVICE_PRESENT
    {
        BaseType_t yieldRequired = xSemaphoreGiveFromISR(sAsyncOpCompletionSem, &yieldRequired);
        if (yieldRequired == pdTRUE)
        {
            portYIELD_FROM_ISR(yieldRequired);
        }
    }
#else // defined(SOFTDEVICE_PRESENT) && SOFTDEVICE_PRESENT
    xSemaphoreGive(sAsyncOpCompletionSem);
#endif // !defined(SOFTDEVICE_PRESENT) || !SOFTDEVICE_PRESENT
}

} // unnamed namespace


WEAVE_ERROR NRF5Config::Init()
{
    WEAVE_ERROR err;
    ret_code_t fdsRes;

    // Create a semaphore to signal the completion of async FDS operations.
    sAsyncOpCompletionSem = xSemaphoreCreateBinary();
    VerifyOrExit(sAsyncOpCompletionSem != NULL, err = WEAVE_ERROR_NO_MEMORY);

    // Register an FDS event handler.
    fdsRes = fds_register(HandleFDSEvent);
    SuccessOrExit(err = MapFDSError(fdsRes));

    // Initialize the FDS module.
    err = DoAsyncFDSOp(FDSAsyncOp::kInit);
    SuccessOrExit(err);

exit:
    return err;
}

WEAVE_ERROR NRF5Config::ReadConfigValue(Key key, bool & val)
{
    WEAVE_ERROR err;
    fds_record_desc_t recDesc;
    fds_flash_record_t rec;
    uint32_t wordVal;
    bool needClose = false;

    err = OpenRecord(key, recDesc, rec);
    SuccessOrExit(err);
    needClose = true;

    VerifyOrExit(rec.p_header->length_words == 1, err = WEAVE_ERROR_INVALID_ARGUMENT);

    wordVal = Encoding::LittleEndian::Get32((const uint8_t *)rec.p_data);
    val = (wordVal != 0);

exit:
    if (needClose)
    {
        fds_record_close(&recDesc);
    }
    return err;
}

WEAVE_ERROR NRF5Config::ReadConfigValue(Key key, uint32_t & val)
{
    WEAVE_ERROR err;
    fds_record_desc_t recDesc;
    fds_flash_record_t rec;
    bool needClose = false;

    err = OpenRecord(key, recDesc, rec);
    SuccessOrExit(err);
    needClose = true;

    VerifyOrExit(rec.p_header->length_words == 1, err = WEAVE_ERROR_INVALID_ARGUMENT);

    val = Encoding::LittleEndian::Get32((const uint8_t *)rec.p_data);

exit:
    if (needClose)
    {
        fds_record_close(&recDesc);
    }
    return err;
}

WEAVE_ERROR NRF5Config::ReadConfigValue(Key key, uint64_t & val)
{
    WEAVE_ERROR err;
    fds_record_desc_t recDesc;
    fds_flash_record_t rec;
    bool needClose = false;

    err = OpenRecord(key, recDesc, rec);
    SuccessOrExit(err);
    needClose = true;

    VerifyOrExit(rec.p_header->length_words == 2, err = WEAVE_ERROR_INVALID_ARGUMENT);

    val = Encoding::LittleEndian::Get64((const uint8_t *)rec.p_data);

exit:
    if (needClose)
    {
        fds_record_close(&recDesc);
    }
    return err;
}

WEAVE_ERROR NRF5Config::ReadConfigValueStr(Key key, char * buf, size_t bufSize, size_t & outLen)
{
    WEAVE_ERROR err;
    fds_flash_record_t rec;
    fds_record_desc_t recDesc;
    bool needClose = false;
    const uint8_t * strEnd;

    err = OpenRecord(key, recDesc, rec);
    SuccessOrExit(err);
    needClose = true;

    strEnd = (const uint8_t *)memchr(rec.p_data, '0', rec.p_header->length_words * kFDSWordSize);
    VerifyOrExit(strEnd != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);

    outLen = strEnd - (const uint8_t *)rec.p_data;

    VerifyOrExit(bufSize > outLen, err = WEAVE_ERROR_BUFFER_TOO_SMALL);

    memcpy(buf, rec.p_data, outLen + 1);

exit:
    if (needClose)
    {
        fds_record_close(&recDesc);
    }
    return err;
}

WEAVE_ERROR NRF5Config::ReadConfigValueBin(Key key, uint8_t * buf, size_t bufSize, size_t & outLen)
{
    WEAVE_ERROR err;
    fds_flash_record_t rec;
    fds_record_desc_t recDesc;
    bool needClose = false;

    err = OpenRecord(key, recDesc, rec);
    SuccessOrExit(err);
    needClose = true;

    VerifyOrExit(rec.p_header->length_words >= 2, err = WEAVE_ERROR_INVALID_ARGUMENT);

    // First two bytes are length.
    outLen = Encoding::LittleEndian::Get16((const uint8_t *)rec.p_data);

    VerifyOrExit((rec.p_header->length_words * kFDSWordSize) >= (outLen + 2), err = WEAVE_ERROR_INVALID_ARGUMENT);

    VerifyOrExit(bufSize >= outLen, err = WEAVE_ERROR_BUFFER_TOO_SMALL);

    memcpy(buf, ((const uint8_t *)rec.p_data) + 2, outLen);

exit:
    if (needClose)
    {
        fds_record_close(&recDesc);
    }
    return err;
}

WEAVE_ERROR NRF5Config::WriteConfigValue(Key key, bool val)
{
    WEAVE_ERROR err;
    fds_record_t rec;
    uint32_t storedVal = (val) ? 1 : 0;

    memset(&rec, 0, sizeof(rec));
    rec.file_id = GetFileId(key);
    rec.key = GetRecordKey(key);
    rec.data.p_data = &storedVal;
    rec.data.length_words = 1;

    err = DoAsyncFDSOp(FDSAsyncOp::kAddOrUpdateRecord, rec);
    SuccessOrExit(err);

    WeaveLogProgress(DeviceLayer, "FDS set: %04" PRIX16 "/%04" PRIX16 " = %s", GetFileId(key), GetRecordKey(key), val ? "true" : "false");

exit:
    return err;
}

WEAVE_ERROR NRF5Config::WriteConfigValue(Key key, uint32_t val)
{
    WEAVE_ERROR err;
    fds_record_t rec;

    memset(&rec, 0, sizeof(rec));
    rec.file_id = GetFileId(key);
    rec.key = GetRecordKey(key);
    rec.data.p_data = &val;
    rec.data.length_words = 1;

    err = DoAsyncFDSOp(FDSAsyncOp::kAddOrUpdateRecord, rec);
    SuccessOrExit(err);

    WeaveLogProgress(DeviceLayer, "FDS set: 0x%04" PRIX16 "/0x%04" PRIX16 " = %" PRIu32 " (0x%" PRIX32 ")", GetFileId(key), GetRecordKey(key), val, val);

exit:
    return err;
}

WEAVE_ERROR NRF5Config::WriteConfigValue(Key key, uint64_t val)
{
    WEAVE_ERROR err;
    fds_record_t rec;

    memset(&rec, 0, sizeof(rec));
    rec.file_id = GetFileId(key);
    rec.key = GetRecordKey(key);
    rec.data.p_data = &val;
    rec.data.length_words = 2;

    err = DoAsyncFDSOp(FDSAsyncOp::kAddOrUpdateRecord, rec);
    SuccessOrExit(err);

    WeaveLogProgress(DeviceLayer, "FDS set: 0x%04" PRIX16 "/0x%04" PRIX16 " = %" PRIu64 " (0x%" PRIX64 ")", GetFileId(key), GetRecordKey(key), val, val);

exit:
    return err;
}

WEAVE_ERROR NRF5Config::WriteConfigValueStr(Key key, const char * str)
{
    return WriteConfigValueStr(key, str, (str != NULL) ? strlen(str) : 0);
}

WEAVE_ERROR NRF5Config::WriteConfigValueStr(Key key, const char * str, size_t strLen)
{
    WEAVE_ERROR err;
    uint8_t * storedVal = NULL;

    if (str != NULL)
    {
        fds_record_t rec;
        uint32_t storedValWords = FDSWords(strLen + 1);

        storedVal = (uint8_t *)nrf_malloc(storedValWords * kFDSWordSize);
        VerifyOrExit(storedVal != NULL, err = WEAVE_ERROR_NO_MEMORY);

        memcpy(storedVal, str, strLen);
        storedVal[strLen] = 0;

        memset(&rec, 0, sizeof(rec));
        rec.file_id = GetFileId(key);
        rec.key = GetRecordKey(key);
        rec.data.p_data = storedVal;
        rec.data.length_words = storedValWords;

        err = DoAsyncFDSOp(FDSAsyncOp::kAddOrUpdateRecord, rec);
        SuccessOrExit(err);

        WeaveLogProgress(DeviceLayer, "FDS set: 0x%04" PRIX16 "/0x%04" PRIX16 " = \"%s\"", GetFileId(key), GetRecordKey(key), (const char *)storedVal);
    }

    else
    {
        err = ClearConfigValue(key);
        SuccessOrExit(err);
    }

exit:
    if (storedVal != NULL)
    {
        nrf_free(storedVal);
    }
    return err;
}

WEAVE_ERROR NRF5Config::WriteConfigValueBin(Key key, const uint8_t * data, size_t dataLen)
{
    WEAVE_ERROR err;
    uint8_t * storedVal = NULL;

    if (data != NULL)
    {
        fds_record_t rec;
        uint32_t storedValWords = FDSWords(dataLen + 2);

        storedVal = (uint8_t *)nrf_malloc(storedValWords * kFDSWordSize);
        VerifyOrExit(storedVal != NULL, err = WEAVE_ERROR_NO_MEMORY);

        // First two bytes encode the length.
        Encoding::LittleEndian::Put16(storedVal, (uint16_t)dataLen);

        memcpy(storedVal + 2, data, dataLen);

        memset(&rec, 0, sizeof(rec));
        rec.file_id = GetFileId(key);
        rec.key = GetRecordKey(key);
        rec.data.p_data = storedVal;
        rec.data.length_words = storedValWords;

        err = DoAsyncFDSOp(FDSAsyncOp::kAddOrUpdateRecord, rec);
        SuccessOrExit(err);

        WeaveLogProgress(DeviceLayer, "FDS set: 0x%04" PRIX16 "/0x%04" PRIX16 " = (blob length %" PRId32 ")", GetFileId(key), GetRecordKey(key), dataLen);
    }

    else
    {
        err = ClearConfigValue(key);
        SuccessOrExit(err);
    }

exit:
    if (storedVal != NULL)
    {
        nrf_free(storedVal);
    }
    return err;
}

WEAVE_ERROR NRF5Config::ClearConfigValue(Key key)
{
    WEAVE_ERROR err;
    fds_record_t rec;

    memset(&rec, 0, sizeof(rec));
    rec.file_id = GetFileId(key);
    rec.key = GetRecordKey(key);

    err = DoAsyncFDSOp(FDSAsyncOp::kDeleteRecord, rec);
    SuccessOrExit(err);

    WeaveLogProgress(DeviceLayer, "FDS delete: 0x%04" PRIX16 "/0x%04" PRIX16, GetFileId(key), GetRecordKey(key));

exit:
    return err;
}

bool NRF5Config::ConfigValueExists(Key key)
{
    ret_code_t fdsRes;
    fds_record_desc_t recDesc;
    fds_find_token_t findToken;

    // Search for the requested record.
    memset(&findToken, 0, sizeof(findToken));
    fdsRes = fds_record_find(GetFileId(key), GetRecordKey(key), &recDesc, &findToken);

    // Return true iff the record was found.
    return fdsRes == FDS_SUCCESS;
}

} // namespace Internal
} // namespace DeviceLayer
} // namespace Weave
} // namespace nl
