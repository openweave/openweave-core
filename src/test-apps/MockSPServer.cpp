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
 *      This file implements a derived unsolicited responder
 *      (i.e., server) for the Weave Service Provisioning profile used
 *      for the Weave mock device command line functional testing
 *      tool.
 *
 */

#define __STDC_FORMAT_MACROS

#include <inttypes.h>
#include <stdio.h>

#include "ToolCommon.h"
#include "MockSPServer.h"
#include "MockDDServer.h"
#include <Weave/Support/CodeUtils.h>
#include <Weave/Core/WeaveTLV.h>
#include <Weave/Profiles/WeaveProfiles.h>
#include <Weave/Profiles/device-description/DeviceDescription.h>
#include <Weave/Profiles/common/CommonProfile.h>
#include <Weave/Support/ErrorStr.h>
#include "CASEOptions.h"

using namespace nl::Weave;
using namespace nl::Weave::TLV;
using namespace nl::Weave::Profiles;
using namespace nl::Weave::Profiles::ServiceProvisioning;
using namespace nl::Weave::Profiles::DeviceDescription;

extern MockDeviceDescriptionServer MockDDServer;

MockServiceProvisioningServer::MockServiceProvisioningServer()
{
    mPersistedServiceId = 0;
    mPersistedAccountId = NULL;
    mPersistedServiceConfig = NULL;
    mPersistedServiceConfigLen = 0;
    mPairingServerCon = NULL;
    mPairingServerBinding = NULL;
}

WEAVE_ERROR MockServiceProvisioningServer::Init(WeaveExchangeManager *exchangeMgr)
{
    WEAVE_ERROR err;

    // Initialize the base class.
    err = this->ServiceProvisioningServer::Init(exchangeMgr);
    SuccessOrExit(err);

    // Tell the base class that it should delegate service provisioning requests to us.
    SetDelegate(this);

    static char defaultPairingServerAddr[64];
    strcpy(defaultPairingServerAddr, "127.0.0.1");

#if WEAVE_CONFIG_ENABLE_TARGETED_LISTEN
    if (FabricState->ListenIPv4Addr == IPAddress::Any)
    {
        if (FabricState->ListenIPv6Addr != IPAddress::Any)
            FabricState->ListenIPv6Addr.ToString(defaultPairingServerAddr, sizeof(defaultPairingServerAddr));
    }
    else
        FabricState->ListenIPv4Addr.ToString(defaultPairingServerAddr, sizeof(defaultPairingServerAddr));
#endif

    PairingEndPointId = FabricState->LocalNodeId;
    PairingServerAddr = defaultPairingServerAddr;

    // Clear our state.
    mPersistedServiceId = 0;
    mPersistedAccountId = NULL;
    mPersistedServiceConfig = NULL;
    mPersistedServiceConfigLen = 0;
    mPairingServerCon = NULL;
    mPairingServerBinding = NULL;

exit:
    return err;
}

WEAVE_ERROR MockServiceProvisioningServer::Shutdown()
{
    ClearPersistedService();
    return this->ServiceProvisioningServer::Shutdown();
}

void MockServiceProvisioningServer::Reset()
{
    ClearPersistedService();
}

