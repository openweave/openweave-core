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
 *          Contains non-inline method definitions for the
 *          GenericThreadStackManagerImpl_OpenThread<> template.
 */

#ifndef GENERIC_THREAD_STACK_MANAGER_IMPL_OPENTHREAD_IPP
#define GENERIC_THREAD_STACK_MANAGER_IMPL_OPENTHREAD_IPP

#include <Weave/DeviceLayer/internal/WeaveDeviceLayerInternal.h>
#include <Weave/DeviceLayer/ThreadStackManager.h>
#include <Weave/DeviceLayer/OpenThread/GenericThreadStackManagerImpl_OpenThread.h>
#include <Weave/DeviceLayer/OpenThread/OpenThreadUtils.h>
#include <Weave/Profiles/network-provisioning/NetworkProvisioning.h>
#include <Weave/DeviceLayer/internal/DeviceNetworkInfo.h>
#include <Weave/Support/crypto/WeaveRNG.h>

#include <openthread/thread.h>
#include <openthread/tasklet.h>
#include <openthread/link.h>
#include <openthread/dataset.h>
#include <openthread/dataset_ftd.h>

using namespace ::nl::Weave::Profiles::NetworkProvisioning;

extern "C" void otSysProcessDrivers(otInstance *aInstance);

namespace nl {
namespace Weave {
namespace DeviceLayer {
namespace Internal {

// Assert some presumptions in this code
static_assert(DeviceNetworkInfo::kMaxThreadNetworkNameLength == OT_NETWORK_NAME_MAX_SIZE);
static_assert(DeviceNetworkInfo::kThreadExtendedPANIdLength == OT_EXT_PAN_ID_SIZE);
static_assert(DeviceNetworkInfo::kThreadMeshPrefixLength == OT_MESH_LOCAL_PREFIX_SIZE);
static_assert(DeviceNetworkInfo::kThreadNetworkKeyLength == OT_MASTER_KEY_SIZE);
static_assert(DeviceNetworkInfo::kThreadPSKcLength == OT_PSKC_MAX_SIZE);

// Fully instantiate the generic implementation class in whatever compilation unit includes this file.
template class GenericThreadStackManagerImpl_OpenThread<ThreadStackManagerImpl>;

/**
 * Called by OpenThread to alert the ThreadStackManager of a change in the state of the Thread stack.
 *
 * By default, applications never need to call this method directly.  However, applications that
 * wish to receive OpenThread state change call-backs directly from OpenThread (e.g. by calling
 * otSetStateChangedCallback() with their own callback function) can call this method to pass
 * state change events to the ThreadStackManager.
 */
template<class ImplClass>
void GenericThreadStackManagerImpl_OpenThread<ImplClass>::OnOpenThreadStateChange(uint32_t flags, void * context)
{
    WeaveDeviceEvent event;
    event.Type = DeviceEventType::kThreadStateChange;
    event.ThreadStateChange.RoleChanged = (flags & OT_CHANGED_THREAD_ROLE) != 0;
    event.ThreadStateChange.AddressChanged = (flags & (OT_CHANGED_IP6_ADDRESS_ADDED|OT_CHANGED_IP6_ADDRESS_REMOVED)) != 0;
    event.ThreadStateChange.NetDataChanged = (flags & OT_CHANGED_THREAD_NETDATA) != 0;
    event.ThreadStateChange.ChildNodesChanged = (flags & (OT_CHANGED_THREAD_CHILD_ADDED|OT_CHANGED_THREAD_CHILD_REMOVED)) != 0;
    event.ThreadStateChange.OpenThread.Flags = flags;
    PlatformMgr().PostEvent(&event);
}

template<class ImplClass>
void GenericThreadStackManagerImpl_OpenThread<ImplClass>::_ProcessThreadActivity(void)
{
    otTaskletsProcess(mOTInst);
    otSysProcessDrivers(mOTInst);
}

template<class ImplClass>
bool GenericThreadStackManagerImpl_OpenThread<ImplClass>::_HaveRouteToAddress(const IPAddress & destAddr)
{
    bool res = false;

    // Lock OpenThread
    Impl()->LockThreadStack();

    // No routing of IPv4 over Thread.
    VerifyOrExit(!destAddr.IsIPv4(), res = false);

    // If the device is attached to a Thread network...
    if (IsThreadAttachedNoLock())
    {
        // Link-local addresses are always presumed to be routable, provided the device is attached.
        if (destAddr.IsIPv6LinkLocal())
        {
            ExitNow(res = true);
        }

        // Iterate over the routes known to the OpenThread stack looking for a route that covers the
        // destination address.  If found, consider the address routable.
        // Ignore any routes advertised by this device.
        // If the destination address is a ULA, ignore default routes. Border routers advertising
        // default routes are not expected to be capable of routing Weave fabric ULAs unless they
        // advertise those routes specifically.
        {
            otError otErr;
            otNetworkDataIterator routeIter = OT_NETWORK_DATA_ITERATOR_INIT;
            otExternalRouteConfig routeConfig;
            const bool destIsULA = destAddr.IsIPv6ULA();

            while ((otErr = otNetDataGetNextRoute(Impl()->OTInstance(), &routeIter, &routeConfig)) == OT_ERROR_NONE)
            {
                const IPPrefix prefix = ToIPPrefix(routeConfig.mPrefix);
                char addrStr[64];
                prefix.IPAddr.ToString(addrStr, sizeof(addrStr));
                if (!routeConfig.mNextHopIsThisDevice &&
                    (!destIsULA || routeConfig.mPrefix.mLength > 0) &&
                    ToIPPrefix(routeConfig.mPrefix).MatchAddress(destAddr))
                {
                    ExitNow(res = true);
                }
            }
        }
    }

exit:

    // Unlock OpenThread
    Impl()->UnlockThreadStack();

    return res;
}

template<class ImplClass>
void GenericThreadStackManagerImpl_OpenThread<ImplClass>::_OnPlatformEvent(const WeaveDeviceEvent * event)
{
    if (event->Type == DeviceEventType::kThreadStateChange)
    {
#if WEAVE_DETAIL_LOGGING

        Impl()->LockThreadStack();

        LogOpenThreadStateChange(mOTInst, event->ThreadStateChange.OpenThread.Flags);

        Impl()->UnlockThreadStack();

#endif // WEAVE_DETAIL_LOGGING
    }
}

template<class ImplClass>
bool GenericThreadStackManagerImpl_OpenThread<ImplClass>::_IsThreadEnabled(void)
{
    otDeviceRole curRole;

    Impl()->LockThreadStack();
    curRole = otThreadGetDeviceRole(mOTInst);
    Impl()->UnlockThreadStack();

    return (curRole != OT_DEVICE_ROLE_DISABLED);
}

template<class ImplClass>
WEAVE_ERROR GenericThreadStackManagerImpl_OpenThread<ImplClass>::_SetThreadEnabled(bool val)
{
    otError otErr = OT_ERROR_NONE;

    Impl()->LockThreadStack();

    bool isEnabled = (otThreadGetDeviceRole(mOTInst) != OT_DEVICE_ROLE_DISABLED);
    if (val != isEnabled)
    {
        otErr = otThreadSetEnabled(mOTInst, val);
    }

    Impl()->UnlockThreadStack();

    return MapOpenThreadError(otErr);
}

template<class ImplClass>
bool GenericThreadStackManagerImpl_OpenThread<ImplClass>::_IsThreadProvisioned(void)
{
    bool provisioned;

    Impl()->LockThreadStack();
    provisioned = otDatasetIsCommissioned(mOTInst);
    Impl()->UnlockThreadStack();

    return provisioned;
}

template<class ImplClass>
bool GenericThreadStackManagerImpl_OpenThread<ImplClass>::_IsThreadAttached(void)
{
    otDeviceRole curRole;

    Impl()->LockThreadStack();
    curRole = otThreadGetDeviceRole(mOTInst);
    Impl()->UnlockThreadStack();

    return (curRole != OT_DEVICE_ROLE_DISABLED && curRole != OT_DEVICE_ROLE_DETACHED);
}

template<class ImplClass>
WEAVE_ERROR GenericThreadStackManagerImpl_OpenThread<ImplClass>::_GetThreadProvision(DeviceNetworkInfo & netInfo, bool includeCredentials)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    otOperationalDataset activeDataset;

