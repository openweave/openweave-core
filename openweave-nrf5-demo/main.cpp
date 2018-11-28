#include <stdbool.h>
#include <stdint.h>
#include "boards.h"
#include "nrf_log.h"
#include "nrf_delay.h"
#include "nrf_crypto.h"

#include <Weave/DeviceLayer/WeaveDeviceLayer.h>

using namespace ::nl;
using namespace ::nl::Inet;
using namespace ::nl::Weave;
using namespace ::nl::Weave::DeviceLayer;

/**
 * @brief Function for application main entry.
 */
int main(void)
{
    WEAVE_ERROR err;

    /* Configure board. */
    bsp_board_init(BSP_INIT_LEDS);

    err = nrf_crypto_init();
    if (err != WEAVE_NO_ERROR)
    {
        return 0;
    }

    // Initialize the Weave stack.
    err = PlatformMgr().InitWeaveStack();
    if (err != WEAVE_NO_ERROR)
    {
        return 0;
    }

    /* Toggle LEDs. */
    while (true)
    {
        for (int i = 0; i < LEDS_NUMBER; i++)
        {
            bsp_board_led_invert(i);
            nrf_delay_ms(250);
        }
    }

    return 0;
}

/**
 *@}
 **/