void MockServiceProvisioningServer::Preconfig()
{
    // This dummy service config object contains the following information:
    //
    //    Trusted Certificates:
    //        The Nest Development Root Certificate
    //        A dummy "account" certificate with a common name of "DUMMY-ACCOUNT-ID" (see below)
    //
    //    Directory End Point:
    //        Endpoint Id: 18B4300200000001 (the service directory endpoint)
    //        Endpoint Host Name: frontdoor.integration.nestlabs.com
    //        Endpoint Port: 11095 (the weave default port)
    //
    // The dummy account certificate is:
    //
    //    1QAABAABADABCE4vMktB1zrbJAIENwMsgRBEVU1NWS1BQ0NPVU5ULUlEGCYEy6j6GyYFSzVPQjcG
    //    LIEQRFVNTVktQUNDT1VOVC1JRBgkBwImCCUAWiMwCjkEK9nbWmLvurFTKg+ZY7eKMMWKQSmlGU5L
    //    C/N+2sXpszXwdRhtSV2GxEQlB0G006nv7rQq1gpdneA1gykBGDWCKQEkAgUYNYQpATYCBAIEARgY
    //    NYEwAghCPJVfRh5S2xg1gDACCEI8lV9GHlLbGDUMMAEdAIphhmI9F7LSz9JtOT3kJWngkeoFanXO
    //    3UXrg88wAhx0tCukbRRlt7dxmlqvZNKIYG6zsaAxypJvyvJDGBg=
    //
    // The corresponding private key is:
    //
    //    1QAABAACACYBJQBaIzACHLr840+Gv3w4EnAr+aMQv0+b8+8wD6VETUI6Z2owAzkEK9nbWmLvurFT
    //    Kg+ZY7eKMMWKQSmlGU5LC/N+2sXpszXwdRhtSV2GxEQlB0G006nv7rQq1gpdneAY
    //
    // The following is a fabric access token containing the dummy account certificate and
    // private key.  This can be used to authenticate to the mock device when it has been
    // configured to use the dummy service config.
    //
    //    1QAABAAJADUBMAEITi8yS0HXOtskAgQ3AyyBEERVTU1ZLUFDQ09VTlQtSUQYJgTLqPobJgVLNU9C
    //    NwYsgRBEVU1NWS1BQ0NPVU5ULUlEGCQHAiYIJQBaIzAKOQQr2dtaYu+6sVMqD5ljt4owxYpBKaUZ
    //    TksL837axemzNfB1GG1JXYbERCUHQbTTqe/utCrWCl2d4DWDKQEYNYIpASQCBRg1hCkBNgIEAgQB
    //    GBg1gTACCEI8lV9GHlLbGDWAMAIIQjyVX0YeUtsYNQwwAR0AimGGYj0XstLP0m05PeQlaeCR6gVq
    //    dc7dReuDzzACHHS0K6RtFGW3t3GaWq9k0ohgbrOxoDHKkm/K8kMYGDUCJgElAFojMAIcuvzjT4a/
    //    fDgScCv5oxC/T5vz7zAPpURNQjpnajADOQQr2dtaYu+6sVMqD5ljt4owxYpBKaUZTksL837axemz
    //    NfB1GG1JXYbERCUHQbTTqe/utCrWCl2d4BgY
    //
    //
    static const char dummyAccountId[] = "DUMMY-ACCOUNT-ID";
    static const uint8_t dummyServiceConfig[] =
    {
        0xd5, 0x00, 0x00, 0x0f, 0x00, 0x01, 0x00, 0x36, 0x01, 0x15, 0x30, 0x01, 0x08, 0x4e, 0x2f, 0x32,
        0x4b, 0x41, 0xd7, 0x3a, 0xdb, 0x24, 0x02, 0x04, 0x37, 0x03, 0x2c, 0x81, 0x10, 0x44, 0x55, 0x4d,
        0x4d, 0x59, 0x2d, 0x41, 0x43, 0x43, 0x4f, 0x55, 0x4e, 0x54, 0x2d, 0x49, 0x44, 0x18, 0x26, 0x04,
        0xcb, 0xa8, 0xfa, 0x1b, 0x26, 0x05, 0x4b, 0x35, 0x4f, 0x42, 0x37, 0x06, 0x2c, 0x81, 0x10, 0x44,
        0x55, 0x4d, 0x4d, 0x59, 0x2d, 0x41, 0x43, 0x43, 0x4f, 0x55, 0x4e, 0x54, 0x2d, 0x49, 0x44, 0x18,
        0x24, 0x07, 0x02, 0x26, 0x08, 0x25, 0x00, 0x5a, 0x23, 0x30, 0x0a, 0x39, 0x04, 0x2b, 0xd9, 0xdb,
        0x5a, 0x62, 0xef, 0xba, 0xb1, 0x53, 0x2a, 0x0f, 0x99, 0x63, 0xb7, 0x8a, 0x30, 0xc5, 0x8a, 0x41,
        0x29, 0xa5, 0x19, 0x4e, 0x4b, 0x0b, 0xf3, 0x7e, 0xda, 0xc5, 0xe9, 0xb3, 0x35, 0xf0, 0x75, 0x18,
        0x6d, 0x49, 0x5d, 0x86, 0xc4, 0x44, 0x25, 0x07, 0x41, 0xb4, 0xd3, 0xa9, 0xef, 0xee, 0xb4, 0x2a,
        0xd6, 0x0a, 0x5d, 0x9d, 0xe0, 0x35, 0x83, 0x29, 0x01, 0x18, 0x35, 0x82, 0x29, 0x01, 0x24, 0x02,
        0x05, 0x18, 0x35, 0x84, 0x29, 0x01, 0x36, 0x02, 0x04, 0x02, 0x04, 0x01, 0x18, 0x18, 0x35, 0x81,
        0x30, 0x02, 0x08, 0x42, 0x3c, 0x95, 0x5f, 0x46, 0x1e, 0x52, 0xdb, 0x18, 0x35, 0x80, 0x30, 0x02,
        0x08, 0x42, 0x3c, 0x95, 0x5f, 0x46, 0x1e, 0x52, 0xdb, 0x18, 0x35, 0x0c, 0x30, 0x01, 0x1d, 0x00,
        0x8a, 0x61, 0x86, 0x62, 0x3d, 0x17, 0xb2, 0xd2, 0xcf, 0xd2, 0x6d, 0x39, 0x3d, 0xe4, 0x25, 0x69,
        0xe0, 0x91, 0xea, 0x05, 0x6a, 0x75, 0xce, 0xdd, 0x45, 0xeb, 0x83, 0xcf, 0x30, 0x02, 0x1c, 0x74,
        0xb4, 0x2b, 0xa4, 0x6d, 0x14, 0x65, 0xb7, 0xb7, 0x71, 0x9a, 0x5a, 0xaf, 0x64, 0xd2, 0x88, 0x60,
        0x6e, 0xb3, 0xb1, 0xa0, 0x31, 0xca, 0x92, 0x6f, 0xca, 0xf2, 0x43, 0x18, 0x18, 0x15, 0x30, 0x01,
        0x09, 0x00, 0xa8, 0x34, 0x22, 0xe9, 0xd9, 0x75, 0xe4, 0x55, 0x24, 0x02, 0x04, 0x57, 0x03, 0x00,
        0x27, 0x13, 0x01, 0x00, 0x00, 0xee, 0xee, 0x30, 0xb4, 0x18, 0x18, 0x26, 0x04, 0x95, 0x23, 0xa9,
        0x19, 0x26, 0x05, 0x15, 0xc1, 0xd2, 0x2c, 0x57, 0x06, 0x00, 0x27, 0x13, 0x01, 0x00, 0x00, 0xee,
        0xee, 0x30, 0xb4, 0x18, 0x18, 0x24, 0x07, 0x02, 0x24, 0x08, 0x15, 0x30, 0x0a, 0x31, 0x04, 0x78,
        0x52, 0xe2, 0x9c, 0x92, 0xba, 0x70, 0x19, 0x58, 0x46, 0x6d, 0xae, 0x18, 0x72, 0x4a, 0xfb, 0x43,
        0x0d, 0xf6, 0x07, 0x29, 0x33, 0x0d, 0x61, 0x55, 0xe5, 0x65, 0x46, 0x8e, 0xba, 0x0d, 0xa5, 0x3f,
        0xb5, 0x17, 0xc0, 0x47, 0x64, 0x44, 0x02, 0x18, 0x4f, 0xa8, 0x11, 0x24, 0x50, 0xd4, 0x7b, 0x35,
        0x83, 0x29, 0x01, 0x29, 0x02, 0x18, 0x35, 0x82, 0x29, 0x01, 0x24, 0x02, 0x60, 0x18, 0x35, 0x81,
        0x30, 0x02, 0x08, 0x42, 0x0c, 0xac, 0xf6, 0xb4, 0x64, 0x71, 0xe6, 0x18, 0x35, 0x80, 0x30, 0x02,
        0x08, 0x42, 0x0c, 0xac, 0xf6, 0xb4, 0x64, 0x71, 0xe6, 0x18, 0x35, 0x0c, 0x30, 0x01, 0x19, 0x00,
        0xbe, 0x0e, 0xda, 0xa1, 0x63, 0x5a, 0x8e, 0xf1, 0x52, 0x17, 0x45, 0x80, 0xbd, 0xdc, 0x94, 0x12,
        0xd4, 0xcc, 0x1c, 0x2c, 0x33, 0x4e, 0x29, 0xdc, 0x30, 0x02, 0x19, 0x00, 0x8b, 0xe7, 0xee, 0x2e,
        0x11, 0x17, 0x14, 0xae, 0x92, 0xda, 0x2b, 0x3b, 0x6d, 0x2f, 0xd7, 0x5d, 0x9e, 0x5f, 0xcd, 0xb8,
        0xba, 0x2f, 0x65, 0x76, 0x18, 0x18, 0x18, 0x35, 0x02, 0x27, 0x01, 0x01, 0x00, 0x00, 0x00, 0x02,
        0x30, 0xb4, 0x18, 0x36, 0x02, 0x15, 0x2c, 0x01, 0x22, 0x66, 0x72, 0x6f, 0x6e, 0x74, 0x64, 0x6f,
        0x6f, 0x72, 0x2e, 0x69, 0x6e, 0x74, 0x65, 0x67, 0x72, 0x61, 0x74, 0x69, 0x6f, 0x6e, 0x2e, 0x6e,
        0x65, 0x73, 0x74, 0x6c, 0x61, 0x62, 0x73, 0x2e, 0x63, 0x6f, 0x6d, 0x18, 0x18, 0x18, 0x18
    };

    ClearPersistedService();
    PersistNewService(0x18B4300100000001ULL, dummyAccountId, strlen(dummyAccountId), dummyServiceConfig, sizeof(dummyServiceConfig));
}

