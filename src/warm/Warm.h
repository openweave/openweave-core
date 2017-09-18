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
 *      This file defines API's and platform abstractions
 *      for Nest Labs' Weave Address and Routing Module (WARM).
 *
 *      \page Weave Address Assignment
 *      \section intro Introduction:
 *        Because there are a number of inter-dependencies between interfaces and because
 *        different Device configurations can require different TCP/IP address and route
 *        assignment, it was deemed essential that the logic for controlling IP address
 *        and Route assignment be consolidated into a single module. WARM serves the purpose
 *        of properly adding and removing TCP/IP addresses and Routes to Weave related IP interfaces
 *        as those interfaces transition from active<->inactive.
 *
 *        WARM is intended to be configured at compile time via WarmProjectConfig.h and WarmConfig.h.
 *        WarmProjectConfig.h must accurately reflect the supported features of the device upon which
 *        WARM will execute.
 *
 *        WARM is a portable module that limits its dependency on how a TCP/IP stack and Thread Interface
 *        are configured. For this purpose WARM relies on a Set of Platform API's which
 *        must be implemented by the Platform Integrator.  Furthermore, the Platform Integrator is
 *        responsible for making the various nl::Warm API calls from appropriate execution points within
 *        the Platform code base.
 *
 *      \section theory Theory of Operation:
 *        The Platform code base will call nl::Warm API's to announce a change of State for related
 *        features such as the WiFi interface and Thread interface.  A call to any of these nl::Warm API's
 *        may result in a call by WARM to Platform::RequestInvokeActions().  Platform::RequestInvokeActions()
 *        must be implemented to perform the necessary operations that will call Warm::InvokeActions().  This
 *        process at first glance may appear un-necessarily indirect.  Why wouldn't WARM call InvokeActions
 *        directly?  The answer to that question, is to allow for any task in a multi-tasking system to call
 *        the nl::Warm State Change API's, and to provide a mechanism so that only a specific task will call
 *        the Platform:: API's.  After taking the Platform requirements into consideration, the Platform
 *        Integrator may choose to implement Platform::RequestInvokeActions() so that it posts an event to
 *        the appropriate task that will react by calling Warm::InvokeActions().  If, for a given platform, it is
 *        decided that no such multi-tasking concerns exist, Platform::RequestInvokeActions() can be implemented
 *        to call Warm::InvokeActions() directly.
 *
 *        When Warm::InvokeActions() is called the WARM logic will examine the current System State and
 *        make any necessary Platform:: API calls in order to bring the address and routing State in line
 *        with the System and Configuration State.  These calls are made in a pre-defined order and if any of these API's
 *        return kPlatformResultInProgress, execution of the ordered list will suspend and exit.  Furthermore, when
 *        one of these API's returns kPlatformResultInProgress, it is interpreted that the operation
 *        will complete asynchronously and that the WARM logic should wait for that operation to complete.
 *        Upon operation completion, the Platform code should call Warm::ReportActionComplete(), passing in a
 *        result of kPlatformResultSuccess or kPlatformResultFailure.  Upon receiving this call the WARM
 *        logic will again call Platform::RequestInvokeActions() in order to restart execution of the
 *        ordered action list.
 *
 *        In this way WARM does not require its own task but can instead rely on another task to call into Warm
 *        as appropriate.  Additionally, any task may call one or more of the System State change API's, thus
 *        simplifying integration.
 *
 */

#ifndef WARM_H_
#define WARM_H_

#include <Warm/WarmConfig.h>
#include <InetLayer/IPAddress.h>
#include <InetLayer/IPPrefix.h>
#include <Weave/Core/WeaveCore.h>

#if WARM_CONFIG_SUPPORT_WEAVE_TUNNEL
#include <Weave/Profiles/weave-tunneling/WeaveTunnelAgent.h>
#endif

using namespace nl::Weave;

/**
 *   @namespace nl::Weave::Warm
 *
 *   @brief
 *     This namespace includes interfaces for the Weave Address and Routing Module, a
 *     portable Module for configuring Weave IP addresses and Routes.
 */
