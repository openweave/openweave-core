/*
 *
 *    Copyright (c) 2018 Nest Labs, Inc.
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
 *          GenericNetworkProvisioningServerImpl<> template.
 */

#ifndef GENERIC_NETWORK_PROVISIONING_SERVER_IMPL_IPP
#define GENERIC_NETWORK_PROVISIONING_SERVER_IMPL_IPP

#include <Weave/DeviceLayer/internal/WeaveDeviceLayerInternal.h>
#include <Weave/DeviceLayer/internal/NetworkProvisioningServer.h>
#include <Weave/DeviceLayer/internal/GenericNetworkProvisioningServerImpl.h>

namespace nl {
namespace Weave {
namespace DeviceLayer {
namespace Internal {

using namespace ::nl::Weave::TLV;
using namespace ::nl::Weave::Profiles::Common;
using namespace ::nl::Weave::Profiles::NetworkProvisioning;

using Profiles::kWeaveProfile_Common;
using Profiles::kWeaveProfile_NetworkProvisioning;

template<class ImplClass>
WEAVE_ERROR GenericNetworkProvisioningServerImpl<ImplClass>::_Init()
{
    WEAVE_ERROR err;

    // Call init on the server base class.
    err = ServerBaseClass::Init(&::nl::Weave::DeviceLayer::ExchangeMgr);
    SuccessOrExit(err);

    // Set the pointer to the delegate object.
    SetDelegate(this);

    mState = kState_Idle;

exit:
    return err;
}

template<class ImplClass>
void GenericNetworkProvisioningServerImpl<ImplClass>::_StartPendingScan()
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    // Do nothing if there's no pending ScanNetworks request outstanding, or if a scan is already in progress.
    if (mState != kState_ScanNetworks_Pending)
    {
        ExitNow();
    }

    // Defer the scan if the Connection Manager says the system is in a state
    // where a WiFi scan cannot be started (e.g. if the system is connecting to
    // an AP and can't scan and connect at the same time). The Connection Manager
    // is responsible for calling this method again when the system is read to scan.
    if (!ConnectivityMgr().CanStartWiFiScan())
    {
        ExitNow();
    }

    mState = kState_ScanNetworks_InProgress;

    // Delegate to the implementation subclass to initiate the WiFi scan operation.
    err = Impl()->InitiateWiFiScan();
    SuccessOrExit(err);

exit:
    // If an error occurred, send a Internal Error back to the requestor.
    if (err != WEAVE_NO_ERROR)
    {
        SendStatusReport(kWeaveProfile_Common, kStatus_InternalError, err);
        mState = kState_Idle;
    }
}

template<class ImplClass>
void GenericNetworkProvisioningServerImpl<ImplClass>::_OnPlatformEvent(const WeaveDeviceEvent * event)
{
    // Handle a change in Internet connectivity...
    if (event->Type == WeaveDeviceEvent::kEventType_InternetConnectivityChange)
    {
        // If the system now has IPv4 Internet connectivity, continue the process of
        // performing the TestConnectivity request.
        if (ConnectivityMgr().HaveIPv4InternetConnectivity())
        {
            ContinueTestConnectivity();
        }
    }
}

template<class ImplClass>
WEAVE_ERROR GenericNetworkProvisioningServerImpl<ImplClass>::HandleScanNetworks(uint8_t networkType)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    VerifyOrExit(mState == kState_Idle, err = WEAVE_ERROR_INCORRECT_STATE);

    // Verify the expected network type.
    if (networkType != kNetworkType_WiFi)
    {
        err = SendStatusReport(kWeaveProfile_NetworkProvisioning, kStatusCode_UnsupportedNetworkType);
        ExitNow();
    }

    // Reject the request if the application is currently in control of the WiFi station.
    if (RejectIfApplicationControlled(true))
    {
        ExitNow();
    }

    // Enter the ScanNetworks Pending state and delegate to the implementation class to start the scan.
    mState = kState_ScanNetworks_Pending;
    Impl()->StartPendingScan();

exit:
    return err;
}

template<class ImplClass>
WEAVE_ERROR GenericNetworkProvisioningServerImpl<ImplClass>::HandleAddNetwork(PacketBuffer * networkInfoTLV)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    NetworkInfo netInfo;

