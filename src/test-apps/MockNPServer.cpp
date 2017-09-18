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
 *      (i.e., server) for the Weave Network Provisioning profile used
 *      for the Weave mock device command line functional testing
 *      tool.
 *
 */

#define __STDC_LIMIT_MACROS
#define __STDC_FORMAT_MACROS

#include <inttypes.h>
#include <limits.h>
#include <stdio.h>

#include "ToolCommon.h"
#include "MockNPServer.h"
#include <Weave/Support/CodeUtils.h>
#include <Weave/Core/WeaveTLV.h>
#include <Weave/Profiles/WeaveProfiles.h>
#include <Weave/Profiles/common/CommonProfile.h>
#include "MockOpActions.h"

extern MockOpActions OpActions;

using namespace nl::Weave::TLV;
using namespace nl::Weave::Profiles;
using namespace nl::Weave::Profiles::NetworkProvisioning;

MockNetworkProvisioningServer::MockNetworkProvisioningServer()
{
    NextNetworkId = 1;

    // NOTE: If you change this code, be sure to adjust MockNetworkProvisioningServer::Preconfig()
    // accordingly.

    ScanResults[0].NetworkType = kNetworkType_WiFi;
    ScanResults[0].WiFiSSID = strdup("Wireless-1");
    ScanResults[0].WiFiMode = kWiFiMode_Managed;
    ScanResults[0].WiFiRole = kWiFiRole_Station;
    ScanResults[0].WiFiSecurityType = kWiFiSecurityType_None;
    ScanResults[0].WirelessSignalStrength = 30;

    ScanResults[1].NetworkType = kNetworkType_WiFi;
    ScanResults[1].WiFiSSID = strdup("Wireless-2");
    ScanResults[1].WiFiMode = kWiFiMode_Managed;
    ScanResults[1].WiFiRole = kWiFiRole_Station;
    ScanResults[1].WiFiSecurityType = kWiFiSecurityType_WEP;
    ScanResults[1].WirelessSignalStrength = 10;

    ScanResults[2].NetworkType = kNetworkType_WiFi;
    ScanResults[2].WiFiSSID = strdup("Wireless-3");
    ScanResults[2].WiFiMode = kWiFiMode_Managed;
    ScanResults[2].WiFiRole = kWiFiRole_Station;
    ScanResults[2].WiFiSecurityType = kWiFiSecurityType_WPAPersonal;
    ScanResults[2].WirelessSignalStrength = -11;

    ScanResults[3].NetworkType = kNetworkType_Thread;
    ScanResults[3].ThreadNetworkName = strdup("Thread-1");
    ScanResults[3].ThreadExtendedPANId = (uint8_t *)malloc(8);
    for (int i = 0; i < 8; i++)
        ScanResults[3].ThreadExtendedPANId[i] = i + 1;
}

WEAVE_ERROR MockNetworkProvisioningServer::Init(WeaveExchangeManager *exchangeMgr)
{
    WEAVE_ERROR err;

    err = this->NetworkProvisioningServer::Init(exchangeMgr);
    SuccessOrExit(err);

    SetDelegate(this);

exit:
    return err;
}

WEAVE_ERROR MockNetworkProvisioningServer::Shutdown()
{
    return this->NetworkProvisioningServer::Shutdown();
}

void MockNetworkProvisioningServer::Reset()
{
    for (int i = 0; i < kMaxProvisionedNetworks; i++)
        ProvisionedNetworks[i].Clear();
    for (int i = 0; i < kMaxScanResults; i++)
        ScanResults[i].NetworkId = -1;
    NextNetworkId = 1;
}

void MockNetworkProvisioningServer::Preconfig()
{
    Reset();

    ProvisionedNetworks[0].NetworkId = NextNetworkId++;
    ProvisionedNetworks[0].NetworkType = kNetworkType_WiFi;
    ProvisionedNetworks[0].WiFiSSID = strdup("Wireless-3");
    ProvisionedNetworks[0].WiFiMode = kWiFiMode_Managed;
    ProvisionedNetworks[0].WiFiRole = kWiFiRole_Station;
    ProvisionedNetworks[0].WiFiSecurityType = kWiFiSecurityType_WPAPersonal;
    ProvisionedNetworks[0].WiFiKey = (uint8_t *)strdup("apassword");
    ProvisionedNetworks[0].WiFiKeyLen = strlen((const char *)ProvisionedNetworks[0].WiFiKey);

    ProvisionedNetworks[1].NetworkId = NextNetworkId++;
    ProvisionedNetworks[1].NetworkType = kNetworkType_Thread;
    ProvisionedNetworks[1].ThreadNetworkName = strdup("Thread-1");
    ProvisionedNetworks[1].ThreadExtendedPANId = (uint8_t *)malloc(8);
    for (int i = 0; i < 8; i++)
        ProvisionedNetworks[1].ThreadExtendedPANId[i] = i + 1;
    ProvisionedNetworks[1].ThreadNetworkKey = (uint8_t *)strdup("thisisathreadkey"); // must be 16 bytes
    ProvisionedNetworks[1].ThreadNetworkKeyLen = strlen((const char *)ProvisionedNetworks[1].ThreadNetworkKey);
}

