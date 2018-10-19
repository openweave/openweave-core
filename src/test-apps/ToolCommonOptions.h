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
 *      Common command-line option handling code for test applications.
 *
 */

#ifndef TOOLCOMMONOPTIONS_H_
#define TOOLCOMMONOPTIONS_H_

#include <SystemLayer/SystemLayer.h>
#include <InetLayer/InetLayer.h>
#include <Weave/Core/WeaveCore.h>

#include <Weave/Support/nlargparser.hpp>

using namespace nl::ArgParser;

namespace nl {
namespace Weave {
namespace Profiles {
namespace ServiceDirectory {
struct ServiceConnectBeginArgs;
}; // ServiceDirectory
}; // Profiles
}; // Weave
}; // nl

#define TOOL_OPTIONS_ENV_VAR_NAME "WEAVE_TEST_OPTIONS"

enum
{
    kToolCommonOpt_NodeAddr                     = 1000,
    kToolCommonOpt_NodeCert,
    kToolCommonOpt_NodeKey,
    kToolCommonOpt_CACert,
    kToolCommonOpt_NoCACert,
    kToolCommonOpt_EventDelay,
    kToolCommonOpt_FaultInjection,
    kToolCommonOpt_FaultTestIterations,
    kToolCommonOpt_DebugResourceUsage,
    kToolCommonOpt_PrintFaultCounters,
    kToolCommonOpt_ExtraCleanupTime,
    kToolCommonOpt_CASEConfig,
    kToolCommonOpt_AllowedCASEConfigs,
    kToolCommonOpt_DebugCASE,
    kToolCommonOpt_CASEUseKnownECDHKey,
    kToolCommonOpt_KeyExportConfig,
    kToolCommonOpt_AllowedKeyExportConfigs,
    kToolCommonOpt_AccessToken,
    kToolCommonOpt_DebugLwIP,
    kToolCommonOpt_DeviceSerialNum,
    kToolCommonOpt_DeviceVendorId,
    kToolCommonOpt_DeviceProductId,
    kToolCommonOpt_DeviceProductRevision,
    kToolCommonOpt_DeviceSoftwareVersion,
    kToolCommonOpt_ServiceDirServer,
    kToolCommonOpt_ServiceDirDNSOptions,
    kToolCommonOpt_ServiceDirTargetDNSOptions,
    kToolCommonOpt_IPv4GatewayAddr,
    kToolCommonOpt_WRMPACKDelay,
    kToolCommonOpt_WRMPRetransInterval,
    kToolCommonOpt_WRMPRetransCount,
    kToolCommonOpt_TAKEReauth,
    kToolCommonOpt_PairingCode,
    kToolCommonOpt_PersistentCntrFile,
    kToolCommonOpt_GroupEncKeyId,
    kToolCommonOpt_GroupEncKeyType,
    kToolCommonOpt_GroupEncRootKey,
    kToolCommonOpt_GroupEncEpochKeyNum,
    kToolCommonOpt_GroupEncAppGroupMasterKeyNum,
    kToolCommonOpt_SecurityNone,
    kToolCommonOpt_SecurityCASE,
    kToolCommonOpt_SecurityCASEShared,
    kToolCommonOpt_SecurityPASE,
    kToolCommonOpt_SecurityGroupEnc,
    kToolCommonOpt_SecurityTAKE,
    kToolCommonOpt_GeneralSecurityIdleSessionTimeout,
    kToolCommonOpt_GeneralSecuritySessionEstablishmentTimeout,
};


/**
 * Handler for options that control local network/network interface configuration.
 */
class NetworkOptions : public OptionSetBase
{
public:
    nl::Inet::IPAddress LocalIPv4Addr;
    nl::Inet::IPAddress LocalIPv6Addr;

#if WEAVE_SYSTEM_CONFIG_USE_LWIP
    nl::Inet::IPAddress IPv4GatewayAddr;
    nl::Inet::IPAddress DNSServerAddr;
    const char *TapDeviceName;
    uint8_t LwIPDebugFlags;
    uint32_t EventDelay;
#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP

    NetworkOptions();

    virtual bool HandleOption(const char *progName, OptionSet *optSet, int id, const char *name, const char *arg);
};

extern NetworkOptions gNetworkOptions;


/**
 * Handler for options that control Weave node configuration.
 */
class WeaveNodeOptions : public OptionSetBase
{
public:
    uint64_t FabricId;
    uint64_t LocalNodeId;
    uint16_t SubnetId;
    bool FabricIdSet;
    bool LocalNodeIdSet;
    bool SubnetIdSet;
    const char *PairingCode;

