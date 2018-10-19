/*
 *
 *    Copyright (c) 2015-2017 Nest Labs, Inc.
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
 *      This file implements core functionality for the Nest Weave
 *      Address and Routing Module (WARM).
 *
 */

#include <Warm/Warm.h>
#include <Weave/Support/CodeUtils.h>

namespace nl {

namespace Weave {

namespace Warm {

// Internal Types

typedef enum {
        kInitStateNotInitialized = 0,   /**< This must be 0 so that mInitState's initial value will be kInitStateNotInitialized. */
        kInitStateInitialized
} InitState;

typedef enum {
    kPlatformActionExecutionContinue                      = false,  /**< continue action execution. */
    kPlatformActionExecutionSuspendForAsynchOpCompletion  = true    /**< suspend action execution for asynchronous operation to complete. */
} PlatformActionExecution;

typedef enum {
    kSystemFeatureTypeIsFabricMember           = (1 << 0),    /**< The System's Weave module            IS | IS NOT     a member of a fabric. */
    kSystemFeatureTypeWiFiConnected            = (1 << 1),    /**< The System's WiFi Interface          IS | IS NOT     connected. */
    kSystemFeatureTypeThreadConnected          = (1 << 2),    /**< The System's Thread Interface        IS | IS NOT     connected. */
    kSystemFeatureTypeThreadRoutingEnabled     = (1 << 3),    /**< The System's Thread Routing Feature  IS | IS NOT     enabled. */
    kSystemFeatureTypeBorderRoutingEnabled     = (1 << 4),    /**< The System's Border Routing Feature  IS | IS NOT     enabled. */
    kSystemFeatureTypeTunnelInterfaceEnabled   = (1 << 5),    /**< The System's Tunnel Interface        IS | IS NOT     enabled. */
    kSystemFeatureTypeTunnelState              = (1 << 6),    /**< The System's Tunnel Service          IS | IS NOT     established. */
    kSystemFeatureTypeCellularConnected        = (1 << 7),    /**< The System's Cellular Interface      IS | IS NOT     connected. */

    kSystemFeatureTypeMax                      = (1 << 16)    /**< DO NOT EXCEED; reserved to mark the max available bits. */
} SystemFeatureType;

typedef enum {
    kActionTypeWiFiHostAddress              = (1 << 0),   /**< Add | Remove the IP address for the WiFi Interface on the host's IP stack. */
    kActionTypeThreadHostAddress            = (1 << 1),   /**< Add | Remove the IP address for the Thread Interface on the host's IP stack. */
    kActionTypeThreadThreadAddress          = (1 << 2),   /**< Add | Remove the IP address for the Thread Interface on the Thread Module's IP stack. */
    kActionTypeLegacy6LoWPANHostAddress     = (1 << 3),   /**< Add | Remove the IP address for the Legacy 6LowPAN Interface on the host's IP stack. */
    kActionTypeLegacy6LoWPANThreadAddress   = (1 << 4),   /**< Add | Remove the IP address for the Legacy 6LowPAN Interface on the Thread Module's IP stack. */
    kActionTypeHostRouteThread              = (1 << 5),   /**< Add | Remove the IP route for the Thread Interface on the host's IP stack. */
    kActionTypeThreadAdvertisement          = (1 << 6),   /**< Start | Stop the route advertisement by the Thread Module. */
    kActionTypeThreadRoute                  = (1 << 7),   /**< Add | Remove the IP route on the Thread Module for Border Route support. */
    kActionTypeTunnelHostAddress            = (1 << 8),   /**< Add | Remove the IP address for the Tunnel Interface on the host's IP stack. */
    kActionTypeTunnelHostRoute              = (1 << 9),   /**< Add | Remove the IP route for the Tunnel Interface on the host's IP stack. */
    kActionTypeThreadRoutePriority          = (1 << 10),  /**< Change the Route Priority of the Thread Route on the Thread Module. */
    kActionTypeTunnelServiceRoute           = (1 << 11),  /**< Add | Remove the 64 bit IP route for Service subnet on the host's IP stack. */

    kActionTypeMax                          = (1 << 16)   /**< DO NOT EXCEED; reserved to mark the max available bits. */
} ActionType;

typedef uint16_t FlagsType;

typedef PlatformResult (* ActionFunction)(ActionType inAction, bool inActivate, const uint64_t &inGlobalId, const uint64_t &inInterfaceId);

typedef struct
{
    FlagsType           mNecessaryActiveSystemFeatures; /**< Stores the System Features that that are pre-requisites for taking the affirmative form of the Action. */
    ActionType          mActionType;                    /**< The type of Action to which this Entry pertains. */
    ActionFunction      mAction;                        /**< A function to execute the Action. */
} ActionEntry;

typedef struct {
    InitState                                               mInitState;                    /**< Tracks State of the Module initialization. */
    const WeaveFabricState                                  *mFabricState;                 /**< Stores a fabric State. */
    uint64_t                                                mFabricId;                     /**< Stores the fabric id which was last joined */
    FlagsType                                               mSystemFeatureStateFlags;      /**< Tracks changes for System Feature State. */
    FlagsType                                               mActionStateFlags;             /**< Tracks State of Actions. */
#if WARM_CONFIG_SUPPORT_WEAVE_TUNNEL
    Profiles::WeaveTunnel::Platform::TunnelAvailabilityMode mTunnelRequestedAvailability;  /**< Stores the desired Tunnel availability. */
    Profiles::WeaveTunnel::Platform::TunnelAvailabilityMode mTunnelCurrentAvailability;    /**< Stores the configured Tunnel availability. */
#endif

    // the following members are used to support platform API's that return kPlatformResultInProgress
    volatile bool                                           mActionInProgress;             /**< Tracks whether or not an Action is in progress. */
    ActionType                                              mInProgressAction;             /**< Stores the type of Action that is in progress. */
    bool                                                    mInProgressActionState;        /**< Stores the desired State of the Action when the Action completes. */
} ModuleState;

// Prototypes

static void TakeActions(void);

// Global Variables

static ModuleState sState;
static WarmFabricStateDelegate sFabricStateDelegate;

// These may be unused depending on WARM_CONFIG_SUPPORT_XXX definitions
#if WARM_CONFIG_SUPPORT_THREAD_ROUTING || \
    WARM_CONFIG_SUPPORT_WIFI || \
    WARM_CONFIG_SUPPORT_CELLULAR || \
    WARM_CONFIG_SUPPORT_THREAD_ROUTING || \
    WARM_CONFIG_SUPPORT_WEAVE_TUNNEL || \
    WARM_CONFIG_SUPPORT_BORDER_ROUTING
static const uint8_t kGlobalULAPrefixLength               = 48;
#endif
#if WARM_CONFIG_SUPPORT_WIFI
static const uint8_t kWiFiULAAddressPrefixLength          = 64;
#endif
static const uint8_t kThreadULAAddressPrefixLength        = 64;
#if WARM_CONFIG_SUPPORT_LEGACY6LOWPAN_NETWORK
static const uint8_t kLegacy6LoWPANULAAddressPrefixLength = 64;
#endif

#if WARM_CONFIG_SUPPORT_WEAVE_TUNNEL

static const uint8_t kTunnelAddressPrefixLength           = 128;

#if !WARM_CONFIG_ENABLE_FABRIC_DEFAULT_ROUTING
static const uint8_t kServiceULAAddressPrefixLength       = 64;
#endif // !WARM_CONFIG_ENABLE_FABRIC_DEFAULT_ROUTING

#endif // WARM_CONFIG_SUPPORT_WEAVE_TUNNEL

// Implementation

/**
 *  A static function that returns the current State of a specified action.
 *
 *  @note
 *    Refer to the definition of ActionType for
 *    the set of possible actions.
 *
 *  @param[in] inAction    The action type to query.
 *
 *  @return true if the action is Set, false otherwise.
 *
 */
static bool GetCurrentActionState(ActionType inAction)
{
    return (sState.mActionStateFlags & inAction) ? true : false;
}

/**
 *  A static function that sets the current State of a specified action.
 *
 *  @note
 *    Refer to the definition of ActionType for
 *    the set of possible actions.
 *
 *  @param[in] inAction    The action type to change.
 *  @param[in] inValue     The new State value to adopt.
 *
 */
static void SetCurrentActionState(ActionType inAction, bool inValue)
{
    const bool current = GetCurrentActionState(inAction);

    if (current != inValue) {

        if (inValue) {
            sState.mActionStateFlags |= inAction;
        } else {
            sState.mActionStateFlags &= ~inAction;
        }
    }
}

/**
 *  A static function that gets the current State of a System feature.
 *
 *  @note
 *    Refer to the definition of SystemFeatureType for
 *    the set of possible types.
 *
 *  @param[in] inSystemFeature    The System Feature to query.
 *
 *  @return true if the System Feature is enabled, false otherwise.
 *
 */
static inline bool GetSystemFeatureState(SystemFeatureType inSystemFeature)
{
    return (sState.mSystemFeatureStateFlags & inSystemFeature) ? true : false;
}

/**
 *  A static function that sets the current State of the System Feature.
 *
 *  @note
 *    Refer to the definition of SystemFeatureType for
 *    the set of possible System Features.
 *
 *  @param[in] inSystemFeature    The SystemFeature to set.
 *  @param[in] inValue            The new State value to adopt.
 *
 *  @return true if the System Feature was changed, false otherwise.
 *
 */
static bool SetSystemFeatureState(SystemFeatureType inSystemFeature, bool inValue)
{
    bool retval = false;
    const bool current = GetSystemFeatureState(inSystemFeature);

    if (current != inValue) {

        if (inValue) {
            sState.mSystemFeatureStateFlags |= inSystemFeature;
        } else {
            sState.mSystemFeatureStateFlags &= ~inSystemFeature;
        }

        retval = true;
    }

    return retval;
}

/**
 *  A static function that determines whether the specified action should be performed.
 *
 *  @brief
 *    This function examines the condition of the System Feature State flags and determines
 *    whether the specified action should be enabled or disabled.  The function then
 *    examines the current state of the action and if the action is not set to
 *    the value required by the System Feature's State, then the function returns true
 *    along with the desired action state in outActivate
 *
 *
 *  @param[in] inAction                         The action to be queried.
 *  @param[in] inNecessarySystemFeatureState    The State flags necessary for the action to be active.
 *  @param[out] outActivate                     The desired state of the action.
 *
 *  @return true if the action is not currently in the desired state, false otherwise.
 *
 */
static bool ShouldPerformAction(ActionType inAction, FlagsType inNecessarySystemFeatureState, bool &outActivate)
{
    bool retval = false;
    // Compare the necessary System Feature State flags to the current state of the action.
    const bool desiredActionState = (inNecessarySystemFeatureState == (sState.mSystemFeatureStateFlags & inNecessarySystemFeatureState));

#if WARM_CONFIG_SUPPORT_WEAVE_TUNNEL && WARM_CONFIG_SUPPORT_BORDER_ROUTING
    if (inAction == kActionTypeThreadRoutePriority)
    {
        if (desiredActionState &&
            sState.mTunnelRequestedAvailability != sState.mTunnelCurrentAvailability)
        {
            retval = true;  // instruct the caller to take action.
        }
    }
    else
#endif // WARM_CONFIG_SUPPORT_WEAVE_TUNNEL && WARM_CONFIG_SUPPORT_BORDER_ROUTING
    {
        if (GetCurrentActionState(inAction) != desiredActionState)
        {
            retval = true; // instruct the caller to take action.
            outActivate = desiredActionState; // provide the caller with the desired state of the action
        }
    }

    return retval;
}

/**
 *  A static function that records the result of a platform API Action call.
 *
 *  @brief
 *    This module makes requests to perform actions via platform specific API's.
 *    The API's are required to report the kPlatformResultSuccess|kPlatformResultFailure|kPlatformResultInProgress
 *    result of that action request.  This function records that result and returns
 *    true if the result is in-progress and further actions should be delayed.
 *
 *  @param[in] inResult         The platform API result.
 *  @param[in] inAction         The action that the platform API attempted.
 *  @param[in] inActionState    The new State of the action if the result was success.
 *
 *  @return true the platform API is asynchronously processing the request, false otherwise.
 *
 */
static PlatformActionExecution RecordPlatformResult(PlatformResult inResult, ActionType inAction, bool inActionState)
{
    PlatformActionExecution retval = kPlatformActionExecutionContinue;

    switch (inResult)
    {
        case kPlatformResultSuccess:
            SetCurrentActionState(inAction, inActionState);

#if WARM_CONFIG_SUPPORT_WEAVE_TUNNEL && WARM_CONFIG_SUPPORT_BORDER_ROUTING
            if ((inAction == kActionTypeThreadRoute && inActionState) ||
                (inAction == kActionTypeThreadRoutePriority))
            {
                sState.mTunnelCurrentAvailability = sState.mTunnelRequestedAvailability;
            }
#endif // WARM_CONFIG_SUPPORT_WEAVE_TUNNEL && WARM_CONFIG_SUPPORT_BORDER_ROUTING
            break;

        case kPlatformResultFailure:
            // return false to keep going. By not setting the ActionState the Action will be retried.
            // TODO: perhaps return true here to prevent continuation of actions on this iteration.
            break;

        case kPlatformResultInProgress:
            retval = kPlatformActionExecutionSuspendForAsynchOpCompletion; // instruct caller to stop executing actions.
            // record that an action is in progress and use this in ReportActionComplete()
            sState.mInProgressAction = inAction;
            sState.mInProgressActionState = inActionState;
            sState.mActionInProgress = true;
            break;
    }

    return retval;
}

/**
 *  A static function that Sets the System Feature state and notifies the platform
 *  that event state has changed.
 *
 *  @brief
 *    Called by the EventStateChange API's to perform the necessary reaction
 *    operations.
 *
 *  @param[in] inSystemFeatureType    The State that changed corresponding to the API called.
 *  @param[in] inState                The new value for the state.
 *
 */
static void SystemFeatureStateChangeHandler(SystemFeatureType inSystemFeatureType, bool inState)
{
    bool stateDidTransition;

    Platform::CriticalSectionEnter();
    {
        // If the state change is "becoming a new fabric member", update the copy of fabric id in local state.
        // Local copy of fabric id is used for calculating the addresses/routes to be added/remove to ensure
        // that the correct addresses/routes are removed when leaving the fabric (i.e., FabricState is cleared
        // and FabricState->FabricId is set to zero).
        if ((inSystemFeatureType == kSystemFeatureTypeIsFabricMember) && (inState == kInterfaceStateUp))
        {
            sState.mFabricId = sState.mFabricState->FabricId;
        }

        stateDidTransition = SetSystemFeatureState(inSystemFeatureType, inState);
    }
    Platform::CriticalSectionExit();

    if (stateDidTransition)
    {
        // Notify the platform layer that internal core State has changed and the platform
        // needs to call InvokeActions either synchronously or asynchronously as appropriate.
        Platform::RequestInvokeActions();
    }
}

/**
 *  A function called to announce a State change for the Weave Fabric feature.
 *
 *  @note
 *    While this function is similar to the other WARM APIs, it is used internally by WarmFabricStateDelegate class.
 *    This function is called when the device joins or leaves a Weave fabric. If the device boots
 *    up as a member of a fabric, this function should be called after Init() and after the fabricId has
 *    been stored in the Weave FabricState.
 *    If this call results in a change of State WARM will call RequestInvokeActions.
 *
 *  @param[in] inState    kInterfaceStateUp if the system is a member of a Weave fabric, kInterfaceStateDown otherwise.
 *
 */
static void FabricStateChange(InterfaceState inState)
{
    SystemFeatureStateChangeHandler(kSystemFeatureTypeIsFabricMember, inState);
}

/**
 *  This method is invoked by WeaveFabricState when joining/creating a new fabric.
 *
 *  @param[in] fabricState   The WeaveFabricState which is changed.
 *  @param[in] newFabricId   The new fabric id.
 *
 */
void WarmFabricStateDelegate::DidJoinFabric(WeaveFabricState *fabricState, uint64_t newFabricId)
{
    FabricStateChange(kInterfaceStateUp);
}

/**
 *  This method is invoked by WeaveFabricState when leaving/clearing a fabric.
 *
 *  @param[in] fabricState   The WeaveFabricState which is changed.
 *  @param[in] oldFabricId   The old/previous fabric id.
 *
 */
void WarmFabricStateDelegate::DidLeaveFabric(WeaveFabricState *fabricState, uint64_t oldFabricId)
{
    FabricStateChange(kInterfaceStateDown);
}

/**
 *  A WARM API to perform one time module initialization.
 *
 *  @note
 *    It must be called prior to any other API calls.
 *
 *  @retval WEAVE_NO_ERROR               On successful initialization.
 *  @retval WEAVE_ERROR_INCORRECT_STATE  When Init is called more than once.
 *  @retval other                        Error code otherwise.
 *
 */
WEAVE_ERROR Init(WeaveFabricState &inFabricState)
{
    WEAVE_ERROR err;

    if (sState.mInitState != kInitStateNotInitialized)
    {
        return WEAVE_ERROR_INCORRECT_STATE;
    }

    sState.mSystemFeatureStateFlags = 0;
    sState.mActionStateFlags        = 0;
    sState.mActionInProgress        = false;
    sState.mFabricState             = &inFabricState;

    err = Platform::Init(&sFabricStateDelegate);
    SuccessOrExit(err);

    // Set the WeaveFabricState delegate first.
    inFabricState.SetDelegate(&sFabricStateDelegate);

    Platform::CriticalSectionEnter();
    {
        // Then update/inform WARM of the current fabric state. This should be done within
        // the critical section to protect against race situation caused by delegate callbacks
        // (which can be invoked from another task/process context)
        FabricStateChange((inFabricState.FabricId != 0)?  kInterfaceStateUp : kInterfaceStateDown);
    }
    Platform::CriticalSectionExit();

    sState.mInitState = kInitStateInitialized;

exit:
    return err;
}

/**
 *  A WARM API to acquire a ULA for a specified interface type.
 *
 *  @note
 *    Platform code should call this only after WARM has been initialized. Calling this API
 *    prior to initialization will result in an error.
 *
 *  @param[in] inInterfaceType    The type of interface for which a ULA is desired.
 *  @param[out] outAddress        An address object used to hold the resulting ULA.
 *
 *  @retval WEAVE_NO_ERROR                On success.
 *  @retval WEAVE_ERROR_INCORRECT_STATE   If this API is called while WARM is
 *                                        not a member of a Fabric.
 *  @retval WEAVE_ERROR_INVALID_ARGUMENT  If this API is called with an invalid Interface Type.
 *
 */
WEAVE_ERROR GetULA(InterfaceType inInterfaceType, Inet::IPAddress &outAddress)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint64_t    globalId;
    uint64_t    interfaceId;
    uint16_t    subnet;