WEAVE_ERROR MockNetworkProvisioningServer::HandleScanNetworks(uint8_t networkType)
{
    {
        char ipAddrStr[64];
        mCurOp->PeerAddr.ToString(ipAddrStr, sizeof(ipAddrStr));
        printf("ScanNetworks request received from node %" PRIX64 " (%s)\n", mCurOp->PeerNodeId, ipAddrStr);
        printf("  Requested Network Type: %d\n", networkType);
    }

    mOpArgs.networkType = networkType;

    CompleteOrDelayCurrentOp("scan-networks");

    return WEAVE_NO_ERROR;
}

WEAVE_ERROR MockNetworkProvisioningServer::ValidateNetworkConfig(NetworkInfo& netConfig)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    if (netConfig.NetworkType == kNetworkType_NotSpecified)
    {
        printf("Invalid network configuration: network type not specified\n");
        err = SendStatusReport(kWeaveProfile_NetworkProvisioning, kStatusCode_InvalidNetworkConfiguration);
        SuccessOrExit(err);
        ExitNow(err = WEAVE_ERROR_INVALID_ARGUMENT);
    }

    if (netConfig.NetworkType == kNetworkType_WiFi)
    {
        if (netConfig.WiFiSSID == NULL)
        {
            printf("Invalid network configuration: Missing WiFi SSID\n");
            err = SendStatusReport(kWeaveProfile_NetworkProvisioning, kStatusCode_InvalidNetworkConfiguration);
            SuccessOrExit(err);
            ExitNow(err = WEAVE_ERROR_INVALID_ARGUMENT);
        }

        if (netConfig.WiFiMode == kWiFiMode_NotSpecified)
        {
            printf("Invalid network configuration: Missing WiFi Mode\n");
            err = SendStatusReport(kWeaveProfile_NetworkProvisioning, kStatusCode_InvalidNetworkConfiguration);
            SuccessOrExit(err);
            ExitNow(err = WEAVE_ERROR_INVALID_ARGUMENT);
        }

        if (netConfig.WiFiRole == kWiFiRole_NotSpecified)
        {
            printf("Invalid network configuration: Missing WiFi Role\n");
            err = SendStatusReport(kWeaveProfile_NetworkProvisioning, kStatusCode_InvalidNetworkConfiguration);
            SuccessOrExit(err);
            ExitNow(err = WEAVE_ERROR_INVALID_ARGUMENT);
        }

        if (netConfig.WiFiSecurityType == kWiFiSecurityType_NotSpecified)
        {
            printf("Invalid network configuration: Missing WiFi Security Type\n");
            err = SendStatusReport(kWeaveProfile_NetworkProvisioning, kStatusCode_InvalidNetworkConfiguration);
            SuccessOrExit(err);
            ExitNow(err = WEAVE_ERROR_INVALID_ARGUMENT);
        }

        if (netConfig.WiFiMode != kWiFiMode_Managed)
        {
            printf("Unsupported WiFi Mode: %d\n", netConfig.WiFiMode);
            err = SendStatusReport(kWeaveProfile_NetworkProvisioning, kStatusCode_UnsupportedWiFiMode);
            SuccessOrExit(err);
            ExitNow(err = WEAVE_ERROR_INVALID_ARGUMENT);
        }

        if (netConfig.WiFiRole != kWiFiRole_Station)
        {
            printf("Unsupported WiFi Role: %d\n", netConfig.WiFiRole);
            err = SendStatusReport(kWeaveProfile_NetworkProvisioning, kStatusCode_UnsupportedWiFiRole);
            SuccessOrExit(err);
            ExitNow(err = WEAVE_ERROR_INVALID_ARGUMENT);
        }

        if (netConfig.WiFiSecurityType != kWiFiSecurityType_None && netConfig.WiFiSecurityType != kWiFiSecurityType_WEP
                && netConfig.WiFiSecurityType != kWiFiSecurityType_WPAPersonal
                && netConfig.WiFiSecurityType != kWiFiSecurityType_WPA2Personal
                && netConfig.WiFiSecurityType != kWiFiSecurityType_WPA2MixedPersonal)
        {
            printf("Unsupported WiFi Security Type: %d\n", netConfig.WiFiSecurityType);
            err = SendStatusReport(kWeaveProfile_NetworkProvisioning, kStatusCode_UnsupportedWiFiSecurityType);
            SuccessOrExit(err);
            ExitNow(err = WEAVE_ERROR_INVALID_ARGUMENT);
        }

        if (netConfig.WiFiSecurityType != kWiFiSecurityType_None && netConfig.WiFiKey == NULL)
        {
            printf("Invalid network configuration: Missing WiFi Key\n");
            err = SendStatusReport(kWeaveProfile_NetworkProvisioning, kStatusCode_InvalidNetworkConfiguration);
            SuccessOrExit(err);
            ExitNow(err = WEAVE_ERROR_INVALID_ARGUMENT);
        }
    }

    else if (netConfig.NetworkType == kNetworkType_Thread)
    {
        if (netConfig.ThreadNetworkName == NULL)
        {
            printf("Invalid network configuration: Missing Thread network name\n");
            err = SendStatusReport(kWeaveProfile_NetworkProvisioning, kStatusCode_InvalidNetworkConfiguration);
            SuccessOrExit(err);
            ExitNow(err = WEAVE_ERROR_INVALID_ARGUMENT);
        }

        if (netConfig.ThreadExtendedPANId == NULL)
        {
            printf("Invalid network configuration: Missing Thread extended PAN id\n");
            err = SendStatusReport(kWeaveProfile_NetworkProvisioning, kStatusCode_InvalidNetworkConfiguration);
            SuccessOrExit(err);
            ExitNow(err = WEAVE_ERROR_INVALID_ARGUMENT);
        }

        if (netConfig.ThreadNetworkKey == NULL || netConfig.ThreadNetworkKeyLen == 0)
        {
            printf("Invalid network configuration: Missing Thread network key\n");
            err = SendStatusReport(kWeaveProfile_NetworkProvisioning, kStatusCode_InvalidNetworkConfiguration);
            SuccessOrExit(err);
            ExitNow(err = WEAVE_ERROR_INVALID_ARGUMENT);
        }

        if (netConfig.ThreadNetworkKeyLen == 0)
        {
            printf("Invalid network configuration: Zero-length Thread network key\n");
            err = SendStatusReport(kWeaveProfile_NetworkProvisioning, kStatusCode_InvalidNetworkConfiguration);
            SuccessOrExit(err);
            ExitNow(err = WEAVE_ERROR_INVALID_ARGUMENT);
        }
    }

    else
    {
        printf("Unsupported network type: %d\n", netConfig.NetworkType);
        err = SendStatusReport(kWeaveProfile_NetworkProvisioning, kStatusCode_UnsupportedNetworkType);
        SuccessOrExit(err);
        ExitNow(err = WEAVE_ERROR_INVALID_ARGUMENT);
    }

    printf("Network configuration valid\n");

