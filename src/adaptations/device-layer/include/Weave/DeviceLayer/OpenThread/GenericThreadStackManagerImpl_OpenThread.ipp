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
#include <Weave/Support/TraitEventUtils.h>
#include <nest/trait/network/TelemetryNetworkWpanTrait.h>

#include <openthread/thread.h>
#include <openthread/tasklet.h>
#include <openthread/link.h>
#include <openthread/dataset.h>
#include <openthread/dataset_ftd.h>
#include <openthread/thread_ftd.h>

using namespace ::nl::Weave::Profiles::NetworkProvisioning;
using namespace Schema::Nest::Trait::Network;

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
WEAVE_ERROR GenericThreadStackManagerImpl_OpenThread<ImplClass>::_GetAndLogThreadStatsCounters(void)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    otError otErr;
    nl::Weave::Profiles::DataManagement_Current::event_id_t eventId;
    Schema::Nest::Trait::Network::TelemetryNetworkWpanTrait::NetworkWpanStatsEvent counterEvent = { 0 };
    const otMacCounters *macCounters;
    const otIpCounters *ipCounters;
    otOperationalDataset activeDataset;
    otDeviceRole role;

    Impl()->LockThreadStack();

    // Get Mac Counters
    macCounters = otLinkGetCounters(mOTInst);

    // Rx Counters
    counterEvent.phyRx                = macCounters->mRxTotal;
    counterEvent.macUnicastRx         = macCounters->mRxUnicast;
    counterEvent.macBroadcastRx       = macCounters->mRxBroadcast;
    counterEvent.macRxData            = macCounters->mRxData;
    counterEvent.macRxDataPoll        = macCounters->mRxDataPoll;
    counterEvent.macRxBeacon          = macCounters->mRxBeacon;
    counterEvent.macRxBeaconReq       = macCounters->mRxBeaconRequest;
    counterEvent.macRxOtherPkt        = macCounters->mRxOther;
    counterEvent.macRxFilterWhitelist = macCounters->mRxAddressFiltered;
    counterEvent.macRxFilterDestAddr  = macCounters->mRxDestAddrFiltered;

    // Tx Counters
    counterEvent.phyTx          = macCounters->mTxTotal;
    counterEvent.macUnicastTx   = macCounters->mTxUnicast;
    counterEvent.macBroadcastTx = macCounters->mTxBroadcast;
    counterEvent.macTxAckReq    = macCounters->mTxAckRequested;
    counterEvent.macTxNoAckReq  = macCounters->mTxNoAckRequested;
    counterEvent.macTxAcked     = macCounters->mTxAcked;
    counterEvent.macTxData      = macCounters->mTxData;
    counterEvent.macTxDataPoll  = macCounters->mTxDataPoll;
    counterEvent.macTxBeacon    = macCounters->mTxBeacon;
    counterEvent.macTxBeaconReq = macCounters->mTxBeaconRequest;
    counterEvent.macTxOtherPkt  = macCounters->mTxOther;
    counterEvent.macTxRetry     = macCounters->mTxRetry;

    // Tx Error Counters
    counterEvent.macTxFailCca = macCounters->mTxErrCca;

    // Rx Error Counters
    counterEvent.macRxFailDecrypt         = macCounters->mRxErrSec;
    counterEvent.macRxFailNoFrame         = macCounters->mRxErrNoFrame;
    counterEvent.macRxFailUnknownNeighbor = macCounters->mRxErrUnknownNeighbor;
    counterEvent.macRxFailInvalidSrcAddr  = macCounters->mRxErrInvalidSrcAddr;
    counterEvent.macRxFailFcs             = macCounters->mRxErrFcs;
    counterEvent.macRxFailOther           = macCounters->mRxErrOther;

    // Get Ip Counters
    ipCounters = otThreadGetIp6Counters(mOTInst);

    // Ip Counters
    counterEvent.ipTxSuccess = ipCounters->mTxSuccess;
    counterEvent.ipRxSuccess = ipCounters->mRxSuccess;
    counterEvent.ipTxFailure = ipCounters->mTxFailure;
    counterEvent.ipRxFailure = ipCounters->mRxFailure;

    if (otDatasetIsCommissioned(mOTInst))
    {
        otErr = otDatasetGetActive(mOTInst, &activeDataset);
        VerifyOrExit(otErr == OT_ERROR_NONE, err = MapOpenThreadError(otErr));

        if (activeDataset.mComponents.mIsChannelPresent)
        {
            counterEvent.channel = activeDataset.mChannel;
        }
    }

    role = otThreadGetDeviceRole(mOTInst);

    switch (role)
    {
    case OT_DEVICE_ROLE_LEADER:
        counterEvent.nodeType |= TelemetryNetworkWpanTrait::NODE_TYPE_LEADER;
        // Intentional fall-through: if it's a leader, then it's also a router
    case OT_DEVICE_ROLE_ROUTER:
        counterEvent.nodeType |= TelemetryNetworkWpanTrait::NODE_TYPE_ROUTER;
        break;
    case OT_DEVICE_ROLE_CHILD:
    case OT_DEVICE_ROLE_DISABLED:
    case OT_DEVICE_ROLE_DETACHED:
    default:
        counterEvent.nodeType = 0;
        break;
    }

    counterEvent.threadType = TelemetryNetworkWpanTrait::THREAD_TYPE_OPENTHREAD;

    WeaveLogProgress(DeviceLayer,
                     "Rx Counters:\n"
                     "PHY Rx Total:                 %d\n"
                     "MAC Rx Unicast:               %d\n"
                     "MAC Rx Broadcast:             %d\n"
                     "MAC Rx Data:                  %d\n"
                     "MAC Rx Data Polls:            %d\n"
                     "MAC Rx Beacons:               %d\n"
                     "MAC Rx Beacon Reqs:           %d\n"
                     "MAC Rx Other:                 %d\n"
                     "MAC Rx Filtered Whitelist:    %d\n"
                     "MAC Rx Filtered DestAddr:     %d\n",
                     counterEvent.phyRx, counterEvent.macUnicastRx, counterEvent.macBroadcastRx, counterEvent.macRxData,
                     counterEvent.macRxDataPoll, counterEvent.macRxBeacon, counterEvent.macRxBeaconReq, counterEvent.macRxOtherPkt,
                     counterEvent.macRxFilterWhitelist, counterEvent.macRxFilterDestAddr);

    WeaveLogProgress(DeviceLayer,
                     "Tx Counters:\n"
                     "PHY Tx Total:                 %d\n"
                     "MAC Tx Unicast:               %d\n"
                     "MAC Tx Broadcast:             %d\n"
                     "MAC Tx Data:                  %d\n"
                     "MAC Tx Data Polls:            %d\n"
                     "MAC Tx Beacons:               %d\n"
                     "MAC Tx Beacon Reqs:           %d\n"
                     "MAC Tx Other:                 %d\n"
                     "MAC Tx Retry:                 %d\n"
                     "MAC Tx CCA Fail:              %d\n",
                     counterEvent.phyTx, counterEvent.macUnicastTx, counterEvent.macBroadcastTx, counterEvent.macTxData,
                     counterEvent.macTxDataPoll, counterEvent.macTxBeacon, counterEvent.macTxBeaconReq, counterEvent.macTxOtherPkt,
                     counterEvent.macTxRetry, counterEvent.macTxFailCca);

    WeaveLogProgress(DeviceLayer,
                     "Failure Counters:\n"
                     "MAC Rx Decrypt Fail:          %d\n"
                     "MAC Rx No Frame Fail:         %d\n"
                     "MAC Rx Unknown Neighbor Fail: %d\n"
                     "MAC Rx Invalid Src Addr Fail: %d\n"
                     "MAC Rx FCS Fail:              %d\n"
                     "MAC Rx Other Fail:            %d\n",
                     counterEvent.macRxFailDecrypt, counterEvent.macRxFailNoFrame, counterEvent.macRxFailUnknownNeighbor,
                     counterEvent.macRxFailInvalidSrcAddr, counterEvent.macRxFailFcs, counterEvent.macRxFailOther);

    eventId = nl::LogEvent(&counterEvent);
    WeaveLogProgress(DeviceLayer, "OpenThread Telemetry Stats Event Id: %u\n", eventId);

    Impl()->UnlockThreadStack();

