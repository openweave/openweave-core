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
 *      This file implements a command line utility for encoding and
 *      decoding Weave Device Descriptors.
 *
 *      Please see the document "Nest Weave: Factory Provisioning
 *      Specification" for more information about the format of the
 *      Nest Weave Device Descriptor.
 *
 */

#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif
#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif

#include <libgen.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <time.h>
#include <inttypes.h>

#include "ToolCommon.h"
#include <Weave/WeaveVersion.h>
#include <Weave/Core/WeaveEncoding.h>
#include <Weave/Profiles/device-description/DeviceDescription.h>

using namespace nl::Weave::Profiles;
using namespace nl::Weave::Profiles::DeviceDescription;
using namespace nl::Weave::Encoding;

#define TOOL_NAME "weave-device-descriptor"

#define COPYRIGHT_STRING "Copyright (c) 2013-2017 Nest Labs, Inc.\nAll rights reserved.\n"

static bool HandleEncodeOption(const char *progName, OptionSet *optSet, int id, const char *name, const char *arg);
static bool HandleDecodeArg(const char *progName, int argc, char *argv[]);
static bool ParseDate(const char *dateStr, uint16_t& year, uint8_t& month, uint8_t& day);
static void PrintDeviceDescriptor(const WeaveDeviceDescriptor& deviceDesc, const char *prefix);

static HelpOptions gGeneralHelpOptions(
    TOOL_NAME,
    "Usage: weave-device-descriptor <operation> [<options...>]\n",
    WEAVE_VERSION_STRING "\n" COPYRIGHT_STRING,
    "Tool for encoding and decoding Weave device descriptors.\n"
    "\n"
    "OPERATIONS:\n"
    "\n"
    "  encode\n"
    "       Encode a weave device descriptor given information supplied on\n"
    "       the command line.\n"
    "\n"
    "  decode\n"
    "       Decode and print a weave device descriptor read from stdin.\n"
    "\n"
    "Type 'weave-device-descriptor <operation> --help' for help on a particular\n"
    "operation.\n"
    "\n"
);

static OptionSet *gToolOptionSets[] =
{
    &gGeneralHelpOptions,
    NULL
};

static OptionDef gEncodeOptionDefs[] =
{
    { "vendor",             kArgumentRequired, 'V' },
    { "product",            kArgumentRequired, 'p' },
    { "revision",           kArgumentRequired, 'r' },
    { "mfg-date",           kArgumentRequired, 'm' },
    { "802-15-4-mac",       kArgumentRequired, '8' },
    { "wifi-mac",           kArgumentRequired, 'w' },
    { "serial-num",         kArgumentRequired, 's' },
    { "device-id",          kArgumentRequired, 'd' },
    { "ssid",               kArgumentRequired, 'S' },
    { "ssid-suffix",        kArgumentRequired, 'H' },
    { "pairing-code",       kArgumentRequired, 'P' },
    { "software-version",   kArgumentRequired, 'n' },
    { "tlv",                kNoArgument,       'T' },
    { NULL }
};

static const char *const gEncodeOptionHelp =
    "  -V, --vendor <num> | nest\n"
    "       The device vendor id, or 'nest' for the Nest vendor id.\n"
    "\n"
    "  -p, --product <num>\n"
    "       The device product id.\n"
    "\n"
    "  -r, --revision <num>\n"
    "       The device revision number.\n"
    "\n"
    "  -s, --serial-num <string>\n"
    "       The device's serial number.\n"
    "\n"
    "  -d, --device-id <hex-string>\n"
    "       The device's Weave node id, given as a hex string.\n"
    "\n"
    "  -m, --mfg-date <YYYY>/<MM>/<DD> | <YYYY>/<MM>\n"
    "       The device manufacturing date.\n"
    "\n"
    "  -n, --software-version <string>\n"
    "       The device's software version. Note that this field is not supported in\n"
    "       the text form of a device descriptor.\n"
    "\n"
    "  -8, --802-15-4-mac <mac>\n"
    "       The device's 802.15.4 MAC address given as a hex string (colons optional).\n"
    "\n"
    "  -w, --wifi-mac <mac>\n"
    "       The device's 802.11 MAC address given as a hex string (colons optional).\n"
    "\n"
    "  -S, --ssid <string>\n"
    "  -H, --ssid-suffix <string>\n"
    "       The SSID or SSID suffix for the device's WiFi rendezvous network.\n"
    "\n"
    "  -P, --pairing-code <string>\n"
    "       The device's pairing code.\n"
    "\n"
    "  -T, --tlv\n"
    "       Encode the descriptor in TLV format, instead of text format.\n"
    "\n";

