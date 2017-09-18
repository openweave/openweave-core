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

#include <Weave/Profiles/bulk-data-transfer/Development/BDXManagedNamespace.hpp>
#include <Weave/Core/WeaveCore.h>
#include <Weave/Profiles/WeaveProfiles.h>
#include <Weave/Profiles/ProfileCommon.h>
#include <Weave/Profiles/service-directory/ServiceDirectory.h>

#include "BulkDataTransfer.h"


namespace nl {
namespace Weave {
namespace Profiles {
namespace BulkDataTransfer {

class BdxDelegate
{
public:
    BdxDelegate(void);

    WEAVE_ERROR Init(WeaveExchangeManager *anExchangeMgr,
                     InetLayer *anInetLayer);
    WEAVE_ERROR Shutdown(void);

#if WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
    WEAVE_ERROR EstablishWeaveConnection(ServiceDirectory::WeaveServiceManager &aServiceMgr, WeaveAuthMode anAuthMode);
#endif

    void StartBdxUploadingFile(void);

    bool UploadInProgress(void)
    {
        return mInProgress;
    };

protected:
    virtual void BdxSendAcceptHandler(SendAccept *aSendAcceptMsg) = 0;
    virtual void BdxRejectHandler(StatusReport *aReport) = 0;
    virtual void BdxGetBlockHandler(uint64_t *aLength,
                                    uint8_t **aDataBlock,
                                    bool *aLastBlock) = 0;
    virtual void BdxXferErrorHandler(StatusReport *aXferError) = 0;
    virtual void BdxXferDoneHandler(void) = 0;
    virtual void BdxErrorHandler(WEAVE_ERROR aErrorCode) = 0;

    virtual char *BdxGetFileName(void) = 0;

private:
    static void BdxSendAcceptHandlerEntry(void *anAppState, SendAccept *aSendAcceptMsg);
    static void BdxRejectHandlerEntry(void *anAppState, StatusReport *aReport);
    static void BdxGetBlockHandlerEntry(void *anAppState,
                                        uint64_t *aLength,
                                        uint8_t **aDataBlock,
                                        bool *aLastBlock);
    static void BdxXferErrorHandlerEntry(void *anAppState, StatusReport *aXferError);
    static void BdxXferDoneHandlerEntry(void *anAppState);
    static void BdxErrorHandlerEntry(void *anAppState, WEAVE_ERROR anErrorCode);

#if WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
    static void HandleWeaveServiceMgrStatusEntry(void *anAppState, uint32_t aProfileId, uint16_t aStatusCode);
#endif

    static void HandleWeaveConnectionCompleteEntry(WeaveConnection *aCon, WEAVE_ERROR aConErr);
    static void HandleWeaveConnectionClosedEntry(WeaveConnection *aCon, WEAVE_ERROR aConErr);

protected:
#if WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
    virtual void HandleWeaveServiceMgrStatus(void *anAppState, uint32_t aProfileId, uint16_t aStatusCode);
#endif

    virtual void HandleWeaveConnectionComplete(WeaveConnection *aCon, WEAVE_ERROR aConErr);
    virtual void HandleWeaveConnectionClosed(WeaveConnection *aCon, WEAVE_ERROR aConErr);

private:
    BdxClient mBdxClient;
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
