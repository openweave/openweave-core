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
 *      This file implements a process to effect a functional test for
 *      the legacy version of the Weave Data Management (WDM) protocol
 *      client / subscriber implementation.
 *
 */

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

// This code tests the legacy version of Weave Data Management
#define WEAVE_CONFIG_DATA_MANAGEMENT_NAMESPACE kWeaveManagedNamespace_Legacy

#include "ToolCommon.h"
#include "CASEOptions.h"
#include <Weave/Core/WeaveTLV.h>
#include <Weave/Profiles/ProfileCommon.h>
#include <Weave/Profiles/service-directory/ServiceDirectory.h>
#include <Weave/Profiles/data-management/DataManagement.h>
#include <Weave/Profiles/data-management/ProfileDatabase.h>
#include <Weave/Profiles/device-description/DeviceDescription.h>
#include <Weave/Profiles/vendor/nestlabs/device-description/NestProductIdentifiers.hpp>

using namespace ::nl::Weave::TLV;
using namespace ::nl::Weave::Profiles::Common;
using namespace ::nl::Weave::Profiles::DataManagement;

#if WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
using namespace ::nl::Weave::Profiles::ServiceDirectory;
#endif

using namespace ::nl::Weave::Profiles::DeviceDescription;

#define TOOL_NAME "wdmtest"

#define StandardTimeout 10000 // this is in milliseconds

bool UpdateDone = false;
bool RelocationDone = false;

/*
 * we use the NestProtect profile (the Topaz Bucket in the
 * old parlance) as a test case.
 *
 * these are the tags for top-level elements in the profile.
 */

enum
{
    // context-specific tags
    kTag_SmokeStatus =        0,
    kTag_CoStatus =           1,
    kTag_HeatStatus =         2,
    kTag_HushedState =        3,
    kTag_GestureHushEnable =  8,
    kTag_HeadsUpEnable =      9,
    kTag_NightLightEnable =   10,
};

/*
 * and these are the profile status codes.
 */

enum
{
    dspStatus_None =         0,
    dspStatus_HU1 =         1,
    dspStatus_HU2 =         2,
    dspStatus_Alarm =        3,
};

static HelpOptions gHelpOptions(
    TOOL_NAME,
    "Usage: " TOOL_NAME " [<options...>]\n",
    WEAVE_VERSION_STRING "\n" WEAVE_TOOL_COPYRIGHT
);

static OptionSet *gToolOptionSets[] =
{
    &gNetworkOptions,
    &gWeaveNodeOptions,
    &gCASEOptions,
    &gDeviceDescOptions,
    &gFaultInjectionOptions,
    &gHelpOptions,
    NULL
};


/*
 * to perform a relocation test we use the Structure
 * profile (bucket) with a special instance ID.
 */

unsigned char BogusInstance[] = "fbeb75b0-6ad8-11e4-a2e3-22000a6d8bca";
uint32_t BogusInstanceLength = 36;

// here's a root server record for the directory

#if WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
const struct
{
    uint8_t entryCtrl;
    uint64_t endPointId;
    uint8_t itemCtrl;
    uint8_t strLen;
    char nameStr[35];
} __attribute__((packed)) rootDirectory = { 0x41, 0x18B4300200000001ULL, 0, 34, "frontdoor.integration.nestlabs.com" };

// and here's an accessor

WEAVE_ERROR getRootDirectory(uint8_t *aDirectory, uint16_t aDirectoryLen)
{
    memcpy(aDirectory, (uint8_t *)&rootDirectory, 45);

    return WEAVE_NO_ERROR;
}
#endif // WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
/*
 * we define a concrete settings database with one profile
 * instance in it, i.e. the Topaz device settings profile.
 */