    switch (inInterfaceType)
    {
        case kInterfaceTypeLegacy6LoWPAN:
            subnet = nl::Weave::kWeaveSubnetId_ThreadAlarm;
            break;

        case kInterfaceTypeThread:
            subnet = nl::Weave::kWeaveSubnetId_ThreadMesh;
            break;

        case kInterfaceTypeWiFi:
            subnet = nl::Weave::kWeaveSubnetId_PrimaryWiFi;
            break;

        default:
            err = WEAVE_ERROR_INVALID_ARGUMENT;
            goto exit;
    }

    Platform::CriticalSectionEnter();
    {
        if (GetSystemFeatureState(kSystemFeatureTypeIsFabricMember))
        {
            globalId = nl::Weave::WeaveFabricIdToIPv6GlobalId(sState.mFabricId);
            interfaceId = nl::Weave::WeaveNodeIdToIPv6InterfaceId(sState.mFabricState->LocalNodeId);
        } else {
            err = WEAVE_ERROR_INCORRECT_STATE;
        }
    }
    Platform::CriticalSectionExit();

    SuccessOrExit(err);

    outAddress = nl::Inet::IPAddress::MakeULA(globalId, subnet, interfaceId);

exit:
    return err;
}

/**
 *  A WARM API to acquire the FabricState object that was provided to Warm during Init.
 *
 *  @note
 *    Platform code should call this only after WARM has been initialized. Calling this API
 *    prior to initialization will result in an error.
 *
 *  @param[out] outFabricState        A pointer reference to a fabricState object.
 *
 *  @retval WEAVE_NO_ERROR                On success.
 *  @retval WEAVE_ERROR_INCORRECT_STATE   If this API is called before WARM
 *                                        has been initialized.
 *
 */
WEAVE_ERROR GetFabricState(const WeaveFabricState *&outFabricState)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    if (sState.mInitState != kInitStateInitialized)
    {
        err = WEAVE_ERROR_INCORRECT_STATE;
    }