exit:
    return err;
}

template<class ImplClass>
WEAVE_ERROR GenericThreadStackManagerImpl_OpenThread<ImplClass>::_GetAndLogThreadTopologyMinimal(void)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    otError otErr;
    nl::Weave::Profiles::DataManagement_Current::event_id_t eventId;
    Schema::Nest::Trait::Network::TelemetryNetworkWpanTrait::NetworkWpanTopoMinimalEvent topologyEvent = { 0 };
    const otExtAddress *extAddress;

    Impl()->LockThreadStack();

    topologyEvent.rloc16 = otThreadGetRloc16(mOTInst);

    // Router ID is the top 6 bits of the RLOC
    topologyEvent.routerId = (topologyEvent.rloc16 >> 10) & 0x3f;

    topologyEvent.leaderRouterId = otThreadGetLeaderRouterId(mOTInst);

    otErr = otThreadGetParentAverageRssi(mOTInst, &topologyEvent.parentAverageRssi);
    VerifyOrExit(otErr == OT_ERROR_NONE, err = MapOpenThreadError(otErr));

    otErr = otThreadGetParentLastRssi(mOTInst, &topologyEvent.parentLastRssi);
    VerifyOrExit(otErr == OT_ERROR_NONE, err = MapOpenThreadError(otErr));

    topologyEvent.partitionId = otThreadGetPartitionId(mOTInst);

    extAddress = otLinkGetExtendedAddress(mOTInst);

    topologyEvent.extAddress.mBuf = (uint8_t *)extAddress;
    topologyEvent.extAddress.mLen = sizeof(otExtAddress);

    topologyEvent.instantRssi = otPlatRadioGetRssi(mOTInst);

    WeaveLogProgress(DeviceLayer,
                     "Thread Topology:\n"
                     "RLOC16:           %04X\n"
                     "Router ID:        %u\n"
                     "Leader Router ID: %u\n"
                     "Parent Avg RSSI:  %d\n"
                     "Parent Last RSSI: %d\n"
                     "Partition ID:     %d\n"
                     "Extended Address: %02X%02X:%02X%02X:%02X%02X:%02X%02X\n"
                     "Instant RSSI:     %d\n",
                     topologyEvent.rloc16, topologyEvent.routerId, topologyEvent.leaderRouterId, topologyEvent.parentAverageRssi,
                     topologyEvent.parentLastRssi, topologyEvent.partitionId, topologyEvent.extAddress.mBuf[0], topologyEvent.extAddress.mBuf[1],
                     topologyEvent.extAddress.mBuf[2], topologyEvent.extAddress.mBuf[3], topologyEvent.extAddress.mBuf[4],
                     topologyEvent.extAddress.mBuf[5], topologyEvent.extAddress.mBuf[6], topologyEvent.extAddress.mBuf[7], topologyEvent.instantRssi);

    eventId = nl::LogEvent(&topologyEvent);
    WeaveLogProgress(DeviceLayer, "OpenThread Telemetry Stats Event Id: %u\n", eventId);

    Impl()->UnlockThreadStack();