WEAVE_ERROR MockServiceProvisioningServer::HandleRegisterServicePairAccount(RegisterServicePairAccountMessage& msg)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    char ipAddrStr[64];
    mCurClientOp->PeerAddr.ToString(ipAddrStr, sizeof(ipAddrStr));

    // NOTE: The arguments to HandleRegisterServicePairAccount() are temporary copies which
    // must be copied or discarded by the time the function returns.

    printf("RegisterServicePairAccount request received from node %" PRIX64 " (%s)\n", mCurClientOp->PeerNodeId, ipAddrStr);
    printf("  Service Id: %016" PRIX64 "\n", msg.ServiceId);
    printf("  Account Id: "); fwrite(msg.AccountId, 1, msg.AccountIdLen, stdout); printf("\n");
    printf("  Service Config (%d bytes): \n", (int)msg.ServiceConfigLen);
    DumpMemory(msg.ServiceConfig, msg.ServiceConfigLen, "    ", 16);
    printf("  Pairing Token (%d bytes): \n", (int)msg.PairingTokenLen);
    DumpMemory(msg.PairingToken, msg.PairingTokenLen, "    ", 16);
    printf("  Pairing Init Data (%d bytes): \n", (int)msg.PairingInitDataLen);
    DumpMemory(msg.PairingInitData, msg.PairingInitDataLen, "    ", 16);

    // Verify that the new service id does not match an existing service.
    //
    // Services cannot be re-registered; they must be updated or unregistered.
    if (mPersistedServiceId == msg.ServiceId)
    {
        SendStatusReport(kWeaveProfile_ServiceProvisioning, kStatusCode_ServiceAlreadyRegistered);
        ExitNow();
    }

    // If we've reached the maximum number of provisioned services return a TooManyServices error.
    //
    // The Mock device only supports a single provisioned service. This will be true for Topaz 1.0 as well. However other
    // types of devices may support multiple provisioned services up to some limit.
    if (mPersistedServiceConfig != NULL)
    {
        SendStatusReport(kWeaveProfile_ServiceProvisioning, kStatusCode_TooManyServices);
        ExitNow();
    }

    // Validate the service config. We don't want to get any further along before making sure the data is good.
    if (!ServiceProvisioningServer::IsValidServiceConfig(msg.ServiceConfig, msg.ServiceConfigLen))
    {
        SendStatusReport(kWeaveProfile_ServiceProvisioning, kStatusCode_InvalidServiceConfig);
        ExitNow();
    }

    if (kPairingTransport_TCP == PairingTransport)
    {
        // At this point, the device must send a PairDeviceToAccount request to the service endpoint that handles
        // device pairing.  The process for doing this is roughly as follows...
        //
        //    1 - Use the directory endpoint address in the service config to connect and authenticate to the service's
        //        directory server.  Once connected, request the pairing service endpoint using the Directory Protocol.
        //
        //    2 - Connect and authenticate to the pairing server and issue a PairDeviceToAccount request containing:
        //           -- Account Id
        //           -- Device's Fabric Id
        //           -- Service Pairing Token
        //           -- Pairing Initialization Data
        //
        //    3 - Pairing service will respond with a StatusReport message indicating success or error. If an error is
        //        returned, the StatusReport is returned to the application that made the RegisterServicePairAccount
        //        request.
        //
        // NOTE that the steps above that require authentication will require the device to extract and use the service
        // CA certificates contained in the supplied service configuration data.

        // Initiate a connection to the configured pairing server.
        err = StartConnectToPairingServer();
        SuccessOrExit(err);
    }
    else if (kPairingTransport_WRM == PairingTransport)
    {
        err = PrepareBindingForPairingServer();
        SuccessOrExit(err);
    }
    else
    {
        err = WEAVE_ERROR_INCORRECT_STATE;
    }