exit:
    return err;
}

WEAVE_ERROR MockNetworkProvisioningServer::HandleAddNetwork(PacketBuffer* networkInfoTLV)
{
    {
        char ipAddrStr[64];
        mCurOp->PeerAddr.ToString(ipAddrStr, sizeof(ipAddrStr));
        printf("AddNetwork request received from node %" PRIX64 " (%s)\n", mCurOp->PeerNodeId, ipAddrStr);
    }

    mOpArgs.networkInfoTLV = networkInfoTLV;

    CompleteOrDelayCurrentOp("add-network");

    return WEAVE_NO_ERROR;
}

WEAVE_ERROR MockNetworkProvisioningServer::HandleUpdateNetwork(PacketBuffer* networkInfoTLV)
{
    {
        char ipAddrStr[64];
        mCurOp->PeerAddr.ToString(ipAddrStr, sizeof(ipAddrStr));
        printf("UpdateNetwork request received from node %" PRIX64 " (%s)\n", mCurOp->PeerNodeId, ipAddrStr);
    }

    mOpArgs.networkInfoTLV = networkInfoTLV;

    CompleteOrDelayCurrentOp("update-network");

    return WEAVE_NO_ERROR;
}

WEAVE_ERROR MockNetworkProvisioningServer::HandleRemoveNetwork(uint32_t networkId)
{
    {
        char ipAddrStr[64];
        mCurOp->PeerAddr.ToString(ipAddrStr, sizeof(ipAddrStr));
        printf("RemoveNetwork request received from node %" PRIX64 " (%s)\n", mCurOp->PeerNodeId, ipAddrStr);
        printf("  Network Id: %u\n", networkId);
    }

    mOpArgs.networkId = networkId;

    CompleteOrDelayCurrentOp("remove-network");

    return WEAVE_NO_ERROR;
}

