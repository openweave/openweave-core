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
 *      This file defines a derived unsolicited responder
 *      (i.e., server) for the Weave Image Announce protocol of the
 *      Software Update (SWU) profile used for the Weave mock device
 *      command line functional testing tool.
 *
 */

#ifndef MOCKIASERVER_H_
#define MOCKIASERVER_H_

#include <Weave/Core/WeaveCore.h>
#include <Weave/Profiles/software-update/SoftwareUpdateProfile.h>
#include <Weave/Profiles/software-update/WeaveImageAnnounceServer.h>

using nl::Inet::IPAddress;
using namespace nl::Weave;
using namespace nl::Weave::Profiles;
using namespace nl::Weave::Profiles::SoftwareUpdate;

class MockImageAnnounceServer : private WeaveImageAnnounceServer, private IWeaveImageAnnounceServerDelegate
{
public:
    MockImageAnnounceServer();

    WEAVE_ERROR Init(WeaveExchangeManager *exchangeMgr);
    WEAVE_ERROR Shutdown();

    typedef void (*MessageReceivedFnct)(ExchangeContext *ec);
    MessageReceivedFnct OnImageAnnounceReceived;

    WEAVE_ERROR CreateExchangeCtx(WeaveConnection *con);
    WEAVE_ERROR CreateExchangeCtx(const uint64_t &peerNodeId, const IPAddress &peerAddr);

protected:
    virtual void OnImageAnnounce(ExchangeContext* ec);

private:
    ExchangeContext *mCurServerOp;
    void CloseExistingExchangeCtx();
};

#endif /* MOCKIASERVER_H_ */