exit:
    return err;
}

WEAVE_ERROR MockServiceProvisioningServer::PrepareBindingForPairingServer()
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    IPAddress endPointAddr;

    VerifyOrExit(IPAddress::FromString(PairingServerAddr, endPointAddr), err = WEAVE_ERROR_INVALID_ADDRESS);

    mPairingServerBinding = ExchangeMgr->NewBinding(HandlePairingServerBindingEvent, this);
    VerifyOrExit(mPairingServerBinding != NULL, err = WEAVE_ERROR_NO_MEMORY);

    // Prepare the binding. Will finish asynchronously.
    // TODO: [TT] PairingEndPointId appears to default to the local node id.
    //            Shouldn't it default to kServiceEndpoint_ServiceProvisioning instead,
    //            if this is how it's used?
    err = mPairingServerBinding->BeginConfiguration()
            .Target_NodeId(PairingEndPointId)
            .TargetAddress_IP(endPointAddr)
            .Transport_UDP_WRM()
            .Security_None()
            .PrepareBinding();
    SuccessOrExit(err);

exit:
    if (err != WEAVE_NO_ERROR)
    {
        if (mPairingServerBinding != NULL)
        {
            mPairingServerBinding->Release();
            mPairingServerBinding = NULL;
        }
    }
    return err;
}