class StubSettingsDatabase :
    public ProfileDatabase
{
public:
    /*
     * here's a version of the Topaz device settings profile schema
     * for testing purposes.
     */

    class DeviceSettingsProfileData :
        public ProfileData
    {
    public:
        DeviceSettingsProfileData()
        {
            mVersion = 0;

            mSmokeStatus = dspStatus_None;
            mCoStatus = dspStatus_None;
            mHeatStatus = dspStatus_None;
            mIsHushed = false;
            mGestureHushIsEnabled = true;
            mHeadsUpIsEnabled = true;
            mNightLightIsEnabled = false;
        };

        WEAVE_ERROR init(uint8_t aSmokeStatus, uint8_t aCoStatus, uint8_t aHeatStatus, bool aHushed, bool aGestureHush, bool aHeadsUp, bool aNightLight)
        {
            mSmokeStatus = aSmokeStatus;
            mCoStatus = aCoStatus;
            mHeatStatus = aHeatStatus;
            mIsHushed = aHushed;
            mGestureHushIsEnabled = aGestureHush;
            mHeadsUpIsEnabled = aHeadsUp;
            mNightLightIsEnabled = aNightLight;

            return WEAVE_NO_ERROR;
        };

        WEAVE_ERROR StoreItem(const uint64_t &aTag, TLVReader &aDataRdr)
        {
            WEAVE_ERROR err;

            if (aTag == ContextTag(kTag_SmokeStatus))
                err = aDataRdr.Get(mSmokeStatus);

            else if (aTag == ContextTag(kTag_CoStatus))
                err = aDataRdr.Get(mCoStatus);

            else if (aTag == ContextTag(kTag_HeatStatus))
                err = aDataRdr.Get(mHeatStatus);

            else if (aTag == ContextTag(kTag_HushedState))
                err = aDataRdr.Get(mIsHushed);

            else if (aTag == ContextTag(kTag_GestureHushEnable))
                err = aDataRdr.Get(mGestureHushIsEnabled);

            else if (aTag == ContextTag(kTag_HeadsUpEnable))
                err = aDataRdr.Get(mHeadsUpIsEnabled);

            else if (aTag == ContextTag(kTag_NightLightEnable))
                err = aDataRdr.Get(mNightLightIsEnabled);
            else

                /*
                 * ignore unknown tags...
                 */

                err = WEAVE_NO_ERROR;

            return err;
        }

        WEAVE_ERROR Retrieve(nl::Weave::TLV::TLVReader &aPathRdr, nl::Weave::TLV::TLVWriter &aDataWrtr)
        {
            return WEAVE_NO_ERROR;
        }

        // data members

        uint32_t mVersion;

        uint16_t mSmokeStatus;
        uint16_t mCoStatus;
        uint16_t mHeatStatus;
        bool mIsHushed;
        bool mGestureHushIsEnabled;
        bool mHeadsUpIsEnabled;
        bool mNightLightIsEnabled;

    } TopazProfileData;

    /*
     * a concrete settings database class needs a way to look up
     * profile data based on the profile ID (and to fail if it's
     * not found).
     */

    WEAVE_ERROR LookupProfileData(uint32_t aProfileId, TLVReader *aInstanceIdRdr, ProfileData **aTarget)
    {
        WEAVE_ERROR err = WEAVE_NO_ERROR;

        // we have the luxury of ignoring the instance ID here.

        switch (aProfileId)
        {
        case kWeaveProfile_NestProtect:
            *aTarget = &TopazProfileData;

            break;

        default:
            err = WEAVE_ERROR_INVALID_PROFILE_ID;

            break;
        }

        return err;
    }

} SettingsDatabase;

// and some dummy data

static uint8_t TLVData[100];
ReferencedTLVData PathList;
ReferencedTLVData DataList;

/*
 * in order to use the "new improved" data management, we have to create
 * a sub-class of the WDM client and supply the relevant methods as follows.
 */