WEAVE_ERROR MockNetworkProvisioningServer::HandleGetNetworks(uint8_t flags)
{
    {
        char ipAddrStr[64];
        mCurOp->PeerAddr.ToString(ipAddrStr, sizeof(ipAddrStr));
        printf("GetNetworks request received from node %" PRIX64 " (%s)\n", mCurOp->PeerNodeId, ipAddrStr);
        printf("  Flags: %d\n", flags);
    }

    mOpArgs.flags = flags;

    CompleteOrDelayCurrentOp("get-networks");

    return WEAVE_NO_ERROR;
}

WEAVE_ERROR MockNetworkProvisioningServer::HandleEnableNetwork(uint32_t networkId)
{
    {
        char ipAddrStr[64];
        mCurOp->PeerAddr.ToString(ipAddrStr, sizeof(ipAddrStr));
        printf("EnableNetwork request received from node %" PRIX64 " (%s)\n", mCurOp->PeerNodeId, ipAddrStr);
        printf("  Network Id: %u\n", networkId);
    }

    mOpArgs.networkId = networkId;

    CompleteOrDelayCurrentOp("enable-network");

    return WEAVE_NO_ERROR;
}

WEAVE_ERROR MockNetworkProvisioningServer::HandleDisableNetwork(uint32_t networkId)
{
    {
        char ipAddrStr[64];
        mCurOp->PeerAddr.ToString(ipAddrStr, sizeof(ipAddrStr));
        printf("DisableNetwork request received from node %" PRIX64 " (%s)\n", mCurOp->PeerNodeId, ipAddrStr);
        printf("  Network Id: %u\n", networkId);
    }

    mOpArgs.networkId = networkId;

    CompleteOrDelayCurrentOp("disable-network");

    return WEAVE_NO_ERROR;
}

WEAVE_ERROR MockNetworkProvisioningServer::HandleTestConnectivity(uint32_t networkId)
{
    {
        char ipAddrStr[64];
        mCurOp->PeerAddr.ToString(ipAddrStr, sizeof(ipAddrStr));
        printf("TestConnectivity request received from node %" PRIX64 " (%s)\n", mCurOp->PeerNodeId, ipAddrStr);
        printf("  Network Id: %u\n", networkId);
    }

    mOpArgs.networkId = networkId;

    CompleteOrDelayCurrentOp("test-connectivity");

    return WEAVE_NO_ERROR;
}

WEAVE_ERROR MockNetworkProvisioningServer::HandleSetRendezvousMode(uint16_t rendezvousMode)
{
    {
        char ipAddrStr[64];
        mCurOp->PeerAddr.ToString(ipAddrStr, sizeof(ipAddrStr));
        printf("SetRendezvousMode request received from node %" PRIX64 " (%s)\n", mCurOp->PeerNodeId, ipAddrStr);
        printf("  Rendezvous Mode: %u\n", rendezvousMode);
    }

    mOpArgs.rendezvousMode = rendezvousMode;

    CompleteOrDelayCurrentOp("set-rendezvous-mode");

    return WEAVE_NO_ERROR;
}

void MockNetworkProvisioningServer::EnforceAccessControl(nl::Weave::ExchangeContext *ec, uint32_t msgProfileId, uint8_t msgType,
            const nl::Weave::WeaveMessageInfo *msgInfo, AccessControlResult& result)
{
    if (sSuppressAccessControls)
    {
        result = kAccessControlResult_Accepted;
    }

    NetworkProvisioningDelegate::EnforceAccessControl(ec, msgProfileId, msgType, msgInfo, result);
}

bool MockNetworkProvisioningServer::IsPairedToAccount() const
{
    return (gCASEOptions.ServiceConfig != NULL);
}

void MockNetworkProvisioningServer::HandleOpDelayComplete(System::Layer* lSystemLayer, void* aAppState, System::Error aError)
{
    MockNetworkProvisioningServer* lServer = reinterpret_cast<MockNetworkProvisioningServer*>(aAppState);
    lServer->CompleteCurrentOp();
}

void MockNetworkProvisioningServer::CompleteOrDelayCurrentOp(const char *opName)
{
    uint32_t delay = OpActions.GetDelay(opName);
    if (delay > 0)
    {
        printf("Delaying operation by %lums\n", (unsigned long)delay);
        ::SystemLayer.StartTimer(delay, HandleOpDelayComplete, this);
    }
    else
        CompleteCurrentOp();
}

