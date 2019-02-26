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

#include <Weave/DeviceLayer/internal/WeaveDeviceLayerInternal.h>
#include <Weave/DeviceLayer/internal/ServiceProvisioningServer.h>

using namespace ::nl;
using namespace ::nl::Weave;
using namespace ::nl::Weave::Profiles;
using namespace ::nl::Weave::Profiles::ServiceProvisioning;

namespace nl {
namespace Weave {
namespace DeviceLayer {
namespace Internal {

ServiceProvisioningServer ServiceProvisioningServer::sInstance;

WEAVE_ERROR ServiceProvisioningServer::Init(void)
{
    WEAVE_ERROR err;

    // Call init on the server base class.
    err = ServerBaseClass::Init(&::nl::Weave::DeviceLayer::ExchangeMgr);
    SuccessOrExit(err);

    // Set the pointer to the delegate object.
    SetDelegate(this);

    mProvServiceBinding = NULL;
    mWaitingForServiceTunnel = false;

exit:
    return err;
}

WEAVE_ERROR ServiceProvisioningServer::HandleRegisterServicePairAccount(RegisterServicePairAccountMessage & msg)
{
    WEAVE_ERROR err;
    uint64_t curServiceId;

    // Check if a service is already provisioned. If so respond with "Too Many Services".
    err = ConfigurationMgr().GetServiceId(curServiceId);
    if (err == WEAVE_NO_ERROR)
    {
        err = sInstance.SendStatusReport(kWeaveProfile_ServiceProvisioning,
                (curServiceId == msg.ServiceId) ? kStatusCode_ServiceAlreadyRegistered : kStatusCode_TooManyServices);
        ExitNow();
    }
    if (err == WEAVE_DEVICE_ERROR_CONFIG_NOT_FOUND)
    {
        err = WEAVE_NO_ERROR;
    }
    SuccessOrExit(err);

    // Validate the service config. We don't want to get any further along before making sure the data is good.
    if (!ServiceProvisioningServer::IsValidServiceConfig(msg.ServiceConfig, msg.ServiceConfigLen))
    {
        err = sInstance.SendStatusReport(kWeaveProfile_ServiceProvisioning, kStatusCode_InvalidServiceConfig);
        ExitNow();
    }

    WeaveLogProgress(DeviceLayer, "Registering new service: %" PRIx64 " (account id %*s)", msg.ServiceId, (int)msg.AccountIdLen, msg.AccountId);

    // Store the service id and the service config in persistent storage.
    err = ConfigurationMgr().StoreServiceProvisioningData(msg.ServiceId, msg.ServiceConfig, msg.ServiceConfigLen, NULL, 0);
    SuccessOrExit(err);

    // Post an event alerting other subsystems to the change in the service provisioning state.
    {
        WeaveDeviceEvent event;
        event.Type = DeviceEventType::kServiceProvisioningChange;
        event.ServiceProvisioningChange.IsServiceProvisioned = true;
        event.ServiceProvisioningChange.ServiceConfigUpdated = false;
        PlatformMgr().PostEvent(&event);
    }

#if !WEAVE_DEVICE_CONFIG_DISABLE_ACCOUNT_PAIRING

    // Initiate the process of sending a PairDeviceToAccount request to the Service Provisioning service.
    PlatformMgr().ScheduleWork(AsyncStartPairDeviceToAccount);

#else // !WEAVE_DEVICE_CONFIG_DISABLE_ACCOUNT_PAIRING

    // Store the account id in persistent storage.
    err = ConfigurationMgr().StorePairedAccountId(msg.AccountId, msg.AccountIdLen);
    SuccessOrExit(err);

    // Post an event alerting other subsystems that the device is now paired to an account.
    {
        WeaveDeviceEvent event;
        event.Type = DeviceEventType::kAccountPairingChange;
        event.AccountPairingChange.IsPairedToAccount = true;
        PlatformMgr().PostEvent(&event);
    }

    // Send a success StatusReport for the RegisterServicePairDevice request.
    SendSuccessResponse();

#endif // !WEAVE_DEVICE_CONFIG_DISABLE_ACCOUNT_PAIRING

exit:
    return err;
}

WEAVE_ERROR ServiceProvisioningServer::HandleUpdateService(UpdateServiceMessage& msg)
{
    WEAVE_ERROR err;
    uint64_t curServiceId;

    // Verify that the service id matches the existing service.  If not respond with "No Such Service".
    err = ConfigurationMgr().GetServiceId(curServiceId);
    if (err == WEAVE_DEVICE_ERROR_CONFIG_NOT_FOUND || curServiceId != msg.ServiceId)
    {
        err = sInstance.SendStatusReport(kWeaveProfile_ServiceProvisioning, kStatusCode_NoSuchService);
        ExitNow();
    }
    SuccessOrExit(err);

    // Validate the service config. We don't want to get any further along before making sure the data is good.
    if (!ServiceProvisioningServer::IsValidServiceConfig(msg.ServiceConfig, msg.ServiceConfigLen))
    {
        err = sInstance.SendStatusReport(kWeaveProfile_ServiceProvisioning, kStatusCode_InvalidServiceConfig);
        ExitNow();
    }

    // Save the new service configuration in device persistent storage, replacing the existing value.
    err = ConfigurationMgr().StoreServiceConfig(msg.ServiceConfig, msg.ServiceConfigLen);
    SuccessOrExit(err);

    // Post an event alerting other subsystems that the service config has changed.
    {
        WeaveDeviceEvent event;
        event.Type = DeviceEventType::kServiceProvisioningChange;
        event.ServiceProvisioningChange.IsServiceProvisioned = true;
        event.ServiceProvisioningChange.ServiceConfigUpdated = true;
        PlatformMgr().PostEvent(&event);
    }

    // Send "Success" back to the requestor.
    err = sInstance.SendSuccessResponse();
    SuccessOrExit(err);

exit:
    return err;
}

WEAVE_ERROR ServiceProvisioningServer::HandleUnregisterService(uint64_t serviceId)
{
    WEAVE_ERROR err;
    uint64_t curServiceId;

    // Verify that the service id matches the existing service.  If not respond with "No Such Service".
    err = ConfigurationMgr().GetServiceId(curServiceId);
    if (err == WEAVE_DEVICE_ERROR_CONFIG_NOT_FOUND || curServiceId != serviceId)
    {
        err = sInstance.SendStatusReport(kWeaveProfile_ServiceProvisioning, kStatusCode_NoSuchService);
        ExitNow();
    }
    SuccessOrExit(err);

    // Clear the persisted service provisioning data, if present.
    err = ConfigurationMgr().ClearServiceProvisioningData();
    SuccessOrExit(err);

    // Send "Success" back to the requestor.
    err = sInstance.SendSuccessResponse();
    SuccessOrExit(err);

exit:
    return err;
}

bool ServiceProvisioningServer::IsPairedToAccount(void) const
{
    return ConfigurationMgr().IsServiceProvisioned() && ConfigurationMgr().IsPairedToAccount();
}

void ServiceProvisioningServer::OnPlatformEvent(const WeaveDeviceEvent * event)
{
#if !WEAVE_DEVICE_CONFIG_DISABLE_ACCOUNT_PAIRING

    // If a tunnel to the service has been established...
    if (event->Type == DeviceEventType::kServiceTunnelStateChange &&
        event->ServiceTunnelStateChange.Result == kConnectivity_Established)
    {
        // If a RegisterServicePairAccount request is pending and the system is waiting for
        // the service tunnel to be established, initiate the PairDeviceToAccount request now.
        if (mCurClientOp != NULL && mWaitingForServiceTunnel)
        {
            StartPairDeviceToAccount();
        }
    }

#endif // !WEAVE_DEVICE_CONFIG_DISABLE_ACCOUNT_PAIRING
}

#if !WEAVE_DEVICE_CONFIG_DISABLE_ACCOUNT_PAIRING

void ServiceProvisioningServer::StartPairDeviceToAccount(void)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    // If the system does not currently have a tunnel established with the service, wait a
    // period of time for it to be established.
    if (!ConnectivityMgr().IsServiceTunnelConnected())
    {
        mWaitingForServiceTunnel = true;

        err = SystemLayer.StartTimer(WEAVE_DEVICE_CONFIG_SERVICE_PROVISIONING_CONNECTIVITY_TIMEOUT,
                HandleServiceTunnelTimeout,
                NULL);
        SuccessOrExit(err);
        ExitNow();

        WeaveLogProgress(DeviceLayer, "Waiting for service tunnel to complete RegisterServicePairDevice action");
    }