class WDMTestClient :
    public DMClient
{
    WEAVE_ERROR ViewConfirm(const uint64_t &aResponderId, StatusReport &aStatus, uint16_t aTxnId)
    {
        WEAVE_ERROR err = WEAVE_NO_ERROR;

        if (aStatus.mProfileId == kWeaveProfile_Common && aStatus.mStatusCode == kStatus_Relocated)
        {

            if (RelocationDone)
            {
                /*
                 * as it happens, the relocations go on forever since we're
                 * asking for a bogus service. just stop.
                 */

                printf("second relocation request received, exiting\n");

                Done = true;
            }

            else
            {
                printf("recieved a relocation request\n");

                RelocationDone = true;
            }
        }

        else
        {
            printf("view non-success status [%x, %x, %d]\n", aStatus.mProfileId, aStatus.mStatusCode, aStatus.mError);

            Done = true;
        }

        return err;
    }

    WEAVE_ERROR ViewConfirm(const uint64_t &aResponderId, ReferencedTLVData &aDataList, uint16_t aTxnId)
    {
        WEAVE_ERROR err = WEAVE_NO_ERROR;
        TLVWriter writer;

        if (!UpdateDone)
        {
            /*
             * first, we install the new data
             */

            err = SettingsDatabase.Store(aDataList);

            if (err == WEAVE_NO_ERROR)
            {
                printf("ViewConfirm: successfully executed view\nstarting update\n");
            }

            else
            {
                printf("ViewConfirm: could not install data. err = %d\n", err);

                goto exit;
            }

            /*
             * OK. that worked. now send an update
             */

            writer.Init(TLVData, 100);

            err = StartDataList(writer);
            SuccessOrExit(err);

            err = StartDataListElement(writer);
            SuccessOrExit(err);

            EncodePath(writer,
                       ContextTag(kTag_WDMDataListElementPath),
                       kWeaveProfile_NestProtect,
                       gWeaveNodeOptions.LocalNodeId,
                       1,
                       ContextTag(kTag_NightLightEnable));

            // don't bother with the version and write the data and get out

            writer.PutBoolean(ContextTag(kTag_WDMDataListElementData), true);

            err = EndDataListElement(writer);
            SuccessOrExit(err);

            err = EndList(writer);
            SuccessOrExit(err);

            DataList.init((uint16_t)writer.GetLengthWritten(), 100, TLVData);

            err = UpdateRequest(DataList, 3, StandardTimeout);
        }

        else if (!RelocationDone)
        {
            printf("viewed again after update\n");

            if (SettingsDatabase.TopazProfileData.mNightLightIsEnabled == true)
                printf("WOOHOO!\n");

            /*
             * now test relocation viewing the structure bucket with a bogus instance
             */

            writer.Init(TLVData, 100);

            err = StartPathList(writer);
            SuccessOrExit(err);

            EncodePath(writer, AnonymousTag, kWeaveProfile_Structure, BogusInstanceLength, BogusInstance, 0);

            err = EndList(writer);
            SuccessOrExit(err);

            PathList.init((uint16_t)writer.GetLengthWritten(), 100, TLVData);

            // now try a view request
            ViewRequest(PathList, 2, StandardTimeout);
        }

        else
        {
            printf("relocation performed\n");

            Done = true;
        }

    exit:

        return err;
    }

    WEAVE_ERROR SubscribeConfirm(const uint64_t &aResponderId, StatusReport &aStatus, uint16_t aTxnId)
    {
        WEAVE_ERROR err = WEAVE_NO_ERROR;

        return err;
    }

    WEAVE_ERROR SubscribeConfirm(const uint64_t &aResponderId, const TopicIdentifier &aTopicId, uint16_t aTxnId)
    {
        WEAVE_ERROR err = WEAVE_NO_ERROR;

        return err;
    }

    WEAVE_ERROR SubscribeConfirm(const uint64_t &aResponderId, const TopicIdentifier &aTopicId, ReferencedTLVData &aDataList, uint16_t aTxnId)
    {
        WEAVE_ERROR err = WEAVE_NO_ERROR;

        return err;
    }

    WEAVE_ERROR UnsubscribeIndication(const uint64_t &aPublisherId, const TopicIdentifier &aTopicId, StatusReport &aReport)
    {
    printf("processing: <unsubscribe indication 0x%" PRIx64 ", 0x%" PRIx64 ">\n", aPublisherId, aTopicId);

    return WEAVE_NO_ERROR;
    }

    WEAVE_ERROR UpdateConfirm(const uint64_t &aResponderId, StatusReport &aStatus, uint16_t aTxnId)
    {
        WEAVE_ERROR err = WEAVE_NO_ERROR;

        UpdateDone = true;

        if (aStatus.mStatusCode == kStatus_Success)
        {
            TLVWriter writer;

            printf("update success!\n");

            // format a path list for the Topaz Bucket

            writer.Init(TLVData, 100);

            err = StartPathList(writer);
            SuccessOrExit(err);

            EncodePath(writer, AnonymousTag, kWeaveProfile_NestProtect, gWeaveNodeOptions.LocalNodeId, 0);

            err = EndList(writer);
            SuccessOrExit(err);

            PathList.init((uint16_t)writer.GetLengthWritten(), 100, TLVData);

            // now try a view request
            ViewRequest(PathList, 2, StandardTimeout);
        }

        else
        {
            printf("update status = %d\n", aStatus.mStatusCode);

            Done = true;
        }

    exit:

        return err;
    }

    WEAVE_ERROR UpdateConfirm(const uint64_t &aResponderId, ReferencedTLVData &aVersionList, uint16_t aTxnId)
    {
        WEAVE_ERROR err = WEAVE_NO_ERROR;

        return err;
    }

    WEAVE_ERROR CancelSubscriptionIndication(const uint64_t &aRequestorId, const TopicIdentifier &aTopicId)
    {
        WEAVE_ERROR err = WEAVE_NO_ERROR;

        return err;
    }

    WEAVE_ERROR CancelSubscriptionConfirm(const uint64_t &aResponderId, const TopicIdentifier &aTopicId, StatusReport &aStatus, uint16_t aTxnId)
    {
        WEAVE_ERROR err = WEAVE_NO_ERROR;

        return err;
    }

    WEAVE_ERROR NotifyIndication(const TopicIdentifier &aTopicId, ReferencedTLVData &aDataList)
    {
        WEAVE_ERROR err = WEAVE_NO_ERROR;

        return err;
    }

    void IncompleteIndication(const uint64_t &aPeerNodeId, StatusReport &aReport)
    {
    }

};

int main(int argc, char *argv[])
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    WDMTestClient client;
#if WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
    WeaveServiceManager svcMgr;
    uint8_t cache[500];
