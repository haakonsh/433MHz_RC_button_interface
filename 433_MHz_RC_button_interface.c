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
#include "nrf_drv_timer.h"
#include "nrf_gpio.h"

const nrf_drv_timer_t TIMER = NRF_DRV_TIMER_INSTANCE(0);

void timer_rc_button_evt_handler(nrf_timer_event_t event_type, void* p_context)
{
    static uint8_t count, value;

    if(nrf_gpio_pin_read(INPUT_PIN)){value++;}

    count++;

    if(count == 20){
        count = 0;
        if(!value){
            //YEAH! We dids it, whoop!
            nrf_gpio_pin_toggle(OUTPUT_PIN);
            // enable GPIOTE in evt.
        }
        value = 0;
    }

}

/** @brief Function initialization and configuration of RTC driver instance.
 */
void timer_init(void)
{
    uint32_t err_code;

    nrf_drv_timer_config_t timer_cfg = {
        .frequency          = NRF_TIMER_FREQ_1MHz,
        .mode               = (nrf_timer_mode_t)TIMER_DEFAULT_CONFIG_MODE,
        .bit_width          = NRF_TIMER_BIT_WIDTH_16,
        .interrupt_priority = TIMER_DEFAULT_CONFIG_IRQ_PRIORITY,
        .p_context          = NULL
    };

    //Initialize TIMER instance
    err_code = nrf_drv_timer_init(&TIMER, &timer_cfg, timer_rc_button_evt_handler);
    APP_ERROR_CHECK(err_code);

    // Set compare channel to trigger interrupts
    nrf_drv_timer_compare(&TIMER, NRF_TIMER_CC_CHANNEL0, TIMER_TICKS, true);

    nrf_drv_timer_extended_compare(
        &TIMER, NRF_TIMER_CC_CHANNEL1, (TIMER_TICKS * 2), NRF_TIMER_SHORT_COMPARE1_CLEAR_MASK, true);

    //enable timer instance
    nrf_drv_timer_enable(&TIMER);

}