    netInfo.Reset();

    Impl()->LockThreadStack();

    if (otDatasetIsCommissioned(mOTInst))
    {
        otError otErr = otDatasetGetActive(mOTInst, &activeDataset);
        err = MapOpenThreadError(otErr);
    }
    else
    {
        err = WEAVE_ERROR_INCORRECT_STATE;
    }

    Impl()->UnlockThreadStack();

    SuccessOrExit(err);

    netInfo.NetworkType = kNetworkType_Thread;
    netInfo.NetworkId = kThreadNetworkId;
    netInfo.FieldPresent.NetworkId = true;
    if (activeDataset.mComponents.mIsNetworkNamePresent)
    {
        strncpy(netInfo.ThreadNetworkName, (const char *)activeDataset.mNetworkName.m8, sizeof(netInfo.ThreadNetworkName));
    }
    if (activeDataset.mComponents.mIsExtendedPanIdPresent)
    {
        memcpy(netInfo.ThreadExtendedPANId, activeDataset.mExtendedPanId.m8, sizeof(netInfo.ThreadExtendedPANId));
        netInfo.FieldPresent.ThreadExtendedPANId = true;
    }
    if (activeDataset.mComponents.mIsMeshLocalPrefixPresent)
    {
        memcpy(netInfo.ThreadMeshPrefix, activeDataset.mMeshLocalPrefix.m8, sizeof(netInfo.ThreadMeshPrefix));
        netInfo.FieldPresent.ThreadMeshPrefix = true;
    }
    if (includeCredentials)
    {
        if (activeDataset.mComponents.mIsMasterKeyPresent)
        {
            memcpy(netInfo.ThreadNetworkKey, activeDataset.mMasterKey.m8, sizeof(netInfo.ThreadNetworkKey));
            netInfo.FieldPresent.ThreadNetworkKey = true;
        }
        if (activeDataset.mComponents.mIsPSKcPresent)
        {
            memcpy(netInfo.ThreadPSKc, activeDataset.mPSKc.m8, sizeof(netInfo.ThreadPSKc));
            netInfo.FieldPresent.ThreadPSKc = true;
        }
    }
    if (activeDataset.mComponents.mIsPanIdPresent)
    {
        netInfo.ThreadPANId = activeDataset.mPanId;
    }
    if (activeDataset.mComponents.mIsChannelPresent)
    {
        netInfo.ThreadChannel = activeDataset.mChannel;
    }

exit:
    return err;
}

template<class ImplClass>
WEAVE_ERROR GenericThreadStackManagerImpl_OpenThread<ImplClass>::_SetThreadProvision(const DeviceNetworkInfo & netInfo)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    otError otErr;
    otOperationalDataset newDataset;