exit:
    if (err != WEAVE_NO_ERROR)
    {
        WeaveLogError(DeviceLayer, "GetAndLogThreadTopologyMinimul failed: %s", nl::ErrorStr(err));
    }

    return err;
}

#define TELEM_NEIGHBOR_TABLE_SIZE (64)
#define TELEM_PRINT_BUFFER_SIZE (64)

template<class ImplClass>
WEAVE_ERROR GenericThreadStackManagerImpl_OpenThread<ImplClass>::_GetAndLogThreadTopologyFull(void)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    otError otErr;
    nl::Weave::Profiles::DataManagement_Current::EventOptions neighborTopoOpts(true);
    nl::Weave::Profiles::DataManagement_Current::event_id_t eventId;
    Schema::Nest::Trait::Network::TelemetryNetworkWpanTrait::NetworkWpanTopoFullEvent fullTopoEvent = { 0 };
    Schema::Nest::Trait::Network::TelemetryNetworkWpanTrait::TopoEntryEvent neighborTopoEvent = { 0 };
    otIp6Address * leaderAddr = NULL;
    uint8_t * networkData = NULL;
    uint8_t * stableNetworkData = NULL;
    uint8_t networkDataLen = 0;
    uint8_t stableNetworkDataLen = 0;
    const otExtAddress * extAddress;
    otNeighborInfo neighborInfo[TELEM_NEIGHBOR_TABLE_SIZE];
    otNeighborInfoIterator iter;
    otNeighborInfoIterator iterCopy;
    char printBuf[TELEM_PRINT_BUFFER_SIZE];

    Impl()->LockThreadStack();

    fullTopoEvent.rloc16 = otThreadGetRloc16(mOTInst);

    // Router ID is the top 6 bits of the RLOC
    fullTopoEvent.routerId = (fullTopoEvent.rloc16 >> 10) & 0x3f;

    fullTopoEvent.leaderRouterId = otThreadGetLeaderRouterId(mOTInst);

    otErr = otThreadGetLeaderRloc(mOTInst, leaderAddr);
    VerifyOrExit(otErr == OT_ERROR_NONE, err = MapOpenThreadError(otErr));

    fullTopoEvent.leaderAddress.mBuf = leaderAddr->mFields.m8;
    fullTopoEvent.leaderAddress.mLen = OT_IP6_ADDRESS_SIZE;

    fullTopoEvent.leaderWeight = otThreadGetLeaderWeight(mOTInst);

    fullTopoEvent.leaderLocalWeight = otThreadGetLocalLeaderWeight(mOTInst);

    otErr = otNetDataGet(mOTInst, false, networkData, &networkDataLen);
    VerifyOrExit(otErr == OT_ERROR_NONE, err = MapOpenThreadError(otErr));

    fullTopoEvent.networkData.mBuf = networkData;
    fullTopoEvent.networkData.mLen = networkDataLen;

    fullTopoEvent.networkDataVersion = otNetDataGetVersion(mOTInst);

    otErr = otNetDataGet(mOTInst, true, stableNetworkData, &stableNetworkDataLen);
    VerifyOrExit(otErr == OT_ERROR_NONE, err = MapOpenThreadError(otErr));

    fullTopoEvent.stableNetworkData.mBuf = stableNetworkData;
    fullTopoEvent.stableNetworkData.mLen = stableNetworkDataLen;

    fullTopoEvent.stableNetworkDataVersion = otNetDataGetStableVersion(mOTInst);

    // Deprecated property
    fullTopoEvent.preferredRouterId = -1;

    extAddress = otLinkGetExtendedAddress(mOTInst);

    fullTopoEvent.extAddress.mBuf = (uint8_t *)extAddress;
    fullTopoEvent.extAddress.mLen = sizeof(otExtAddress);

    fullTopoEvent.partitionId = otThreadGetPartitionId(mOTInst);

    fullTopoEvent.instantRssi = otPlatRadioGetRssi(mOTInst);

    iter = OT_NEIGHBOR_INFO_ITERATOR_INIT;
    iterCopy = OT_NEIGHBOR_INFO_ITERATOR_INIT;
    fullTopoEvent.neighborTableSize = 0;
    fullTopoEvent.childTableSize = 0;

    while (otThreadGetNextNeighborInfo(mOTInst, &iter, &neighborInfo[iter]) == OT_ERROR_NONE)
    {
        fullTopoEvent.neighborTableSize++;
        if (neighborInfo[iterCopy].mIsChild)
        {
            fullTopoEvent.childTableSize++;
        }
        iterCopy = iter;
    }

    WeaveLogProgress(DeviceLayer,
                     "Thread Topology:\n"
                     "RLOC16:                %04X\n"
                     "Router ID:             %u\n"
                     "Leader Router ID:      %u\n"
                     "Leader Address:        %02X%02X:%02X%02X:%02X%02X:%02X%02X:%02X%02X:%02X%02X:%02X%02X:%02X%02X\n"
                     "Leader Weight:         %d\n"
                     "Local Leader Weight:   %d\n"
                     "Network Data Len:      %d\n"
                     "Network Data Version:  %d\n"
                     "Extended Address:      %02X%02X:%02X%02X:%02X%02X:%02X%02X\n"
                     "Partition ID:          %X\n"
                     "Instant RSSI:          %d\n"
                     "Neighbor Table Length: %d\n"
                     "Child Table Length:    %d\n",
                     fullTopoEvent.rloc16, fullTopoEvent.routerId, fullTopoEvent.leaderRouterId,
                     fullTopoEvent.leaderAddress.mBuf[0], fullTopoEvent.leaderAddress.mBuf[1], fullTopoEvent.leaderAddress.mBuf[2], fullTopoEvent.leaderAddress.mBuf[3],
                     fullTopoEvent.leaderAddress.mBuf[4], fullTopoEvent.leaderAddress.mBuf[5], fullTopoEvent.leaderAddress.mBuf[6], fullTopoEvent.leaderAddress.mBuf[7],
                     fullTopoEvent.leaderAddress.mBuf[8], fullTopoEvent.leaderAddress.mBuf[9], fullTopoEvent.leaderAddress.mBuf[10], fullTopoEvent.leaderAddress.mBuf[11],
                     fullTopoEvent.leaderAddress.mBuf[12], fullTopoEvent.leaderAddress.mBuf[13], fullTopoEvent.leaderAddress.mBuf[14], fullTopoEvent.leaderAddress.mBuf[15],
                     fullTopoEvent.leaderWeight, fullTopoEvent.leaderLocalWeight, fullTopoEvent.networkData.mLen, fullTopoEvent.networkDataVersion,
                     fullTopoEvent.extAddress.mBuf[0], fullTopoEvent.extAddress.mBuf[1], fullTopoEvent.extAddress.mBuf[2], fullTopoEvent.extAddress.mBuf[3],
                     fullTopoEvent.extAddress.mBuf[4], fullTopoEvent.extAddress.mBuf[5], fullTopoEvent.extAddress.mBuf[6], fullTopoEvent.extAddress.mBuf[7],
                     fullTopoEvent.partitionId, fullTopoEvent.instantRssi, fullTopoEvent.neighborTableSize, fullTopoEvent.childTableSize);

    eventId = nl::LogEvent(&fullTopoEvent);
    WeaveLogProgress(DeviceLayer, "OpenThread Full Topology Event Id: %u\n", eventId);

    // Populate the neighbor event options.
    // This way the neighbor topology entries are linked to the actual topology full event.
    neighborTopoOpts.relatedEventID = eventId;
    neighborTopoOpts.relatedImportance = TelemetryNetworkWpanTrait::NetworkWpanTopoFullEvent::Schema.mImportance;

    // Handle each neighbor event seperatly.
    for (uint32_t i = 0; i < fullTopoEvent.neighborTableSize; i++)
    {
        otNeighborInfo * neighbor           = &neighborInfo[i];

        neighborTopoEvent.extAddress.mBuf   = neighbor->mExtAddress.m8;
        neighborTopoEvent.extAddress.mLen   = sizeof(uint64_t);

        neighborTopoEvent.rloc16            = neighbor->mRloc16;
        neighborTopoEvent.linkQualityIn     = neighbor->mLinkQualityIn;
        neighborTopoEvent.averageRssi       = neighbor->mAverageRssi;
        neighborTopoEvent.age               = neighbor->mAge;
        neighborTopoEvent.rxOnWhenIdle      = neighbor->mRxOnWhenIdle;
        // TODO: not supported in old version of OpenThread used by Nordic SDK.
        // neighborTopoEvent.fullFunction      = neighbor->mFullThreadDevice;
        neighborTopoEvent.fullFunction      = false;
        neighborTopoEvent.secureDataRequest = neighbor->mSecureDataRequest;
        neighborTopoEvent.fullNetworkData   = neighbor->mFullNetworkData;
        neighborTopoEvent.lastRssi          = neighbor->mLastRssi;
        neighborTopoEvent.linkFrameCounter  = neighbor->mLinkFrameCounter;
        neighborTopoEvent.mleFrameCounter   = neighbor->mMleFrameCounter;
        neighborTopoEvent.isChild           = neighbor->mIsChild;

        if (neighborTopoEvent.isChild)
        {
            otChildInfo * child =  NULL;
            otErr = otThreadGetChildInfoById(mOTInst, neighborTopoEvent.rloc16, child);
            VerifyOrExit(otErr == OT_ERROR_NONE, err = MapOpenThreadError(otErr));

            neighborTopoEvent.timeout            = child->mTimeout;
            neighborTopoEvent.networkDataVersion = child->mNetworkDataVersion;

            neighborTopoEvent.SetTimeoutPresent();
            neighborTopoEvent.SetNetworkDataVersionPresent();

            snprintf(printBuf, TELEM_PRINT_BUFFER_SIZE, ", Timeout: %10lu NetworkDataVersion: %3d",
                               neighborTopoEvent.timeout, neighborTopoEvent.networkDataVersion);
        }
        else
        {
            neighborTopoEvent.SetTimeoutNull();
            neighborTopoEvent.SetNetworkDataVersionNull();

            printBuf[0] = 0;
        }

        WeaveLogProgress(DeviceLayer,
                         "TopoEntry[%u]:     %02X%02X:%02X%02X:%02X%02X:%02X%02X\n"
                         "RLOC:              %04X\n"
                         "Age:               %3d\n"
                         "LQI:               %1d\n"
                         "AvgRSSI:           %3d\n"
                         "LastRSSI:          %3d\n"
                         "LinkFrameCounter:  %10d\n"
                         "MleFrameCounter:   %10d\n"
                         "RxOnWhenIdle:      %c\n"
                         "SecureDataRequest: %c\n"
                         "FullFunction:      %c\n"
                         "FullNetworkData:   %c\n"
                         "IsChild:           %c%s\n",
                         i, neighborTopoEvent.extAddress.mBuf[0], neighborTopoEvent.extAddress.mBuf[1], neighborTopoEvent.extAddress.mBuf[2],
                         neighborTopoEvent.extAddress.mBuf[3], neighborTopoEvent.extAddress.mBuf[4], neighborTopoEvent.extAddress.mBuf[5],
                         neighborTopoEvent.extAddress.mBuf[6], neighborTopoEvent.extAddress.mBuf[7], neighborTopoEvent.rloc16, neighborTopoEvent.age,
                         neighborTopoEvent.linkQualityIn, neighborTopoEvent.averageRssi, neighborTopoEvent.lastRssi, neighborTopoEvent.linkFrameCounter,
                         neighborTopoEvent.mleFrameCounter, neighborTopoEvent.rxOnWhenIdle ? 'Y' : 'n', neighborTopoEvent.secureDataRequest ? 'Y' : 'n',
                         neighborTopoEvent.fullFunction ? 'Y' : 'n', neighborTopoEvent.fullNetworkData ? 'Y' : 'n',
                         neighborTopoEvent.isChild ? 'Y' : 'n', printBuf);

        eventId = nl::LogEvent(&neighborTopoEvent, neighborTopoOpts);
        WeaveLogProgress(DeviceLayer, "OpenThread Neighbor TopoEntry[%u] Event Id: %ld\n", i, eventId);
    }

    Impl()->UnlockThreadStack();

exit:
    if (err != WEAVE_NO_ERROR)
    {
        WeaveLogError(DeviceLayer, "GetAndLogThreadTopologyFull failed: %s", nl::ErrorStr(err));
    }
    return err;
}

template<class ImplClass>
WEAVE_ERROR GenericThreadStackManagerImpl_OpenThread<ImplClass>::_GetPrimary802154MACAddress(uint8_t *buf)
{
    const otExtAddress *extendedAddr = otLinkGetExtendedAddress(mOTInst);
    memcpy(buf, extendedAddr, sizeof(otExtAddress));
    return WEAVE_NO_ERROR;
};

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
