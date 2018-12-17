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
 *      This file defines constants, globals and interfaces common to
 *      and used by all Weave test applications and tools.
 *
 *      NOTE: These do not comprise a public part of the Weave API and
 *            are subject to change without notice.
 *
 */

#ifndef TOOLCOMMON_H_
#define TOOLCOMMON_H_

#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif
#include <stdint.h>

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <time.h>
#include <sys/time.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <pthread.h>

#include <nlfaultinjection.hpp>

#include "ToolCommonOptions.h"
#include "CASEOptions.h"
#include "TAKEOptions.h"
#include "KeyExportOptions.h"
#include "DeviceDescOptions.h"

#include <Weave/WeaveVersion.h>
#include <SystemLayer/SystemLayer.h>
#include <InetLayer/InetLayer.h>
#include <Weave/Core/WeaveCore.h>
#include <Weave/Support/CodeUtils.h>
#include <Weave/Support/ErrorStr.h>
#include <Weave/Core/WeaveStats.h>

#if CONFIG_BLE_PLATFORM_BLUEZ
#include <BleLayer/BleApplicationDelegate.h>
#include <PlatformLayer/Ble/Bluez/BluezBlePlatformDelegate.h>
#include <PlatformLayer/Ble/Bluez/WoBluezLayer.h>
#endif // CONFIG_BLE_PLATFORM_BLUEZ


using namespace nl::Inet;
using namespace nl::Weave;
using namespace nl::Weave::Profiles;

#ifndef nlDEFINE_ALIGNED_VAR
#define nlDEFINE_ALIGNED_VAR(varName, bytes, alignment_type) \
  alignment_type varName[(((bytes)+(sizeof(alignment_type)-1))/sizeof(alignment_type))]
#endif

#define WEAVE_TOOL_COPYRIGHT "Copyright (c) 2018 Google LLC.\nCopyright (c) 2013-2018 Nest Labs, Inc.\nAll rights reserved.\n"

extern System::Layer SystemLayer;

extern InetLayer Inet;
extern BleLayer Ble;

extern uint16_t sTestDefaultUDPSessionKeyId;
extern uint16_t sTestDefaultTCPSessionKeyId;
extern uint16_t sTestDefaultSessionKeyId;

extern bool Done;
extern bool gSigusr1Received;

struct TestNodeCert
{
    uint64_t NodeId;
    const uint8_t *Cert;
    uint16_t CertLength;
    const uint8_t *PrivateKey;
    uint16_t PrivateKeyLength;
};

extern TestNodeCert TestNodeCerts[];

struct TestCACert
{
    uint64_t CAId;
    const uint8_t *Cert;
    uint16_t CertLength;
};

extern TestCACert TestCACerts[];
extern uint64_t TestMockRoot_CAId;
extern uint64_t TestMockServiceEndpointCA_CAId;

extern uint64_t TestDevice1_NodeId;
extern uint8_t TestDevice1_Cert[];
extern uint16_t TestDevice1_CertLength;
extern uint8_t TestDevice1_PrivateKey[];
extern uint16_t TestDevice1_PrivateKeyLength;
extern uint64_t TestDevice2_NodeId;
extern uint8_t TestDevice2_Cert[];
extern uint16_t TestDevice2_CertLength;
extern uint8_t TestDevice2_PrivateKey[];
extern uint16_t TestDevice2_PrivateKeyLength;

extern bool sSuppressAccessControls;