void MockNetworkProvisioningServer::CompleteCurrentOp()
{
    WEAVE_ERROR err;

    switch (mCurOpType)
    {
    case NetworkProvisioning::kMsgType_AddNetwork:
        err = CompleteAddNetwork(mOpArgs.networkInfoTLV);
        break;
    case NetworkProvisioning::kMsgType_DisableNetwork:
        err = CompleteDisableNetwork(mOpArgs.networkId);
        break;
    case NetworkProvisioning::kMsgType_EnableNetwork:
        err = CompleteEnableNetwork(mOpArgs.networkId);
        break;
    case NetworkProvisioning::kMsgType_GetNetworks:
        err = CompleteGetNetworks(mOpArgs.flags);
        break;
    case NetworkProvisioning::kMsgType_RemoveNetwork:
        err = CompleteRemoveNetwork(mOpArgs.networkId);
        break;
    case NetworkProvisioning::kMsgType_ScanNetworks:
        err = CompleteScanNetworks(mOpArgs.networkType);
        break;
    case NetworkProvisioning::kMsgType_SetRendezvousMode:
        err = CompleteSetRendezvousMode(mOpArgs.rendezvousMode);
        break;
    case NetworkProvisioning::kMsgType_TestConnectivity:
        err = CompleteTestConnectivity(mOpArgs.networkId);
        break;
    case NetworkProvisioning::kMsgType_UpdateNetwork:
        err = CompleteUpdateNetwork(mOpArgs.networkInfoTLV);
        break;
    default:
        err = WEAVE_ERROR_INVALID_MESSAGE_TYPE;
        break;
    }

    if (err != WEAVE_NO_ERROR)
        SendStatusReport(kWeaveProfile_Common, Common::kStatus_InternalError, err);
}

WEAVE_ERROR MockNetworkProvisioningServer::CompleteScanNetworks(uint8_t networkType)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    PacketBuffer *respBuf = NULL;
    TLVWriter writer;
    uint16_t resultCount = 0;
    char ipAddrStr[64];
    mCurOp->PeerAddr.ToString(ipAddrStr, sizeof(ipAddrStr));

    if (networkType != kNetworkType_WiFi && networkType != kNetworkType_Thread)
    {
        SendStatusReport(kWeaveProfile_NetworkProvisioning, kStatusCode_UnsupportedNetworkType);
        ExitNow();
    }

    // Make the network ids in the scan results match the ids assigned in the provisioned networks list.
    for (int i = 0; i < kMaxScanResults; i++)
        if (ScanResults[i].NetworkType != kNetworkType_NotSpecified)
        {
            ScanResults[i].NetworkId = -1;
            for (int j = 0; j < kMaxProvisionedNetworks; j++)
                if (ScanResults[i].NetworkType == kNetworkType_WiFi
                        && ProvisionedNetworks[j].NetworkType == kNetworkType_WiFi
                        && strcmp(ScanResults[i].WiFiSSID, ProvisionedNetworks[j].WiFiSSID) == 0
                        && ScanResults[i].WiFiMode == ProvisionedNetworks[j].WiFiMode
                        && ScanResults[i].WiFiRole == ProvisionedNetworks[j].WiFiRole
                        && ScanResults[i].WiFiSecurityType == ProvisionedNetworks[j].WiFiSecurityType)
                {
                    ScanResults[i].NetworkId = ProvisionedNetworks[j].NetworkId;
                    break;
                }
                else if (ScanResults[i].NetworkType == kNetworkType_Thread
                            && ProvisionedNetworks[j].NetworkType == kNetworkType_Thread
                            && strcmp(ScanResults[i].ThreadNetworkName, ProvisionedNetworks[j].ThreadNetworkName) == 0
                            && ScanResults[i].ThreadExtendedPANId == ProvisionedNetworks[j].ThreadExtendedPANId)
                {
                    ScanResults[i].NetworkId = ProvisionedNetworks[j].NetworkId;
                    break;
                }
        }

    respBuf = PacketBuffer::New();
    VerifyOrExit(respBuf != NULL, err = WEAVE_ERROR_NO_MEMORY);

    writer.Init(respBuf);

    err = NetworkInfo::EncodeList(writer, kMaxScanResults, ScanResults, (NetworkType)networkType, NetworkInfo::kEncodeFlag_All, resultCount);
    SuccessOrExit(err);

    err = writer.Finalize();
    SuccessOrExit(err);

    printf("Sending NetworkScanComplete response\n");
    printf("  Network Count: %d\n", resultCount);

    err = SendNetworkScanComplete(resultCount, respBuf);
    respBuf = NULL;
    SuccessOrExit(err);

    return WEAVE_NO_ERROR;

