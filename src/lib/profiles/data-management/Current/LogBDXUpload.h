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
#ifndef _WEAVE_DATA_MANAGEMENT_EVENT_LOGGING_BDX_UPLOAD_CURRENT_H
#define _WEAVE_DATA_MANAGEMENT_EVENT_LOGGING_BDX_UPLOAD_CURRENT_H

#include <Weave/Profiles/data-management/Current/WdmManagedNamespace.h>
#include <Weave/Profiles/data-management/DataManagement.h>

#include <Weave/Profiles/bulk-data-transfer/Development/BDXManagedNamespace.hpp>
#include <Weave/Profiles/bulk-data-transfer/Development/BulkDataTransfer.h>
#include <Weave/Profiles/bulk-data-transfer/Development/BDXMessages.h>
#include <Weave/Profiles/status-report/StatusReportProfile.h>

namespace nl {
namespace Weave {
namespace Profiles {
namespace WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current) {

WEAVE_ERROR BdxSendAcceptHandler(nl::Weave::Profiles::BulkDataTransfer::BDXTransfer *aXfer,
                                 nl::Weave::Profiles::BulkDataTransfer::SendAccept *aSendAcceptMsg);
void BdxRejectHandler(nl::Weave::Profiles::BulkDataTransfer::BDXTransfer *aXfer,
                      nl::Weave::Profiles::StatusReporting::StatusReport *aReport);
void BdxGetBlockHandler(nl::Weave::Profiles::BulkDataTransfer::BDXTransfer *aXfer,
                        uint64_t *aLength,
                        uint8_t **aDataBlock,
                        bool *aIsLastBlock);
void BdxXferErrorHandler(nl::Weave::Profiles::BulkDataTransfer::BDXTransfer *aXfer,
                         nl::Weave::Profiles::StatusReporting::StatusReport *aXferError);

void BdxXferDoneHandler(nl::Weave::Profiles::BulkDataTransfer::BDXTransfer *aXfer);
void BdxErrorHandler(nl::Weave::Profiles::BulkDataTransfer::BDXTransfer *aXfer, WEAVE_ERROR aErrorCode);


class NL_DLL_EXPORT LogBDXUpload {

public:
    enum UploaderState {
        UploaderUninitialized,
        UploaderInitialized,
        UploaderInProgress
    };

    LogBDXUpload(void);
    WEAVE_ERROR Init(LoggingManagement *inLogger);

    WEAVE_ERROR StartUpload(nl::Weave::Binding *aBinding);

    void BlockHandler(nl::Weave::Profiles::BulkDataTransfer::BDXTransfer *aXfer,
                      uint64_t *aLength,
                      uint8_t **aDataBlock,
                      bool *aIsLastBlock);

    void Abort(void);
    void Done(void);
    void Shutdown(void);

    uint32_t GetUploadPosition(void);

    UploaderState                                  mState;
private:
    void ThrottleIfNeeded(void);

    LoggingManagement *                            mLogger;
    nl::Weave::Profiles::BulkDataTransfer::BdxNode mBdxNode;
    ImportanceType                                 mCurrentImportance;
    event_id_t                                     mCurrentEventID;
    event_id_t                                     mLastScheduledEventId[kImportanceType_Last - kImportanceType_First + 1];
    event_id_t                                     mLastTransmittedEventId[kImportanceType_Last - kImportanceType_First + 1];
    uint32_t                                       mUploadPosition;
    bool                                           mThrottled;
    bool                                           mFirstXfer;
};

} // WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current)
} // Profiles
} // Weave
} // nl

#endif // _WEAVE_DATA_MANAGEMENT_EVENT_LOGGING_BDX_UPLOAD_CURRENT_H