extern void InitToolCommon();
extern void UseStdoutLineBuffering();
extern void SetSIGUSR1Handler(void);
extern void InitSystemLayer();
extern void ShutdownSystemLayer();
typedef void (*SignalHandler)(int signum);
extern void SetSignalHandler(SignalHandler handler);
extern void DoneOnHandleSIGUSR1(int);
extern void InitNetwork();
extern void PrintNodeConfig();
extern void ServiceEvents(::timeval& aSleepTime);
extern void InitWeaveStack(bool listen, bool initExchangeMgr);
extern void ShutdownNetwork();
extern void ShutdownWeaveStack();
extern bool ParseCASEConfig(const char *str, uint32_t& output);
extern bool ParseAllowedCASEConfigs(const char *strConst, uint8_t& output);
extern void DumpMemory(const uint8_t *mem, uint32_t len, const char *prefix, uint32_t rowWidth = 16);
extern void DumpMemoryCStyle(const uint8_t *mem, uint32_t len, const char *prefix, uint32_t rowWidth = 16);
extern bool IsZeroBytes(const uint8_t *buf, uint32_t len);
extern void PrintMACAddress(const uint8_t *buf, uint32_t len);
extern void PrintAddresses();
extern uint8_t *ReadFileArg(const char *fileName, uint32_t& len, uint32_t maxLen = UINT32_MAX);
extern void HandleMessageReceiveError(WeaveMessageLayer *msgLayer, WEAVE_ERROR err, const IPPacketInfo *pktInfo);
extern void HandleAcceptConnectionError(WeaveMessageLayer *msgLayer, WEAVE_ERROR err);
extern bool GetTestNodeCert(uint64_t nodeId, const uint8_t *& cert, uint16_t& certLen);
extern bool GetTestNodePrivateKey(uint64_t nodeId, const uint8_t *& key, uint16_t& keyLen);
extern bool GetTestCACert(uint64_t caId, const uint8_t *& cert, uint16_t& certLen);
extern bool GetTestCAPrivateKey(uint64_t caId, const uint8_t *& key, uint16_t& keyLen);

#if CONFIG_BLE_PLATFORM_BLUEZ
void *WeaveBleIOLoop(void *arg);
nl::Ble::Platform::BlueZ::BluezBlePlatformDelegate *getBluezPlatformDelegate();
nl::Ble::Platform::BlueZ::BluezBleApplicationDelegate *getBluezApplicationDelegate();
#endif // CONFIG_BLE_PLATFORM_BLUEZ

inline static void ServiceNetwork(struct ::timeval aSleepTime)
{
    ServiceEvents(aSleepTime);
}

extern void ServiceNetworkUntil(const bool *aDone, const uint32_t *aIntervalMs = NULL);

extern void PrintStatsCounters(nl::Weave::System::Stats::count_t *counters, const char *aPrefix);
extern bool ProcessStats(nl::Weave::System::Stats::Snapshot &aBefore, nl::Weave::System::Stats::Snapshot &aAfter, bool aPrint, const char *aPrefix);
extern void PrintFaultInjectionCounters(void);
extern void SetupFaultInjectionContext(int argc, char *argv[]);
extern void SetupFaultInjectionContext(int argc, char *argv[], int32_t (*aNumEventsAvailable)(void), void (*aInjectAsyncEvents)(int32_t index));
#define TOOL_COMMON_FIRST_APP_ASYNC_EVENT 1
extern void (*gAsyncEventCb)(uint16_t numArgs, int32_t argument);

#if WEAVE_CONFIG_ENABLE_TUNNELING

#if !WEAVE_TUNNEL_CONFIG_WILL_OVERRIDE_ADDR_ROUTING_FUNCS
extern INET_ERROR InterfaceAddAddress(InterfaceId tunIf, IPAddress ipAddr, uint8_t prefixLen);
extern INET_ERROR InterfaceRemoveAddress(InterfaceId tunIf, IPAddress ipAddr, uint8_t prefixLen);
extern INET_ERROR SetRouteToTunnelInterface(InterfaceId tunIf, IPPrefix ipPrefix, TunEndPoint::RouteOp routeAddDel);
#endif // WEAVE_TUNNEL_CONFIG_WILL_OVERRIDE_ADDR_ROUTING_FUNCS

#endif // WEAVE_CONFIG_ENABLE_TUNNELING
using nl::ErrorStr;

inline uint64_t Now()
{
	struct timeval now;
	gettimeofday(&now, NULL);
	return ((uint64_t)now.tv_sec * 1000000) + (uint64_t)now.tv_usec;
}
inline uint64_t NowMs()
{
    return Now()/1000;
}

#define FAIL_ERROR(ERR, MSG) \
	do { \
		if ((ERR) != WEAVE_NO_ERROR) \
		{ \
			fprintf(stderr, "%s: %s\n", (MSG), ErrorStr(ERR)); \
			exit(-1); \
		} \
	} while (0)

#endif /* TOOLCOMMON_H_ */
