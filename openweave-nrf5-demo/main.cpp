#include <stdbool.h>
#include <stdint.h>

#include "boards.h"
#include "nrf_log.h"
#include "nrf_delay.h"
#ifdef SOFTDEVICE_PRESENT
#include "nrf_sdh.h"
#include "nrf_sdh_ble.h"
#include "nrf_sdh_soc.h"
#endif
#include "nrf_drv_clock.h"
#include "nrf_crypto.h"
#include "mem_manager.h"

#if NRF_LOG_ENABLED
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "nrf_log_backend_uart.h"
#endif // NRF_LOG_ENABLED

#include <openthread/instance.h>
#include <openthread/thread.h>
#include <openthread/tasklet.h>
#include <openthread/link.h>
#include <openthread/dataset.h>
#include <openthread/error.h>
#include <openthread/icmp6.h>
///#include <openthread/logging.h>
#include <openthread/platform/openthread-system.h>
extern "C" {
#include <openthread/platform/platform-softdevice.h>
}

#include <Weave/DeviceLayer/WeaveDeviceLayer.h>
#include <Weave/DeviceLayer/nRF5/GroupKeyStoreImpl.h>
#include <Weave/DeviceLayer/internal/testing/ConfigUnitTest.h>
#include <Weave/DeviceLayer/internal/testing/GroupKeyStoreUnitTest.h>
#include <Weave/DeviceLayer/internal/testing/SystemClockUnitTest.h>

using namespace ::nl;
using namespace ::nl::Inet;
using namespace ::nl::Weave;
using namespace ::nl::Weave::DeviceLayer;

#if NRF_LOG_ENABLED

#define LOGGER_STACK_SIZE (800)
#define LOGGER_PRIORITY 1

static TaskHandle_t sLoggerTaskHandle;

static void LoggerTaskMain(void * arg)
{
    UNUSED_PARAMETER(arg);

    while (1)
    {
        NRF_LOG_FLUSH();
        vTaskSuspend(NULL); // Suspend myself
    }
}

extern "C" void vApplicationIdleHook( void )
{
     vTaskResume(sLoggerTaskHandle);
}

#endif //NRF_LOG_ENABLED

#define TEST_TASK_ENABLED 0

#if TEST_TASK_ENABLED

#define TEST_TASK_STACK_SIZE (400)
#define TEST_TASK_PRIORITY 2

static TaskHandle_t sTestTaskHandle;

static void TestTaskMain(void * pvParameter)
{
    WEAVE_ERROR err;

    NRF_LOG_INFO("TEST task started");
    bsp_board_led_invert(BSP_BOARD_LED_1);

#if 0
    Internal::RunSystemClockUnitTest();

    NRF_LOG_INFO("System clock test complete");

    // Test the core configuration interface
    Internal::NRF5Config::RunConfigUnitTest();

    NRF_LOG_INFO("NRF5Config test complete");

    // Test the group key store
    {
        Internal::GroupKeyStoreImpl groupKeyStore;
        err = groupKeyStore.Init();
        APP_ERROR_CHECK(err);
        Internal::RunGroupKeyStoreUnitTest<Internal::GroupKeyStoreImpl>(&groupKeyStore);
    }

    NRF_LOG_INFO("GroupKeyStore test complete");
#endif

    NRF_LOG_INFO("TEST task done");
    bsp_board_led_invert(BSP_BOARD_LED_2);

    while (true)
    {
        vTaskSuspend(NULL); // Suspend myself
    }
}

#endif // TEST_TASK_ENABLED

#define OPENTHREAD_TEST_ENABLED 1

#if OPENTHREAD_TEST_ENABLED

#define OPENTHREAD_TASK_STACK_SIZE (8192)
#define OPENTHREAD_TASK_PRIORITY 1

#define TEST_THREAD_NETWORK_NAME "WARP"
#define TEST_THREAD_NETWORK_PANID 0x7777
#define TEST_THREAD_NETWORK_EXTENDED_PANID { 0x88, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11 }
#define TEST_THREAD_NETWORK_CHANNEL 26
#define TEST_THREAD_NETWORK_MASTER_KEY { 0xB8, 0x98, 0x3A, 0xED, 0x95, 0x40, 0x64, 0xCC, 0x4B, 0xAC, 0xB3, 0xF6, 0xF1, 0x45, 0xD1, 0x98  }