void MockServiceProvisioningServer::HandlePairingServerBindingEvent(void *const appState,
                                                                    const nl::Weave::Binding::EventType event,
                                                                    const nl::Weave::Binding::InEventParam &inParam,
                                                                    nl::Weave::Binding::OutEventParam &outParam)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    WeaveDeviceDescriptor deviceDesc;
    uint8_t deviceInitData[256];
    uint32_t deviceInitDataLen;
    MockServiceProvisioningServer *server = (MockServiceProvisioningServer *) appState;

    // Retrieve the original RegisterServicePairAccount message from the client.
    RegisterServicePairAccountMessage &msg = server->mCurClientOpMsg.RegisterServicePairAccount;

    switch (event)
    {
    case nl::Weave::Binding::kEvent_BindingReady:
        printf("Pairing server binding ready\n");
        // Continues below.
        break;
    case nl::Weave::Binding::kEvent_PrepareFailed:
        printf("Pairing server binding prepare failed: %s\n", nl::ErrorStr(inParam.PrepareFailed.Reason));
        ExitNow(err = inParam.PrepareFailed.Reason);
        break;
    case nl::Weave::Binding::kEvent_BindingFailed:
        printf("Pairing server binding failed: %s\n", nl::ErrorStr(inParam.BindingFailed.Reason));
        ExitNow(err = inParam.BindingFailed.Reason);
        break;
    default:
        nl::Weave::Binding::DefaultEventHandler(appState, event, inParam, outParam);
        ExitNow(err = WEAVE_NO_ERROR);
        break;
    }

    printf("Sending WRM PairDeviceToAccount request to pairing server\n");

    // Encode device descriptor and send as device init data.
    gDeviceDescOptions.GetDeviceDesc(deviceDesc);
    WeaveDeviceDescriptor::EncodeTLV(deviceDesc, deviceInitData, sizeof(deviceInitData), deviceInitDataLen);

    // Send a PairDeviceToAccount request to the pairing server via WRM.
    err = server->SendPairDeviceToAccountRequest(server->mPairingServerBinding,
        msg.ServiceId, server->FabricState->FabricId,
        msg.AccountId, msg.AccountIdLen,
        msg.PairingToken, msg.PairingTokenLen,
        msg.PairingInitData, msg.PairingInitDataLen,
        deviceInitData, deviceInitDataLen);
    SuccessOrExit(err);

exit:
    if (err != WEAVE_NO_ERROR)
    {
        server->SendStatusReport(kWeaveProfile_ServiceProvisioning, kStatusCode_PairingServerError, err);
        if (server->mPairingServerBinding != NULL)
        {
            server->mPairingServerBinding->Release();
            server->mPairingServerBinding = NULL;
        }
    }
}

