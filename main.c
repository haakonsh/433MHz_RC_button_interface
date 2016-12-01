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

nrf_drv_gpiote_pin_t input_pin =    INPUT_PIN;  // input pin

nrf_ppi_channel_t ppi_channel_1, ppi_channel_2;

void gpiote_init(void)
{
    err_code = nrf_drv_gpiote_init();
    APP_ERROR_CHECK(err_code);

    nrf_drv_gpiote_in_config_t input_pin_cfg = GPIOTE_CONFIG_IN_SENSE_LOTOHI(true);
    input_pin_cfg.pull = NRF_GPIO_PIN_PULLDOWN;

    err_code = nrf_drv_gpiote_in_init(input_pin, &input_pin_cfg, NULL);
    APP_ERROR_CHECK(err_code);
}

/** @brief Function initialization and configuration of timer driver instance.
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

void ppi_init(void)
{
    ret_code_t err_code;
    uint32_t timer_task_addr, timer_evt_addr;
    uint32_t gpiote_evt_addr;

    err_code = nrf_drv_ppi_init();
    APP_ERROR_CHECK(err_code);

    err_code = nrf_drv_ppi_channel_alloc(&ppi_channel_1);
    APP_ERROR_CHECK(err_code);

    err_code = nrf_drv_ppi_channel_alloc(&ppi_channel_2);
    APP_ERROR_CHECK(err_code);

    timer_task_addr = nrf_drv_timer_task_address_get(&TIMER, NRF_TIMER_TASK_START);
    gpiote_evt_addr = nrf_drv_gpiote_in_event_addr_get(input_pin);

    // starts the timer when input_pin goes hi.
    err_code = nrf_drv_ppi_channel_assign(ppi_channel_1, gpiote_evt_addr, timer_task_addr);
    APP_ERROR_CHECK(err_code);

    timer_task_addr = nrf_drv_timer_task_address_get(&TIMER, NRF_TIMER_TASK_CLEAR);
    timer_evt_addr = nrf_drv_timer_event_address_get(&TIMER, NRF_TIMER_EVENT_COMPARE_1);

    // clears the timer after a data clock cycle
    err_code = nrf_drv_ppi_channel_assign(ppi_channel_2, timer_evt_addr, timer_task_addr);
    APP_ERROR_CHECK(err_code);

    err_code = nrf_drv_ppi_channel_enable(ppi_channel_1);
    APP_ERROR_CHECK(err_code);

    err_code = nrf_drv_ppi_channel_enable(ppi_channel_2);
    APP_ERROR_CHECK(err_code);

    nrf_drv_gpiote_in_event_enable(input_pin, false);

}

/**
 * @brief Function for main application entry.
 */
int main(void)
{
    gpiote_init();
    ppi_init();
    timer_init();

    while (1)
    {
        //__WFE();    //replace with the soft_device sleep function.
        //__SEV();
        //__WFE();

    }
}

/** @} */