#endif

    InitToolCommon();

    SetSIGUSR1Handler();

    if (argc == 1)
    {
        gHelpOptions.PrintBriefUsage(stderr);
        exit(EXIT_FAILURE);
    }

    if (!ParseArgsFromEnvVar(TOOL_NAME, TOOL_OPTIONS_ENV_VAR_NAME, gToolOptionSets, NULL, true) ||
        !ParseArgs(TOOL_NAME, argc, argv, gToolOptionSets))
    {
        exit(EXIT_FAILURE);
    }

    if (gNetworkOptions.LocalIPv6Addr != IPAddress::Any)
    {
        if (!gNetworkOptions.LocalIPv6Addr.IsIPv6ULA())
        {
            printf("ERROR: Local address must be an IPv6 ULA\n");
            exit(-1);
        }
        gWeaveNodeOptions.FabricId = gNetworkOptions.LocalIPv6Addr.GlobalId();
        gWeaveNodeOptions.LocalNodeId = IPv6InterfaceIdToWeaveNodeId(gNetworkOptions.LocalIPv6Addr.InterfaceId());
        gWeaveNodeOptions.SubnetId = gNetworkOptions.LocalIPv6Addr.Subnet();
    }

    // Initialize a DeviceDescriptor

    WeaveDeviceDescriptor deviceDesc;

    deviceDesc.DeviceId = gWeaveNodeOptions.LocalNodeId;
    deviceDesc.FabricId = gWeaveNodeOptions.FabricId;
    deviceDesc.VendorId = kWeaveVendor_NestLabs;
    deviceDesc.ProductId = nl::Weave::Profiles::Vendor::Nestlabs::DeviceDescription::kNestWeaveProduct_Topaz;
    deviceDesc.ProductRevision = 1;
    deviceDesc.ManufacturingDate.Year = 2013;
    deviceDesc.ManufacturingDate.Month = 1;
    deviceDesc.ManufacturingDate.Day = 1;
    memset(deviceDesc.Primary802154MACAddress, 0x11, sizeof(deviceDesc.Primary802154MACAddress));
    memset(deviceDesc.PrimaryWiFiMACAddress, 0x22, sizeof(deviceDesc.PrimaryWiFiMACAddress));
    strcpy(deviceDesc.RendezvousWiFiESSID, "MOCK-1111");
    strcpy(deviceDesc.SerialNumber, "mock-device");
    strcpy(deviceDesc.SoftwareVersion, "mock-device/1.0");
    deviceDesc.DeviceFeatures =
        WeaveDeviceDescriptor::kFeature_HomeAlarmLinkCapable |
        WeaveDeviceDescriptor::kFeature_LinePowered;

    uint8_t deviceInitData[256];
    uint32_t deviceInitDataLen;
    WeaveDeviceDescriptor::EncodeTLV(deviceDesc, deviceInitData, sizeof(deviceInitData), deviceInitDataLen);

    gCASEOptions.NodePayload = deviceInitData;
    gCASEOptions.NodePayloadLength = deviceInitDataLen;


    // initialize dummy path list

    TLVWriter writer;
    TLVType pathListContainer;

    writer.Init(TLVData, 100);
    err = writer.StartContainer(ProfileTag(kWeaveProfile_WDM, kTag_WDMPathList), kTLVType_Array, pathListContainer);
    SuccessOrExit(err);

    err = EncodePath(writer, AnonymousTag, kWeaveProfile_NestProtect, gWeaveNodeOptions.LocalNodeId, 0);
    SuccessOrExit(err);

    err = writer.EndContainer(pathListContainer);
    SuccessOrExit(err);

    writer.Finalize();

    err = PathList.init((uint16_t)writer.GetLengthWritten(), 100, TLVData);
    SuccessOrExit(err);

    // set up NWK

    InitSystemLayer();
    InitNetwork();
    InitWeaveStack(false, true);

    PrintNodeConfig();

#if WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
    svcMgr.init(&ExchangeMgr, cache, 500, getRootDirectory, kWeaveAuthMode_CASE_ServiceEndPoint);

    // set up the WDM engine

    err = client.Init(&ExchangeMgr);
    SuccessOrExit(err);

    err = client.BindRequest(&svcMgr, kWeaveAuthMode_CASE_ServiceEndPoint);
    SuccessOrExit(err);

    // now try a view request

    err = client.ViewRequest(PathList, 1, StandardTimeout);
    SuccessOrExit(err);
#endif

    while (!Done)
    {
        struct timeval sleepTime;
        sleepTime.tv_sec = 0;
        sleepTime.tv_usec = 100000;
        ServiceNetwork(sleepTime);
    }

exit:
    if (err != WEAVE_NO_ERROR)
        printf("test failed. err = %d\n", err);

    return err == WEAVE_NO_ERROR ? EXIT_SUCCESS : EXIT_FAILURE;
}