    // Form a Thread operational dataset from the given network parameters.
    memset(&newDataset, 0, sizeof(newDataset));
    newDataset.mComponents.mIsActiveTimestampPresent = true;
    newDataset.mComponents.mIsPendingTimestampPresent = true;
    if (netInfo.ThreadNetworkName[0] != 0)
    {
        strncpy((char *)newDataset.mNetworkName.m8, netInfo.ThreadNetworkName, sizeof(newDataset.mNetworkName.m8));
        newDataset.mComponents.mIsNetworkNamePresent = true;
    }
    if (netInfo.FieldPresent.ThreadExtendedPANId)
    {
        memcpy(newDataset.mExtendedPanId.m8, netInfo.ThreadExtendedPANId, sizeof(newDataset.mExtendedPanId.m8));
        newDataset.mComponents.mIsExtendedPanIdPresent = true;
    }
    if (netInfo.FieldPresent.ThreadMeshPrefix)
    {
        memcpy(newDataset.mMeshLocalPrefix.m8, netInfo.ThreadMeshPrefix, sizeof(newDataset.mMeshLocalPrefix.m8));
        newDataset.mComponents.mIsMeshLocalPrefixPresent = true;
    }
    if (netInfo.FieldPresent.ThreadNetworkKey)
    {
        memcpy(newDataset.mMasterKey.m8, netInfo.ThreadNetworkKey, sizeof(newDataset.mMasterKey.m8));
        newDataset.mComponents.mIsMasterKeyPresent = true;
    }
    if (netInfo.FieldPresent.ThreadPSKc)
    {
        memcpy(newDataset.mPSKc.m8, netInfo.ThreadPSKc, sizeof(newDataset.mPSKc.m8));
        newDataset.mComponents.mIsPSKcPresent = true;
    }
    if (netInfo.ThreadPANId != kThreadPANId_NotSpecified)
    {
        newDataset.mPanId = netInfo.ThreadPANId;
        newDataset.mComponents.mIsPanIdPresent = true;
    }
    if (netInfo.ThreadChannel != kThreadChannel_NotSpecified)
    {
        newDataset.mChannel = netInfo.ThreadChannel;
        newDataset.mComponents.mIsChannelPresent = true;
    }

    // Set the dataset as the active dataset for the node.
    Impl()->LockThreadStack();
    otErr = otDatasetSetActive(mOTInst, &newDataset);
    Impl()->UnlockThreadStack();

