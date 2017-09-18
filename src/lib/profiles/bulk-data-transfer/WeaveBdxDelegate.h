/*
 *
 *    Copyright (c) 2013-2017 Nest Labs, Inc.
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
 *      Delegate class for handling Bulk Data Transfer operations.
 *
 */

#ifndef _NL_WEAVE_BDX_DELEGATE_H_
#define _NL_WEAVE_BDX_DELEGATE_H_

#include <Weave/Profiles/bulk-data-transfer/BDXManagedNamespace.hpp>
#include <Weave/Core/WeaveCore.h>
#include <Weave/Profiles/WeaveProfiles.h>
#include <Weave/Profiles/ProfileCommon.h>
#include <Weave/Profiles/service-directory/ServiceDirectory.h>

#include "BulkDataTransfer.h"


namespace nl {
namespace Weave {
namespace Profiles {
namespace WeaveMakeManagedNamespaceIdentifier(BDX, kWeaveManagedNamespaceDesignation_Current) {

class WeaveBdxDelegate
{
public:
    WeaveBdxDelegate(void);

    WEAVE_ERROR Init(WeaveExchangeManager *pExchangeMgr,
                     InetLayer *pInetLayer);
    WEAVE_ERROR Shutdown(void);

#if WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
    WEAVE_ERROR EstablishWeaveConnection(ServiceDirectory::WeaveServiceManager &aServiceMgr, WeaveAuthMode anAuthMode);
#endif

    void StartBdxUploadingFile(void);

    bool UploadInProgress(void) { return mInProgress; };

protected:
    virtual void BdxSendAcceptHandler(SendAccept *aSendAcceptMsg) = 0;
    virtual void BdxRejectHandler(StatusReport *aReport) = 0;
    virtual void BdxGetBlockHandler(uint64_t *pLength,
                                    uint8_t **aDataBlock,
                                    bool *isLastBlock) = 0;
    virtual void BdxXferErrorHandler(StatusReport *aXferError) = 0;
    virtual void BdxXferDoneHandler(void) = 0;
    virtual void BdxErrorHandler(WEAVE_ERROR anErrorCode) = 0;

    virtual char *BdxGetFileName(void) = 0;

private:
    static void BdxSendAcceptHandlerEntry(void *aAppState, SendAccept *aSendAcceptMsg);
    static void BdxRejectHandlerEntry(void *aAppState, StatusReport *aReport);
    static void BdxGetBlockHandlerEntry(void *aAppState,
                                        uint64_t *pLength,
                                        uint8_t **aDataBlock,
                                        bool *isLastBlock);
    static void BdxXferErrorHandlerEntry(void *aAppState, StatusReport *aXferError);
    static void BdxXferDoneHandlerEntry(void *aAppState);
    static void BdxErrorHandlerEntry(void *aAppState, WEAVE_ERROR anErrorCode);

#if WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
    static void HandleWeaveServiceMgrStatusEntry(void *anAppState, uint32_t aProfileId, uint16_t aStatusCode);
#endif

    static void HandleWeaveConnectionCompleteEntry(WeaveConnection *con, WEAVE_ERROR conErr);
    static void HandleWeaveConnectionClosedEntry(WeaveConnection *con, WEAVE_ERROR conErr);

protected:
#if WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
    virtual void HandleWeaveServiceMgrStatus(void *anAppState, uint32_t aProfileId, uint16_t aStatusCode);
#endif

    virtual void HandleWeaveConnectionComplete(WeaveConnection *con, WEAVE_ERROR conErr);
    virtual void HandleWeaveConnectionClosed(WeaveConnection *con, WEAVE_ERROR conErr);

private:
    WeaveBdxClient mBdxClient;
    InetLayer *mpInetLayer;
    WeaveExchangeManager *mpExchangeMgr;

    static bool mInProgress;

protected:
    uint16_t mMaxBlockSize;
    uint64_t mStartOffset;
    uint64_t mLength;

};
} // BulkDataTransfer
} // Profiles
} // Weave
} // nl

#endif // _NL_WEAVE_BDX_DELEGATE_H_