exit:
    if (respBuf != NULL)
        PacketBuffer::Free(respBuf);
    return err;
}

WEAVE_ERROR MockNetworkProvisioningServer::CompleteAddNetwork(PacketBuffer* networkInfoTLV)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    NetworkInfo newNetworkConfig;
    NetworkInfo *targetNetworkConfig = NULL;
    TLVReader reader;

    reader.Init(networkInfoTLV);

    err = reader.Next();
    SuccessOrExit(err);

    err = newNetworkConfig.Decode(reader);
    SuccessOrExit(err);

    printf("  Network Config:\n");
    PrintNetworkInfo(newNetworkConfig, "    ");

    newNetworkConfig.WirelessSignalStrength = INT16_MIN;

    err = ValidateNetworkConfig(newNetworkConfig);
    if (err == WEAVE_ERROR_INVALID_ARGUMENT)
        ExitNow(err = WEAVE_NO_ERROR);
    SuccessOrExit(err);

    for (int i = 0; i < kMaxProvisionedNetworks; i++)
    {
        NetworkInfo& curElem = ProvisionedNetworks[i];
        if (curElem.NetworkType == kNetworkType_NotSpecified)
        {
            if (targetNetworkConfig == NULL)
                targetNetworkConfig = &curElem;
            continue;
        }

        if (newNetworkConfig.NetworkType == kNetworkType_WiFi && curElem.NetworkType == kNetworkType_WiFi
                && strcmp(curElem.WiFiSSID, newNetworkConfig.WiFiSSID) == 0
                && curElem.WiFiMode == newNetworkConfig.WiFiMode && curElem.WiFiRole == newNetworkConfig.WiFiRole
                && curElem.WiFiSecurityType == newNetworkConfig.WiFiSecurityType)
        {
            targetNetworkConfig = &curElem;
            break;
        }

        if (newNetworkConfig.NetworkType == kNetworkType_Thread && curElem.NetworkType == kNetworkType_Thread
                && strcmp(curElem.ThreadNetworkName, newNetworkConfig.ThreadNetworkName) == 0
                && memcmp(curElem.ThreadExtendedPANId, newNetworkConfig.ThreadExtendedPANId, 8) == 0)
        {
            targetNetworkConfig = &curElem;
            break;
        }
    }

    if (targetNetworkConfig == NULL)
    {
        err = SendStatusReport(kWeaveProfile_NetworkProvisioning, kStatusCode_TooManyNetworks);
        ExitNow();
    }

    if (targetNetworkConfig->NetworkType != kNetworkType_NotSpecified)
        newNetworkConfig.NetworkId = targetNetworkConfig->NetworkId;
    else
        newNetworkConfig.NetworkId = NextNetworkId++;

    newNetworkConfig.CopyTo(*targetNetworkConfig);

    printf("Sending AddNetworkComplete response\n");
    printf("  Network Id: %lu\n", (unsigned long)targetNetworkConfig->NetworkId);

    err = SendAddNetworkComplete(targetNetworkConfig->NetworkId);
    SuccessOrExit(err);

exit:
    PacketBuffer::Free(networkInfoTLV);
    return err;
}

WEAVE_ERROR MockNetworkProvisioningServer::CompleteUpdateNetwork(PacketBuffer* networkInfoTLV)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    NetworkInfo networkConfigUpdate, updatedNetworkConfig;
    NetworkInfo *existingNetworkConfig = NULL;
    TLVReader reader;

    reader.Init(networkInfoTLV);

    err = reader.Next();
    SuccessOrExit(err);

    err = networkConfigUpdate.Decode(reader);
    SuccessOrExit(err);

    printf("  Updated Network Config:\n");
    PrintNetworkInfo(networkConfigUpdate, "    ");

    if (networkConfigUpdate.NetworkId == -1)
    {
        err = SendStatusReport(kWeaveProfile_NetworkProvisioning, kStatusCode_InvalidNetworkConfiguration);
        ExitNow();
    }

    for (int i = 0; i < kMaxProvisionedNetworks; i++)
        if (ProvisionedNetworks[i].NetworkType != kNetworkType_NotSpecified
                && ProvisionedNetworks[i].NetworkId == networkConfigUpdate.NetworkId)
        {
            existingNetworkConfig = &ProvisionedNetworks[i];
            break;
        }

    if (existingNetworkConfig == NULL)
    {
        printf("Specified network id not found\n");
        err = SendStatusReport(kWeaveProfile_NetworkProvisioning, kStatusCode_UnknownNetwork);
        ExitNow();
    }

    err = existingNetworkConfig->CopyTo(updatedNetworkConfig);
    SuccessOrExit(err);

    err = networkConfigUpdate.MergeTo(updatedNetworkConfig);
    SuccessOrExit(err);

    err = ValidateNetworkConfig(updatedNetworkConfig);
    if (err == WEAVE_ERROR_INVALID_ARGUMENT)
        ExitNow(err = WEAVE_NO_ERROR);
    SuccessOrExit(err);

    err = updatedNetworkConfig.CopyTo(*existingNetworkConfig);
    SuccessOrExit(err);

    err = SendSuccessResponse();
    SuccessOrExit(err);

