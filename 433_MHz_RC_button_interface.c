#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nrf.h"
#include "nrf_ppi.h"
#include "nrf_drv_ppi.h"
#include "nrf_drv_gpiote.h"
#include "nrf_drv_rtc.h"
#include "433_MHz_RC_button_interface.h"
#include "nrf_gpiote.h"
#include "app_error.h"
#include "nrf_drv_clock.h"

const nrf_drv_timer_t TIMER = NRF_DRV_TIMER_INSTANCE(0);


nrf_drv_gpiote_pin_t rc_button_pin =    INPUT_PIN;  // input pin

void timer_rc_button_evt_handler(nrf_drv_rtc_int_type_t int_type)
{

}

/** @brief Function starting the internal LFCLK XTAL oscillator.
 */
void lfclk_config(void)
{
    ret_code_t err_code = nrf_drv_clock_init();
    APP_ERROR_CHECK(err_code);

    nrf_drv_clock_lfclk_request(NULL);
}

/** @brief Function initialization and configuration of RTC driver instance.
 */
void timer_init(void)
{
    uint32_t err_code;


    nrf_drv_timer_config_t timer_cfg = {
        .frequency          = 4,        // 1MHz
        .mode               = (nrf_timer_mode_t)TIMER_DEFAULT_CONFIG_MODE,          \
        .bit_width          = 0,\       // 16 bit
        .interrupt_priority = TIMER_DEFAULT_CONFIG_IRQ_PRIORITY,                    \
        .p_context          = NULL
    }

    //Initialize TIMER instance
    err_code = nrf_drv_timer_init(&TIMER, &timer_cfg, timer_rc_button_evt_handler);
    APP_ERROR_CHECK(err_code);

    // Set compare channel to trigger interrupts
    nrf_drv_timer_extended_compare(
         &TIMER, NRF_TIMER_CC_CHANNEL0, time_ticks, NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK, true);

    //Power on RTC instance

}