static OptionSet gEncodeOptions =
{
    HandleEncodeOption,
    gEncodeOptionDefs,
    "ENCODE OPTIONS",
    gEncodeOptionHelp,
};

static HelpOptions gEncodeHelpOptions(
    TOOL_NAME,
    "Usage: " TOOL_NAME " encode [<options...>]\n",
    WEAVE_VERSION_STRING "\n" COPYRIGHT_STRING,
    "Encode a weave device descriptor given information supplied on the command line.\n"
);

static OptionSet *gEncodeOptionSets[] =
{
    &gEncodeOptions,
    &gEncodeHelpOptions,
    NULL
};

static HelpOptions gDecodeHelpOptions(
    TOOL_NAME,
    "Usage: " TOOL_NAME " decode [<options...>]\n"
    "       " TOOL_NAME " decode [<options...>] <text-device-descriptor>\n",
    WEAVE_VERSION_STRING "\n" COPYRIGHT_STRING,
    "Decode and print a weave device descriptor read from stdin or the command line.\n"
);

static OptionSet *gDecodeOptionSets[] =
{
    &gDecodeHelpOptions,
    NULL
};

const char *Operation = NULL;
WeaveDeviceDescriptor DeviceDesc;
bool UseTLV = false;
const char *DecodeArg = NULL;

int main(int argc, char *argv[])
{
    WEAVE_ERROR err;

    nl::Weave::Logging::SetLogFilter(nl::Weave::Logging::kLogCategory_None);

    InitToolCommon();

    if (argc < 2)
    {
        gGeneralHelpOptions.PrintBriefUsage(stderr);
        exit(EXIT_FAILURE);
    }

    if (strcmp(argv[1], "encode") == 0)
    {
        uint8_t encodeBuf[128];
        uint32_t encodedLen;

        argv[1] = argv[0];

        if (!ParseArgs(TOOL_NAME "(encode)", argc-1, argv+1, gEncodeOptionSets))
        {
            exit(EXIT_FAILURE);
        }

        if (UseTLV)
            err = WeaveDeviceDescriptor::EncodeTLV(DeviceDesc, encodeBuf, sizeof(encodeBuf), encodedLen);
        else
            err = WeaveDeviceDescriptor::EncodeText(DeviceDesc, (char *)encodeBuf, sizeof(encodeBuf), encodedLen);
        FAIL_ERROR(err, "Encode failed");

        if (write(STDOUT_FILENO, encodeBuf, (size_t)encodedLen) != (ssize_t)encodedLen ||
            (!UseTLV && write(STDOUT_FILENO, "\n", 1) != 1))
        {
            perror("Output error");
            exit(EXIT_FAILURE);
        }
    }

    else if (strcmp(argv[1], "decode") == 0)
    {
        static uint8_t readBuf[2048];
        uint32_t lenRead = 0;
        uint32_t encodedLen;
        int readRes;

        argv[1] = argv[0];

        if (!ParseArgs(TOOL_NAME "(decode)", argc-1, argv+1, gDecodeOptionSets, HandleDecodeArg))
        {
            exit(EXIT_FAILURE);
        }

        if (DecodeArg != NULL)
        {
            strncpy((char *)readBuf, DecodeArg, sizeof(readBuf));
            encodedLen = strlen(DecodeArg);
        }

        else
        {
            while ((readRes = read(STDIN_FILENO, readBuf + lenRead, sizeof(readBuf) - lenRead)) > 0) {
                lenRead += readRes;
                if (lenRead == sizeof(readBuf))
                {
                    fprintf(stderr, "Input too long.\n");
                    exit(EXIT_FAILURE);
                }
            }
            if (readRes < 0)
            {
                perror("Input error");
                exit(EXIT_FAILURE);
            }

            encodedLen = lenRead;
        }

        err = WeaveDeviceDescriptor::Decode(readBuf, encodedLen, DeviceDesc);
        FAIL_ERROR(err, "Decode failed");

        PrintDeviceDescriptor(DeviceDesc, "");
    }

    else
    {
        if (!ParseArgs(TOOL_NAME, argc, argv, gToolOptionSets))
        {
            exit(EXIT_FAILURE);
        }
    }

    return EXIT_SUCCESS;
}

