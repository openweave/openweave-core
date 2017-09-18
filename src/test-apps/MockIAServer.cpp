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

/**
 *    @file
 *      This file implements a derived unsolicited responder
 *      (i.e., server) for the Weave Image Announce protocol of the
 *      Software Update (SWU) profile used for the Weave mock device
 *      command line functional testing tool.
 *
 */

#include <stdio.h>
#include <Weave/Core/WeaveCore.h>
#include <Weave/Profiles/WeaveProfiles.h>
#include "MockIAServer.h"
#include <Weave/Profiles/common/CommonProfile.h>

MockImageAnnounceServer::MockImageAnnounceServer()
{
    mCurServerOp = NULL;
}

WEAVE_ERROR MockImageAnnounceServer::Init(WeaveExchangeManager *exchangeMgr)
{
    WEAVE_ERROR err;

    err = this->WeaveImageAnnounceServer::Init(exchangeMgr, this);
    return err;
}

WEAVE_ERROR MockImageAnnounceServer::Shutdown()
{
    CloseExistingExchangeCtx();
    return WEAVE_NO_ERROR;
}

WEAVE_ERROR MockImageAnnounceServer::CreateExchangeCtx(const uint64_t &peerNodeId, const IPAddress &peerAddr)
{
     CloseExistingExchangeCtx();

     // Create a new exchange context.
     mCurServerOp = ExchangeMgr.NewContext(peerNodeId, peerAddr,  this);
     if (mCurServerOp == NULL)
         return WEAVE_ERROR_NO_MEMORY;
     return WEAVE_NO_ERROR;
}

WEAVE_ERROR MockImageAnnounceServer::CreateExchangeCtx(WeaveConnection *con)
{
     CloseExistingExchangeCtx();

     // Create a new exchange context.
     mCurServerOp = ExchangeMgr.NewContext(con, this);
     if (mCurServerOp == NULL)
         return WEAVE_ERROR_NO_MEMORY;
     return WEAVE_NO_ERROR;
}

void MockImageAnnounceServer::CloseExistingExchangeCtx()
{
    if (mCurServerOp != NULL)
    {
         mCurServerOp->Close();
         mCurServerOp = NULL;
    }
}

void MockImageAnnounceServer::OnImageAnnounce(ExchangeContext* ec)
{
   printf("1. OnImageAnnounce...\n");

   MockImageAnnounceServer *server = (MockImageAnnounceServer *) mCurServerOp->AppState;
   if (server->OnImageAnnounceReceived == NULL)
   {
        printf("2. OnImageAnnounceReceived is NULL\n");
        return;
   }
   server->OnImageAnnounceReceived(mCurServerOp);
   CloseExistingExchangeCtx();
   printf("3. OnImageAnnounce done\n");

}