    VerifyOrExit(mState == kState_Idle, err = WEAVE_ERROR_INCORRECT_STATE);

    // Parse the supplied network configuration info.
    {
        TLV::TLVReader reader;
        reader.Init(networkInfoTLV);
        err = netInfo.Decode(reader);
        SuccessOrExit(err);
    }

    // Discard the request buffer.
    PacketBuffer::Free(networkInfoTLV);
    networkInfoTLV = NULL;

    // Reject the request if the application is currently in control of the WiFi station.
    if (RejectIfApplicationControlled(true))
    {
        ExitNow();
    }

    // Check the validity of the new WiFi station provision. If not acceptable, respond to
    // the requestor with an appropriate StatusReport.
    {
        uint32_t statusProfileId;
        uint16_t statusCode;
        err = ValidateWiFiStationProvision(netInfo, statusProfileId, statusCode);
        if (err != WEAVE_NO_ERROR)
        {
            err = SendStatusReport(statusProfileId, statusCode, err);
            ExitNow();
        }
    }

    // If the WiFi station is not already configured, disable the WiFi station interface.
    // This ensures that the device will not automatically connect to the new network until
    // an EnableNetwork request is received.
    if (!ConnectivityMgr().IsWiFiStationProvisioned())
    {
        err = ConnectivityMgr().SetWiFiStationMode(ConnectivityManager::kWiFiStationMode_Disabled);
        SuccessOrExit(err);
    }

    // Delegate to the implementation subclass to set the WiFi station provision.
    err = Impl()->SetWiFiStationProvision(netInfo);
    SuccessOrExit(err);

    // Tell the ConnectivityManager there's been a change to the station provision.
    ConnectivityMgr().OnWiFiStationProvisionChange();

    // Send an AddNetworkComplete message back to the requestor.
    SendAddNetworkComplete(kWiFiStationNetworkId);

exit:
    PacketBuffer::Free(networkInfoTLV);
    return err;
}

template<class ImplClass>
WEAVE_ERROR GenericNetworkProvisioningServerImpl<ImplClass>::HandleUpdateNetwork(PacketBuffer * networkInfoTLV)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    NetworkInfo netInfo, netInfoUpdates;

    VerifyOrExit(mState == kState_Idle, err = WEAVE_ERROR_INCORRECT_STATE);

    // Parse the supplied network configuration info.
    {
        TLV::TLVReader reader;
        reader.Init(networkInfoTLV);
        err = netInfoUpdates.Decode(reader);
        SuccessOrExit(err);
    }

    // Discard the request buffer.
    PacketBuffer::Free(networkInfoTLV);
    networkInfoTLV = NULL;

    // Reject the request if the application is currently in control of the WiFi station.
    if (RejectIfApplicationControlled(true))
    {
        ExitNow();
    }

    // If the network id field isn't present, immediately reply with an error.
    if (!netInfoUpdates.NetworkIdPresent)
    {
        err = SendStatusReport(kWeaveProfile_NetworkProvisioning, kStatusCode_InvalidNetworkConfiguration);
        ExitNow();
    }

    // Verify that the specified network exists.
    if (!ConnectivityMgr().IsWiFiStationProvisioned() || netInfoUpdates.NetworkId != kWiFiStationNetworkId)
    {
        err = SendStatusReport(kWeaveProfile_NetworkProvisioning, kStatusCode_UnknownNetwork);
        SuccessOrExit(err);
        ExitNow();
    }

    // Delegate to the implementation subclass to get the existing station provision.
    err = Impl()->GetWiFiStationProvision(netInfo, true);
    SuccessOrExit(err);

    // Merge in the updated information.
    err = netInfoUpdates.MergeTo(netInfo);
    SuccessOrExit(err);

    // Check the validity of the updated station provision. If not acceptable, respond to the requestor with an
    // appropriate StatusReport.
    {
        uint32_t statusProfileId;
        uint16_t statusCode;
        err = ValidateWiFiStationProvision(netInfo, statusProfileId, statusCode);
        if (err != WEAVE_NO_ERROR)
        {
            err = SendStatusReport(statusProfileId, statusCode, err);
            ExitNow();
        }
    }