bool HandleEncodeOption(const char *progName, OptionSet *optSet, int id, const char *name, const char *arg)
{
    int32_t val;
    uint32_t len;

    switch (id)
    {
    case 'V':
        if (!ParseInt(arg, val, 0) || val < 0 || val > UINT16_MAX)
        {
            if (strcasecmp(arg, "nest") == 0 || strcasecmp(arg, "nestlabs") == 0)
            {
                val = kWeaveVendor_NestLabs;
            }
            else
            {
                PrintArgError("%s: Invalid value specified for vendor id: %s\n", progName, arg);
                return false;
            }
        }
        DeviceDesc.VendorId = val;
        break;
    case 'p':
        if (!ParseInt(arg, val, 0) || val < 0 || val > UINT16_MAX)
        {
            PrintArgError("%s: Invalid value specified for product id: %s\n", progName, arg);
            return false;
        }
        DeviceDesc.ProductId = val;
        break;
    case 'r':
        if (!ParseInt(arg, val, 0) || val < 0 || val > UINT16_MAX)
        {
            PrintArgError("%s: Invalid value specified for product revision: %s\n", progName, arg);
            return false;
        }
        DeviceDesc.ProductRevision = val;
        break;
    case 'm':
        if (!ParseDate(arg, DeviceDesc.ManufacturingDate.Year, DeviceDesc.ManufacturingDate.Month, DeviceDesc.ManufacturingDate.Day))
        {
            PrintArgError("%s: Invalid value specified for manufacturing date: %s\n", progName, arg);
            return false;
        }
        break;
    case '8':
        if (!ParseHexString(arg, strlen(arg), DeviceDesc.Primary802154MACAddress, 8, len) || len != 8)
        {
            PrintArgError("%s: Invalid value specified for 802.15.4 MAC address: %s\n", progName, arg);
            return false;
        }
        break;
    case 'w':
        if (!ParseHexString(arg, strlen(arg), DeviceDesc.PrimaryWiFiMACAddress, 6, len) || len != 6)
        {
            PrintArgError("%s: Invalid value specified for 802.15.4 MAC address: %s\n", progName, arg);
            return false;
        }
        break;
    case 's':
        if (strlen(arg) > WeaveDeviceDescriptor::kMaxSerialNumberLength)
        {
            PrintArgError("%s: Invalid value specified for device serial number: %s\n", progName, arg);
            return false;
        }
        strcpy(DeviceDesc.SerialNumber, arg);
        break;
    case 'n':
        if (strlen(arg) > WeaveDeviceDescriptor::kMaxSoftwareVersionLength)
        {
            PrintArgError("%s: Invalid value specified for device software version: %s\n", progName, arg);
            return false;
        }
        strcpy(DeviceDesc.SoftwareVersion, arg);
        break;
    case 'd':
    {
        union
        {
            uint64_t deviceId;
            uint8_t deviceIdBytes[8];
        };

        if (!ParseHexString(arg, strlen(arg), deviceIdBytes, sizeof(deviceIdBytes), len) || len != sizeof(deviceIdBytes))
        {
            PrintArgError("%s: Invalid value specified for device id: %s\n", progName, arg);
            return false;
        }
        DeviceDesc.DeviceId = BigEndian::HostSwap64(deviceId);
        break;
    }
    case 'S':
    case 'H':
        if (strlen(arg) > WeaveDeviceDescriptor::kMaxRendezvousWiFiESSID)
        {
            PrintArgError("%s: Invalid value specified for device rendezvous WiFi SSID or SSID suffix: %s\n", progName, arg);
            return false;
        }
        strcpy(DeviceDesc.RendezvousWiFiESSID, arg);
        if (id == 'S')
        {
            DeviceDesc.Flags &= ~WeaveDeviceDescriptor::kFlag_IsRendezvousWiFiESSIDSuffix;
        }
        else
        {
            DeviceDesc.Flags |= WeaveDeviceDescriptor::kFlag_IsRendezvousWiFiESSIDSuffix;
        }
        break;
    case 'P':
        if (strlen(arg) > WeaveDeviceDescriptor::kMaxPairingCodeLength)
        {
            PrintArgError("%s: Invalid value specified for device pairing code: %s\n", progName, arg);
            return false;
        }
        strcpy(DeviceDesc.PairingCode, arg);
        break;
    case 'T':
        UseTLV = true;
        break;
    default:
        PrintArgError("%s: INTERNAL ERROR: Unhandled option: %s\n", progName, name);
        return false;
    }

    return true;
}

static bool HandleDecodeArg(const char *progName, int argc, char *argv[])
{
    if (argc > 0)
    {
        if (argc > 1)
        {
            PrintArgError("%s: Unexpected argument: %s\n", progName, argv[1]);
            return false;
        }

        if (strcmp(argv[0], "-") != 0)
        {
            DecodeArg = argv[0];
        }
    }

    return true;
}

