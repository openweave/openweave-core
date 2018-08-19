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
#include <Weave/DeviceLayer/internal/ServiceDirectoryManager.h>
#include <Weave/DeviceLayer/TimeSyncManager.h>
#include <Weave/Core/WeaveTLV.h>
#include <Weave/Profiles/service-directory/ServiceDirectory.h>
#include <Weave/Profiles/service-provisioning/ServiceProvisioning.h>
#include <new>

using namespace ::nl;
using namespace ::nl::Weave;
using namespace ::nl::Weave::TLV;
using namespace ::nl::Weave::Encoding;
using namespace ::nl::Weave::Profiles;

namespace nl {
namespace Weave {
namespace DeviceLayer {
namespace Internal {

#if WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY

ServiceDirectory::WeaveServiceManager ServiceDirectoryMgr;

namespace {

uint8_t ServiceDirectoryCache[WEAVE_DEVICE_CONFIG_SERVICE_DIRECTORY_CACHE_SIZE];

extern WEAVE_ERROR GetRootDirectoryEntry(uint8_t *buf, uint16_t bufSize);

} // unnamed namespace


WEAVE_ERROR InitServiceDirectoryManager(void)
{
    WEAVE_ERROR err;

    new (&ServiceDirectoryMgr) ServiceDirectory::WeaveServiceManager();

    err = ServiceDirectoryMgr.init(&ExchangeMgr,
            ServiceDirectoryCache, sizeof(ServiceDirectoryCache),
            GetRootDirectoryEntry,
            kWeaveAuthMode_CASE_ServiceEndPoint,
#if WEAVE_DEVICE_CONFIG_ENABLE_SERVICE_DIRECTORY_TIME_SYNC
            TimeSyncManager::MarkServiceDirRequestStart,
            TimeSyncManager::ProcessServiceDirTimeData);
#else
            NULL, NULL);
#endif
    if (err != WEAVE_NO_ERROR)
    {
        WeaveLogError(DeviceLayer, "ServiceDirectoryMgr.init() failed: %s", ErrorStr(err));
    }

