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
 *      Implementation of DeviceDescOptions object, which handles parsing of command line options
 *      that specify descriptive information about the simulated "device" used in test applications.
 *
 */


#include "ToolCommon.h"
#include <Weave/Support/CodeUtils.h>
#include <Weave/Profiles/WeaveProfiles.h>
#include <Weave/Profiles/common/CommonProfile.h>
#include <Weave/Profiles/vendor/nestlabs/device-description/NestProductIdentifiers.hpp>
#include "DeviceDescOptions.h"

using namespace nl::Weave::Profiles::DeviceDescription;

DeviceDescOptions gDeviceDescOptions;

DeviceDescOptions::DeviceDescOptions()
{
    static OptionDef optionDefs[] =
    {
        { "serial-num",       kArgumentRequired, kToolCommonOpt_DeviceSerialNum        },
        { "vendor-id",        kArgumentRequired, kToolCommonOpt_DeviceVendorId         },
        { "product-id",       kArgumentRequired, kToolCommonOpt_DeviceProductId        },
        { "product-rev",      kArgumentRequired, kToolCommonOpt_DeviceProductRevision  },
        { "software-version", kArgumentRequired, kToolCommonOpt_DeviceSoftwareVersion  },
        { }
    };
    OptionDefs = optionDefs;

    HelpGroupName = "DEVICE DESCRIPTION OPTIONS";

    OptionHelp =
        "  --serial-num <string>\n"
        "       Device serial number. Defaults to \"mock-device\".\n"
        "\n"
        "  --vendor-id <int>\n"
        "       Device vendor id.  Defaults to 0x235A (Nest Labs)\n"
        "\n"
        "  --product-id <int>\n"
        "       Device product id. Defaults to 5 (Nest Protect).\n"
        "\n"
        "  --product-rev <int>\n"
        "       Device product revision. Defaults to 1.\n"
        "\n"
        "  --software-version <string>\n"
        "       Device software version string. Defaults to \"mock-device/1.0\".\n"
        "\n";

    // Setup Defaults.
    BaseDeviceDesc.Clear();
    BaseDeviceDesc.VendorId = kWeaveVendor_NestLabs;
    BaseDeviceDesc.ProductId = nl::Weave::Profiles::Vendor::Nestlabs::DeviceDescription::kNestWeaveProduct_Onyx;
    BaseDeviceDesc.ProductRevision = 1;
    BaseDeviceDesc.ManufacturingDate.Year = 2013;
    BaseDeviceDesc.ManufacturingDate.Month = 1;
    BaseDeviceDesc.ManufacturingDate.Day = 1;
    memset(BaseDeviceDesc.Primary802154MACAddress, 0x11, sizeof(BaseDeviceDesc.Primary802154MACAddress));
    memset(BaseDeviceDesc.PrimaryWiFiMACAddress, 0x22, sizeof(BaseDeviceDesc.PrimaryWiFiMACAddress));
    strcpy(BaseDeviceDesc.RendezvousWiFiESSID, "MOCK-1111");
    strcpy(BaseDeviceDesc.SerialNumber, "mock-device");
    strcpy(BaseDeviceDesc.SoftwareVersion, "mock-device/1.0");
    BaseDeviceDesc.DeviceFeatures =
        WeaveDeviceDescriptor::kFeature_HomeAlarmLinkCapable |
        WeaveDeviceDescriptor::kFeature_LinePowered;
    // NOTE: For security reasons, pairing codes should only ever appear in device descriptors that are
    // encoded into QR codes. options.BaseDeviceDesc contains the device descriptor fields that get sent over the
    // network (e.g. in an IdentifyDevice exchange).  Therefore the PairingCode field should never be
    // set here.
}

void DeviceDescOptions::GetDeviceDesc(WeaveDeviceDescriptor& deviceDesc)
{
    deviceDesc = BaseDeviceDesc;
    deviceDesc.DeviceId = FabricState.LocalNodeId;
    deviceDesc.FabricId = FabricState.FabricId;
    memset(deviceDesc.PairingCode, 0, sizeof(deviceDesc.PairingCode));
}

bool DeviceDescOptions::HandleOption(const char *progName, OptionSet *optSet, int id, const char *name, const char *arg)
{
    DeviceDescOptions& options = *static_cast<DeviceDescOptions *>(optSet);

    switch (id)
    {
    case kToolCommonOpt_DeviceSerialNum:
        if (strlen(arg) > WeaveDeviceDescriptor::kMaxSerialNumberLength)
        {
            PrintArgError("%s: Invalid value specified for device serial number (value too long): %s\n", progName, arg);
            return false;
        }
        strncpy(options.BaseDeviceDesc.SerialNumber, arg, sizeof(options.BaseDeviceDesc.SerialNumber));
        break;
    case kToolCommonOpt_DeviceVendorId:
        if (!ParseInt(arg, options.BaseDeviceDesc.VendorId) || options.BaseDeviceDesc.VendorId == 0 || options.BaseDeviceDesc.VendorId >= 0xFFF0)
        {
            PrintArgError("%s: Invalid value specified for device vendor ID: %s\n", progName, arg);
            return false;
        }
        break;
    case kToolCommonOpt_DeviceProductId:
        if (!ParseInt(arg, options.BaseDeviceDesc.ProductId) || options.BaseDeviceDesc.ProductId == 0 || options.BaseDeviceDesc.ProductId == 0xFFFF)
        {
            PrintArgError("%s: Invalid value specified for device product ID: %s\n", progName, arg);
            return false;
        }
        break;
    case kToolCommonOpt_DeviceProductRevision:
        if (!ParseInt(arg, options.BaseDeviceDesc.ProductRevision))
        {
            PrintArgError("%s: Invalid value specified for device product revision: %s\n", progName, arg);
            return false;
        }
        break;
    case kToolCommonOpt_DeviceSoftwareVersion:
        if (strlen(arg) > WeaveDeviceDescriptor::kMaxSoftwareVersionLength)
        {
            PrintArgError("%s: Invalid value specified for device software version (value too long): %s\n", progName, arg);
            return false;
        }
        strncpy(options.BaseDeviceDesc.SoftwareVersion, arg, sizeof(options.BaseDeviceDesc.SoftwareVersion));
        break;
    default:
        PrintArgError("%s: INTERNAL ERROR: Unhandled option: %s\n", progName, name);
        return false;
    }

    return true;
}