WEAVE_ERROR MockServiceProvisioningServer::StartConnectToPairingServer()
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    WeaveConnection *con = NULL;
    IPAddress endPointAddr;

    printf("Initiating connection to pairing server at %s\n", PairingServerAddr);

    if (!IPAddress::FromString(PairingServerAddr, endPointAddr))
        ExitNow(err = WEAVE_ERROR_INVALID_ADDRESS);

    con = ExchangeMgr->MessageLayer->NewConnection();
    VerifyOrExit(con != NULL, err = WEAVE_ERROR_TOO_MANY_CONNECTIONS);

    con->AppState = this;
    con->OnConnectionComplete = HandlePairingServerConnectionComplete;
    con->OnConnectionClosed = HandlePairingServerConnectionClosed;

    // TODO: [TT] PairingEndPointId appears to default to the local node id.
    //            Shouldn't it default to kServiceEndpoint_ServiceProvisioning instead,
    //            if this is how it's used?
    err = con->Connect(PairingEndPointId, endPointAddr);
    SuccessOrExit(err);

    mPairingServerCon = con;
    con = NULL;

exit:
    if (con != NULL)
        con->Close();
    return err;
}

void MockServiceProvisioningServer::HandlePairingServerConnectionComplete(WeaveConnection *con, WEAVE_ERROR conErr)
{
    WEAVE_ERROR err;
    MockServiceProvisioningServer *server = (MockServiceProvisioningServer *) con->AppState;

    // If the connection failed, clean-up and deliver a failure back to the client.
    if (conErr != WEAVE_NO_ERROR)
    {
        printf("Connection to pairing server failed: %s\n", nl::ErrorStr(conErr));

        server->mPairingServerCon->Close();
        server->mPairingServerCon = NULL;
        server->SendStatusReport(kWeaveProfile_ServiceProvisioning, kStatusCode_PairingServerError, conErr);
        return;
    }

    printf("Connection to pairing server established\n");

    // Retrieve the original RegisterServicePairAccount message from the client.
    RegisterServicePairAccountMessage &clientMsg = server->mCurClientOpMsg.RegisterServicePairAccount;

    printf("Sending TCP PairDeviceToAccount request to pairing server\n");

    // Encode device descriptor and send as device init data.
    WeaveDeviceDescriptor deviceDesc;
    gDeviceDescOptions.GetDeviceDesc(deviceDesc);
    uint8_t deviceInitData[256];
    uint32_t deviceInitDataLen;
    WeaveDeviceDescriptor::EncodeTLV(deviceDesc, deviceInitData, sizeof(deviceInitData), deviceInitDataLen);

    // Send a PairDeviceToAccount request to the pairing server.
    err = server->SendPairDeviceToAccountRequest(server->mPairingServerCon, clientMsg.ServiceId, server->FabricState->FabricId,
            clientMsg.AccountId, clientMsg.AccountIdLen,
            clientMsg.PairingToken, clientMsg.PairingTokenLen,
            clientMsg.PairingInitData, clientMsg.PairingInitDataLen,
            deviceInitData, deviceInitDataLen
            );

    if (err != WEAVE_NO_ERROR)
    {
        server->mPairingServerCon->Close();
        server->mPairingServerCon = NULL;
        server->SendStatusReport(kWeaveProfile_Common, Common::kStatus_InternalError, err);
        return;
    }
}

void MockServiceProvisioningServer::HandlePairDeviceToAccountResult(WEAVE_ERROR err, uint32_t serverStatusProfileId, uint16_t serverStatusCode)
{
    if (mPairingServerCon)
    {
        // The server operation is now complete so close the connection.
        mPairingServerCon->Close();
        mPairingServerCon = NULL;
    }
    else if (mPairingServerBinding)
    {
        // The server operation is now complete, so release the binding.
        mPairingServerBinding->Release();
        mPairingServerBinding = NULL;
    }

    // If the PairDeviceToAccount request was successful...
    if (err == WEAVE_NO_ERROR)
    {
        printf("Received success response from pairing server\n");

        // Retrieve the original RegisterServicePairAccount message from the client.
        RegisterServicePairAccountMessage &clientMsg = mCurClientOpMsg.RegisterServicePairAccount;

        // Save the service information in device persistent storage.
        // (On the mock device we merely store it in memory).
        //
        err = PersistNewService(clientMsg.ServiceId, clientMsg.AccountId, clientMsg.AccountIdLen, clientMsg.ServiceConfig, clientMsg.ServiceConfigLen);
        if (err != WEAVE_NO_ERROR)
        {
            SendStatusReport(kWeaveProfile_Common, Common::kStatus_InternalError, err);
            return;
        }

        SendSuccessResponse();
    }

    // Otherwwise, relay the result from the pairing server back to the client.
    else if (err == WEAVE_ERROR_STATUS_REPORT_RECEIVED)
    {
        printf("Received StatusReport from pairing server: %s\n", nl::StatusReportStr(serverStatusProfileId, serverStatusCode));
        SendStatusReport(serverStatusProfileId, serverStatusCode, WEAVE_NO_ERROR);
    }
    else
    {
        printf("Error talking to pairing server: %s\n", nl::ErrorStr(err));
        SendStatusReport(kWeaveProfile_ServiceProvisioning, kStatusCode_PairingServerError, err);
    }
}