static otInstance * sOpenThreadInstance;
static TaskHandle_t sOpenThreadTaskHandle;

void otTaskletsSignalPending(otInstance * p_instance)
{
    if (sOpenThreadTaskHandle)
    {
        xTaskNotifyGive(sOpenThreadTaskHandle);
    }
}

void otSysEventSignalPending(void)
{
    if (sOpenThreadTaskHandle)
    {
        BaseType_t yieldRequired;

        vTaskNotifyGiveFromISR(sOpenThreadTaskHandle, &yieldRequired);
        if (yieldRequired == pdTRUE)
        {
            portYIELD_FROM_ISR(yieldRequired);
        }
    }
}

static void OnOpenThreadStateChange(uint32_t flags, void * p_context)
{
    const char * netName;
    const otNetifAddress * addr;

    netName = otThreadGetNetworkName(sOpenThreadInstance);

    NRF_LOG_INFO("OpenThread State Changed (Flags: 0x%08x)", flags);


    NRF_LOG_INFO("   Device Role: %d", otThreadGetDeviceRole(sOpenThreadInstance));
    NRF_LOG_INFO("   Network Name: %s", otThreadGetNetworkName(sOpenThreadInstance));
    NRF_LOG_INFO("   PAN Id: 0x%04X", otLinkGetPanId(sOpenThreadInstance));
    {
        const otExtendedPanId * exPanId = otThreadGetExtendedPanId(sOpenThreadInstance);
        char exPanIdStr[32];
        snprintf(exPanIdStr, sizeof(exPanIdStr), "0x%02X%02X%02X%02X%02X%02X%02X%02X",
                 exPanId->m8[0], exPanId->m8[1], exPanId->m8[2], exPanId->m8[3],
                 exPanId->m8[4], exPanId->m8[5], exPanId->m8[6], exPanId->m8[7]);
        NRF_LOG_INFO("   Extended PAN Id: %s", exPanIdStr);
    }
    NRF_LOG_INFO("   Channel: %d", otLinkGetChannel(sOpenThreadInstance));

    NRF_LOG_INFO("   Interface Addresses:");

    for (const otNetifAddress * addr = otIp6GetUnicastAddresses(sOpenThreadInstance); addr != NULL; addr = addr->mNext)
    {
        char ipAddrStr[64];
        nl::Inet::IPAddress ipAddr;

        memcpy(ipAddr.Addr, addr->mAddress.mFields.m32, sizeof(ipAddr.Addr));

        ipAddr.ToString(ipAddrStr, sizeof(ipAddrStr));

        NRF_LOG_INFO("        %s/%d%s%s", ipAddrStr,
                     addr->mPrefixLength,
                     addr->mValid ? " valid" : "",
                     addr->mPreferred ? " preferred" : "");
    }
}

static void OnOpenThreadReceive(otMessage *aMessage, void *aContext)
{
    NRF_LOG_INFO("OnOpenThreadReceive()");
    otMessageFree(aMessage);
}

static void OpenThreadTaskMain(void * arg)
{
    while (1)
    {
//        NRF_LOG_INFO("OpenThreadTaskMain awake");
        otTaskletsProcess(sOpenThreadInstance);
        otSysProcessDrivers(sOpenThreadInstance);

        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    }
}

#endif // OPENTHREAD_TEST_ENABLED


#if defined(SOFTDEVICE_PRESENT) && SOFTDEVICE_PRESENT

static void OnSoCEvent(uint32_t sys_evt, void * p_context)
{
    UNUSED_PARAMETER(p_context);

#if OPENTHREAD_TEST_ENABLED

    otSysSoftdeviceSocEvtHandler(sys_evt);

#endif // OPENTHREAD_TEST_ENABLED
}

#endif // defined(SOFTDEVICE_PRESENT) && SOFTDEVICE_PRESENT