    SuccessOrExit(err);

    outFabricState = sState.mFabricState;

exit:
    return err;
}

/**
 *  A WARM API called by a dedicated task to perform various platform API actions.
 *
 *  @brief
 *    This represents the entry point to perform the actions necessary which will satisfy
 *    the current System State.  If for example the Thread stack transitioned from
 *    disabled to enabled, then this function would make the necessary platform calls
 *    to assign the thread host address etc.  This function should be called by platform
 *    code only in response to a Warm call to RequestInvokeActions.  Calling InvokeActions
 *    will result in one or more calls to nl::Warm::Platform API's.
 *    Developers should therefore implement RequestInvokeActions and the caller of
 *    InvokeActions() appropriately. It might be appropriate for RequestInvokeActions to post
 *    an event to the task which would call InvokeActions() for example.  Conversely, if the system
 *    is single threaded, then RequestInvokeActions could be implemented to call InvokeActions()
 *    directly.
 *
 */
void InvokeActions(void)
{
    Platform::CriticalSectionEnter();
    {
        if (!sState.mActionInProgress)
        {
            // this represents what should be the one-and-only call to TakeActions().
            TakeActions();
        }
    }
    Platform::CriticalSectionExit();
}

/**
 *  A WARM API called to announce the completion of a previous asynchronous platform API call.
 *
 *  @brief
 *    It is assumed that platform action API's may need to perform asynchronous operations.
 *    If this is true then the platform API will return kPlatformResultInProgress.
 *    When this happens new Address and Routing Actions will be suspended until the system
 *    calls ReportActionComplete to announce the completion of the operation.
 *
 *  @param[in]  inResult    The result of the pending action. must be one of: {kPlatformResultSuccess | kPlatformResultFailure}
 *
 */
void ReportActionComplete(PlatformResult inResult)
{
    VerifyOrExit(((inResult != kPlatformResultInProgress) && (sState.mActionInProgress)), (void)0);

    Platform::CriticalSectionEnter();
    {
        RecordPlatformResult(inResult, sState.mInProgressAction, sState.mInProgressActionState);
        sState.mActionInProgress = false;
    }
    Platform::CriticalSectionExit();

    Platform::RequestInvokeActions();

exit:
    return;
}

#if WARM_CONFIG_SUPPORT_CELLULAR

/**
 *  A WARM API called to announce a State change for the Cellular interface.
 *
 *  @note
 *    Platform code should call this function when the Cellular interface transitions between up <-> down.
 *    If this call results in a change of State WARM will call RequestInvokeActions.
 *
 *  @param[in] inState    kInterfaceStateUp if the Cellular interface is up, kInterfaceStateDown otherwise.
 *
 */
void CellularInterfaceStateChange(InterfaceState inState)
{
    SystemFeatureStateChangeHandler(kSystemFeatureTypeCellularConnected, inState);
}

#endif // WARM_CONFIG_SUPPORT_CELLULAR

#if WARM_CONFIG_SUPPORT_WIFI

/**
 *  One of the Action methods. Sets the Host Address for the WiFi Interface.
 *
 *  @param[in] inAction         The action type.
 *  @param[in] inActivate       The desired State true == activate, false == deactivate.
 *  @param[in] inGlobalId       A reference to the Weave Global ID if it's necessary to calculate an address.
 *  @param[in] inInterfaceId    A reference to the device's interface ID if it's necessary to calculate an address.
 *
 *  @return Forwards the result from Platform::HostAddress().
 *
 */
static PlatformResult WiFiHostAddressAction(ActionType inAction, bool inActivate, const uint64_t &inGlobalId, const uint64_t &inInterfaceId)
{
    const Inet::IPAddress address = nl::Inet::IPAddress::MakeULA(inGlobalId, nl::Weave::kWeaveSubnetId_PrimaryWiFi, inInterfaceId);

    return Platform::AddRemoveHostAddress(kInterfaceTypeWiFi, address, kWiFiULAAddressPrefixLength, inActivate);
}

/**
 *  A WARM API called to announce a State change for the WiFi interface.
 *
 *  @note
 *    Platform code should call this function when the WiFi interface transitions between up <-> down.
 *    If this call results in a change of State WARM will call RequestInvokeActions.
 *
 *  @param[in] inState    kInterfaceStateUp if the WiFi interface is up, kInterfaceStateDown otherwise.
 *
 */
void WiFiInterfaceStateChange(InterfaceState inState)
{
    SystemFeatureStateChangeHandler(kSystemFeatureTypeWiFiConnected, inState);
}

#endif // WARM_CONFIG_SUPPORT_WIFI

/**
 *  A utility to construct a 48 bit prefix from a globalID.
 *
 *  @param[in] inGlobalID       A reference to the Weave Global ID.
 *  @param[out] outPrefix       The Prefix to initialize.
 *
 */
#if WARM_CONFIG_SUPPORT_THREAD_ROUTING || \
    WARM_CONFIG_SUPPORT_WIFI || \
    WARM_CONFIG_SUPPORT_CELLULAR || \
    WARM_CONFIG_SUPPORT_WEAVE_TUNNEL || \
    WARM_CONFIG_SUPPORT_BORDER_ROUTING
static void MakePrefix(const uint64_t &inGlobalID, const uint16_t subnetId, const uint8_t inPrefixLen,
                       Inet::IPPrefix &outPrefix)
{
    const Inet::IPAddress address = nl::Inet::IPAddress::MakeULA(inGlobalID, subnetId, 0);

    outPrefix.IPAddr = address;
    outPrefix.Length = inPrefixLen;
}
#endif // WARM_CONFIG_SUPPORT_THREAD_ROUTING || WARM_CONFIG_SUPPORT_WIFI...

#if WARM_CONFIG_SUPPORT_THREAD

/**
 *  One of the Action methods. Sets the Host Address for the Thread Interface.
 *
 *  @param[in] inAction         The action type.
 *  @param[in] inActivate       The desired State true == activate, false == deactivate.
 *  @param[in] inGlobalId       A reference to the Weave Global ID if it's necessary to calculate an address.
 *  @param[in] inInterfaceId    A reference to the device's interface ID if it's necessary to calculate an address.
 *
 *  @return Forwards the result from Platform::HostAddress().
 *
 */
static PlatformResult ThreadHostAddressAction(ActionType inAction, bool inActivate, const uint64_t &inGlobalId, const uint64_t &inInterfaceId)
{
    const Inet::IPAddress address = nl::Inet::IPAddress::MakeULA(inGlobalId, nl::Weave::kWeaveSubnetId_ThreadMesh, inInterfaceId);

    return Platform::AddRemoveHostAddress(kInterfaceTypeThread, address, kThreadULAAddressPrefixLength, inActivate);
}

/**
 *  One of the Action methods. Sets the Thread Address for the Thread Interface.
 *
 *  @param[in] inAction         The action type.
 *  @param[in] inActivate       The desired State true == activate, false == deactivate.
 *  @param[in] inGlobalId       A reference to the Weave Global ID if it's necessary to calculate an address.
 *  @param[in] inInterfaceId    A reference to the device's interface ID if it's necessary to calculate an address.
 *
 *  @return Forwards the result from Platform::ThreadAddress().
 *
 */
static PlatformResult ThreadThreadAddressAction(ActionType inAction, bool inActivate, const uint64_t &inGlobalId, const uint64_t &inInterfaceId)
{
    const Inet::IPAddress address = nl::Inet::IPAddress::MakeULA(inGlobalId, nl::Weave::kWeaveSubnetId_ThreadMesh, inInterfaceId);

    return Platform::AddRemoveThreadAddress(kInterfaceTypeThread, address, inActivate);
}

/**
 *  One of the Action methods. Sets the Host Route for the Thread Interface.
 *
 *  @param[in] inAction         The action type.
 *  @param[in] inActivate       The desired State true == activate, false == deactivate.
 *  @param[in] inGlobalId       A reference to the Weave Global ID if it's necessary to calculate an address.
 *  @param[in] inInterfaceId    A reference to the device's interface ID if it's necessary to calculate an address.
 *
 *  @return Forwards the result from Platform::HostRoute().
 *
 */
static PlatformResult ThreadHostRouteAction(ActionType inAction, bool inActivate, const uint64_t &inGlobalId, const uint64_t &inInterfaceId)
{
    Inet::IPPrefix prefix;
    PlatformResult result = kPlatformResultSuccess;

#if WARM_CONFIG_SUPPORT_WIFI || WARM_CONFIG_SUPPORT_CELLULAR

#if WARM_CONFIG_ENABLE_FABRIC_DEFAULT_ROUTING
    MakePrefix(inGlobalId, 0, kGlobalULAPrefixLength, prefix);
#else
    MakePrefix(inGlobalId, nl::Weave::kWeaveSubnetId_Service, kServiceULAAddressPrefixLength, prefix);
#endif // WARM_CONFIG_ENABLE_FABRIC_DEFAULT_ROUTING

#if WARM_CONFIG_ENABLE_BACKUP_ROUTING_OVER_THREAD
    result = Platform::AddRemoveHostRoute(kInterfaceTypeThread, prefix, kRoutePriorityLow, inActivate);
#endif // WARM_CONFIG_ENABLE_BACKUP_ROUTING_OVER_THREAD

#else

    prefix = prefix.Zero;
    prefix.Length = 0;

    result = Platform::AddRemoveHostRoute(kInterfaceTypeThread, prefix, kRoutePriorityLow, inActivate);
#endif // WARM_CONFIG_SUPPORT_WIFI || WARM_CONFIG_SUPPORT_CELLULAR

    return result;
}

/**
 *  A WARM API called to announce a State change for the Thread interface.
 *
 *  @note
 *    Platform code should call this function when the Thread interface transitions between up <-> down.
 *    If this call results in a change of State WARM will call RequestInvokeActions.
 *
 *  @param[in] inState    kInterfaceStateUp if the Thread interface is up, kInterfaceStateDown otherwise.
 *
 */
void ThreadInterfaceStateChange(InterfaceState inState)
{
    SystemFeatureStateChangeHandler(kSystemFeatureTypeThreadConnected, inState);
}

#endif // WARM_CONFIG_SUPPORT_THREAD

#if WARM_CONFIG_SUPPORT_LEGACY6LOWPAN_NETWORK

/**
 *  One of the Action methods. Sets the Host Address for the Legacy Interface.
 *
 *  @param[in] inAction         The action type.
 *  @param[in] inActivate       The desired State true == activate, false == deactivate.
 *  @param[in] inGlobalId       A reference to the Weave Global ID if it's necessary to calculate an address.
 *  @param[in] inInterfaceId    A reference to the device's interface ID if it's necessary to calculate an address.
 *
 *  @return Forwards the result from Platform::HostAddress().
 *
 */
static PlatformResult LegacyHostAddressAction(ActionType inAction, bool inActivate, const uint64_t &inGlobalId, const uint64_t &inInterfaceId)
{
    const Inet::IPAddress address = nl::Inet::IPAddress::MakeULA(inGlobalId, nl::Weave::kWeaveSubnetId_ThreadAlarm, inInterfaceId);

    return Platform::AddRemoveHostAddress(kInterfaceTypeLegacy6LoWPAN, address, kLegacy6LoWPANULAAddressPrefixLength, inActivate);
}

/**
 *  One of the Action methods. Sets the Thread Address for the Legacy 6LoWPAN Interface.
 *
 *  @param[in] inAction         The action type.
 *  @param[in] inActivate       The desired State true == activate, false == deactivate.
 *  @param[in] inGlobalId       A reference to the Weave Global ID if it's necessary to calculate an address.
 *  @param[in] inInterfaceId    A reference to the device's interface ID if it's necessary to calculate an address.
 *
 *  @return Forwards the result from Platform::ThreadAddress().
 *
 */
static PlatformResult LegacyThreadAddressAction(ActionType inAction, bool inActivate, const uint64_t &inGlobalId, const uint64_t &inInterfaceId)
{
    const Inet::IPAddress address = nl::Inet::IPAddress::MakeULA(inGlobalId, nl::Weave::kWeaveSubnetId_ThreadAlarm, inInterfaceId);

    return Platform::AddRemoveThreadAddress(kInterfaceTypeLegacy6LoWPAN, address, inActivate);
}

#endif // WARM_CONFIG_SUPPORT_LEGACY6LOWPAN_NETWORK

#if WARM_CONFIG_SUPPORT_THREAD_ROUTING

/**
 *  One of the Action methods. Sets the Thread Advertisement State
 *
 *  @param[in] inAction         The action type.
 *  @param[in] inActivate       The desired State true == activate, false == deactivate.
 *  @param[in] inGlobalId       A reference to the Weave Global ID if it's necessary to calculate an address.
 *  @param[in] inInterfaceId    A reference to the device's interface ID if it's necessary to calculate an address.
 *
 *  @return Forwards the result from Platform::ThreadAdvertisement().
 *
 */
static PlatformResult ThreadAdvertisementAction(ActionType inAction, bool inActivate, const uint64_t &inGlobalId, const uint64_t &inInterfaceId)
{
    Inet::IPPrefix prefix;

    MakePrefix(inGlobalId, 0, kGlobalULAPrefixLength, prefix);

    return Platform::StartStopThreadAdvertisement(kInterfaceTypeThread, prefix, inActivate);
}

/**
 *  A WARM API called to announce a State change for the Thread Routing feature.
 *
 *  @note
 *    Platform code should call this function when the ThreadRouting feature transitions between active <-> deactive.
 *    If this call results in a change of State WARM will call RequestInvokeActions.
 *
 *  @param[in] inState    kInterfaceStateUp if the Thread routing feature is up, kInterfaceStateDown otherwise.
 *
 */
void ThreadRoutingStateChange(InterfaceState inState)
{
    SystemFeatureStateChangeHandler(kSystemFeatureTypeThreadRoutingEnabled, inState);
}

#endif // WARM_CONFIG_SUPPORT_THREAD_ROUTING

#if WARM_CONFIG_SUPPORT_WEAVE_TUNNEL

/**
 *  One of the Action methods. Sets the HostAddress for the Tunnel Interface.
 *
 *  @param[in] inAction         The action type.
 *  @param[in] inActivate       The desired State true == activate, false == deactivate.
 *  @param[in] inGlobalId       A reference to the Weave Global ID if it's necessary to calculate an address.
 *  @param[in] inInterfaceId    A reference to the device's interface ID if it's necessary to calculate an address.
 *
 *  @return Forwards the result from Platform::HostAddress().
 *
 */
static PlatformResult TunnelHostAddressAction(ActionType inAction, bool inActivate, const uint64_t &inGlobalId, const uint64_t &inInterfaceId)
{
    // Prioritize using the Thread address over the WiFi address in order to improve connectivity
    // in cases where the WiFi network is unavailable and another path exists to the fabic.  See
    // the Weave Device Local Addressing and Routing Behavior document for more detail.
#if WARM_CONFIG_SUPPORT_THREAD
    const uint16_t kTunnelSubnet = nl::Weave::kWeaveSubnetId_ThreadMesh;
#else
    const uint16_t kTunnelSubnet = nl::Weave::kWeaveSubnetId_PrimaryWiFi;
#endif

    const Inet::IPAddress address = nl::Inet::IPAddress::MakeULA(inGlobalId, kTunnelSubnet, inInterfaceId);

    return Platform::AddRemoveHostAddress(kInterfaceTypeTunnel, address, kTunnelAddressPrefixLength, inActivate);
}

/**
 *  One of the Action methods. Sets the HostRoute for the Tunnel Interface.
 *
 *  @param[in] inAction         The action type.
 *  @param[in] inActivate       The desired State true == activate, false == deactivate.
 *  @param[in] inGlobalId       A reference to the Weave Global ID if it's necessary to calculate an address.
 *  @param[in] inInterfaceId    A reference to the device's interface ID if it's necessary to calculate an address.
 *
 *  @return Forwards the result from Platform::HostRoute().
 *
 */
static PlatformResult TunnelHostRouteAction(ActionType inAction, bool inActivate, const uint64_t &inGlobalId, const uint64_t &inInterfaceId)
{
    Inet::IPPrefix prefix;

#if WARM_CONFIG_ENABLE_FABRIC_DEFAULT_ROUTING
    MakePrefix(inGlobalId, 0, kGlobalULAPrefixLength, prefix);
#else
    MakePrefix(inGlobalId, nl::Weave::kWeaveSubnetId_Service, kServiceULAAddressPrefixLength, prefix);
#endif

    return Platform::AddRemoveHostRoute(kInterfaceTypeTunnel, prefix, kRoutePriorityMedium, inActivate);
}

/**
 *  A WARM API called to announce a State change for the Weave Tunnel interface.
 *
 *  @note
 *    This WARM API is called by the Weave Tunnel Agent Platform API's. The Platform code
 *    should not call this API as it would for other API's.
 *    If this call results in a change of State WARM will call RequestInvokeActions.
 *
 *  @param[in] inState           kInterfaceStateUp if the Weave Tunnel interface is up, kInterfaceStateDown otherwise.
 *
 */
static void TunnelInterfaceStateChange(InterfaceState inState)
{
    SystemFeatureStateChangeHandler(kSystemFeatureTypeTunnelInterfaceEnabled, inState);
}

/**
 *  A WARM API called to announce a State change for the Weave Tunnel interface.
 *
 *  @note
 *    This WARM API is called by the Weave Tunnel Agent Platform API's. The Platform code
 *    should not call this API as it would for other API's.
 *    If this call results in a change of State WARM will call RequestInvokeActions.
 *
 *  @param[in] inState           kInterfaceStateUp if the Weave Tunnel Service is established, kInterfaceStateDown otherwise.
 *  @param[in] inAvailability    The availability status to be used later in configuring the tunnel.
 *
 */
static void TunnelServiceStateChange(InterfaceState inState, nl::Weave::Profiles::WeaveTunnel::Platform::TunnelAvailabilityMode inAvailability)
{
    if (inState)
    {
        Platform::CriticalSectionEnter();
        {
            sState.mTunnelRequestedAvailability = inAvailability;
        }
        Platform::CriticalSectionExit();
    }

    SystemFeatureStateChangeHandler(kSystemFeatureTypeTunnelState, inState);
}

/**
 *  A WARM API called to update the priority of the Tunnel Service.
 *
 *  @note
 *    This WARM API is called by the Weave Tunnel Agent Platform API's. The Platform code
 *    should not call this API as it would for other API's.  If this call results
 *    in a change of State WARM will call RequestInvokeActions
 *
 *  @param[in] inAvailability    The new value for the tunnel availability status.
 *
 */
static void TunnelPriorityStateChange(nl::Weave::Profiles::WeaveTunnel::Platform::TunnelAvailabilityMode inAvailability)
{
    bool notify = false;

    Platform::CriticalSectionEnter();
    {
        if (GetSystemFeatureState(kSystemFeatureTypeTunnelState) &&
            sState.mTunnelRequestedAvailability != inAvailability)
        {
            sState.mTunnelRequestedAvailability = inAvailability;
            notify = true;
        }
    }
    Platform::CriticalSectionExit();

    if (notify)
    {
        Platform::RequestInvokeActions();
    }
}

}; // namespace Warm

