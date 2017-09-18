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
 *      the Weave system and Internet access abstraction layer
 *      interfaces on Android.
 *
 */

#include <errno.h>
#include <stdio.h>
#include <sys/time.h>
#include <jni.h>
#include <android/log.h>
#include <sys/types.h>

#include <SystemLayer/SystemLayer.h>

#include "InetLayer.h"
#include "WeaveCore.h"
#include "WeaveEcho.h"

using namespace nl::Inet;
using namespace nl::Weave;
using namespace nl::Weave::Profiles;

using nl::Weave::System::PacketBuffer;

System::Layer SystemLayer;
InetLayer Inet;
WeaveEchoClient EchoClient;
uint64_t LastEchoTime = 0;
int32_t EchoInterval = 100000;
uint32_t EchoCount = 0;
uint32_t MaxEchoCount = 50;
bool EchoResponseReceived = false;

static WEAVE_ERROR InitWeaveStack();
static void ShutdownWeaveStack();
static void ServiceNetwork(struct timeval sleepTime);
static WEAVE_ERROR SendEchoRequest(const char * remoteAddr);
static void HandleEchoResponse(uint64_t nodeId, IPAddress nodeAddr, PacketBuffer *payload);
static uint64_t Now();
static uint64_t now_ms(void);
extern WEAVE_ERROR PingNode(const char * remoteAddr);

#define APP_ERROR_MIN                           0

#define ERROR_NO_RESPONSE_RECEVIED              (APP_ERROR_MIN + 1)
#define ERROR_INVALID_ADDRESS                   (APP_ERROR_MIN + 2)

#define LOG_TAG "NestWeave"

#define LOGV(LOG_TAG, ...) __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__)
#define LOGD(LOG_TAG, ...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGW(LOG_TAG, ...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGI(LOG_TAG, ...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

#define printf(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)

extern "C" jint
Java_com_example_PingTest_PingWrapper_pingNode(JNIEnv* env, jobject object, jstring remoteAddr)
{
    const char * addrString = env->GetStringUTFChars(remoteAddr, 0);
    int result = PingNode(addrString);
    env->ReleaseStringUTFChars(remoteAddr, addrString);
    return result;
}

WEAVE_ERROR PingNode(const char *remoteAddr)
{
    LOGV(LOG_TAG, "PingNode() started. MaxEchoCount: %d EchoInterval: %d EchoCount: %d", MaxEchoCount, EchoInterval, EchoCount);
    WEAVE_ERROR res;

    res = InitWeaveStack();

    if (res != WEAVE_NO_ERROR)
        return res;

    while (true)
    {
        struct timeval sleepTime;
        sleepTime.tv_sec = 0;
        sleepTime.tv_usec = 100000;

        ServiceNetwork(sleepTime);

        if (EchoResponseReceived)
            break;

        if (Now() >= LastEchoTime + EchoInterval)
        {
            LOGV(LOG_TAG, "Now() >= LastEchoTime + EchoInterval, MaxEchoCount: %d EchoInterval: %d EchoCount: %d", MaxEchoCount, EchoInterval, EchoCount);

            if (EchoCount == MaxEchoCount)
            {
                res = ERROR_NO_RESPONSE_RECEVIED;
                break;
            }

            res = SendEchoRequest(remoteAddr);

            if (res != WEAVE_NO_ERROR)
                break;

            LOGV(LOG_TAG, "Sent Echo Request to %s. Result was %d. WEAVE_NO_ERROR = %d", remoteAddr, res, WEAVE_NO_ERROR);
        }
        else
        {
            LOGV(LOG_TAG, "Not enough time elapsed. now_ms() == %llu", Now());
        }
    }

    ShutdownWeaveStack();

    return res;
}

WEAVE_ERROR SendEchoRequest(const char * remoteAddr)
{
    IPAddress ipAddr;
    WEAVE_ERROR res;
    
    if (!IPAddress::FromString(remoteAddr, ipAddr))
    {
        LOGV(LOG_TAG, "IPAddress::FromString returned %d. ERROR_INVALID_ADDRESS = %d", res, ERROR_INVALID_ADDRESS);
        return ERROR_INVALID_ADDRESS;
    }

    PacketBuffer *payloadBuf = PacketBuffer::New();
    if (payloadBuf == NULL)
    {
        printf("Unable to allocate PacketBuffer\n");
        return WEAVE_ERROR_NO_MEMORY;
    }

    char *p = (char *)payloadBuf->Start();
    strcpy(p, "Echo Message\n");
    payloadBuf->SetDataLength((uint16_t)strlen(p));

    LastEchoTime = Now();

    res = EchoClient.SendEchoRequest(ipAddr.InterfaceId(), ipAddr, payloadBuf);
    if (res != WEAVE_NO_ERROR)
    {
        printf("WeaveEchoClient.SendEchoRequest() failed: %ld\n", (long)res);
        return res;
    }

    EchoCount++;

    return WEAVE_NO_ERROR;
}