#if WEAVE_CONFIG_ENABLE_IFJ_SERVICE_FABRIC_JOIN
void MockServiceProvisioningServer::HandleIFJServiceFabricJoinResult(WEAVE_ERROR err, uint32_t serverStatusProfileId, uint16_t serverStatusCode)
{
    if (mPairingServerBinding)
    {
        // The server operation is now complete, so release the binding.
        mPairingServerBinding->Release();
        mPairingServerBinding = NULL;
    }

    // If the IFJServiceFabricJoin request was successful...
    if (err == WEAVE_NO_ERROR)
    {
        printf("Received success response from server\n");

    }

    // Otherwwise, relay the result from the pairing server back to the client.
    else if (err == WEAVE_ERROR_STATUS_REPORT_RECEIVED)
    {
        printf("Received StatusReport from server: %s\n", nl::StatusReportStr(serverStatusProfileId, serverStatusCode));
    }
    else
    {
        printf("Error talking to server: %s\n", nl::ErrorStr(err));
    }
}
#endif // WEAVE_CONFIG_ENABLE_IFJ_SERVICE_FABRIC_JOIN

void MockServiceProvisioningServer::EnforceAccessControl(nl::Weave::ExchangeContext *ec, uint32_t msgProfileId, uint8_t msgType,
            const nl::Weave::WeaveMessageInfo *msgInfo, AccessControlResult& result)
{
    if (sSuppressAccessControls)
    {
        result = kAccessControlResult_Accepted;
    }

    ServiceProvisioningDelegate::EnforceAccessControl(ec, msgProfileId, msgType, msgInfo, result);
}

bool MockServiceProvisioningServer::IsPairedToAccount() const
{
    return (gCASEOptions.ServiceConfig != NULL);
}

void MockServiceProvisioningServer::HandlePairingServerConnectionClosed(WeaveConnection *con, WEAVE_ERROR conErr)
{
    MockServiceProvisioningServer *server = (MockServiceProvisioningServer *) con->AppState;
    if (server->mPairingServerCon != NULL)
    {
        server->mPairingServerCon->Close();
        server->mPairingServerCon = NULL;
    }
}

WEAVE_ERROR MockServiceProvisioningServer::HandleUpdateService(UpdateServiceMessage& msg)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    char ipAddrStr[64];
    mCurClientOp->PeerAddr.ToString(ipAddrStr, sizeof(ipAddrStr));

    // NOTE: The arguments to HandleUpdateService() are temporary copies which
    // must be copied or discarded by the time the function returns.

    printf("UpdateService request received from node %" PRIX64 " (%s)\n", mCurClientOp->PeerNodeId, ipAddrStr);
    printf("  Service Id: %016" PRIX64 "\n", msg.ServiceId);
    printf("  Service Config (%d bytes): \n", (int)msg.ServiceConfigLen);
    DumpMemory(msg.ServiceConfig, msg.ServiceConfigLen, "    ", 16);

    // Verify that the service id matches an existing service.
    //
    if (mPersistedServiceId != msg.ServiceId)
    {
        SendStatusReport(kWeaveProfile_ServiceProvisioning, kStatusCode_NoSuchService);
        ExitNow();
    }

    // Validate the service config. We don't want to get any further along before making sure the data is good.
    if (!ServiceProvisioningServer::IsValidServiceConfig(msg.ServiceConfig, msg.ServiceConfigLen))
    {
        SendStatusReport(kWeaveProfile_ServiceProvisioning, kStatusCode_InvalidServiceConfig);
        ExitNow();
    }

    // Save the new service configuration in device persistent storage, replacing the existing value.
    // (On the mock device we merely store it in memory).
    err = UpdatedPersistedService(msg.ServiceConfig, msg.ServiceConfigLen);
    SuccessOrExit(err);

    // Send a success StatusReport back to the requestor.
    err = SendSuccessResponse();
    SuccessOrExit(err);

exit:
    return err;
}

