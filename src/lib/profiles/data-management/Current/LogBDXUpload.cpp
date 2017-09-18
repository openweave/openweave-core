/*
 *
 *    Copyright (c) 2016-2017 Nest Labs, Inc.
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
#define WEAVE_CONFIG_BDX_NAMESPACE kWeaveManagedNamespace_Development

#include <Weave/Profiles/data-management/Current/WdmManagedNamespace.h>
#include <Weave/Profiles/data-management/DataManagement.h>

#include <Weave/Profiles/bulk-data-transfer/Development/BulkDataTransfer.h>
#include <Weave/Profiles/bulk-data-transfer/Development/BDXMessages.h>
#include <Weave/Profiles/service-directory/ServiceDirectory.h>

using namespace nl::Weave::TLV;

namespace nl {
namespace Weave {
namespace Profiles {
namespace WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current) {

static char sLogFileName[] = "topazlog";

WEAVE_ERROR BdxSendAcceptHandler(nl::Weave::Profiles::BulkDataTransfer::BDXTransfer *aXfer,
                                 nl::Weave::Profiles::BulkDataTransfer::SendAccept *aSendAcceptMsg)
{
    WEAVE_ERROR error = WEAVE_NO_ERROR;
    WeaveLogDetail(BDX, "SendInit Accepted: %hd maxBlockSize, transfer mode is %hd", aSendAcceptMsg->mMaxBlockSize, aXfer->mTransferMode);
    return error;
}


void BdxRejectHandler(nl::Weave::Profiles::BulkDataTransfer::BDXTransfer *aXfer,
                      nl::Weave::Profiles::StatusReporting::StatusReport *aReport)
{
    WeaveLogProgress(BDX, "BDX Init message rejected: %d", aReport->mStatusCode);

    LogBDXUpload *uploader;

    aXfer->Shutdown();

    // forget the upload state
    uploader = static_cast<LogBDXUpload *>(aXfer->mAppState);
    uploader->Abort();
}

void BdxGetBlockHandler(nl::Weave::Profiles::BulkDataTransfer::BDXTransfer *aXfer,
                        uint64_t *aLength,
                        uint8_t **aDataBlock,
                        bool *aIsLastBlock)
{
    LogBDXUpload *uploader;

    VerifyOrExit(aXfer != NULL && aLength != NULL && aDataBlock != NULL && aIsLastBlock != NULL,
                 WeaveLogProgress(BDX, "BDXGetBlockHandler called with invalid args"));
    uploader = static_cast<LogBDXUpload *>(aXfer->mAppState);
    uploader->BlockHandler(aXfer, aLength, aDataBlock, aIsLastBlock);
exit:
    return;
}

void LogBDXUpload::ThrottleIfNeeded(void)
{
    // We throttle if the offload cannot keep up with event
    // generation.  We detect that we cannot keep up by detecting that
    // events are being dropped in the ongoing offload; that's done by
    // checking that the next event in the ongoing transfer
    // (`mCurrentEventID`) is no longer in the queue.
    if (!mThrottled && !mFirstXfer && (mCurrentEventID < mLogger->GetFirstEventID(mCurrentImportance)))
    {
        mThrottled = true;
        mLogger->ThrottleLogger();
    }

    mFirstXfer = false;
}

void LogBDXUpload::BlockHandler(nl::Weave::Profiles::BulkDataTransfer::BDXTransfer *aXfer,
                                uint64_t *aLength,
                                uint8_t **aDataBlock,
                                bool *aIsLastBlock)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    TLVWriter writer;
    bool fullBlock = true;

    writer.Init(*aDataBlock, *aLength);

    // If successful, these values will be reset below.  If the
    // function fails, these will be the return values.
    *aIsLastBlock = true;
    *aLength = 0;

    do {
        if (fullBlock)
        {
            ThrottleIfNeeded();
        }

        err = mLogger->FetchEventsSince(writer, mCurrentImportance, mCurrentEventID);

        // Reached the end of the current importance
        if ((err == WEAVE_END_OF_TLV) || (err == WEAVE_ERROR_TLV_UNDERRUN))
        {
            if (mCurrentImportance == kImportanceType_Last)
            {
                // reached the end of all importances.  We're at the
                // end of the current transfer.  Signal end of
                // transmission.
                err = WEAVE_NO_ERROR;
                *aIsLastBlock = true;
                mCurrentImportance = kImportanceType_First;
                break;
            }
            else
            {
                // Other importance levels are available.  Save the
                // state of this importance, and move to the next one,
                // restoring the last event appropriately
                err = WEAVE_NO_ERROR;
                mLastScheduledEventId[mCurrentImportance - kImportanceType_First] = mCurrentEventID;
                mCurrentImportance = static_cast<ImportanceType>(static_cast<uint32_t>(mCurrentImportance) + 1);
                mCurrentEventID = mLastScheduledEventId[mCurrentImportance - kImportanceType_First];
                mFirstXfer = true;
                fullBlock = false;
                continue;
            }
        }

        // we ran out of buffer space.  The state is already preserved
        // in the current importance, and current event ID; remember
        // that there will be more events to transfer.
        if ((err == WEAVE_ERROR_BUFFER_TOO_SMALL) || (err == WEAVE_ERROR_NO_MEMORY))
        {
            err = WEAVE_NO_ERROR;
            *aIsLastBlock = false;
            break;
        }

        // Any other condition is an error condition.  Exit.
    } while (err == WEAVE_NO_ERROR);

    SuccessOrExit(err);
    // on success, the aLastBlock is already set.
    *aLength = writer.GetLengthWritten();
exit:
    return;
}

void BdxXferErrorHandler(nl::Weave::Profiles::BulkDataTransfer::BDXTransfer *aXfer, nl::Weave::Profiles::StatusReporting::StatusReport *aXferError)
{
    LogBDXUpload *uploader;

    WeaveLogProgress(BDX, "Transfer error: %d", aXferError->mStatusCode);

    aXfer->Shutdown();

    // forget the upload state
    uploader = static_cast<LogBDXUpload *>(aXfer->mAppState);
    uploader->Abort();
}

void BdxXferDoneHandler(nl::Weave::Profiles::BulkDataTransfer::BDXTransfer *aXfer)
{
    LogBDXUpload *uploader;
    WeaveLogDetail(BDX, "Transfer complete!");

    aXfer->Shutdown();

    // remember upload state
    uploader = static_cast<LogBDXUpload *>(aXfer->mAppState);
    uploader->Done();
}

void BdxErrorHandler(nl::Weave::Profiles::BulkDataTransfer::BDXTransfer *aXfer, WEAVE_ERROR aErrorCode)
{
    LogBDXUpload *uploader;
    WeaveLogProgress(BDX, "BDX error: %s", ErrorStr(aErrorCode));
    // We currently don't try to handle errors at all
    aXfer->Shutdown();

    // forget the upload state
    uploader = static_cast<LogBDXUpload *>(aXfer->mAppState);
    uploader->Abort();
}

LogBDXUpload::LogBDXUpload()
{
}

WEAVE_ERROR LogBDXUpload::Init(LoggingManagement *inLogger)
{
    WEAVE_ERROR err;
    mState = UploaderUninitialized;
    mCurrentImportance = kImportanceType_First;
    mCurrentEventID = 0;
    memset(mLastScheduledEventId, 0, sizeof(mLastScheduledEventId));
    memset(mLastTransmittedEventId, 0, sizeof(mLastTransmittedEventId));
    mLogger = inLogger;
    err = mBdxNode.Init(mLogger->mExchangeMgr);
    SuccessOrExit(err);

    mState = UploaderInitialized;
exit:
    return err;
}

WEAVE_ERROR LogBDXUpload::StartUpload(nl::Weave::Binding *aBinding)
{
    nl::Weave::Profiles::BulkDataTransfer::BDXTransfer *xfer;
    ReferencedString logFileName;
    WEAVE_ERROR err;

    logFileName.init(static_cast<uint16_t>(strlen(sLogFileName)), sLogFileName);
    nl::Weave::Profiles::BulkDataTransfer::BDXHandlers handlers =
        {
            BdxSendAcceptHandler, // SendAcceptHandler
            NULL,                 // ReceiveAcceptHandler
            BdxRejectHandler,     // RejectHandler
            BdxGetBlockHandler,   // GetBlockHandler
            NULL,                 // PutBlockHandler
            BdxXferErrorHandler,  // XferErrorHandler
            BdxXferDoneHandler,   // XferDoneHandler
            BdxErrorHandler       // ErrorHandler
        };

    VerifyOrExit(mState == UploaderInitialized, err = WEAVE_ERROR_INCORRECT_STATE);
    VerifyOrExit(aBinding != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);

    // restore the point from which we need to resume
    memcpy(mLastScheduledEventId, mLastTransmittedEventId, sizeof(mLastScheduledEventId));
    mCurrentImportance = kImportanceType_First;
    mCurrentEventID = mLastScheduledEventId[mCurrentImportance - kImportanceType_First];
    mFirstXfer = true;

    // create a transfer object
    xfer = NULL;
    err = mBdxNode.NewTransfer(aBinding, handlers, logFileName, this, xfer);
    SuccessOrExit(err);

    mState = UploaderInProgress;
    xfer->mMaxBlockSize = 1024;
    xfer->mStartOffset = 0;
    xfer->mLength = 0;

    // start transfer
    err = mBdxNode.InitBdxSend(*xfer, true, false, false, NULL);

    if (err != WEAVE_NO_ERROR)
    {
        xfer->Shutdown();

        Abort();
    }

exit:
    return err;
}

void LogBDXUpload :: Abort(void)
{
    // Upload was aborted; last transmitted event Id not updated,
    // instead, we rollback the last scheduled event IDs
    memcpy(mLastScheduledEventId, mLastTransmittedEventId, sizeof(mLastScheduledEventId));
    mState = UploaderInitialized;
    if (mThrottled)
    {
        mLogger->UnthrottleLogger();
        mThrottled = false;
    }
    mLogger->SignalUploadDone();
}

void LogBDXUpload :: Done(void)
{
    //Upload was successful.  Transfer last scheduled event IDs to the
    //last transmitted event IDS
    memcpy(mLastTransmittedEventId, mLastScheduledEventId, sizeof(mLastTransmittedEventId));
    mState = UploaderInitialized;
    mUploadPosition = mLogger->GetBytesWritten();
    if (mThrottled)
    {
        mLogger->UnthrottleLogger();
        mThrottled = false;
    }
    mLogger->SignalUploadDone();
}

void LogBDXUpload :: Shutdown(void)
{
    mBdxNode.Shutdown();
    mState = UploaderUninitialized;
}

uint32_t LogBDXUpload :: GetUploadPosition(void)
{
    return mUploadPosition;
}

} // WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current)
} // Profiles
} // Weave
} // nl
