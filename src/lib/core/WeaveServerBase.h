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
 *      This file defines a common, base object that can be derived
 *      from but not instantiated itself. The object may be used for
 *      implementing Weave profile unsolicited responders (i.e., servers).
 *      It encapsulates validating authenticated requests and sending
 *      status reports. It also provides common data member storage for
 *      fabric state and an exchange manager.
 *
 */

#ifndef WEAVESERVERBASE_H_
#define WEAVESERVERBASE_H_

#include <Weave/Core/WeaveCore.h>

namespace nl {
namespace Weave {

class WeaveServerDelegateBase;


/**
 *  @class WeaveServerBase
 *
 *  @brief
 *    Common, base object for implementing Weave profile unsolicited
 *    responders (servers) that encapsulates validating
 *    authenticated requests and sending status reports and provides
 *    common data member storage for fabric state and an exchange
 *    manager.
 *
 */
class WeaveServerBase
{
public:
    WeaveFabricState     *FabricState;  //!< [READ ONLY] Fabric state object
    WeaveExchangeManager *ExchangeMgr;  //!< [READ ONLY] Exchange manager object

    static WEAVE_ERROR SendStatusReport(ExchangeContext *ec, uint32_t statusProfileId, uint16_t statusCode, WEAVE_ERROR sysError);

    static WEAVE_ERROR SendStatusReport(ExchangeContext *ec, uint32_t statusProfileId, uint16_t statusCode, WEAVE_ERROR sysError, uint16_t sendFlags);

protected:
    WeaveServerBase(void) { }

    bool EnforceAccessControl(ExchangeContext *ec, uint32_t msgProfileId, uint8_t msgType,
            const WeaveMessageInfo *msgInfo, WeaveServerDelegateBase *delegate);

private:
    WeaveServerBase(const WeaveServerBase&); // not defined
};


/**
 * A common base class for implementing Weave server delegate objects.
 */
class WeaveServerDelegateBase
{
    friend class WeaveServerBase;

protected:
    WeaveServerDelegateBase(void) { }

    typedef uint8_t AccessControlResult;

    enum
    {
        kAccessControlResult_NotDetermined          = 0,    //< The message has not yet been accepted or rejected.
        kAccessControlResult_Accepted               = 1,    //< The message has been accepted.
        kAccessControlResult_Rejected               = 2,    //< The message has been rejected, and a default response should be sent.
        kAccessControlResult_Rejected_RespSent      = 3,    //< The message has been rejected, and a response has already been sent.
        kAccessControlResult_Rejected_Silent        = 4,    //< The message has been rejected, but no response should be sent.
    };

    virtual void EnforceAccessControl(ExchangeContext *ec, uint32_t msgProfileId, uint8_t msgType,
            const WeaveMessageInfo *msgInfo, AccessControlResult& result);

private:

    enum
    {
        kAccessControlResult_IsFinal                = 0x80, //< A flag indicating that access control evaluation is complete and the result is final.
        kAccessControlResult_FinalAccepted          = kAccessControlResult_Accepted | kAccessControlResult_IsFinal
                                                            //< Access control evaluation is complete and the message has been accepted.
    };
};




} // namespace Weave
} // namespace nl

#endif /* WEAVESERVERBASE_H_ */