exit:
    PacketBuffer::Free(networkInfoTLV);
    return err;
}

WEAVE_ERROR MockNetworkProvisioningServer::CompleteRemoveNetwork(uint32_t networkId)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    NetworkInfo *existingNetworkConfig = NULL;

    for (int i = 0; i < kMaxProvisionedNetworks; i++)
        if (ProvisionedNetworks[i].NetworkType != kNetworkType_NotSpecified
                && ProvisionedNetworks[i].NetworkId == networkId)
        {
            existingNetworkConfig = &ProvisionedNetworks[i];
            break;
        }

    if (existingNetworkConfig == NULL)
    {
        printf("Specified network id not found\n");
        err = SendStatusReport(kWeaveProfile_NetworkProvisioning, kStatusCode_UnknownNetwork);
        ExitNow();
    }

    existingNetworkConfig->Clear();

    err = SendSuccessResponse();
    SuccessOrExit(err);

exit:
    return err;
}

WEAVE_ERROR MockNetworkProvisioningServer::CompleteGetNetworks(uint8_t flags)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    PacketBuffer *respBuf = NULL;
    TLVWriter writer;
    uint16_t resultCount = 0;
    char ipAddrStr[64];
    mCurOp->PeerAddr.ToString(ipAddrStr, sizeof(ipAddrStr));

    respBuf = PacketBuffer::New();
    VerifyOrExit(respBuf != NULL, err = WEAVE_ERROR_NO_MEMORY);

    writer.Init(respBuf);

    err = NetworkInfo::EncodeList(writer, kMaxProvisionedNetworks, ProvisionedNetworks, kNetworkType_NotSpecified, flags, resultCount);
    SuccessOrExit(err);

    err = writer.Finalize();
    SuccessOrExit(err);

    printf("Sending GetNetworksComplete response\n");
    printf("  Network Count: %d\n", resultCount);

    err = SendGetNetworksComplete(resultCount, respBuf);
    respBuf = NULL;
    SuccessOrExit(err);

    return WEAVE_NO_ERROR;

exit:
    if (respBuf != NULL)
        PacketBuffer::Free(respBuf);
    return err;
}

WEAVE_ERROR MockNetworkProvisioningServer::CompleteEnableNetwork(uint32_t networkId)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    NetworkInfo *existingNetworkConfig = NULL;

    for (int i = 0; i < kMaxProvisionedNetworks; i++)
        if (ProvisionedNetworks[i].NetworkType != kNetworkType_NotSpecified
                && ProvisionedNetworks[i].NetworkId == networkId)
        {
            existingNetworkConfig = &ProvisionedNetworks[i];
            break;
        }

    if (existingNetworkConfig == NULL)
    {
        printf("Specified network id not found\n");
        err = SendStatusReport(kWeaveProfile_NetworkProvisioning, kStatusCode_UnknownNetwork);
        ExitNow();
    }

    err = SendSuccessResponse();
    SuccessOrExit(err);

exit:
    return err;
}

WEAVE_ERROR MockNetworkProvisioningServer::CompleteDisableNetwork(uint32_t networkId)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    NetworkInfo *existingNetworkConfig = NULL;

    for (int i = 0; i < kMaxProvisionedNetworks; i++)
        if (ProvisionedNetworks[i].NetworkType != kNetworkType_NotSpecified
                && ProvisionedNetworks[i].NetworkId == networkId)
        {
            printf("Specified network id not found\n");
            existingNetworkConfig = &ProvisionedNetworks[i];
            break;
        }

    if (existingNetworkConfig == NULL)
    {
        err = SendStatusReport(kWeaveProfile_NetworkProvisioning, kStatusCode_UnknownNetwork);
        ExitNow();
    }

    err = SendSuccessResponse();
    SuccessOrExit(err);