int main(void)
{
    ret_code_t ret;

    // Initialize clock driver.
    ret = nrf_drv_clock_init();
    APP_ERROR_CHECK(ret);

    nrf_drv_clock_lfclk_request(NULL);

    // Wait for the clock to be ready.
    while (!nrf_clock_lf_is_running()) { }

#if NRF_LOG_ENABLED

    // Initialize logging component
    ret = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(ret);

    // Initialize logging backends
    NRF_LOG_DEFAULT_BACKENDS_INIT();

    // Start LOGGER task.
    if (xTaskCreate(LoggerTaskMain, "LOGGER", LOGGER_STACK_SIZE / sizeof(StackType_t), NULL, LOGGER_PRIORITY, &sLoggerTaskHandle) != pdPASS)
    {
        APP_ERROR_HANDLER(0);
    }
#endif

    NRF_LOG_INFO("==================================================");
    NRF_LOG_INFO("test-app starting");

    // Configure LED-pins as outputs
    bsp_board_init(BSP_INIT_LEDS);

    bsp_board_led_invert(BSP_BOARD_LED_0);

#if defined(SOFTDEVICE_PRESENT) && SOFTDEVICE_PRESENT

    NRF_LOG_INFO("Enabling SoftDevice");

    ret = nrf_sdh_enable_request();
    if (ret != NRF_SUCCESS)
    {
        NRF_LOG_INFO("nrf_sdh_enable_request() failed");
        APP_ERROR_HANDLER(ret);
    }

    NRF_LOG_INFO("Waiting for SoftDevice to be enabled");

    while (!nrf_sdh_is_enabled()) { }

    // Register a handler for SOC events.
    NRF_SDH_SOC_OBSERVER(m_soc_observer, NRF_SDH_SOC_STACK_OBSERVER_PRIO, OnSoCEvent, NULL);

    NRF_LOG_INFO("SoftDevice enable complete");

#endif // defined(SOFTDEVICE_PRESENT) && SOFTDEVICE_PRESENT

    ret = nrf_mem_init();
    if (ret != NRF_SUCCESS)
    {
        NRF_LOG_INFO("nrf_mem_init() failed");
        APP_ERROR_HANDLER(ret);
    }

    ret = nrf_crypto_init();
    if (ret != NRF_SUCCESS)
    {
        NRF_LOG_INFO("nrf_crypto_init() failed");
        APP_ERROR_HANDLER(ret);
    }

#if defined(SOFTDEVICE_PRESENT) && SOFTDEVICE_PRESENT

    {
        uint32_t appRAMStart = 0;

        // Configure the BLE stack using the default settings.
        // Fetch the start address of the application RAM.
        ret = nrf_sdh_ble_default_cfg_set(WEAVE_DEVICE_LAYER_BLE_CONN_CFG_TAG, &appRAMStart);
        APP_ERROR_CHECK(ret);

        // Enable BLE stack.
        ret = nrf_sdh_ble_enable(&appRAMStart);
        APP_ERROR_CHECK(ret);
    }

#endif // defined(SOFTDEVICE_PRESENT) && SOFTDEVICE_PRESENT

#if OPENTHREAD_TEST_ENABLED

    {
        otError otRet;

        otSysInit(0, NULL);

        sOpenThreadInstance = otInstanceInitSingle();
        if (sOpenThreadInstance == NULL)
        {
            NRF_LOG_INFO("otInstanceInitSingle() failed");
            APP_ERROR_HANDLER(0);
        }

        otSetDynamicLogLevel(sOpenThreadInstance, OT_LOG_LEVEL_DEBG);

        otRet = otSetStateChangedCallback(sOpenThreadInstance, OnOpenThreadStateChange, NULL);
        if (otRet != OT_ERROR_NONE)
        {
            NRF_LOG_INFO("otSetStateChangedCallback() failed");
            APP_ERROR_HANDLER(otRet);
        }

//        otInstanceErasePersistentInfo(sOpenThreadInstance);

        if (!otDatasetIsCommissioned(sOpenThreadInstance))
        {
            NRF_LOG_INFO("Commissioning test Thread network");

            otRet = otThreadSetNetworkName(sOpenThreadInstance, TEST_THREAD_NETWORK_NAME);
            if (otRet != OT_ERROR_NONE)
            {
                NRF_LOG_INFO("otThreadSetNetworkName() failed");
                APP_ERROR_HANDLER(otRet);
            }

            otRet = otLinkSetPanId(sOpenThreadInstance, TEST_THREAD_NETWORK_PANID);
            if (otRet != OT_ERROR_NONE)
            {
                NRF_LOG_INFO("otLinkSetPanId() failed");
                APP_ERROR_HANDLER(otRet);
            }

            {
                struct otExtendedPanId exPanId = { TEST_THREAD_NETWORK_EXTENDED_PANID };
                otRet = otThreadSetExtendedPanId(sOpenThreadInstance, &exPanId);
                if (otRet != OT_ERROR_NONE)
                {
                    NRF_LOG_INFO("otThreadSetExtendedPanId() failed");
                    APP_ERROR_HANDLER(otRet);
                }
            }

            otRet = otLinkSetChannel(sOpenThreadInstance, TEST_THREAD_NETWORK_CHANNEL);
            if (otRet != OT_ERROR_NONE)
            {
                NRF_LOG_INFO("otLinkSetChannel() failed");
                APP_ERROR_HANDLER(otRet);
            }

            {
                struct otMasterKey masterKey = { TEST_THREAD_NETWORK_MASTER_KEY };
                otRet = otThreadSetMasterKey(sOpenThreadInstance, &masterKey);
                if (otRet != OT_ERROR_NONE)
                {
                    NRF_LOG_INFO("otThreadSetMasterKey() failed");
                    APP_ERROR_HANDLER(otRet);
                }
            }
        }
        else
        {
            NRF_LOG_INFO("Thread network already commissioned");
        }

        {
            otLinkModeConfig linkMode;

            memset(&linkMode, 0, sizeof(linkMode));
            linkMode.mRxOnWhenIdle       = true;
            linkMode.mSecureDataRequests = true;
            linkMode.mDeviceType         = true;
            linkMode.mNetworkData        = true;

            otRet = otThreadSetLinkMode(sOpenThreadInstance, linkMode);
            if (otRet != OT_ERROR_NONE)
            {
                NRF_LOG_INFO("otThreadSetLinkMode() failed");
                APP_ERROR_HANDLER(otRet);
            }

            otRet = otIp6SetEnabled(sOpenThreadInstance, true);
            if (otRet != OT_ERROR_NONE)
            {
                NRF_LOG_INFO("otIp6SetEnabled() failed");
                APP_ERROR_HANDLER(otRet);
            }
        }

        otIp6SetReceiveCallback(sOpenThreadInstance, OnOpenThreadReceive, NULL);

        otRet = otThreadSetEnabled(sOpenThreadInstance, true);
        if (otRet != OT_ERROR_NONE)
        {
            NRF_LOG_INFO("otThreadSetEnabled() failed");
            APP_ERROR_HANDLER(otRet);
        }

        NRF_LOG_INFO("OpenThread initialization complete");
    }

#endif // OPENTHREAD_TEST_ENABLED

    ret = ::nl::Weave::DeviceLayer::PlatformMgr().InitWeaveStack();
    if (ret != WEAVE_NO_ERROR)
    {
        NRF_LOG_INFO("PlatformMgr().InitWeaveStack() failed");
        APP_ERROR_HANDLER(ret);
    }

    ret = ::nl::Weave::DeviceLayer::PlatformMgr().StartEventLoopTask();
    if (ret != WEAVE_NO_ERROR)
    {
        NRF_LOG_INFO("PlatformMgr().StartEventLoopTask() failed");
        APP_ERROR_HANDLER(ret);
    }

#if TEST_TASK_ENABLED

    // Start Test task
    if (xTaskCreate(TestTaskMain, "TST", TEST_TASK_STACK_SIZE / sizeof(StackType_t), NULL, TEST_TASK_PRIORITY, &sTestTaskHandle) != pdPASS)
    {
        NRF_LOG_INFO("Failed to create TEST task");
    }

#endif // TEST_TASK_ENABLED

#if OPENTHREAD_TEST_ENABLED

    // Start OpenThread task
    if (xTaskCreate(OpenThreadTaskMain, "OT", OPENTHREAD_TASK_STACK_SIZE / sizeof(StackType_t), NULL, OPENTHREAD_TASK_PRIORITY, &sOpenThreadTaskHandle) != pdPASS)
    {
        NRF_LOG_INFO("Failed to create OpenThread task");
    }

#endif // OPENTHREAD_TEST_ENABLED

    // Activate deep sleep mode
    SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;

    /* Start FreeRTOS scheduler. */
    vTaskStartScheduler();

    // Should never get here
    NRF_LOG_INFO("vTaskStartScheduler() failed");
    APP_ERROR_HANDLER(0);
}