    // Delegate to the implementation subclass to set the station provision.
    err = Impl()->SetWiFiStationProvision(netInfo);
    SuccessOrExit(err);

    // Tell the ConnectivityManager there's been a change to the station provision.
    ConnectivityMgr().OnWiFiStationProvisionChange();

    // Tell the requestor we succeeded.
    err = SendSuccessResponse();
    SuccessOrExit(err);

exit:
    PacketBuffer::Free(networkInfoTLV);
    return err;
}

template<class ImplClass>
WEAVE_ERROR GenericNetworkProvisioningServerImpl<ImplClass>::HandleRemoveNetwork(uint32_t networkId)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    VerifyOrExit(mState == kState_Idle, err = WEAVE_ERROR_INCORRECT_STATE);

    // Reject the request if the application is currently in control of the WiFi station.
    if (RejectIfApplicationControlled(true))
    {
        ExitNow();
    }

    // Verify that the specified network exists.
    if (!ConnectivityMgr().IsWiFiStationProvisioned() || networkId != kWiFiStationNetworkId)
    {
        err = SendStatusReport(kWeaveProfile_NetworkProvisioning, kStatusCode_UnknownNetwork);
        SuccessOrExit(err);
        ExitNow();
    }

    // Delegate to the implementation subclass to clear the WiFi station provision.
    err = Impl()->ClearWiFiStationProvision();
    SuccessOrExit(err);

    // Tell the ConnectivityManager there's been a change to the station provision.
    ConnectivityMgr().OnWiFiStationProvisionChange();

    // Respond with a Success response.
    err = SendSuccessResponse();
    SuccessOrExit(err);

exit:
    return err;
}

template<class ImplClass>
WEAVE_ERROR GenericNetworkProvisioningServerImpl<ImplClass>::HandleGetNetworks(uint8_t flags)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    NetworkInfo netInfo;
    PacketBuffer * respBuf = NULL;
    TLVWriter writer;
    uint8_t resultCount;
    const bool includeCredentials = (flags & kGetNetwork_IncludeCredentials) != 0;

    VerifyOrExit(mState == kState_Idle, err = WEAVE_ERROR_INCORRECT_STATE);

    // Reject the request if the application is currently in control of the WiFi station.
    if (RejectIfApplicationControlled(true))
    {
        ExitNow();
    }

    respBuf = PacketBuffer::New();
    VerifyOrExit(respBuf != NULL, err = WEAVE_ERROR_NO_MEMORY);

    writer.Init(respBuf);

    // Delegate to the implementation subclass to get the WiFi station provision.
    err = Impl()->GetWiFiStationProvision(netInfo, includeCredentials);
    if (err == WEAVE_NO_ERROR)
    {
        resultCount = 1;
    }
    else if (err == WEAVE_ERROR_INCORRECT_STATE)
    {
        resultCount = 0;
    }
    else
    {
        ExitNow();
    }

    err = NetworkInfo::EncodeArray(writer, &netInfo, resultCount);
    SuccessOrExit(err);

    err = writer.Finalize();
    SuccessOrExit(err);

    err = SendGetNetworksComplete(resultCount, respBuf);
    respBuf = NULL;
    SuccessOrExit(err);

exit:
    PacketBuffer::Free(respBuf);
    return err;
}

template<class ImplClass>
WEAVE_ERROR GenericNetworkProvisioningServerImpl<ImplClass>::HandleEnableNetwork(uint32_t networkId)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    VerifyOrExit(mState == kState_Idle, err = WEAVE_ERROR_INCORRECT_STATE);

    // Reject the request if the application is currently in control of the WiFi station.
    if (RejectIfApplicationControlled(true))
    {
        ExitNow();
    }

    // Verify that the specified network exists.
    if (!ConnectivityMgr().IsWiFiStationProvisioned() || networkId != kWiFiStationNetworkId)
    {
        err = SendStatusReport(kWeaveProfile_NetworkProvisioning, kStatusCode_UnknownNetwork);
        SuccessOrExit(err);
        ExitNow();
    }

    // Tell the ConnectivityManager to enable the WiFi station interface.
    // Note that any effects of enabling the WiFi station interface (e.g. connecting to an AP) happen
    // asynchronously with this call.
    err = ConnectivityMgr().SetWiFiStationMode(ConnectivityManager::kWiFiStationMode_Enabled);
    SuccessOrExit(err);

    // Send a Success response back to the client.
    SendSuccessResponse();