namespace nl {
namespace Weave {
namespace Warm {

/**
 *  An enum of possible platform API return values.
 *
 */
typedef enum {
    kPlatformResultSuccess = 0,  // The API completed successfully.
    kPlatformResultFailure,      // The API execution failed.
    kPlatformResultInProgress    // The operation is in-progress and will complete asynchronously.
} PlatformResult;

/**
 *  An enum of possible interface types.
 *
 *  @note
 *    Do not change the elements in this enum as it is used as an index into arrays.
 *    Products will typically support a subset of these possible interfaces.
 *
 */
typedef enum {
    kInterfaceTypeLegacy6LoWPAN = 0,    // Thread alarm interface.
    kInterfaceTypeThread,               // Thread interface.
    kInterfaceTypeWiFi,                 // The WiFi interface.
    kInterfaceTypeTunnel,               // The Tunnel interface.
    kInterfaceTypeCellular,             // The Cellular interface.

    kInterfaceTypeMax
} InterfaceType;

/**
 *  An enum of possible route priorities so that one route can be given priority over another.
 *
 */
typedef enum {
    kRoutePriorityLow = 0,
    kRoutePriorityMedium,
    kRoutePriorityHigh
} RoutePriority;

/**
 *  An enum of possible Interface State values.
 *
 */
typedef enum {
    kInterfaceStateUp   = true,
    kInterfaceStateDown = false
} InterfaceState;

/**
 * @class WarmFabricStateDelegate
 *
 * @brief
 *    This is an internal class to WarmCore. It implements the FabricStateDelegate interface.
 *    An instance of this class (namely sWarmFabricStateDelegate), is set as the delegate of
 *    WeaveFabricState. Warm uses this to be notified of Fabric state changes.
 */
class WarmFabricStateDelegate
    : public FabricStateDelegate
{
public:
    /**
     *  This method is invoked by WeaveFabricState when joining/creating a new fabric.
     *
     *  @param[in] fabricState   The WeaveFabricState which is changed
     *  @param[in] newFabricId   The new fabric id
     *
     *  @return none.
     */
    virtual void DidJoinFabric(WeaveFabricState *fabricState, uint64_t newFabricId);

    /**
     *  This method is invoked by WeaveFabricState when leaving/clearing a fabric.
     *
     *  @param[in] fabricState   The WeaveFabricState which is changed
     *  @param[in] oldFabricId   The old/previous fabric id
     *
     *  @return none.
     */
    virtual void DidLeaveFabric(WeaveFabricState *fabricState, uint64_t oldFabricId);
};

/**
 *   @namespace nl::Warm::Platform
 *
 *   @brief
 *     This namespace includes all interfaces within Warm.  Functions in this
 *     namespace are to be implemented by platforms that use Warm,
 *     according to the needs/constraints of the particular environment.
 *
 */
namespace Platform {
    /**
     *  A platform API that Warm will call as part of nl::Warm::Init execution.
     *
     *  @note
     *    Any Platform specific initialization for Warm should be performed by this function.
     *    Initializing an object to support CriticalSectionEnter is one expected action of this API.
     *
     *  @param[in]  inFabricStateDelegate    A pointer to the fabricStateDelegate object used by Warm
     *                                       to receive updates for the fabric state.
     *
     *  @return WEAVE_NO_ERROR on success, error code otherwise.
     *
     */
    extern WEAVE_ERROR  Init(WarmFabricStateDelegate *inFabricStateDelegate);

    /**
     *  A platform API that Warm will call to protect access to internal State.
     *
     *  @note
     *    Compliments CriticalSectionExit.
     *    If all Warm execution occurs in a single Task context this API can be stubbed.
     *
     *  @return none.
     *
     */
    extern void CriticalSectionEnter(void);

    /**
     *  A platform API that Warm will call to release protected access to internal State.
     *
     *  @note
     *    Compliments CriticalSectionEnter.
     *    If all Warm execution occurs in a single Task context this API can be stubbed.
     *
     *  @return none.
     *
     */
    extern void CriticalSectionExit(void);

    /**
     *  A platform API that Warm will call to announce that the platform should call InvokeActions.
     *
     *  @note
     *    In order to allow Platform API's to be called from a dedicated task, Warm will call this
     *    API when it desires the Platform to call InvokeActions.  If no multi-task concerns exist,
     *    the Platform can call InvokeActions directly.
     *
     *  @return none.
     *
     */
    extern void RequestInvokeActions(void);

    /**
     *  A platform API that Warm will call to add / remove a host IP address to the specified interface on the Host TCP/IP stack.
     *
     *  @note
     *    This function must be able to acquire access to the specified interface.
     *
     *  @param[in]  inInterfaceType    The Interface to be modified.
     *
     *  @param[in]  inAddress          The IP address to be added/removed.
     *
     *  @param[in]  inPrefixLength     The Prefix length of the inAddress.
     *
     *  @param[in]  inAdd              true to add the address, false to remove the address.
     *
     *  @return kPlatformResultSuccess    - If the operation completed successfully.
     *  @return kPlatformResultFailure    - If the operation failed.
     *  @return kPlatformResultInProgress - If the operation will complete asynchronously.
     *                                      The platform must then call ReportActionComplete with the final result.
     *
     */
    extern PlatformResult AddRemoveHostAddress           (InterfaceType inInterfaceType, const Inet::IPAddress &inAddress, uint8_t inPrefixLength, bool inAdd);

