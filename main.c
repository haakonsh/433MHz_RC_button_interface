/* Copyright (c) 2014 Nordic Semiconductor. All Rights Reserved.
 *
 * The information contained herein is property of Nordic Semiconductor ASA.
 * Terms and conditions of usage are described in detail in NORDIC
 * SEMICONDUCTOR STANDARD SOFTWARE LICENSE AGREEMENT.
 *
 * Licensees are granted free, non-transferable use of the information. NO
 * WARRANTY of ANY KIND is provided. This heading must NOT be removed from
 * the file.
 *
 */

/** @file
 * @defgroup nrf_dev_timer_example_main main.c
 * @{
 * @ingroup nrf_dev_timer_example
 * @brief Timer Example Application main file.
 *
 * This file contains the source code for a sample application using Timer0.
 *
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nrf.h"
#include "nrf_gpiote.h"
#include "app_error.h"
#include "nrf_ppi.h"
#include "nrf_drv_ppi.h"
#include "nrf_drv_gpiote.h"
#include "433_MHz_RC_button_interface.h"
#include "nrf_drv_timer.h"
#include "main.h"

// // TODO: 1. Detect the preamble accurately, and signal the user.
//
// // TODO: 2. Port the RTC + pin sampler to TIMER + pin sampler.
//
// // TODO: 3. Merge 1. and 2.
//
// // TODO: 4. Read DATA bits and turn on DK LED's.
//
// // TODO: 5. Celebrate.

const nrf_drv_timer_t TIMER1 = NRF_DRV_TIMER_INSTANCE(0);

/** @brief Function initialization and configuration of RTC driver instance.
 */
void timers_init(void)
{
    uint32_t err_code;

    nrf_drv_timer_config_t timer_cfg = {
        .frequency          = NRF_TIMER_FREQ_1MHz,
        .mode               = (nrf_timer_mode_t)TIMER_DEFAULT_CONFIG_MODE,
        .bit_width          = NRF_TIMER_BIT_WIDTH_16,
        .interrupt_priority = TIMER_DEFAULT_CONFIG_IRQ_PRIORITY,
        .p_context          = NULL
    };

    //Initialize TIMER instances
    err_code = nrf_drv_timer_init(&TIMER1, &timer_cfg, timer1_evt_handler);
    APP_ERROR_CHECK(err_code);

    // Set compare channel to trigger interrupts
    nrf_drv_timer_compare(&TIMER1, NRF_TIMER_CC_CHANNEL0, CLK/8, true);

    nrf_drv_timer_extended_compare(
        &TIMER1, NRF_TIMER_CC_CHANNEL1, (CLK/8) * 2, NRF_TIMER_SHORT_COMPARE1_CLEAR_MASK, true);

    //enable timer instance
    nrf_drv_timer_enable(&TIMER1);

}

void timer1_evt_handler(nrf_timer_event_t event_type, void* p_context)
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


/**
 * @brief Function for main application entry.
 */
int main(void)
{
//    uint32_t err_code = NRF_SUCCESS;

    nrf_gpio_cfg_input(INPUT_PIN, NRF_GPIO_PIN_PULLDOWN);
    nrf_gpio_cfg_output(OUTPUT_PIN);

    timers_init();

    while (1)
    {
        //__WFE();    //replace with the soft_device sleep function.
        //__SEV();
        //__WFE();
    }
}

/** @} */