exit:
    return err;
}

template<class ImplClass>
WEAVE_ERROR GenericNetworkProvisioningServerImpl<ImplClass>::HandleDisableNetwork(uint32_t networkId)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    VerifyOrExit(mState == kState_Idle, err = WEAVE_ERROR_INCORRECT_STATE);

    // Reject the request if the application is currently in control of the WiFi station.
    if (RejectIfApplicationControlled(true))
    {
        ExitNow();
    }

    // Verify that the specified network exists.
    if (!ConnectivityMgr().IsWiFiStationProvisioned() || networkId != kWiFiStationNetworkId)
    {
        err = SendStatusReport(kWeaveProfile_NetworkProvisioning, kStatusCode_UnknownNetwork);
        ExitNow();
    }

    // Tell the ConnectivityManager to disable the WiFi station interface.
    // Note that any effects of disabling the WiFi station interface (e.g. disconnecting from an AP) happen
    // asynchronously with this call.
    err = ConnectivityMgr().SetWiFiStationMode(ConnectivityManager::kWiFiStationMode_Disabled);
    SuccessOrExit(err);

    // Respond with a Success response.
    err = SendSuccessResponse();
    SuccessOrExit(err);

exit:
    return err;
}

template<class ImplClass>
WEAVE_ERROR GenericNetworkProvisioningServerImpl<ImplClass>::HandleTestConnectivity(uint32_t networkId)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    VerifyOrExit(mState == kState_Idle, err = WEAVE_ERROR_INCORRECT_STATE);

    // Reject the request if the application is currently in control of the WiFi station.
    if (RejectIfApplicationControlled(true))
    {
        ExitNow();
    }

    // Verify that the specified network exists.
    if (!ConnectivityMgr().IsWiFiStationProvisioned() || networkId != kWiFiStationNetworkId)
    {
        err = SendStatusReport(kWeaveProfile_NetworkProvisioning, kStatusCode_UnknownNetwork);
        SuccessOrExit(err);
        ExitNow();
    }

    // Tell the ConnectivityManager to enable the WiFi station interface if it hasn't been done already.
    // Note that any effects of enabling the WiFi station interface (e.g. connecting to an AP) happen
    // asynchronously with this call.
    err = ConnectivityMgr().SetWiFiStationMode(ConnectivityManager::kWiFiStationMode_Enabled);
    SuccessOrExit(err);

    // Record that we're waiting for the WiFi station interface to establish connectivity
    // with the Internet and arm a timer that will generate an error if connectivity isn't established
    // within a certain amount of time.
    mState = kState_TestConnectivity_WaitConnectivity;
    SystemLayer.StartTimer(WEAVE_DEVICE_CONFIG_WIFI_CONNECTIVITY_TIMEOUT, HandleConnectivityTimeOut, NULL);

    // Go check for connectivity now.
    ContinueTestConnectivity();

exit:
    return err;
}