// Implementation of WeaveTunnelAgent Platform API's

namespace Profiles {
namespace WeaveTunnel {
namespace Platform {

/**
 *  A TunnelAgent Platform API implementation used by the Tunnel Agent to announce the tunnel interface is enabled.
 *
 *  @param[in] tunIf    The InterfaceId for the tunnel Interface.  Not used in this implementation.
 *
 */
void TunnelInterfaceUp(InterfaceId tunIf)
{
    (void)tunIf;

    Warm::TunnelInterfaceStateChange(Warm::kInterfaceStateUp);
}

/**
 *  A TunnelAgent Platform API implementation used by the Tunnel Agent to announce the tunnel interface is disabled.
 *
 *  @param[in] tunIf    The InterfaceId for the tunnel Interface.  Not used in this implementation.
 *
 */
void TunnelInterfaceDown(InterfaceId tunIf)
{
    (void)tunIf;

    Warm::TunnelInterfaceStateChange(Warm::kInterfaceStateDown);
}

/**
 *  A TunnelAgent Platform API implementation used by the Tunnel Agent to announce a tunnel interface connection.
 *
 *  @param[in] tunIf    The InterfaceId for the tunnel Interface.  Not used in this implementation.
 *  @param[in] tunMode  The initial tunnel availability mode to be adopted by Warm.
 *
 */
void ServiceTunnelEstablished(InterfaceId tunIf, TunnelAvailabilityMode tunMode)
{
    (void)tunIf;

    Warm::TunnelServiceStateChange(Warm::kInterfaceStateUp, tunMode);
}

/**
 *  A TunnelAgent Platform API implementation used by the Tunnel Agent to announce a tunnel interface disconnection.
 *
 *  @param[in] tunIf    The InterfaceId for the tunnel Interface.  Not used in this implementation.
 *
 */
void ServiceTunnelDisconnected(InterfaceId tunIf)
{
    (void)tunIf;

    Warm::TunnelServiceStateChange(Warm::kInterfaceStateDown, Warm::sState.mTunnelRequestedAvailability);
}

/**
 *  A TunnelAgent Platform API implementation used by the Tunnel Agent to announce a Tunnel availability change.
 *
 *  @param[in] tunIf    The InterfaceId for the tunnel Interface.  Not used in this implementation.
 *  @param[in] tunMode  The new tunnel availability mode to be adopted by Warm.
 *
 */
void ServiceTunnelModeChange(InterfaceId tunIf, TunnelAvailabilityMode tunMode)
{
    (void)tunIf;

    Warm::TunnelPriorityStateChange(tunMode);
}

/**
 *  A TunnelAgent Platform API implementation used by the Tunnel Agent to enable Border Routing
 *  through Warm.
 *
 */
void EnableBorderRouting(void)
{
#if WARM_CONFIG_SUPPORT_BORDER_ROUTING
    Warm::BorderRouterStateChange(Warm::kInterfaceStateUp);
#endif // WARM_CONFIG_SUPPORT_BORDER_ROUTING
}

/**
 *  A TunnelAgent Platform API implementation used by the Tunnel Agent to disable Border Routing
 *  through Warm.
 *
 */
void DisableBorderRouting(void)
{
#if WARM_CONFIG_SUPPORT_BORDER_ROUTING
    Warm::BorderRouterStateChange(Warm::kInterfaceStateDown);
#endif // WARM_CONFIG_SUPPORT_BORDER_ROUTING
}

}; // namespace Platform

}; // namespace WeaveTunnel

}; // namespace Profiles