bool ParseDate(const char *dateStr, uint16_t& year, uint8_t& month, uint8_t& day)
{
    struct tm date;

    if (strptime(dateStr, "%Y/%m/%d", &date) == NULL)
    {
        if (strptime(dateStr, "%Y/%m", &date) == NULL)
            return false;
        date.tm_mday = 0;
    }

    date.tm_year += 1900;

    if (date.tm_year < 2001 || date.tm_year > 2099)
        return false;

    year = date.tm_year;
    month = date.tm_mon + 1;
    day = date.tm_mday;

    return true;
}

void PrintDeviceDescriptor(const WeaveDeviceDescriptor& deviceDesc, const char *prefix)
{
    if (DeviceDesc.DeviceId != 0)
        printf("%sDevice Id: %016" PRIX64 "\n", prefix, DeviceDesc.DeviceId);
    if (DeviceDesc.FabricId != 0)
        printf("%sFabric Id: %016" PRIX64 "\n", prefix, DeviceDesc.FabricId);
    if (DeviceDesc.VendorId != 0)
        printf("%sVendor Code: %04" PRIX16 "\n", prefix, DeviceDesc.VendorId);
    if (DeviceDesc.ProductId != 0)
        printf("%sProduct Code: %04" PRIX16 "\n", prefix, DeviceDesc.ProductId);
    if (DeviceDesc.ProductRevision != 0)
        printf("%sProduct Revision: %" PRIu16 "\n", prefix, DeviceDesc.ProductRevision);
    if (DeviceDesc.SerialNumber[0] != 0)
        printf("%sSerial Number: %s\n", prefix, DeviceDesc.SerialNumber);
    if (DeviceDesc.SoftwareVersion[0] != 0)
        printf("%sSoftware Version: %s\n", prefix, DeviceDesc.SoftwareVersion);
    if (DeviceDesc.ManufacturingDate.Year != 0 && DeviceDesc.ManufacturingDate.Month != 0)
    {
        printf("%sManufacturing Date: ", prefix);
        printf("%04" PRIu16 "/", DeviceDesc.ManufacturingDate.Year);
        printf("%02" PRIu8 "", DeviceDesc.ManufacturingDate.Month);
        if (DeviceDesc.ManufacturingDate.Day != 0)
            printf("/%02" PRIu8 "\n", DeviceDesc.ManufacturingDate.Day);
        else
            printf("\n");
    }
    if (!IsZeroBytes(DeviceDesc.Primary802154MACAddress, sizeof(DeviceDesc.Primary802154MACAddress)))
    {
        printf("%sPrimary 802.15.4 MAC: ", prefix);
        PrintMACAddress(DeviceDesc.Primary802154MACAddress, sizeof(DeviceDesc.Primary802154MACAddress));
        printf("\n");
    }
    if (!IsZeroBytes(DeviceDesc.PrimaryWiFiMACAddress, sizeof(DeviceDesc.PrimaryWiFiMACAddress)))
    {
        printf("%sPrimary WiFi MAC: ", prefix);
        PrintMACAddress(DeviceDesc.PrimaryWiFiMACAddress, sizeof(DeviceDesc.PrimaryWiFiMACAddress));
        printf("\n");
    }
    if (DeviceDesc.RendezvousWiFiESSID[0] != 0)
        printf("%sRendezvous WiFi SSID%s: %s\n", prefix,
               ((DeviceDesc.Flags & WeaveDeviceDescriptor::kFlag_IsRendezvousWiFiESSIDSuffix) != 0) ? " Suffix" : "",
               DeviceDesc.RendezvousWiFiESSID);
    if (DeviceDesc.PairingCode[0] != 0)
        printf("%sPairing Code: %s\n", prefix, DeviceDesc.PairingCode);
    if (DeviceDesc.PairingCompatibilityVersionMajor != 0)
        printf("%sPairing Compatibility Major Version: %" PRIu16 "\n", prefix, DeviceDesc.PairingCompatibilityVersionMajor);
    if (DeviceDesc.PairingCompatibilityVersionMinor != 0)
        printf("%sPairing Compatibility Minor Version: %" PRIu16 "\n", prefix, DeviceDesc.PairingCompatibilityVersionMinor);
    if (DeviceDesc.DeviceFeatures != 0)
        printf("%sDevice Features: %08" PRIX32 "\n", prefix, DeviceDesc.DeviceFeatures);
}