WEAVE_ERROR MockServiceProvisioningServer::HandleUnregisterService(uint64_t serviceId)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    char ipAddrStr[64];
    mCurClientOp->PeerAddr.ToString(ipAddrStr, sizeof(ipAddrStr));

    printf("UnregisterService request received from node %" PRIX64 " (%s)\n", mCurClientOp->PeerNodeId, ipAddrStr);
    printf("  Service Id: %016" PRIX64 "\n", serviceId);

    // Verify that the service id matches an existing service.
    //
    if (mPersistedServiceId != serviceId)
    {
        SendStatusReport(kWeaveProfile_ServiceProvisioning, kStatusCode_NoSuchService);
        ExitNow();
    }

    // Clear the persisted service.
    ClearPersistedService();

    // Send a success StatusReport back to the requestor.
    err = SendSuccessResponse();
    SuccessOrExit(err);

exit:
    return err;
}

WEAVE_ERROR MockServiceProvisioningServer::PersistNewService(uint64_t serviceId,
                                                             const char *accountId, uint16_t accountIdLen,
                                                             const uint8_t *serviceConfig, uint16_t serviceConfigLen)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    char *accountIdCopy = NULL;
    uint8_t *serviceConfigCopy = NULL;

    accountIdCopy = (char *)malloc(accountIdLen + 1);
    VerifyOrExit(accountIdCopy != NULL, err = WEAVE_ERROR_NO_MEMORY);
    memcpy(accountIdCopy, accountId, accountIdLen);
    accountIdCopy[accountIdLen] = 0;

    serviceConfigCopy = (uint8_t *)malloc(serviceConfigLen);
    VerifyOrExit(serviceConfigCopy != NULL, err = WEAVE_ERROR_NO_MEMORY);
    memcpy(serviceConfigCopy, serviceConfig, serviceConfigLen);

    mPersistedServiceId = serviceId;
    mPersistedAccountId = accountIdCopy;
    mPersistedServiceConfig = serviceConfigCopy;
    mPersistedServiceConfigLen = serviceConfigLen;

    // Setup to use service config in subsequence CASE sessions.
    gCASEOptions.ServiceConfig = mPersistedServiceConfig;
    gCASEOptions.ServiceConfigLength = mPersistedServiceConfigLen;

exit:
    if (err != WEAVE_NO_ERROR)
    {
        if (accountIdCopy != NULL)
            free(accountIdCopy);
        if (serviceConfigCopy != NULL)
            free(serviceConfigCopy);
    }
    return err;
}

WEAVE_ERROR MockServiceProvisioningServer::UpdatedPersistedService(const uint8_t *serviceConfig, uint16_t serviceConfigLen)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint8_t *serviceConfigCopy = NULL;

    serviceConfigCopy = (uint8_t *)malloc(serviceConfigLen);
    VerifyOrExit(serviceConfigCopy != NULL, err = WEAVE_ERROR_NO_MEMORY);
    memcpy(serviceConfigCopy, serviceConfig, serviceConfigLen);

    free(mPersistedServiceConfig);
    mPersistedServiceConfig = serviceConfigCopy;
    mPersistedServiceConfigLen = serviceConfigLen;

    // Setup to use service config in subsequence CASE sessions.
    gCASEOptions.ServiceConfig = mPersistedServiceConfig;
    gCASEOptions.ServiceConfigLength = mPersistedServiceConfigLen;

exit:
    if (err != WEAVE_NO_ERROR && serviceConfigCopy != NULL)
        free(serviceConfigCopy);
    return err;
}

void MockServiceProvisioningServer::ClearPersistedService()
{
    mPersistedServiceId = 0;
    if (mPersistedAccountId != NULL)
    {
        free(mPersistedAccountId);
        mPersistedAccountId = NULL;
    }
    if (mPersistedServiceConfig != NULL)
    {
        free(mPersistedServiceConfig);
        mPersistedServiceConfig = NULL;
    }
    mPersistedServiceConfigLen = 0;

    gCASEOptions.ServiceConfig = NULL;
    gCASEOptions.ServiceConfigLength = 0;
}

WEAVE_ERROR MockServiceProvisioningServer::SendStatusReport(uint32_t statusProfileId, uint16_t statusCode, WEAVE_ERROR sysError)
{
    if (statusProfileId == kWeaveProfile_Common && statusCode == Common::kStatus_Success)
        printf("Sending StatusReport: Success\n");
    else
    {
        printf("Sending StatusReport: %s\n", nl::StatusReportStr(statusProfileId, statusCode));
        if (sysError != WEAVE_NO_ERROR)
            printf("   System error: %s\n", nl::ErrorStr(sysError));
    }
    return this->ServiceProvisioningServer::SendStatusReport(statusProfileId, statusCode, sysError);
}