    return err;
}

namespace {

WEAVE_ERROR EncodeRootDirectoryFromServiceConfig(const uint8_t * serviceConfig, uint16_t serviceConfigLen,
        uint8_t * rootDirBuf, uint16_t rootDirBufSize)
{
    WEAVE_ERROR err;
    TLVReader reader;
    TLVType container;
    uint64_t directoryEndpointId;
    uint8_t numHostPortEntries = 0;
    uint8_t * p = rootDirBuf;

    static const uint16_t kMinRootDirSize =
            1 +     // Directory Entry Control Byte
            8;      // Service Endpoint Id

    VerifyOrExit(rootDirBufSize > kMinRootDirSize, err = WEAVE_ERROR_BUFFER_TOO_SMALL);

    reader.Init(serviceConfig, serviceConfigLen);
    reader.ImplicitProfileId = kWeaveProfile_ServiceProvisioning;

    err = reader.Next(kTLVType_Structure, ProfileTag(kWeaveProfile_ServiceProvisioning, ServiceProvisioning::kTag_ServiceConfig));
    SuccessOrExit(err);

    err = reader.EnterContainer(container);
    SuccessOrExit(err);

    err = reader.Next(kTLVType_Array, ContextTag(ServiceProvisioning::kTag_ServiceConfig_CACerts));
    SuccessOrExit(err);

    err = reader.Next(kTLVType_Structure, ContextTag(ServiceProvisioning::kTag_ServiceConfig_DirectoryEndPoint));
    SuccessOrExit(err);

    err = reader.EnterContainer(container);
    SuccessOrExit(err);

    err = reader.Next(kTLVType_UnsignedInteger, ContextTag(ServiceProvisioning::kTag_ServiceEndPoint_Id));
    SuccessOrExit(err);

    err = reader.Get(directoryEndpointId);
    SuccessOrExit(err);

    err = reader.Next(kTLVType_Array, ContextTag(ServiceProvisioning::kTag_ServiceEndPoint_Addresses));
    SuccessOrExit(err);

    err = reader.EnterContainer(container);
    SuccessOrExit(err);

    // Encode the initial portion of the directory entry.
    Write8(p, 0x40);                                // Directory Entry Control Byte (Entry Type = Host/Port List, Host/Port List Length = 0)
    LittleEndian::Write64(p, directoryEndpointId);  // Service Endpoint Id

    while (numHostPortEntries < 7 && (err = reader.Next()) == WEAVE_NO_ERROR)
    {
        const uint8_t * hostName;
        uint32_t hostNameLen;
        uint16_t port;

        VerifyOrExit(reader.GetType() == kTLVType_Structure, err = WEAVE_ERROR_INVALID_TLV_ELEMENT);

        err = reader.EnterContainer(container);
        SuccessOrExit(err);

        err = reader.Next(kTLVType_UTF8String, ContextTag(ServiceProvisioning::kTag_ServiceEndPointAddress_HostName));
        SuccessOrExit(err);

        err = reader.GetDataPtr(hostName);
        SuccessOrExit(err);

        hostNameLen = reader.GetLength();
        VerifyOrExit(hostNameLen <= NL_DNS_HOSTNAME_MAX_LEN, err = WEAVE_ERROR_INVALID_ARGUMENT);

        err = reader.Next(kTLVType_UnsignedInteger, ContextTag(ServiceProvisioning::kTag_ServiceEndPointAddress_Port));
        if (err == WEAVE_END_OF_TLV)
        {
            port = WEAVE_PORT;
            err = WEAVE_NO_ERROR;
        }
        else
        {
            SuccessOrExit(err);
            err = reader.Get(port);
            SuccessOrExit(err);
        }

        const size_t encodedEntrySize =
                1 +                         // Host/Port Entry Control Byte
                1 +                         // Host Name length
                hostNameLen +               // Host Name string
                2;                          // Port

        const ptrdiff_t remainingSpace = (rootDirBuf + rootDirBufSize) - p;

        if (remainingSpace < encodedEntrySize)
        {
            VerifyOrExit(numHostPortEntries > 1, err = WEAVE_ERROR_BUFFER_TOO_SMALL);
            break;
        }

        // Encode the Host/Port entry
        Write8(p, 0x08);                    // Host/Port Entry Control Byte (Type = Fully Qualified, Suffix Index Present = false, Port Id Present = true)
        Write8(p, (uint8_t)hostNameLen);    // Host Name length
        memcpy(p, hostName, hostNameLen);   // Host Name string
        p += hostNameLen;
        LittleEndian::Write16(p, port);     // Port

        numHostPortEntries++;

        err = reader.ExitContainer(kTLVType_Array);
        SuccessOrExit(err);
    }
    VerifyOrExit(err == WEAVE_NO_ERROR || err == WEAVE_END_OF_TLV, /* */);
    err = WEAVE_NO_ERROR;

    // Service config must include at least on root directory entry.
    VerifyOrExit(numHostPortEntries >= 1, err = WEAVE_ERROR_INVALID_ARGUMENT);

    // Encode the number of Host/Port entries in the Directory Entry Control Byte.
    *rootDirBuf += numHostPortEntries;

exit:
    return err;
}

WEAVE_ERROR GetRootDirectoryEntry(uint8_t * rootDirBuf, uint16_t rootDirBufSize)
{
    WEAVE_ERROR err;
    uint8_t * serviceConfig = NULL;
    size_t serviceConfigLen;

    // Determine the length of the service configuration.
    err = ConfigurationMgr().GetServiceConfig(NULL, 0, serviceConfigLen);
    SuccessOrExit(err);

    // Allocate a buffer to hold the service config data.
    serviceConfig = (uint8_t *)malloc(serviceConfigLen);
    VerifyOrExit(serviceConfig != NULL, err = WEAVE_ERROR_NO_MEMORY);

    // Fetch the service config from the configuration manager.
    err = ConfigurationMgr().GetServiceConfig(serviceConfig, serviceConfigLen, serviceConfigLen);
    SuccessOrExit(err);

    // Encode a root service directory entry from the information in the service config.
    err = EncodeRootDirectoryFromServiceConfig(serviceConfig, serviceConfigLen, rootDirBuf, rootDirBufSize);
    SuccessOrExit(err);

exit:
    if (serviceConfig != NULL)
    {
        free(serviceConfig);
    }
    return err;
}

} // unnamed namespace

#endif // WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY

} // namespace Internal
} // namespace DeviceLayer
} // namespace Weave
} // namespace nl