    /**
     *  A platform API that Warm will call to add / remove a IP address to the specified interface on the Thread TCP/IP stack.
     *
     *  @note
     *    This function must be able to acquire access to the specified interface.
     *
     *  @param[in]  inInterfaceType    The Interface to be modified.
     *
     *  @param[in]  inAddress          The IP address to be added/removed.
     *
     *  @param[in]  inPrefixLength     The Prefix length of the inAddress.
     *
     *  @param[in]  inAdd              true to add the address, false to remove the address.
     *
     *  @return kPlatformResultSuccess    - If the operation completed successfully.
     *  @return kPlatformResultFailure    - If the operation failed.
     *  @return kPlatformResultInProgress - If the operation will complete asynchronously.
     *                                      The platform must then call ReportActionComplete with the final result.
     *
     */
    extern PlatformResult AddRemoveThreadAddress         (InterfaceType inInterfaceType, const Inet::IPAddress &inAddress, bool inAdd);

    /**
     *  A platform API that Warm will call to start / stop advertisement of a IP prefix on the Thread interface.
     *
     *  @note
     *    This function must be able to acquire access to the specified interface.
     *
     *  @param[in]  inInterfaceType    The Interface to be modified.
     *
     *  @param[in]  inPrefix           The IP prefix for which advertising should be started / stopped.
     *
     *  @param[in]  inStart            true to start advertising, false to stop advertising.
     *
     *  @return kPlatformResultSuccess    - If the operation completed successfully.
     *  @return kPlatformResultFailure    - If the operation failed.
     *  @return kPlatformResultInProgress - If the operation will complete asynchronously.
     *                                      The platform must then call ReportActionComplete with the final result.
     *
     */
    extern PlatformResult StartStopThreadAdvertisement   (InterfaceType inInterfaceType, const Inet::IPPrefix &inPrefix, bool inStart);

    /**
     *  A platform API that Warm will call to add / remove a IP route for the specified interface on the host TCP/IP stack.
     *
     *  @note
     *    This function must be able to acquire access to the specified interface.
     *
     *  @param[in]  inInterfaceType    The Interface to be modified.
     *
     *  @param[in]  inPrefix           The IP prefix to add / remove.
     *
     *  @param[in]  inPriority         The priority to use when the route is assigned.
     *
     *  @param[in]  inAdd              true to add the prefix as a route, false to remove the prefix as a route.
     *
     *  @return kPlatformResultSuccess    - If the operation completed successfully.
     *  @return kPlatformResultFailure    - If the operation failed.
     *  @return kPlatformResultInProgress - If the operation will complete asynchronously.
     *                                      The platform must then call ReportActionComplete with the final result.
     *
     */
    extern PlatformResult AddRemoveHostRoute             (InterfaceType inInterfaceType, const Inet::IPPrefix &inPrefix, RoutePriority inPriority, bool inAdd);

    /**
     *  A platform API that Warm will call to add / remove a IP route for the specified interface on the Thread TCP/IP stack.
     *
     *  @note
     *    This function must be able to acquire access to the specified interface.
     *
     *  @param[in]  inInterfaceType    The Interface to be modified.
     *
     *  @param[in]  inPrefix           The IP prefix to assign / remove.
     *
     *  @param[in]  inPriority         The priority to use when the route is assigned.
     *
     *  @param[in]  inAdd              true to add the prefix as a route, false to remove the prefix as a route.
     *
     *  @return kPlatformResultSuccess    - If the operation completed successfully.
     *  @return kPlatformResultFailure    - If the operation failed.
     *  @return kPlatformResultInProgress - If the operation will complete asynchronously.
     *                                      The platform must then call ReportActionComplete with the final result.
     */
    extern PlatformResult AddRemoveThreadRoute           (InterfaceType inInterfaceType, const Inet::IPPrefix &inPrefix, RoutePriority inPriority, bool inAdd);

