/*
 *
 *    Copyright (c) 2016-2017 Nest Labs, Inc.
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
 *      This file defines an automated test suite for functional testing
 *      of the Weave Tunnel.
 */

#ifndef TEST_WEAVE_TUNNEL_H_
#define TEST_WEAVE_TUNNEL_H_

#include <Weave/Core/WeaveCore.h>
#include <Weave/Core/WeaveServerBase.h>
#include <Weave/Profiles/WeaveProfiles.h>

#include <nlunit-test.h>

#define DEFAULT_TEST_DURATION_MILLISECS               (5000)
#define RECONNECT_TEST_DURATION_MILLISECS             (2 * DEFAULT_TEST_DURATION_MILLISECS)
#define RECONNECT_RESET_TEST_DURATION_MILLISECS       (10 * DEFAULT_TEST_DURATION_MILLISECS)
#define TEST_SLEEP_TIME_WITHIN_LOOP_MICROSECS         (100000)
#define TEST_SLEEP_TIME_WITHIN_LOOP_SECS              (0)
#define TEST_TUNNEL_RECONNECT_HOSTNAME                "tunnel01.frontend.nestlabs.com"
#define TEST_CONN_ATTEMPTS_BEFORE_RESET               (4)
#define BACKOFF_RESET_IMMEDIATE_THRESHOLD_SECS        (1)
#define BACKOFF_RESET_RANDOMIZED_THRESHOLD_SECS       (11)

#if WEAVE_CONFIG_ENABLE_TUNNELING
using namespace ::nl::Inet;
using namespace ::nl::Weave;
enum
{
    kWeaveProfile_TunnelTest_Start                    = 201,
    kWeaveProfile_TunnelTest_End                      = 202,
    kWeaveProfile_TunnelTest_RequestTunnelConnDrop    = 203,
};

enum
{
    kTestNum_TestWeaveTunnelAgentInit                           = 0,
    kTestNum_TestWeaveTunnelAgentConfigure                      = 1,
    kTestNum_TestWeaveTunnelAgentShutdown                       = 2,
    kTestNum_TestStartTunnelWithoutInit                         = 3,
    kTestNum_TestBackToBackStartStopStart                       = 4,
    kTestNum_TestTunnelOpenCompleteThenStopStart                = 5,
    kTestNum_TestReceiveStatusReportForTunnelOpen               = 6,
    kTestNum_TestTunnelRestrictedRoutingOnTunnelOpen            = 7,
    kTestNum_TestTunnelOpenThenTunnelClose                      = 8,
    kTestNum_TestStandaloneTunnelSetup                          = 9,
    kTestNum_TestTunnelNoStatusReportReconnect                  = 10,
    kTestNum_TestTunnelErrorStatusReportReconnect               = 11,
    kTestNum_TestTunnelErrorStatusReportOnTunnelClose           = 12,
    kTestNum_TestTunnelConnectionDownReconnect                  = 13,
    kTestNum_TestCallTunnelDownAfterMaxReconnects               = 14,
    kTestNum_TestReceiveReconnectFromService                    = 15,
    kTestNum_TestWARMRouteAddWhenTunnelEstablished              = 16,
    kTestNum_TestWARMRouteDeleteWhenTunnelStopped               = 17,
    kTestNum_TestWeavePingOverTunnel                            = 18,
    kTestNum_TestQueueingOfTunneledPackets                      = 19,
    kTestNum_TestTunnelStatistics                               = 20,
    kTestNum_TestTCPUserTimeoutOnAddrRemoval                    = 21,
    kTestNum_TestTunnelLivenessSendAndRecvResponse              = 22,
    kTestNum_TestTunnelLivenessDisconnectOnNoResponse           = 23,
    kTestNum_TestTunnelResetReconnectBackoffImmediately         = 24,
    kTestNum_TestTunnelResetReconnectBackoffRandomized          = 25,
    kTestNum_TestTunnelNoStatusReportResetReconnectBackoff      = 26,
    kTestNum_TestTunnelRestrictedRoutingOnStandaloneTunnelOpen  = 27,
    kTestNum_TestTunnelTCPIdle                                  = 28,
};

#endif // WEAVE_CONFIG_ENABLE_TUNNELING
#endif // TEST_WEAVE_TUNNEL_H_
