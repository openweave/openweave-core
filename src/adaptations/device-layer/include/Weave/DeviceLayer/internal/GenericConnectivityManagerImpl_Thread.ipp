/*
 *
 *    Copyright (c) 2019 Nest Labs, Inc.
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
 *          Provides an generic implementation of ConnectivityManager features
 *          for use on platforms that support Thread.
 */

#ifndef GENERIC_CONNECTIVITY_MANAGER_IMPL_THREAD_IPP
#define GENERIC_CONNECTIVITY_MANAGER_IMPL_THREAD_IPP

#include <Weave/DeviceLayer/internal/WeaveDeviceLayerInternal.h>
#include <Weave/DeviceLayer/internal/GenericConnectivityManagerImpl_Thread.h>
#include <Warm/Warm.h>

namespace nl {
namespace Weave {
namespace DeviceLayer {
namespace Internal {

// Fully instantiate the generic implementation class in whatever compilation unit includes this file.
template class GenericConnectivityManagerImpl_Thread<ConnectivityManagerImpl>;

template<class ImplClass>
void GenericConnectivityManagerImpl_Thread<ImplClass>::_OnPlatformEvent(const WeaveDeviceEvent * event)
{
    // If there is a change to the Thread state...
    if (event->Type == DeviceEventType::kThreadConnectivityChange)
    {
        // If the state of the Thread interface has changed, notify WARM accordingly.
        if (event->ThreadConnectivityChange.Result != kConnectivity_NoChange)
        {
            Warm::ThreadInterfaceStateChange(event->ThreadConnectivityChange.Result == kConnectivity_Established
                    ? Warm::kInterfaceStateUp : Warm::kInterfaceStateDown);
        }

        // If the local Thread addresses have changed OR if the Thread network data has changed,
        // determine if there's been a change in service connectivity via Thread.
        if (event->ThreadConnectivityChange.AddressChanged || event->ThreadConnectivityChange.NetDataChanged)
        {
            UpdateServiceConnectivity();
        }

        if (event->ThreadConnectivityChange.AddressChanged)
        {
            WEAVE_ERROR err = MessageLayer.RefreshEndpoints();
            if (err != WEAVE_NO_ERROR)
            {
                WeaveLogDetail(DeviceLayer, "MessageLayer.RefreshEndpoints() failed: %s", nl::ErrorStr(err));
            }
        }
    }

    // If there is a change to the device's membership in a fabric, re-evaluate service connectivity via Thread.
    else if (event->Type == DeviceEventType::kFabricMembershipChange)
    {
        UpdateServiceConnectivity();
    }
}

template<class ImplClass>
void GenericConnectivityManagerImpl_Thread<ImplClass>::UpdateServiceConnectivity(void)
{
    bool haveServiceConnectivity = false;

    // Evaluate whether there is connectivity to the Weave service subnet via the Thread network.
    //
    // If the device is a member of a fabric, then service connectivity is assessed by checking if the
    // local Thread stack has a route to the Weave service subnet.  This route will typically be a /48
    // Weave ULA route that has been advertised by one or more Weave border router devices in the Thread
    // network.  If no such route exists, then it is likely that there are no functioning Weave border
    // routers, and thus no route to the service via Thread.
    //
    // If the device is NOT a member of a fabric, then there can be no Weave service connectivity via Thread.
    //
    if (FabricState.FabricId != kFabricIdNotSpecified)
    {
        const uint64_t fabricGlobalId = WeaveFabricIdToIPv6GlobalId(FabricState.FabricId);
        IPAddress serviceAddr = IPAddress::MakeULA(fabricGlobalId, nl::Weave::kWeaveSubnetId_Service, 1);
        haveServiceConnectivity = ThreadStackMgr().HaveRouteToAddress(serviceAddr);
        WeaveLogDetail(DeviceLayer, "haveServiceConnectivity = %s", haveServiceConnectivity ? "true" : "false");
    }

    // If service connectivity via Thread has changed, post an event signaling the change.
    if (GetFlag(mFlags, kFlag_HaveServiceConnectivity) != haveServiceConnectivity)
    {
        WeaveLogProgress(DeviceLayer, "ConnectivityManager: Service connectivity via Thread %s", (haveServiceConnectivity) ? "established" : "lost");

        SetFlag(mFlags, kFlag_HaveServiceConnectivity, haveServiceConnectivity);

        {
            WeaveDeviceEvent event;
            event.Clear();
            event.Type = DeviceEventType::kServiceConnectivityChange;
            event.ServiceConnectivityChange.ViaThread.Result = (haveServiceConnectivity) ? kConnectivity_Established : kConnectivity_Lost;
            event.ServiceConnectivityChange.ViaTunnel.Result = kConnectivity_NoChange;
            event.ServiceConnectivityChange.Result = (Impl()->HaveServiceConnectivityViaTunnel())
                    ? kConnectivity_NoChange
                    : event.ServiceConnectivityChange.ViaThread.Result;
            PlatformMgr().PostEvent(&event);
        }
    }
}


} // namespace Internal
} // namespace DeviceLayer
} // namespace Weave
} // namespace nl

#endif // GENERIC_CONNECTIVITY_MANAGER_IMPL_THREAD_IPP