    VerifyOrExit(otErr == OT_ERROR_NONE, err = MapOpenThreadError(otErr));

exit:
    return err;
}

template<class ImplClass>
void GenericThreadStackManagerImpl_OpenThread<ImplClass>::_ClearThreadProvision(void)
{
    Impl()->LockThreadStack();
    otThreadSetEnabled(mOTInst, false);
    otInstanceErasePersistentInfo(mOTInst);
    Impl()->UnlockThreadStack();
}

template<class ImplClass>
bool GenericThreadStackManagerImpl_OpenThread<ImplClass>::_HaveMeshConnectivity(void)
{
    bool res;
    otDeviceRole curRole;

    Impl()->LockThreadStack();

    // Get the current Thread role.
    curRole = otThreadGetDeviceRole(mOTInst);

    // If Thread is disabled, or the node is detached, then the node has no mesh connectivity.
    if (curRole == OT_DEVICE_ROLE_DISABLED || curRole == OT_DEVICE_ROLE_DETACHED)
    {
        res = false;
    }

    // If the node is a child, that implies the existence of a parent node which provides connectivity
    // to the mesh.
    else if (curRole == OT_DEVICE_ROLE_CHILD)
    {
        res = true;
    }

    // Otherwise, if the node is acting as a router, scan the Thread neighbor table looking for at least
    // one other node that is also acting as router.
    else
    {
        otNeighborInfoIterator neighborIter = OT_NEIGHBOR_INFO_ITERATOR_INIT;
        otNeighborInfo neighborInfo;

        res = false;

        while (otThreadGetNextNeighborInfo(mOTInst, &neighborIter, &neighborInfo) == OT_ERROR_NONE)
        {
            if (!neighborInfo.mIsChild)
            {
                res = true;
                break;
            }
        }
    }

    Impl()->UnlockThreadStack();

    return res;
}

template<class ImplClass>
WEAVE_ERROR GenericThreadStackManagerImpl_OpenThread<ImplClass>::DoInit(otInstance * otInst)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    otError otErr;

    // Arrange for OpenThread errors to be translated to text.
    RegisterOpenThreadErrorFormatter();

    mOTInst = NULL;

    // If an OpenThread instance hasn't been supplied, call otInstanceInitSingle() to
    // create or acquire a singleton instance of OpenThread.
    if (otInst == NULL)
    {
        otInst = otInstanceInitSingle();
        VerifyOrExit(otInst != NULL, err = MapOpenThreadError(OT_ERROR_FAILED));
    }

    mOTInst = otInst;

    // Arrange for OpenThread to call the OnOpenThreadStateChange method whenever a
    // state change occurs.  Note that we reference the OnOpenThreadStateChange method
    // on the concrete implementation class so that that class can override the default
    // method implementation if it chooses to.
    otErr = otSetStateChangedCallback(otInst, ImplClass::OnOpenThreadStateChange, NULL);
    VerifyOrExit(otErr == OT_ERROR_NONE, err = MapOpenThreadError(otErr));

    // TODO: generalize this
    {
        otLinkModeConfig linkMode;

        memset(&linkMode, 0, sizeof(linkMode));
        linkMode.mRxOnWhenIdle       = true;
        linkMode.mSecureDataRequests = true;
        linkMode.mDeviceType         = true;
        linkMode.mNetworkData        = true;

        otErr = otThreadSetLinkMode(otInst, linkMode);
        VerifyOrExit(otErr == OT_ERROR_NONE, err = MapOpenThreadError(otErr));
    }

    // TODO: not supported in old version of OpenThread used by Nordic SDK.
    // otIp6SetSlaacEnabled(otInst, false);

    otErr = otIp6SetEnabled(otInst, true);
    VerifyOrExit(otErr == OT_ERROR_NONE, err = MapOpenThreadError(otErr));

    if (otThreadGetDeviceRole(mOTInst) == OT_DEVICE_ROLE_DISABLED && otDatasetIsCommissioned(otInst))
    {
        otErr = otThreadSetEnabled(otInst, true);
        VerifyOrExit(otErr == OT_ERROR_NONE, err = MapOpenThreadError(otErr));
    }

exit:
    return err;
}

template<class ImplClass>
bool GenericThreadStackManagerImpl_OpenThread<ImplClass>::IsThreadAttachedNoLock(void)
{
    otDeviceRole curRole = otThreadGetDeviceRole(mOTInst);
    return (curRole != OT_DEVICE_ROLE_DISABLED && curRole != OT_DEVICE_ROLE_DETACHED);
}


} // namespace Internal
} // namespace DeviceLayer
} // namespace Weave
} // namespace nl


#endif // GENERIC_THREAD_STACK_MANAGER_IMPL_OPENTHREAD_IPP