    // TODO (arg clean up): add common function to infer ids from local address and remove duplicate code from tool main functions.

    WeaveNodeOptions();
    ~WeaveNodeOptions();

    virtual bool HandleOption(const char *progName, OptionSet *optSet, int id, const char *name, const char *arg);
};

extern WeaveNodeOptions gWeaveNodeOptions;


/**
 * Handler for options that control Weave Reliable Messaging protocol configuration.
 */
class WRMPOptions : public OptionSetBase
{
public:
    uint16_t ACKDelay;
    uint32_t RetransInterval;
    uint8_t RetransCount;

    WRMPOptions();

    nl::Weave::WRMPConfig GetWRMPConfig(void) const;

    virtual bool HandleOption(const char *progName, OptionSet *optSet, int id, const char *name, const char *arg);
};

extern WRMPOptions gWRMPOptions;


/**
 * Handler for options that control Weave Security protocol configuration.
 */
class WeaveSecurityMode : public OptionSetBase
{
public:

    enum
    {
        kNone,
        kCASE,
        kCASEShared,
        kPASE,
        kTAKE,
        kGroupEnc
    };

    uint32_t SecurityMode;

    WeaveSecurityMode();

    virtual bool HandleOption(const char *progName, OptionSet *optSet, int id, const char *name, const char *arg);
};

extern WeaveSecurityMode gWeaveSecurityMode;


/**
 * Handler for options that control the configuration of Weave message encryption using group keys.
 */
class GroupKeyEncOptions : public OptionSetBase
{
public:
    GroupKeyEncOptions();

    uint32_t GetEncKeyId() const;

    virtual bool HandleOption(const char *progName, OptionSet *optSet, int id, const char *name, const char *arg);

private:
    uint32_t EncKeyId;
    uint32_t EncKeyType;
    uint32_t RootKeyId;
    uint32_t EpochKeyId;
    uint32_t AppGroupMasterKeyId;
};

extern GroupKeyEncOptions gGroupKeyEncOptions;

/**
 * Handler for options that control the configuration of security related parameters
 */
class GeneralSecurityOptions : public OptionSetBase
{
public:
    GeneralSecurityOptions();

    uint32_t GetIdleSessionTimeout() const;
    uint32_t GetSessionEstablishmentTimeout() const;

    virtual bool HandleOption(const char *progName, OptionSet *optSet, int id, const char *name, const char *arg);

private:
    uint32_t IdleSessionTimeout;
    uint32_t SessionEstablishmentTimeout;
};

extern GeneralSecurityOptions gGeneralSecurityOptions;


/**
 * Handler for options that control Weave service directory client configuration.
 */
class ServiceDirClientOptions : public OptionSetBase
{
    using ServiceConnectBeginArgs = ::nl::Weave::Profiles::ServiceDirectory::ServiceConnectBeginArgs;

public:
    const char *ServerHost;
    uint16_t ServerPort;
#if WEAVE_CONFIG_ENABLE_DNS_RESOLVER
    uint8_t DNSOptions_ServiceDirEndpoint;
    uint8_t DNSOptions_TargetEndpoint;
#endif

    ServiceDirClientOptions();

    WEAVE_ERROR GetRootDirectoryEntry(uint8_t *buf, uint16_t bufSize);
    void OverrideConnectArguments(ServiceConnectBeginArgs & args);

    virtual bool HandleOption(const char *progName, OptionSet *optSet, int id, const char *name, const char *arg);
};

extern ServiceDirClientOptions gServiceDirClientOptions;
extern WEAVE_ERROR GetRootServiceDirectoryEntry(uint8_t *buf, uint16_t bufSize);
extern void OverrideServiceConnectArguments(::nl::Weave::Profiles::ServiceDirectory::ServiceConnectBeginArgs & args);


/**
 * Handler for options that control fault injection testing behavior.
 */
class FaultInjectionOptions : public OptionSetBase
{
public:
    uint32_t TestIterations;
    bool DebugResourceUsage;
    bool PrintFaultCounters;
    uint32_t ExtraCleanupTimeMsec;

    FaultInjectionOptions();

    virtual bool HandleOption(const char *progName, OptionSet *optSet, int id, const char *name, const char *arg);
};

extern FaultInjectionOptions gFaultInjectionOptions;

extern bool ParseDNSOptions(const char * progName, const char *argName, const char * arg, uint8_t & dnsOptions);


#endif // TOOLCOMMONOPTIONS_H_