void HandleEchoResponse(uint64_t nodeId, IPAddress nodeAddr, PacketBuffer *payload)
{
    EchoResponseReceived = true;
}

WEAVE_ERROR InitWeaveStack()
{
    WEAVE_ERROR res;

#if WEAVE_SYSTEM_CONFIG_USE_LWIP
    tcpip_init(NULL, NULL);
#endif

    res = SystemLayer.Init();
    if (res != WEAVE_SYSTEM_NO_ERROR)
        printf("SystemLayer.Init failed: %ld\n", (long)res);

    res = Inet.Init(SystemLayer);
    if (res != WEAVE_NO_ERROR)
        printf("InetLayer.Init failed: %ld\n", (long)res);

    if (res == WEAVE_NO_ERROR)
    {
        // Initialize the FabricState object.
        res = FabricState.Init();
        if (res != WEAVE_NO_ERROR)
          printf("FabricState.Init failed: %ld\n", (long)res);
    }

    if (res == WEAVE_NO_ERROR)
    {
        FabricState.FabricId = kFabricIdDefaultForTest;
        FabricState.LocalNodeId = 1;
        FabricState.DefaultSubnet = 1;

        // Initialize the WeaveMessageLayer object.
        res = MessageLayer.Init(&Inet, &FabricState);
        if (res != WEAVE_NO_ERROR)
            printf("WeaveMessageLayer.Init failed: %ld\n", (long)res);
    }

    if (res == WEAVE_NO_ERROR)
    {
        // Initialize the Exchange Manager object.
        res = ExchangeMgr.Init(&MessageLayer);
        if (res != WEAVE_NO_ERROR)
            printf("WeaveExchangeManager.Init failed: %ld\n", (long)res);
    }

    if (res == WEAVE_NO_ERROR)
    {
        // Initialize the EchoClient application.
        res = EchoClient.Init(&ExchangeMgr);
        if (res != WEAVE_NO_ERROR)
            printf("WeaveEchoClient.Init failed: %ld\n", (long)res);
    }

    // Arrange to get a callback whenever an Echo Response is received.
    if (res == WEAVE_NO_ERROR)
    {
        EchoClient.OnEchoResponseReceived = HandleEchoResponse;
    }

    if (res != WEAVE_NO_ERROR)
        ShutdownWeaveStack();

    return res;
}

void ShutdownWeaveStack()
{
    EchoClient.Shutdown();
    ExchangeMgr.Shutdown();
    MessageLayer.Shutdown();
    FabricState.Shutdown();
    Inet.Shutdown();
    SystemLayer.Shutdown();
#if WEAVE_SYSTEM_CONFIG_USE_LWIP
    tcpip_finish(NULL, NULL);
#endif
}

void ServiceNetwork(struct timeval sleepTime)
{
    fd_set readFDs, writeFDs, exceptFDs;
    int numFDs = 0;

    FD_ZERO(&readFDs);
    FD_ZERO(&writeFDs);
    FD_ZERO(&exceptFDs);

    if (Inet.State == InetLayer::kState_Initialized)
        Inet.PrepareSelect(numFDs, &readFDs, &writeFDs, &exceptFDs);

    int selectRes = select(numFDs, &readFDs, &writeFDs, &exceptFDs, &sleepTime);
    if (selectRes < 0)
        printf("select failed: %ld\n", (long)(errno));

    if (selectRes > 0 && Inet.State == InetLayer::kState_Initialized)
        Inet.HandleIO(&readFDs, &writeFDs, &exceptFDs);
}

uint64_t Now()
{
    struct timeval now;
    gettimeofday(&now, NULL);
    return ((uint64_t)now.tv_sec * 1000000) + (uint64_t)now.tv_usec;
}

#ifdef DEFINE_MAIN 

int main()
{
    WEAVE_ERROR err = PingNode(2);
    printf("%ld\n", (long)err);
}

#endif
