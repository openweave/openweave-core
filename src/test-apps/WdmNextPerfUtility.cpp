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
 *      This file implements performance test utility for the Weave Data Management(WDM) Next Profile
 *      including latency and consumed virtual memory statistic.
 *
 *
 */

#include "WdmNextPerfUtility.h"

#ifdef ENABLE_WDMPERFDATA
#include <stdio.h>
#include <sys/time.h>


WdmNextPerfUtility *WdmNextPerfUtility::mInstance = 0;

WdmNextPerfUtility *WdmNextPerfUtility::Instance()
{
    if (mInstance)
        return mInstance;
    mInstance = new WdmNextPerfUtility;
    return mInstance;
}

void WdmNextPerfUtility::Remove()
{
    if (mInstance)
        delete mInstance;
}

WdmNextPerfUtility::WdmNextPerfUtility()
{
    int status;
    status = gettimeofday(&mLastTime, NULL);
    if (status != 0)
        printf("gettimeofday error in WdmNextPerfUtility()\n");
}

WdmNextPerfUtility::~WdmNextPerfUtility() { }

/**
 * This is the implementation to obtain time latency since last call and save current time.
 *
 */
void WdmNextPerfUtility::operator()()
{
    int status = gettimeofday(&mCurrentTime, NULL);
    if (status != 0)
        printf("gettimeofday error in WdmNextPerfUtility()\n");
    mDeltaTime.tv_sec = mCurrentTime.tv_sec - mLastTime.tv_sec;
    mDeltaTime.tv_usec = mCurrentTime.tv_usec - mLastTime.tv_usec;
    if (mDeltaTime.tv_usec < 0)
    {
        mDeltaTime.tv_usec = -1 * mDeltaTime.tv_usec;
        mDeltaTime.tv_sec = mDeltaTime.tv_sec - 1;
    }

    mLastTime = mCurrentTime;
}

/**
 * This is implementation to save the current consumed virtual memory and time latency since last run to mAllData.
 *
 */
void WdmNextPerfUtility::SetPerf()
{
    mPerfData.vmsize = -1;
    mPerfData.vmrss = -1;

#if defined (__APPLE__) && defined (__MACH__)

    mMac_mem_info_count = TASK_BASIC_INFO_COUNT;

    if (task_info(mach_task_self(), TASK_BASIC_INFO, (task_info_t)&mMac_mem_info, &mMac_mem_info_count) != KERN_SUCCESS)
    {
        printf("task_info error in mac\n");
    }
    else
    {
        mPerfData.vmsize = (size_t)mMac_mem_info.virtual_size;
        mPerfData.vmrss = (size_t)mMac_mem_info.resident_size;
    }

#elif defined (__unix__) || defined(__linux__)

    FILE* filep = fopen("/proc/self/status", "r");
    if (filep == NULL)
        printf("file /proc/self/status cannot be open");
    char scanned_line[256];

    while (fgets(scanned_line, 256, filep) != NULL)
    {

        if (strncmp(scanned_line, "VmSize", 6) == 0)
        {
            char * step = &scanned_line[0];
            step += 7;
            while ( ! (*step < '9' && *step > '0'))
            {
                step += 1;
            }
            scanned_line[strlen(step) - 3] = '\0';
            mPerfData.vmsize = (size_t)(atoi(step) * 1024);
        }

        if (strncmp(scanned_line, "VmRSS", 5) == 0)
        {
            char * step = &scanned_line[0];
            step += 7;
            while ( ! (*step < '9' && *step > '0'))
            {
                step += 1;
            }
            scanned_line[strlen(step) - 3] = '\0';
            mPerfData.vmrss = (size_t)(atoi(step) * 1024);
            break;
        }
    }
    fclose(filep);

#else // defined (__APPLE__) && defined (__MACH__)

    printf("not supported OS\n");

#endif // defined (__APPLE__) && defined (__MACH__)

    mPerfData.index = mAllData.size() + 1;
    mPerfData.latency = mDeltaTime;
    mAllData.push_back(mPerfData);
}

/**
 * This is implementation to get the performance data in current run.
 *
 */
perfData WdmNextPerfUtility::GetPerf()
{
    return mPerfData;
}


/**
 * This is implementation to report the performance data in current run.
 *
 */
void WdmNextPerfUtility::ReportPerf()
{
    printf("current perf data: index is %d, latency period = %d.%06d seconds, virtual memory is %zu Bytes, resident size is %zu Bytes \n",
           mPerfData.index, static_cast<int>(mPerfData.latency.tv_sec), static_cast<int>(mPerfData.latency.tv_usec), mPerfData.vmsize, mPerfData.vmrss);
}


/**
 * This is implementation to print all perf data so far.
 *
 */
void WdmNextPerfUtility::ReportAll()
{
    for (std::vector<perfData>::iterator element = mAllData.begin(); element != mAllData.end(); ++element)
        printf("All perf data: index is %d, latency period = %d.%06d seconds, virtual memory is %zu Bytes, resident size is %zu Bytes \n",
        element->index, static_cast<int>(element->latency.tv_sec), static_cast<int>(element->latency.tv_usec), element->vmsize, element->vmrss);
}

/**
 * This is implementation to get current timestamp.
 *
 */
int WdmNextPerfUtility::GetCurrentTimestamp(char *buf, size_t num)
{
    struct timeval tv;
    struct tm* time_ptr;
    char detailed_time[30] = { 0 };
    int64_t milliseconds;
    int status = 0;

    VerifyOrExit(buf != NULL, status = -1);
    VerifyOrExit(num >= 50, status = -1);

    status = gettimeofday(&tv, NULL);
    VerifyOrExit(status == 0, perror("gettimeofday"));

    time_ptr = localtime(&tv.tv_sec);
    VerifyOrExit(time_ptr != NULL, status = -1; perror("localtime"));

    status = strftime(detailed_time, sizeof(detailed_time), "%F %T%z", time_ptr);
    VerifyOrExit(status >= 0, perror("strftime"));

    milliseconds = tv.tv_usec / 1000;
    snprintf(buf, num, "%s.%03ld\n", detailed_time, milliseconds);
exit:
    if (status < 0)
    {
        printf("error_in_GetCurrentTimestamp\"");
    }
    return status;
}

/**
 * This is implementation to save all perf data to file.
 */
void WdmNextPerfUtility::SaveToFile()
{
    char time[100] = { 0 };

    std::ofstream output_file("./WdmPerfData");

    if (GetCurrentTimestamp(time, sizeof(time)) >= 0)
    {
        output_file << "Save perf data at timestamp: " << time;
    }

    for (std::vector<perfData>::iterator element = mAllData.begin(); element != mAllData.end(); ++element)
    {
        output_file << "All perf data: index is " << element->index
        << ", latency period = " << static_cast<int>(element->latency.tv_sec) << "." <<
        static_cast<int>(element->latency.tv_usec) << "seconds"
        << ", virtual memory is " << element->vmsize
        << ", resident size is " << element->vmrss << std::endl;
    }
    output_file.close();
}
#endif //ENABLE_WDMPERFDATA