    /**
     *  A platform API that Warm will call to change the priority of an existing IP route for the specified interface on the Thread TCP/IP stack.
     *
     *  @note
     *    This function must be able to acquire access to the specified interface.
     *
     *  @param[in]  inInterfaceType    The Interface to be modified.
     *
     *  @param[in]  inPrefix           The IP prefix to modify.
     *
     *  @param[in]  inPriority         The new priority to apply to the route.
     *
     *  @return kPlatformResultSuccess    - If the operation completed successfully.
     *  @return kPlatformResultFailure    - If the operation failed.
     *  @return kPlatformResultInProgress - If the operation will complete asynchronously.
     *                                      The platform must then call ReportActionComplete with the final result.
     *
     */
    extern PlatformResult SetThreadRoutePriority   (InterfaceType inInterfaceType, const Inet::IPPrefix &inPrefix, RoutePriority inPriority);

}; // namespace Platform

/**
 *  A WARM API to perform one time module initialization.
 *
 *  @note
 *    It must be called prior to any other nl::Warm API calls.
 *
 *  @param[in]  inFabricState    A reference to a valid WeaveFabricState.
 *
 *  @return WEAVE_NO_ERROR on success, error code otherwise.
 *
 */
WEAVE_ERROR Init(WeaveFabricState &inFabricState);

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
 *    directly.  See this documentation \ref warmpage1 for more information.
 *
 *  @return none.
 *
 */
void InvokeActions(void);

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
 *  @return none.
 *
 */
void ReportActionComplete(PlatformResult inResult);

/**
 *  A WARM API called to announce a State change for the WiFi interface.
 *
 *  @note
 *    Platform code should call this function when the WiFi interface transitions between up <-> down.
 *    If this call results in a change of State WARM will call RequestInvokeActions.
 *
 *  @param[in] inState    kInterfaceStateUp if the WiFi interface is up, kInterfaceStateDown otherwise.
 *
 *  @return none.
 *
 */
void WiFiInterfaceStateChange(InterfaceState inState);

/**
 *  A WARM API called to announce a State change for the Thread interface.
 *
 *  @note
 *    Platform code should call this function when the Thread interface transitions between up <-> down.
 *    If this call results in a change of State WARM will call RequestInvokeActions.
 *
 *  @param[in] inState    kInterfaceStateUp if the Thread interface is up, kInterfaceStateDown otherwise.
 *
 *  @return none.
 *
 */
void ThreadInterfaceStateChange(InterfaceState inState);

/**
 *  A WARM API called to announce a State change for the Cellular interface.
 *
 *  @note
 *    Platform code should call this function when the Cellular interface transitions between up <-> down.
 *    If this call results in a change of State WARM will call RequestInvokeActions.
 *
 *  @param[in] inState    kInterfaceStateUp if the Cellular interface is up, kInterfaceStateDown otherwise.
 *
 *  @return none.
 *
 */
void CellularInterfaceStateChange(InterfaceState inState);

/**
 *  A WARM API called to announce a State change for the Thread Routing feature.
 *
 *  @note
 *    Platform code should call this function when the ThreadRouting feature transitions between active <-> deactive.
 *    If this call results in a change of State WARM will call RequestInvokeActions.
 *
 *  @param[in] inState    kInterfaceStateUp if the Thread routing feature is up, kInterfaceStateDown otherwise.
 *
 *  @return none.
 *
 */
void ThreadRoutingStateChange(InterfaceState inState);

/**
 *  A WARM API called to announce a State change for the Border router feature.
 *
 *  @note
 *    Platform code should call this when the Border Routing feature transitions between active <-> deactive.
 *    If this call results in a change of State WARM will call RequestInvokeActions.
 *
 *  @param[in] inState    kInterfaceStateUp if the Border router feature is up, kInterfaceStateDown otherwise.
 *
 *  @return none.
 *
 */
void BorderRouterStateChange(InterfaceState inState);

/**
 *  A WARM API to acquire a ULA for a specified interface type.
 *
 *  @note
 *    Platform code should call this only after WARM has been initialized. Calling this API
 *    prior to initialization will result in an error.
 *
 *  @param[in] inInterfaceType    The type of interface for which a ULA is desired.
 *
 *  @param[out] outAddress        An address object used to hold the resulting ULA.
 *
 *  @return WEAVE_NO_ERROR               - On success.
 *          WEAVE_ERROR_INCORRECT_STATE  - If this API is called while WARM is
 *                                         not a member of a Fabric.
 *          WEAVE_ERROR_INVALID_ARGUMENT - If this API is called with an invalid Interface Type.
 *
 */
WEAVE_ERROR GetULA(InterfaceType inInterfaceType, Inet::IPAddress &outAddress);

/**
 *  A WARM API to acquire the FabricState object that was provided to Warm during Init.
 *
 *  @note
 *    Platform code should call this only after WARM has been initialized. Calling this API
 *    prior to initialization will result in an error.
 *
 *  @param[out] outFabricState        A pointer reference to a fabricState object.
 *
 *  @return WEAVE_NO_ERROR               - On success.
 *          WEAVE_ERROR_INCORRECT_STATE  - If this API is called before WARM
 *                                         has been initialized.
 *
 */
WEAVE_ERROR GetFabricState(const WeaveFabricState *&outFabricState);

} /* namespace Warm */
} /* namespace Weave */
} /* namespace nl */

#endif /* WARM_H_ */
