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
#include <Weave/DeviceLayer/ThreadStackManager.h>
#include <Weave/DeviceLayer/nRF5/GroupKeyStoreImpl.h>
#include <Weave/DeviceLayer/internal/testing/ConfigUnitTest.h>
#include <Weave/DeviceLayer/internal/testing/GroupKeyStoreUnitTest.h>
#include <Weave/DeviceLayer/internal/testing/SystemClockUnitTest.h>

using namespace ::nl;
using namespace ::nl::Inet;
using namespace ::nl::Weave;
using namespace ::nl::Weave::DeviceLayer;


// ================================================================================
// Test App Feature Config
// ================================================================================

#define TEST_TASK_ENABLED 1
#define RUN_UNIT_TESTS 0
#define WOBLE_ENABLED 1
#define OPENTHREAD_ENABLED 1
#define OPENTHREAD_TEST_NETWORK_ENABLED 1


// ================================================================================
// OpenThread Test Network Information
// ================================================================================

#if OPENTHREAD_TEST_NETWORK_ENABLED

#define OPENTHREAD_TEST_NETWORK_NAME "WARP"
#define OPENTHREAD_TEST_NETWORK_PANID 0x7777
#define OPENTHREAD_TEST_NETWORK_EXTENDED_PANID { 0x88, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11 }
#define OPENTHREAD_TEST_NETWORK_CHANNEL 26
#define OPENTHREAD_TEST_NETWORK_MASTER_KEY { 0xB8, 0x98, 0x3A, 0xED, 0x95, 0x40, 0x64, 0xCC, 0x4B, 0xAC, 0xB3, 0xF6, 0xF1, 0x45, 0xD1, 0x98  }

#endif // OPENTHREAD_TEST_NETWORK_ENABLED



// ================================================================================
// Logging Support
// ================================================================================

#if NRF_LOG_ENABLED

#define LOGGER_STACK_SIZE (800)
#define LOGGER_PRIORITY 3

static TaskHandle_t sLoggerTaskHandle;

static void LoggerTaskMain(void * arg)
{
    UNUSED_PARAMETER(arg);

    NRF_LOG_INFO("Logging task running");

    while (1)
    {
        NRF_LOG_FLUSH();

        // Wait for a signal that more logging output might be pending.
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    }
}

extern "C" void vApplicationIdleHook( void )
{
    xTaskNotifyGive(sLoggerTaskHandle);
}

namespace nl {
namespace Weave {
namespace DeviceLayer {

/**
 * Called whenever a Weave or LwIP log message is emitted.
 */
void OnLogOutput(void)
{
    xTaskNotifyGive(sLoggerTaskHandle);
}

} // namespace DeviceLayer
} // namespace Weave
} // namespace nl

#endif //NRF_LOG_ENABLED


// ================================================================================
// Test Task
// ================================================================================

#if TEST_TASK_ENABLED

#define TEST_TASK_STACK_SIZE (800)
#define TEST_TASK_PRIORITY 1

static TaskHandle_t sTestTaskHandle;

static void TestTaskAlive()
{
    bsp_board_led_invert(BSP_BOARD_LED_2);
}

static void TestTaskMain(void * pvParameter)
{
    NRF_LOG_INFO("Test task running");
    bsp_board_led_invert(BSP_BOARD_LED_1);

#if RUN_UNIT_TESTS
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

    NRF_LOG_INFO("Unit tests done");
#endif


    while (true)
    {
        const TickType_t delay = pdMS_TO_TICKS(1000);
        vTaskDelay(delay);
        TestTaskAlive();
    }
}

#endif // TEST_TASK_ENABLED


// ================================================================================
// SoftDevice Support
// ================================================================================

#if defined(SOFTDEVICE_PRESENT) && SOFTDEVICE_PRESENT

static void OnSoCEvent(uint32_t sys_evt, void * p_context)
{
    UNUSED_PARAMETER(p_context);

#if OPENTHREAD_ENABLED

    otSysSoftdeviceSocEvtHandler(sys_evt);

#endif // OPENTHREAD_ENABLED
}

#endif // defined(SOFTDEVICE_PRESENT) && SOFTDEVICE_PRESENT


// ================================================================================
// J-Link Monitor Mode Debugging Support
// ================================================================================

#if JLINK_MMD

extern "C" void JLINK_MONITOR_OnExit(void)
{

}

extern "C" void JLINK_MONITOR_OnEnter(void)
{

}

extern "C" void JLINK_MONITOR_OnPoll(void)
{

}

#endif // JLINK_MMD



// ================================================================================
// Main Code
// ================================================================================

