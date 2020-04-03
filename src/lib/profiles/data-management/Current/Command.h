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
 *      This file defines command handle for Weave
 *      Data Management (WDM) profile.
 *
 */

#ifndef _WEAVE_DATA_MANAGEMENT_COMMAND_CURRENT_H
#define _WEAVE_DATA_MANAGEMENT_COMMAND_CURRENT_H

#include <Weave/Profiles/data-management/Current/WdmManagedNamespace.h>
#include <Weave/Core/WeaveCore.h>
#include <Weave/Profiles/data-management/Current/TraitCatalog.h>

namespace nl {
namespace Weave {
namespace Profiles {
namespace WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current) {

typedef enum CommandFlags
{
    kCommandFlag_MustBeVersionValid   = 0x0001,  /**< Set when the version field is valid */
    kCommandFlag_InitiationTimeValid  = 0x0002,  /**< Set when the init time is valid */
    kCommandFlag_ActionTimeValid      = 0x0004,  /**< Set when the action time is valid */
    kCommandFlag_ExpiryTimeValid      = 0x0008,  /**< Set when the expiry time is valid */
    kCommandFlag_IsOneWay             = 0x0010,  /**< Set when the command is one-way */
} CommandFlags;

/**
 *  @class Command
 *
 *  @note This class is designed to hide a certain detail in command handling.  Decision has been made to hide the details of
 *    ExchangeContext and authenticator validation, while leaving the handling of PacketBuffers to the application layer.
 *
 *    The utility of this wrapper around command handling is indeed limited, mainly due to the complexity/flexibility involved
 *    in security validation and data serialization/de-serialization.
 *
 *    The details for command validation is still TBD
 *
 *    To adjust the retransmission timing for the In-Progress, Status Report, and also Response message,
 *    the application layer would have to somehow deal with the ExchangeContext object. The best practise
 *    is still TBD, but the application layer has these three choices:
 *
 *    1) Acquire the Exchange Context through #GetExchangeContext and directly evaluate/adjust it.
 *    2) Pre-allocate and configure a Binding during boot up, before any command arrives, and configure it properly
 *    3) Create a temporary Binding using this function BindingPool::NewResponderBindingFromExchangeContext
 *
 *    In both (2) and (3), the application layer can enforce security/timing setting through
 *    Binding::ConfigureExistingExchangeContext. The Binding is never used to generate new exchange contexts for custom commands, so
 *    it doesn't have to be stored within this handle.
 *
 *    The request packet buffer is also not stored within this handle, for there is no obvious use of it. This is especially
 *    true if the application layer can handle this command and send out response directly.
 *
 *    Application layer would receive the packet buffer from the same callback it receives this command handle. If it decides to
 *    handle this command in an async manner, it would have to store both the command handle and the packet buffer.
 *
 */
class NL_DLL_EXPORT Command
{
public:
    /**
     *  @brief
     *    The Command flag bits.
     */
    typedef enum CommandFlags
    {
        kCommandFlag_MustBeVersionValid   = 0x0001,  /**< Set when the version field is valid */
        kCommandFlag_InitiationTimeValid  = 0x0002,  /**< Set when the init time is valid */
        kCommandFlag_ActionTimeValid      = 0x0004,  /**< Set when the action time is valid */
        kCommandFlag_ExpiryTimeValid      = 0x0008,  /**< Set when the expiry time is valid */
        kCommandFlag_IsOneWay             = 0x0010,  /**< Set when the command is one-way */
    } CommandFlags;

    uint64_t commandType;
    uint64_t mustBeVersion;
    int64_t initiationTimeMicroSecond;
    int64_t actionTimeMicroSecond;
    int64_t expiryTimeMicroSecond;

    bool IsMustBeVersionValid(void) const;
    bool IsInitiationTimeValid(void) const;
    bool IsActionTimeValid(void) const;
    bool IsExpiryTimeValid(void) const;
    bool IsOneWay(void) const;

    /**
     *  @brief Retrieve the exchange context object used by this incoming command
     *
     *  @return  A pointer to the exchange context object used by this incoming command
     */
    nl::Weave::ExchangeContext * GetExchangeContext(void) const { return mEC; };

    WEAVE_ERROR ValidateAuthenticator(nl::Weave::System::PacketBuffer * aRequestBuffer);

    WEAVE_ERROR SendInProgress(void);

    WEAVE_ERROR SendResponse(uint32_t traitInstanceVersion, nl::Weave::System::PacketBuffer * apPayload);

    WEAVE_ERROR SendError(uint32_t aProfileId, uint16_t aStatusCode, WEAVE_ERROR aWeaveError);

    void Close(void);

private:
    friend class SubscriptionEngine;

    nl::Weave::ExchangeContext * mEC;

    uint16_t mFlags;                      // Internal Command flags

    // nl::Weave::System::PacketBuffer * mRequestBuffer;

    Command();

    WEAVE_ERROR Init(nl::Weave::ExchangeContext * aEC);

    void SetMustBeVersionValid(bool inVersionValid);
    void SetInitiationTimeValid(bool inInitiationTimeValid);
    void SetActionTimeValid(bool inActionTimeValid);
    void SetExpiryTimeValid(bool inExpiryTimeValid);
    void SetIsOneWay(bool inIsOneWay);

    bool IsFree(void) { return (NULL == mEC); };
};

}; // namespace WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current)
}; // namespace Profiles
}; // namespace Weave
}; // namespace nl

#endif // _WEAVE_DATA_MANAGEMENT_COMMAND_CURRENT_H
