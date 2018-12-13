#include <stdbool.h>
#include <stdint.h>

#include "boards.h"
#include "nrf_log.h"
#include "nrf_delay.h"
#ifdef SOFTDEVICE_PRESENT
#include "nrf_sdh.h"
#include "nrf_sdh_ble.h"
#endif
#include "nrf_drv_clock.h"
#include "nrf_crypto.h"
#include "mem_manager.h"

#if NRF_LOG_ENABLED
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "nrf_log_backend_uart.h"
#endif // NRF_LOG_ENABLED

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

#define TEST_TASK_STACK_SIZE (400)
#define TEST_TASK_PRIORITY 2

static TaskHandle_t sTestTaskHandle;

static void TestTaskMain(void * pvParameter)
{
    WEAVE_ERROR err;

    NRF_LOG_INFO("TEST task started");
    bsp_board_led_invert(BSP_BOARD_LED_1);

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

    NRF_LOG_INFO("TEST task done");
    bsp_board_led_invert(BSP_BOARD_LED_2);

    while (true)
    {
        vTaskSuspend(NULL); // Suspend myself
    }
}


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

    ret = ::nl::Weave::DeviceLayer::PlatformMgr().InitWeaveStack();
    if (ret != WEAVE_NO_ERROR)
    {
        NRF_LOG_INFO("PlatformMgr().InitWeaveStack() failed");
        APP_ERROR_HANDLER(ret);
    }

    // Start Test task
    if (xTaskCreate(TestTaskMain, "TEST", TEST_TASK_STACK_SIZE / sizeof(StackType_t), NULL, TEST_TASK_PRIORITY, &sTestTaskHandle) != pdPASS)
    {
        NRF_LOG_INFO("Failed to create test task");
    }

    // Activate deep sleep mode
    SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;

    /* Start FreeRTOS scheduler. */
    vTaskStartScheduler();

    // Should never get here
    NRF_LOG_INFO("vTaskStartScheduler() failed");
    APP_ERROR_HANDLER(0);
}
