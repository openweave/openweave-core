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
 *      This file defines the class interface for WdmNextPerfUtility.cpp
 *
 *
 */

#include <stdio.h>
#include <sys/time.h>
#include <vector>
#include <fstream>
#include <Weave/Support/CodeUtils.h>

#if defined (__APPLE__) && defined (__MACH__)
#include<mach/mach.h>
#elif defined (__unix__) || defined(__linux__)
#include "stdlib.h"
#include "string.h"
#endif

#define ENABLE_WDMPERFDATA 1


struct perfData{
    int index;
    timeval latency;
    size_t vmsize;
    size_t vmrss;
};


class WdmNextPerfUtility
{
public:
    static WdmNextPerfUtility* Instance();
    static void Remove();
    void operator()();
    void ReportPerf();
    void ReportAll();
    void SetPerf();
    perfData GetPerf();
    void SaveToFile();
    int GetCurrentTimestamp(char *buf, size_t num);

private:
    WdmNextPerfUtility();
    ~WdmNextPerfUtility();

    static WdmNextPerfUtility *mInstance;
    std::vector<perfData> mAllData;
    perfData mPerfData;
    timeval mCurrentTime, mLastTime, mDeltaTime;

#if defined (__APPLE__) && defined (__MACH__)

    task_basic_info mMac_mem_info;
    mach_msg_type_number_t mMac_mem_info_count;

#endif

};