template<class ImplClass>
WEAVE_ERROR GenericNetworkProvisioningServerImpl<ImplClass>::HandleSetRendezvousMode(uint16_t rendezvousMode)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    VerifyOrExit(mState == kState_Idle, err = WEAVE_ERROR_INCORRECT_STATE);

    // If any modes other than EnableWiFiRendezvousNetwork or EnableThreadRendezvous
    // were specified, fail with Common:UnsupportedMessage.
    if ((rendezvousMode & ~(kRendezvousMode_EnableWiFiRendezvousNetwork | kRendezvousMode_EnableThreadRendezvous)) != 0)
    {
        err = SendStatusReport(kWeaveProfile_Common, kStatus_UnsupportedMessage);
        ExitNow();
    }

    // If EnableThreadRendezvous was requested, fail with NetworkProvisioning:UnsupportedNetworkType.
    if ((rendezvousMode & kRendezvousMode_EnableThreadRendezvous) != 0)
    {
        err = SendStatusReport(kWeaveProfile_NetworkProvisioning, kStatusCode_UnsupportedNetworkType);
        ExitNow();
    }

    // Reject the request if the application is currently in control of the WiFi AP.
    if (RejectIfApplicationControlled(false))
    {
        ExitNow();
    }

    // If the request is to start the WiFi "rendezvous network" (a.k.a. the WiFi AP interface)...
    if (rendezvousMode != 0)
    {
        // If the AP interface has been expressly disabled by the application, fail with Common:NotAvailable.
        if (ConnectivityMgr().GetWiFiAPMode() == ConnectivityManager::kWiFiAPMode_Disabled)
        {
            err = SendStatusReport(kWeaveProfile_Common, kStatus_NotAvailable);
            ExitNow();
        }

        // Otherwise, request the ConnectivityManager to demand start the WiFi AP interface.
        // If the interface is already active this will have no immediate effect, except if the
        // interface is in the "demand" mode, in which case this will serve to extend the
        // active time.
        ConnectivityMgr().DemandStartWiFiAP();
    }

    // Otherwise the request is to stop the WiFi rendezvous network, so request the ConnectivityManager
    // to stop the AP interface if it has been demand started.  This will have no effect if the
    // interface is already stopped, or if the application has expressly enabled the interface.
    else
    {
        ConnectivityMgr().StopOnDemandWiFiAP();
    }

    // Respond with a Success response.
    err = SendSuccessResponse();
    SuccessOrExit(err);

exit:
    return err;
}

template<class ImplClass>
bool GenericNetworkProvisioningServerImpl<ImplClass>::IsPairedToAccount() const
{
    return ConfigurationMgr().IsServiceProvisioned() && ConfigurationMgr().IsPairedToAccount();
}

template<class ImplClass>
WEAVE_ERROR GenericNetworkProvisioningServerImpl<ImplClass>::ValidateWiFiStationProvision(
        const NetworkInfo & netInfo, uint32_t & statusProfileId, uint16_t & statusCode)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    const char * logPrefix = "NetworkProvisioningServer: ";

    if (netInfo.NetworkType != kNetworkType_WiFi)
    {
        WeaveLogError(DeviceLayer, "%sUnsupported WiFi station network type: %d", logPrefix, netInfo.NetworkType);
        statusProfileId = kWeaveProfile_NetworkProvisioning;
        statusCode = kStatusCode_UnsupportedNetworkType;
        ExitNow(err = WEAVE_ERROR_INVALID_ARGUMENT);
    }

    if (netInfo.WiFiSSID[0] == 0)
    {
        WeaveLogError(DeviceLayer, "%sMissing WiFi station SSID", logPrefix);
        statusProfileId = kWeaveProfile_NetworkProvisioning;
        statusCode = kStatusCode_InvalidNetworkConfiguration;
        ExitNow(err = WEAVE_ERROR_INVALID_ARGUMENT);
    }

    if (netInfo.WiFiMode != kWiFiMode_Managed)
    {
        if (netInfo.WiFiMode == kWiFiMode_NotSpecified)
        {
            WeaveLogError(DeviceLayer, "%sMissing WiFi station mode", logPrefix);
        }
        else
        {
            WeaveLogError(DeviceLayer, "%sUnsupported WiFi station mode: %d", logPrefix, netInfo.WiFiMode);
        }
        statusProfileId = kWeaveProfile_NetworkProvisioning;
        statusCode = kStatusCode_InvalidNetworkConfiguration;
        ExitNow(err = WEAVE_ERROR_INVALID_ARGUMENT);
    }

    if (netInfo.WiFiRole != kWiFiRole_Station)
    {
        if (netInfo.WiFiRole == kWiFiRole_NotSpecified)
        {
            WeaveLogError(DeviceLayer, "%sMissing WiFi station role", logPrefix);
        }
        else
        {
            WeaveLogError(DeviceLayer, "%sUnsupported WiFi station role: %d", logPrefix, netInfo.WiFiRole);
        }
        statusProfileId = kWeaveProfile_NetworkProvisioning;
        statusCode = kStatusCode_InvalidNetworkConfiguration;
        ExitNow(err = WEAVE_ERROR_INVALID_ARGUMENT);
    }

    if (netInfo.WiFiSecurityType != kWiFiSecurityType_None &&
        netInfo.WiFiSecurityType != kWiFiSecurityType_WEP &&
        netInfo.WiFiSecurityType != kWiFiSecurityType_WPAPersonal &&
        netInfo.WiFiSecurityType != kWiFiSecurityType_WPA2Personal &&
        netInfo.WiFiSecurityType != kWiFiSecurityType_WPA2Enterprise)
    {
        WeaveLogError(DeviceLayer, "%sUnsupported WiFi station security type: %d", logPrefix, netInfo.WiFiSecurityType);
        statusProfileId = kWeaveProfile_NetworkProvisioning;
        statusCode = kStatusCode_UnsupportedWiFiSecurityType;
        ExitNow(err = WEAVE_ERROR_INVALID_ARGUMENT);
    }

    if (netInfo.WiFiSecurityType != kWiFiSecurityType_None && netInfo.WiFiKeyLen == 0)
    {
        WeaveLogError(DeviceLayer, "%sMissing WiFi Key", logPrefix);
        statusProfileId = kWeaveProfile_NetworkProvisioning;
        statusCode = kStatusCode_InvalidNetworkConfiguration;
        ExitNow(err = WEAVE_ERROR_INVALID_ARGUMENT);
    }