    mWaitingForServiceTunnel = false;
    SystemLayer.CancelTimer(HandleServiceTunnelTimeout, NULL);

    WeaveLogProgress(DeviceLayer, "Initiating communication with Service Provisioning service");

    // Create a binding and begin the process of preparing it for talking to the Service Provisioning
    // service. When this completes HandleProvServiceBindingEvent will be called with a BindingReady event.
    mProvServiceBinding = ExchangeMgr->NewBinding(HandleProvServiceBindingEvent, NULL);
    VerifyOrExit(mProvServiceBinding != NULL, err = WEAVE_ERROR_NO_MEMORY);
    err = mProvServiceBinding->BeginConfiguration()
            .Target_ServiceEndpoint(WEAVE_DEVICE_CONFIG_SERVICE_PROVISIONING_ENDPOINT_ID)
            .Transport_UDP_WRM()
            .Exchange_ResponseTimeoutMsec(WEAVE_DEVICE_CONFIG_SERVICE_PROVISIONING_REQUEST_TIMEOUT)
            .Security_SharedCASESession()
            .PrepareBinding();
    SuccessOrExit(err);

exit:
    if (err != WEAVE_NO_ERROR)
    {
        HandlePairDeviceToAccountResult(err, kWeaveProfile_Common, Profiles::Common::kStatus_InternalServerProblem);
    }
}

void ServiceProvisioningServer::SendPairDeviceToAccountRequest(void)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    const RegisterServicePairAccountMessage & regServiceMsg = mCurClientOpMsg.RegisterServicePairAccount;
    uint8_t devDesc[100]; // TODO: make configurable
    size_t devDescLen;