namespace Warm {

#endif // WARM_CONFIG_SUPPORT_WEAVE_TUNNEL

#if WARM_CONFIG_SUPPORT_BORDER_ROUTING

/**
 *  A WARM API called to announce a State change for the Border router feature.
 *
 *  @note
 *    Platform code should call this when the Border Routing feature transitions between active <-> deactive.
 *    If this call results in a change of State WARM will call RequestInvokeActions.
 *
 *  @param[in] inState    kInterfaceStateUp if the Border router feature is up, kInterfaceStateDown otherwise.
 *
 */
void BorderRouterStateChange(InterfaceState inState)
{
    SystemFeatureStateChangeHandler(kSystemFeatureTypeBorderRoutingEnabled, inState);
}

/**
 *  A static function that returns a mapping from TunnelAvailability to RoutePriority
 *
 *  @param[in] inAvailability         The Weave tunnel availability mode.
 *
 *  @return The priority mapped value.
 *
 */
static RoutePriority MapAvailabilityToPriority(Profiles::WeaveTunnel::Platform::TunnelAvailabilityMode inAvailability)
{
    RoutePriority retval = kRoutePriorityMedium;

    switch (inAvailability)
    {
        case Profiles::WeaveTunnel::Platform::kMode_Primary:
        case Profiles::WeaveTunnel::Platform::kMode_PrimaryAndBackup:
            retval = kRoutePriorityMedium;
            break;

        case Profiles::WeaveTunnel::Platform::kMode_BackupOnly:
            retval = kRoutePriorityLow;
            break;
    }

    return retval;
}

/**
 *  One of the Action methods. Sets the Thread Route for the Thread Stack.
 *
 *  @param[in] inAction         The action type.
 *  @param[in] inActivate       The desired State true == activate, false == deactivate.
 *  @param[in] inGlobalId       A reference to the Weave Global ID if it's necessary to calculate an address.
 *  @param[in] inInterfaceId    A reference to the device's interface ID if it's necessary to calculate an address.
 *
 *  @return Forwards the result from Platform::ThreadRoute().
 *
 */
static PlatformResult ThreadThreadRouteAction(ActionType inAction, bool inActivate, const uint64_t &inGlobalId, const uint64_t &inInterfaceId)
{
    Inet::IPPrefix prefix;

    MakePrefix(inGlobalId, 0, kGlobalULAPrefixLength, prefix);

    return Platform::AddRemoveThreadRoute(kInterfaceTypeThread, prefix, MapAvailabilityToPriority(sState.mTunnelRequestedAvailability), inActivate);

}

/**
 *  One of the Action methods. Sets the Thread Route Priority based on the Tunnel Availability.
 *
 *  @param[in] inAction         The action type.
 *  @param[in] inActivate       The desired State true == activate, false == deactivate.
 *  @param[in] inGlobalId       A reference to the Weave Global ID if it's necessary to calculate an address.
 *  @param[in] inInterfaceId    A reference to the device's interface ID if it's necessary to calculate an address.
 *
 *  @return Forwards the result from Platform::ThreadRoutePriority().
 *
 */
static PlatformResult ThreadRoutePriorityAction(ActionType inAction, bool inActivate, const uint64_t &inGlobalId, const uint64_t &inInterfaceId)
{
    Inet::IPPrefix prefix;

    MakePrefix(inGlobalId, 0, kGlobalULAPrefixLength, prefix);

    return Platform::SetThreadRoutePriority(kInterfaceTypeThread, prefix, MapAvailabilityToPriority(sState.mTunnelRequestedAvailability));

}

#endif // WARM_CONFIG_SUPPORT_BORDER_ROUTING

/**
 *  A static function that tests the State of each action and makes a platform API
 *  call to change the action State if necessary.
 *
 *  @brief
 *    This function uses ShouldPerformAction() to determine if an action state
 *    needs to be changed/taken.  If ShouldPerformAction() returns true the function
 *    will call the appropriate action API to perform the action in order to put
 *    it in the desired State.  The result of the action API call is passed
 *    into RecordPlatformResult() and if that function returns true, the execution
 *    of this function is terminated.
 *
 */
static void TakeActions(void)
{
    static const ActionEntry kActions[] =
    {
    // TODO: Order of operations could be important here.  If it is found that a specific order of operations must be
    // maintained, then this structure will need to be re-factored.  The current implementation has a single fixed order
    // which may not be adequate.
#if WARM_CONFIG_SUPPORT_WIFI
        {
            ( kSystemFeatureTypeIsFabricMember |
              kSystemFeatureTypeWiFiConnected ),
            kActionTypeWiFiHostAddress,
            WiFiHostAddressAction
        },
#endif // WARM_CONFIG_SUPPORT_WIFI
#if WARM_CONFIG_SUPPORT_THREAD
        {
            ( kSystemFeatureTypeIsFabricMember |
              kSystemFeatureTypeThreadConnected ),
            kActionTypeThreadHostAddress,
            ThreadHostAddressAction
        },
        {
            ( kSystemFeatureTypeIsFabricMember |
              kSystemFeatureTypeThreadConnected ),
            kActionTypeThreadThreadAddress,
            ThreadThreadAddressAction
        },
        {
            ( kSystemFeatureTypeIsFabricMember |
              kSystemFeatureTypeThreadConnected ),
            kActionTypeHostRouteThread,
            ThreadHostRouteAction
        },
#endif // WARM_CONFIG_SUPPORT_THREAD
#if WARM_CONFIG_SUPPORT_LEGACY6LOWPAN_NETWORK
        {
            ( kSystemFeatureTypeIsFabricMember |
              kSystemFeatureTypeThreadConnected ),
            kActionTypeLegacy6LoWPANHostAddress,
            LegacyHostAddressAction
        },
        {
            ( kSystemFeatureTypeIsFabricMember |
              kSystemFeatureTypeThreadConnected ),
            kActionTypeLegacy6LoWPANThreadAddress,
            LegacyThreadAddressAction
        },
#endif // WARM_CONFIG_SUPPORT_LEGACY6LOWPAN_NETWORK
#if WARM_CONFIG_SUPPORT_THREAD_ROUTING
        {
            ( kSystemFeatureTypeIsFabricMember |
              kSystemFeatureTypeThreadConnected |
              kSystemFeatureTypeThreadRoutingEnabled ),
            kActionTypeThreadAdvertisement,
            ThreadAdvertisementAction
        },
#endif // WARM_CONFIG_SUPPORT_THREAD_ROUTING
#if WARM_CONFIG_SUPPORT_BORDER_ROUTING
        {
            ( kSystemFeatureTypeIsFabricMember |
              kSystemFeatureTypeThreadConnected |
              kSystemFeatureTypeThreadRoutingEnabled |
              kSystemFeatureTypeTunnelInterfaceEnabled |
              kSystemFeatureTypeBorderRoutingEnabled |
              kSystemFeatureTypeTunnelState ),
            kActionTypeThreadRoute,
            ThreadThreadRouteAction
        },
        {
            ( kSystemFeatureTypeIsFabricMember |
              kSystemFeatureTypeThreadConnected |
              kSystemFeatureTypeThreadRoutingEnabled |
              kSystemFeatureTypeTunnelInterfaceEnabled |
              kSystemFeatureTypeBorderRoutingEnabled |
              kSystemFeatureTypeTunnelState ),
            kActionTypeThreadRoutePriority,
            ThreadRoutePriorityAction
        },
#endif // WARM_CONFIG_SUPPORT_BORDER_ROUTING
#if WARM_CONFIG_SUPPORT_WEAVE_TUNNEL
        {
            ( kSystemFeatureTypeIsFabricMember |
              kSystemFeatureTypeTunnelInterfaceEnabled ),
            kActionTypeTunnelHostAddress,
            TunnelHostAddressAction
        },
        {
            ( kSystemFeatureTypeIsFabricMember |
              kSystemFeatureTypeTunnelInterfaceEnabled |
              kSystemFeatureTypeTunnelState),
            kActionTypeTunnelHostRoute,
            TunnelHostRouteAction
        },
#endif // WARM_CONFIG_SUPPORT_WEAVE_TUNNEL
    };

    bool activate = false;
    PlatformResult platformResult;
    const uint64_t globalId = nl::Weave::WeaveFabricIdToIPv6GlobalId(sState.mFabricId);
    const uint64_t interfaceId = nl::Weave::WeaveNodeIdToIPv6InterfaceId(sState.mFabricState->LocalNodeId);
    ActionType theActionType;

    for (size_t i = 0 ; i < sizeof(kActions) / sizeof(kActions[0]) ; i++)
    {
        theActionType = kActions[i].mActionType;

        if (ShouldPerformAction(theActionType, kActions[i].mNecessaryActiveSystemFeatures, activate))
        {
            platformResult = kActions[i].mAction(theActionType, activate, globalId, interfaceId);

            if (kPlatformActionExecutionContinue != RecordPlatformResult(platformResult, theActionType, activate))
            {
                break;
            }
        }
    }
}

}; // namespace Warm

}; // namespace Weave

}; // namespace nl