exit:
    return err;
}

template<class ImplClass>
bool GenericNetworkProvisioningServerImpl<ImplClass>::RejectIfApplicationControlled(bool station)
{
    bool isAppControlled = (station)
        ? ConnectivityMgr().IsWiFiStationApplicationControlled()
        : ConnectivityMgr().IsWiFiAPApplicationControlled();

    // Reject the request if the application is currently in control of the WiFi station.
    if (isAppControlled)
    {
        SendStatusReport(kWeaveProfile_Common, kStatus_NotAvailable);
    }

    return isAppControlled;
}

template<class ImplClass>
void GenericNetworkProvisioningServerImpl<ImplClass>::ContinueTestConnectivity(void)
{
    // If waiting for Internet connectivity to be established, and IPv4 Internet connectivity
    // now exists...
    if (mState == kState_TestConnectivity_WaitConnectivity && ConnectivityMgr().HaveIPv4InternetConnectivity())
    {
        // Reset the state.
        mState = kState_Idle;
        SystemLayer.CancelTimer(HandleConnectivityTimeOut, NULL);

        // Verify that the TestConnectivity request is still outstanding; if so...
        if (GetCurrentOp() == kMsgType_TestConnectivity)
        {
            // TODO: perform positive test of connectivity to the Internet.

            // Send a Success response to the client.
            SendSuccessResponse();
        }
    }
}

template<class ImplClass>
void GenericNetworkProvisioningServerImpl<ImplClass>::HandleConnectivityTimeOut(
        ::nl::Weave::System::Layer * aLayer, void * aAppState, ::nl::Weave::System::Error aError)
{
    WeaveLogError(DeviceLayer, "Time out waiting for Internet connectivity");

    // Reset the state.
    NetworkProvisioningSvrImpl().mState = kState_Idle;
    SystemLayer.CancelTimer(HandleConnectivityTimeOut, NULL);

    // Verify that the TestConnectivity request is still outstanding; if so, send a
    // NetworkProvisioning:NetworkConnectFailed StatusReport to the client.
    if (NetworkProvisioningSvrImpl().GetCurrentOp() == kMsgType_TestConnectivity)
    {
        NetworkProvisioningSvrImpl().SendStatusReport(kWeaveProfile_NetworkProvisioning, kStatusCode_NetworkConnectFailed, WEAVE_ERROR_TIMEOUT);
    }
}

} // namespace Internal
} // namespace DeviceLayer
} // namespace Weave
} // namespace nl


#endif // GENERIC_NETWORK_PROVISIONING_SERVER_IMPL_IPP