    // Generate a device descriptor for the local device in TLV.
    err = ConfigurationMgr().GetDeviceDescriptorTLV(devDesc, sizeof(devDesc), devDescLen);
    SuccessOrExit(err);

    // Call up to a helper function the server base class to encode and send a PairDeviceToAccount request to
    // the Service Provisioning service.  This will ultimately result in a call to HandlePairDeviceToAccountResult
    // with the result.
    //
    // Pass through the values for Service Id, Account Id, Pairing Token and Pairing Init Data that
    // were received in the Register Service message.  For Device Init Data, pass the encoded device
    // descriptor.  Finally, pass the id of the Weave fabric for which the device is a member.
    //
    WeaveLogProgress(DeviceLayer, "Sending PairDeviceToAccount request to Service Provisioning service");
    err = ServerBaseClass::SendPairDeviceToAccountRequest(mProvServiceBinding,
            regServiceMsg.ServiceId, FabricState->FabricId,
            regServiceMsg.AccountId, regServiceMsg.AccountIdLen,
            regServiceMsg.PairingToken, regServiceMsg.PairingTokenLen,
            regServiceMsg.PairingInitData, regServiceMsg.PairingInitDataLen,
            devDesc, devDescLen);
    SuccessOrExit(err);

exit:
    if (err != WEAVE_NO_ERROR)
    {
        HandlePairDeviceToAccountResult(err, kWeaveProfile_Common, Profiles::Common::kStatus_InternalServerProblem);
    }
}

void ServiceProvisioningServer::HandlePairDeviceToAccountResult(WEAVE_ERROR err, uint32_t statusReportProfileId, uint16_t statusReportStatusCode)
{
    // Close the binding if necessary.
    if (mProvServiceBinding != NULL)
    {
        mProvServiceBinding->Close();
        mProvServiceBinding = NULL;
    }

    // Return immediately if for some reason the client's RegisterServicePairAccount request
    // is no longer pending.  Note that, even if the PairDeviceToAccount request succeeded,
    // the device must clear the persisted service configuration in this case because it has
    // lost access to the account id (which was in the RegisterServicePairAccount message)
    // and therefore cannot complete the process of registering the service.
    if (mCurClientOp == NULL)
    {
        ExitNow(err = WEAVE_ERROR_INCORRECT_STATE);
    }

    // If the PairDeviceToAccount request was successful...
    if (err == WEAVE_NO_ERROR)
    {
        const RegisterServicePairAccountMessage & regServiceMsg = mCurClientOpMsg.RegisterServicePairAccount;

        // Store the account id in persistent storage.  This is the final step of registering a
        // service and marks that the device is properly associated with a user's account.
        err = ConfigurationMgr().StorePairedAccountId(regServiceMsg.AccountId, regServiceMsg.AccountIdLen);
        SuccessOrExit(err);

        // Post an event alerting other subsystems that the device is now paired to an account.
        {
            WeaveDeviceEvent event;
            event.Type = DeviceEventType::kAccountPairingChange;
            event.AccountPairingChange.IsPairedToAccount = true;
            PlatformMgr().PostEvent(&event);
        }

        WeaveLogProgress(DeviceLayer, "PairDeviceToAccount request completed successfully");

        // Send a success StatusReport back to the client.
        err = SendSuccessResponse();
        SuccessOrExit(err);
    }

exit:

    if (err != WEAVE_NO_ERROR)
    {
        WeaveLogError(DeviceLayer, "PairDeviceToAccount request failed with %s: %s",
                 (err == WEAVE_ERROR_STATUS_REPORT_RECEIVED) ? "status report from service" : "local error",
                 (err == WEAVE_ERROR_STATUS_REPORT_RECEIVED)
                  ? ::nl::StatusReportStr(statusReportProfileId, statusReportStatusCode)
                  : ::nl::ErrorStr(err));

        // Since we're failing the RegisterServicePairDevice request, clear the persisted service configuration.
        ConfigurationMgr().ClearServiceProvisioningData();

        // Choose an appropriate StatusReport to return if not already given.
        if (statusReportProfileId == 0 && statusReportStatusCode == 0)
        {
            if (err == WEAVE_ERROR_TIMEOUT)
            {
                statusReportProfileId = kWeaveProfile_ServiceProvisioning;
                statusReportStatusCode = Profiles::ServiceProvisioning::kStatusCode_ServiceCommuncationError;
            }
            else
            {
                statusReportProfileId = kWeaveProfile_Common;
                statusReportStatusCode = Profiles::Common::kStatus_InternalServerProblem;
            }
        }

        // Send an error StatusReport back to the client.  Only include the local error code if it isn't
        // WEAVE_ERROR_STATUS_REPORT_RECEIVED.
        SendStatusReport(statusReportProfileId, statusReportStatusCode,
                (err != WEAVE_ERROR_STATUS_REPORT_RECEIVED) ? err : WEAVE_NO_ERROR);
    }
}

void ServiceProvisioningServer::AsyncStartPairDeviceToAccount(intptr_t arg)
{
    sInstance.StartPairDeviceToAccount();
}

void ServiceProvisioningServer::HandleServiceTunnelTimeout(System::Layer * /* unused */, void * /* unused */, System::Error /* unused */)
{
    sInstance.HandlePairDeviceToAccountResult(WEAVE_ERROR_TIMEOUT, 0, 0);
}

void ServiceProvisioningServer::HandleProvServiceBindingEvent(void * appState, Binding::EventType eventType,
            const Binding::InEventParam & inParam, Binding::OutEventParam & outParam)
{
    uint32_t statusReportProfileId;
    uint16_t statusReportStatusCode;

    switch (eventType)
    {
    case Binding::kEvent_BindingReady:
        sInstance.SendPairDeviceToAccountRequest();
        break;
    case Binding::kEvent_PrepareFailed:
        if (inParam.PrepareFailed.StatusReport != NULL)
        {
            statusReportProfileId = inParam.PrepareFailed.StatusReport->mProfileId;
            statusReportStatusCode = inParam.PrepareFailed.StatusReport->mStatusCode;
        }
        else
        {
            statusReportProfileId = kWeaveProfile_ServiceProvisioning;
            statusReportStatusCode = Profiles::ServiceProvisioning::kStatusCode_ServiceCommuncationError;
        }
        sInstance.HandlePairDeviceToAccountResult(inParam.PrepareFailed.Reason,
                statusReportProfileId, statusReportStatusCode);
        break;
    default:
        Binding::DefaultEventHandler(appState, eventType, inParam, outParam);
        break;
    }
}

#else // !WEAVE_DEVICE_CONFIG_DISABLE_ACCOUNT_PAIRING

void ServiceProvisioningServer::HandlePairDeviceToAccountResult(WEAVE_ERROR err, uint32_t statusReportProfileId, uint16_t statusReportStatusCode)
{
}

#endif // !WEAVE_DEVICE_CONFIG_DISABLE_ACCOUNT_PAIRING

#if WEAVE_CONFIG_ENABLE_IFJ_SERVICE_FABRIC_JOIN
void ServiceProvisioningServer::HandleIFJServiceFabricJoinResult(WEAVE_ERROR err, uint32_t statusReportProfileId, uint16_t statusReportStatusCode)
{
}
#endif // WEAVE_CONFIG_ENABLE_IFJ_SERVICE_FABRIC_JOIN

} // namespace Internal
} // namespace DeviceLayer
} // namespace Weave
} // namespace nl