exit:
    return err;
}

WEAVE_ERROR MockNetworkProvisioningServer::CompleteTestConnectivity(uint32_t networkId)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    NetworkInfo *existingNetworkConfig = NULL;

    for (int i = 0; i < kMaxProvisionedNetworks; i++)
        if (ProvisionedNetworks[i].NetworkType != kNetworkType_NotSpecified
                && ProvisionedNetworks[i].NetworkId == networkId)
        {
            existingNetworkConfig = &ProvisionedNetworks[i];
            break;
        }

    if (existingNetworkConfig == NULL)
    {
        printf("Specified network id not found\n");
        err = SendStatusReport(kWeaveProfile_NetworkProvisioning, kStatusCode_UnknownNetwork);
        ExitNow();
    }

    err = SendSuccessResponse();
    SuccessOrExit(err);

exit:
    return err;
}

WEAVE_ERROR MockNetworkProvisioningServer::CompleteSetRendezvousMode(uint16_t rendezvousMode)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    {
        char ipAddrStr[64];
        mCurOp->PeerAddr.ToString(ipAddrStr, sizeof(ipAddrStr));
        printf("SetRendezvousMode request received from node %" PRIX64 " (%s)\n", mCurOp->PeerNodeId, ipAddrStr);
        printf("  Rendezvous Mode: %u\n", rendezvousMode);
    }

    err = SendSuccessResponse();
    SuccessOrExit(err);

exit:
    return err;
}

void MockNetworkProvisioningServer::PrintNetworkInfo(NetworkInfo& netInfo, const char *prefix)
{
    printf("%sNetwork Type: %d\n", prefix, (int)netInfo.NetworkType);
    if (netInfo.NetworkId != -1)
        printf("%sNetwork Id: %ld\n", prefix, (long)netInfo.NetworkId);
    if (netInfo.WiFiSSID != NULL)
        printf("%sWiFi SSID: %s\n", prefix, netInfo.WiFiSSID);
    if (netInfo.WiFiMode != -1)
        printf("%sWiFi Mode: %d\n", prefix, (int)netInfo.WiFiMode);
    if (netInfo.WiFiRole != -1)
        printf("%sWiFi Role: %d\n", prefix, (int)netInfo.WiFiRole);
    if (netInfo.WiFiSecurityType != -1)
        printf("%sWiFi Security Type: %d\n", prefix, (int)netInfo.WiFiSecurityType);
    if (netInfo.WiFiKey != NULL)
    {
        char keyBuf[256];
        uint32_t i = (netInfo.WiFiKeyLen < sizeof(keyBuf)) ? netInfo.WiFiKeyLen : sizeof(keyBuf) - 1;
        memcpy(keyBuf, netInfo.WiFiKey, i);
        keyBuf[i] = 0;
        printf("%sWiFi Key: %s\n", prefix, keyBuf);
    }

    if (netInfo.ThreadNetworkName != NULL)
        printf("%sThread Network Name: %s\n", prefix, netInfo.ThreadNetworkName);
    if (netInfo.ThreadExtendedPANId != NULL)
    {
        printf("%sThread Extended PAN Id: ", prefix);
        for (int i = 0; i < 8; i++)
            printf("%02X", netInfo.ThreadExtendedPANId[i]);
        printf("\n");
    }
    if (netInfo.ThreadNetworkKey != NULL)
    {
        printf("%sThread Network Key: ", prefix);
        for (uint32_t i = 0; i < netInfo.ThreadNetworkKeyLen; i++)
            printf("%02X", netInfo.ThreadNetworkKey[i]);
        printf("\n");
    }
    if (netInfo.WirelessSignalStrength != INT16_MIN)
        printf("%sWireless Signal Strength: %d\n", prefix, (int)netInfo.WirelessSignalStrength);
}

WEAVE_ERROR MockNetworkProvisioningServer::SendStatusReport(uint32_t statusProfileId, uint16_t statusCode, WEAVE_ERROR sysError)
{
    if (statusProfileId == kWeaveProfile_Common && statusCode == Common::kStatus_Success)
        printf("Sending StatusReport: Success\n");
    else if (sysError == WEAVE_NO_ERROR)
        printf("Sending StatusReport: Status code = %u, Status profile = %lu\n", statusCode, (unsigned long)statusProfileId);
    else
        printf("Sending StatusReport: Status code = %u, Status profile = %lu, System error = %d\n", statusCode, (unsigned long)statusProfileId, sysError);
    return this->NetworkProvisioningServer::SendStatusReport(statusProfileId, statusCode, sysError);
}
