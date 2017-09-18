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
 *      This file defines a derived unsolicited responder
 *      (i.e., server) for the Weave Software Update (SWU) profile used
 *      for the Weave mock device command line functional testing
 *      tool.
 *
 */

#ifndef MOCKSWUSERVER_H_
#define MOCKSWUSERVER_H_

#include <Weave/Core/WeaveCore.h>
#include <Weave/Core/WeaveServerBase.h>
#include <Weave/Profiles/software-update/SoftwareUpdateProfile.h>

using nl::Inet::IPAddress;
using namespace nl::Weave;
using namespace nl::Weave::Profiles;
using namespace nl::Weave::Profiles::SoftwareUpdate;

class MockSoftwareUpdateServer : public WeaveServerBase
{
public:
    MockSoftwareUpdateServer();

    WEAVE_ERROR Init(WeaveExchangeManager *exchangeMgr);
    WEAVE_ERROR Shutdown();

    virtual WEAVE_ERROR SendStatusReport(uint32_t statusProfileId, uint16_t statusCode, WEAVE_ERROR sysError = WEAVE_NO_ERROR);

    void SetReferenceImageQuery(ImageQuery *aRefImageQuery);
    WEAVE_ERROR SetFileDesignator(const char *aFileDesignator);
    WEAVE_ERROR SendImageAnnounce(WeaveConnection *con);
    WEAVE_ERROR SendImageAnnounce(uint64_t nodeId, IPAddress nodeAddr);
    WEAVE_ERROR SendImageAnnounce(uint64_t nodeId, IPAddress nodeAddr, uint16_t port);
    WEAVE_ERROR SendImageAnnounce();


protected:
    ExchangeContext *mCurServerOp;
    ImageQuery *mRefImageQuery;
    const char *mFileDesignator;
    PacketBuffer *mCurServerOpBuf;

private:
    static void HandleClientRequest(ExchangeContext *ec, const IPPacketInfo *pktInfo, const WeaveMessageInfo *msgInfo, uint32_t profileId,
            uint8_t msgType, PacketBuffer *payload);
    static WEAVE_ERROR HandleImageQuery(ExchangeContext *ec, const IPPacketInfo *pktInfo, const WeaveMessageInfo *msgInfo, uint32_t profileId,
            uint8_t msgType, PacketBuffer *payload);
    static WEAVE_ERROR HandleServerConnectionClosed(ExchangeContext *ec, WeaveConnection *con, WEAVE_ERROR conErr);

    WEAVE_ERROR SendImageQueryResponse();
    WEAVE_ERROR SendImageQueryStatus();
    WEAVE_ERROR ConvertIntegrityType(char *aIntegrityType, uint8_t aIntegrity);
    WEAVE_ERROR GenerateImageDigest(const char *imagePath, uint8_t integrityType, uint8_t *digest);
};

#endif /* MOCKSWUSERVER_H_ */
