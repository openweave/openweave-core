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
 *      Implementation of the fault-injection utilities for Weave.
 */

#include <string.h>
#include <nlassert.h>

#include <Weave/Support/WeaveFaultInjection.h>


namespace nl {
namespace Weave {
namespace FaultInjection {

static nl::FaultInjection::Record sFaultRecordArray[kFault_NumItems];
static int32_t sFault_WDMNotificationSize_Arguments[1];
static int32_t sFault_FuzzExchangeHeader_Arguments[1];
static class nl::FaultInjection::Manager sWeaveFaultInMgr;
static const char *sManagerName = "Weave";
static const char *sFaultNames[] = {
    "AllocExchangeContext",
    "DropIncomingUDPMsg",
    "AllocBinding",
    "SendAlarm",
    "HandleAlarm",
    "FuzzExchangeHeaderTx",
    "WRMDoubleTx",
    "WRMSendError",
    "BDXBadBlockCounter",
    "BDXAllocTransfer",
    "SMConnectRequestNew",
    "SMLookup",
    "WDMTraitInstanceNew",
    "WDMSubscriptionHandlerNew",
    "WDMSubscriptionClientNew",
    "WDMBadSubscriptionId",
    "WDMSendUnsupportedReqMsgType",
    "WDMNotificationSize",
    "WDMSendCommandExpired",
    "WDMSendCommandBadVersion",
    "CASEKeyConfirm",
    "SecMgrBusy",
};


/**
 * Get the singleton FaultInjection::Manager for Inet faults
 */
nl::FaultInjection::Manager &GetManager(void)
{
    if (0 == sWeaveFaultInMgr.GetNumFaults())
    {
        sWeaveFaultInMgr.Init(kFault_NumItems,
                              sFaultRecordArray,
                              sManagerName,
                              sFaultNames);
        memset(&sFault_WDMNotificationSize_Arguments, 0, sizeof(sFault_WDMNotificationSize_Arguments));
        sFaultRecordArray[kFault_WDM_NotificationSize].mArguments = sFault_WDMNotificationSize_Arguments;
        sFaultRecordArray[kFault_WDM_NotificationSize].mLengthOfArguments =
            static_cast<uint16_t>(sizeof(sFault_WDMNotificationSize_Arguments)/sizeof(sFault_WDMNotificationSize_Arguments[0]));

        memset(&sFault_FuzzExchangeHeader_Arguments, 0, sizeof(sFault_FuzzExchangeHeader_Arguments));
        sFaultRecordArray[kFault_FuzzExchangeHeaderTx].mArguments = sFault_FuzzExchangeHeader_Arguments;
        sFaultRecordArray[kFault_FuzzExchangeHeaderTx].mLengthOfArguments =
            static_cast<uint16_t>(sizeof(sFault_FuzzExchangeHeader_Arguments)/sizeof(sFault_FuzzExchangeHeader_Arguments[0]));

    }
    return sWeaveFaultInMgr;
}

/**
 * Fuzz a byte of a Weave Exchange Header
 *
 * @param[in] p     Pointer to the encoded Exchange Header
 * @param[in] arg   An index from 0 to (WEAVE_FAULT_INJECTION_NUM_FUZZ_VALUES * 5 -1)
 *                  that specifies the byte to be corrupted and the value to use.
 */
NL_DLL_EXPORT void FuzzExchangeHeader(uint8_t *p, int32_t arg)
{
    // Weave is little endian; this function alters the
    // least significant byte of the header fields.
    const uint8_t offsets[] = {
        0, // flags and version
        1, // MessageType
        2, // ExchangeId
        4, // ProfileId
        8  // AckMsgId
    };
    const uint8_t values[WEAVE_FAULT_INJECTION_NUM_FUZZ_VALUES] = { 0x1, 0x2, 0xFF };
    size_t offsetIndex = 0;
    size_t valueIndex = 0;
    size_t numOffsets = sizeof(offsets)/sizeof(offsets[0]);
    offsetIndex = arg % (numOffsets);
    valueIndex = (arg / numOffsets) % WEAVE_FAULT_INJECTION_NUM_FUZZ_VALUES;
    p[offsetIndex] ^= values[valueIndex];
}

} // namespace FaultInjection
} // namespace Weave
} // namespace nl
