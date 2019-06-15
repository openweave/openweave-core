/*
 *
 *    Copyright (c) 2019 Google LLC.
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
 *          ESP32 program to initiate a device reset.
 */

#include "esp_attr.h"
#include "soc/cpu.h"
#include "soc/soc.h"
#include "soc/rtc.h"
#include "soc/rtc_cntl_reg.h"

void main()
{
    // Fire the RTC watchdog after about a second.
    REG_SET_FIELD(RTC_CNTL_CLK_CONF_REG, RTC_CNTL_ANA_CLK_RTC_SEL, RTC_SLOW_FREQ_RTC);
    REG_WRITE(RTC_CNTL_WDTWPROTECT_REG, RTC_CNTL_WDT_WKEY_VALUE);
    REG_WRITE(RTC_CNTL_WDTCONFIG0_REG,
              RTC_CNTL_WDT_EN |
              RTC_CNTL_WDT_FLASHBOOT_MOD_EN_M |
              (RTC_WDT_STG_SEL_RESET_RTC << RTC_CNTL_WDT_STG0_S) |
              (1 << RTC_CNTL_WDT_SYS_RESET_LENGTH_S));
    REG_WRITE(RTC_CNTL_WDTCONFIG1_REG, 150000);
    REG_WRITE(RTC_CNTL_WDTFEED_REG, (1 << RTC_CNTL_WDT_FEED_S));
    
    while (1);
}