int main(void)
{
    ret_code_t ret;

#if JLINK_MMD
    NVIC_SetPriority(DebugMonitor_IRQn, _PRIO_SD_LOWEST);
#endif

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
    NRF_LOG_INFO("openweave-nrf52840-bringup starting");
    NRF_LOG_INFO("==================================================");

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

    NRF_LOG_INFO("Initializing Weave stack");

    ret = PlatformMgr().InitWeaveStack();
    if (ret != WEAVE_NO_ERROR)
    {
        NRF_LOG_INFO("PlatformMgr().InitWeaveStack() failed");
        APP_ERROR_HANDLER(ret);
    }

#if !WOBLE_ENABLED

    ret = ConnectivityMgr().SetWoBLEServiceMode(ConnectivityManager::kWoBLEServiceMode_Disabled);
    if (ret != WEAVE_NO_ERROR)
    {
        NRF_LOG_INFO("ConnectivityMgr().SetWoBLEServiceMode() failed");
        APP_ERROR_HANDLER(ret);
    }

#endif // WOBLE_ENABLED

#if OPENTHREAD_ENABLED

    NRF_LOG_INFO("Initializing OpenThread stack");

    otSysInit(0, NULL);

    ret = ThreadStackMgr().InitThreadStack();
    if (ret != WEAVE_NO_ERROR)
    {
        NRF_LOG_INFO("ThreadStackMgr().InitThreadStack() failed");
        APP_ERROR_HANDLER(ret);
    }

#endif // OPENTHREAD_ENABLED

    NRF_LOG_INFO("Starting Weave task");

    ret = PlatformMgr().StartEventLoopTask();
    if (ret != WEAVE_NO_ERROR)
    {
        NRF_LOG_INFO("PlatformMgr().StartEventLoopTask() failed");
        APP_ERROR_HANDLER(ret);
    }

#if OPENTHREAD_ENABLED

    NRF_LOG_INFO("Starting OpenThread task");

    // Start OpenThread task
    ret = ThreadStackMgrImpl().StartThreadTask();
    if (ret != WEAVE_NO_ERROR)
    {
        NRF_LOG_INFO("ThreadStackMgr().StartThreadTask() failed");
        APP_ERROR_HANDLER(ret);
    }

#endif // OPENTHREAD_ENABLED

#if OPENTHREAD_TEST_NETWORK_ENABLED

    {
        otError otRet;
        otInstance * otInst = ThreadStackMgrImpl().OTInstance();

        if (!otDatasetIsCommissioned(otInst))
        {
            NRF_LOG_INFO("Commissioning test Thread network");

            otRet = otThreadSetNetworkName(otInst, OPENTHREAD_TEST_NETWORK_NAME);
            if (otRet != OT_ERROR_NONE)
            {
                NRF_LOG_INFO("otThreadSetNetworkName() failed");
                APP_ERROR_HANDLER(otRet);
            }

            otRet = otLinkSetPanId(otInst, OPENTHREAD_TEST_NETWORK_PANID);
            if (otRet != OT_ERROR_NONE)
            {
                NRF_LOG_INFO("otLinkSetPanId() failed");
                APP_ERROR_HANDLER(otRet);
            }

            {
                struct otExtendedPanId exPanId = { OPENTHREAD_TEST_NETWORK_EXTENDED_PANID };
                otRet = otThreadSetExtendedPanId(otInst, &exPanId);
                if (otRet != OT_ERROR_NONE)
                {
                    NRF_LOG_INFO("otThreadSetExtendedPanId() failed");
                    APP_ERROR_HANDLER(otRet);
                }
            }

            otRet = otLinkSetChannel(otInst, OPENTHREAD_TEST_NETWORK_CHANNEL);
            if (otRet != OT_ERROR_NONE)
            {
                NRF_LOG_INFO("otLinkSetChannel() failed");
                APP_ERROR_HANDLER(otRet);
            }

            {
                struct otMasterKey masterKey = { OPENTHREAD_TEST_NETWORK_MASTER_KEY };
                otRet = otThreadSetMasterKey(otInst, &masterKey);
                if (otRet != OT_ERROR_NONE)
                {
                    NRF_LOG_INFO("otThreadSetMasterKey() failed");
                    APP_ERROR_HANDLER(otRet);
                }
            }

            otRet = otThreadSetEnabled(otInst, true);
            if (otRet != OT_ERROR_NONE)
            {
                NRF_LOG_INFO("otThreadSetEnabled() failed");
                APP_ERROR_HANDLER(otRet);
            }
        }
        else
        {
            NRF_LOG_INFO("Thread network already commissioned");
        }

        NRF_LOG_INFO("OpenThread initialization complete");
    }

#endif // OPENTHREAD_TEST_NETWORK_ENABLED

#if TEST_TASK_ENABLED

    NRF_LOG_INFO("Starting test task");

    // Start Test task
    if (xTaskCreate(TestTaskMain, "TEST", TEST_TASK_STACK_SIZE / sizeof(StackType_t), NULL, TEST_TASK_PRIORITY, &sTestTaskHandle) != pdPASS)
    {
        NRF_LOG_INFO("Failed to create TEST task");
    }

#endif // TEST_TASK_ENABLED

    // Activate deep sleep mode
    SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;

    NRF_LOG_INFO("Starting FreeRTOS scheduler");

    /* Start FreeRTOS scheduler. */
    vTaskStartScheduler();

    // Should never get here
    NRF_LOG_INFO("vTaskStartScheduler() failed");
    APP_ERROR_HANDLER(0);
}
