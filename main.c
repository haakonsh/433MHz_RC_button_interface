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

// // TODO: 1. Detect the preamble accurately, and signal the user. V
//
// // TODO: 2. Port the RTC + pin sampler to TIMER + pin sampler.   V
//
// // TODO: 3. Merge 1. and 2.                                      V
//
// // TODO: 4. Read DATA bits and turn on DK LED's.
//
// // TODO: 5. Celebrate.

const nrf_drv_timer_t TIMER1 = NRF_DRV_TIMER_INSTANCE(0);
const nrf_drv_timer_t TIMER2 = NRF_DRV_TIMER_INSTANCE(1);

nrf_drv_gpiote_pin_t input_pin =    INPUT_PIN;  // input pin

nrf_ppi_channel_t ppi_channel_1;

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
    err_code = nrf_drv_timer_init(&TIMER2, &timer_cfg, timer2_evt_handler);
    APP_ERROR_CHECK(err_code);

    // Set compare channel to trigger interrupts
    nrf_drv_timer_compare(&TIMER2, NRF_TIMER_CC_CHANNEL0, CLK/8, true);

    /*  ___
       |  |________| = 1480us (DATA_L = 0b1000). CLK/8 = 1480us/8 = 185us --> sample the midpoint of each 'bit'*/
    nrf_drv_timer_extended_compare(
        &TIMER1, NRF_TIMER_CC_CHANNEL1, CLK/8, NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK, true);
    //CLK/4 = 1480us/4 = 370us --> reset the timer after each 'bit'*/
    nrf_drv_timer_extended_compare(
        &TIMER2, NRF_TIMER_CC_CHANNEL1, CLK/4, NRF_TIMER_SHORT_COMPARE1_CLEAR_MASK, true);

    //enable timer instances
    nrf_drv_timer_enable(&TIMER1);
    nrf_drv_timer_enable(&TIMER2);

}

/* handler used to detect the preamble, it will sample the 'input_pin' and increment the variable
'value' if the pin state is high. After 20 samples if none of the samples were high, a preamble is detected,
and a GPIOTE event is enabled. The GPIOTE will start the TIMER2 when the input_pin changes state from low to high.

_____          count = 20       _____
    |____________________|_____|                             */
void timer1_evt_handler(nrf_timer_event_t event_type, void* p_context)
{
    static uint8_t count, value;

    if(nrf_gpio_pin_read(INPUT_PIN)){value++;}

    count++;

    if(count == 20){      //TODO tweak value
        count = 0;
        if(!value){
            nrf_drv_gpiote_in_event_enable(input_pin, false);
            nrf_gpio_pin_toggle(LED1);
            nrf_drv_timer_pause(&TIMER1);
        }
        value = 0;
    }

}
/* handler used to sample the DATA bits */
void timer2_evt_handler(nrf_timer_event_t evt_type, void* p_context)
{
    // The GPIOTE event is only used to start the bit sampling sequence that follows the preamble
    nrf_drv_gpiote_in_event_disable(input_pin);

    uint32_t data = 0;
    rx_to_buffer(evt_type, &buffer_4);
    if(message_received){
        bit_decode(&buffer_4, &buffer);
        data = buffer & DATA_msk; // only interrested in the DATA bits
        switch(data)
        {//TODO If you're not using our DK, set breakpoints in the CASEs for debugging.
            case 1:
                //nrf_gpio_pin_clear(LED1);
                nrf_gpio_pin_toggle(LED1);
                break;
            case 2:
                nrf_gpio_pin_clear(LED2);
                //nrf_gpio_pin_toggle(LED1);
                break;
            case 3:
                nrf_gpio_pin_clear(LED3);
                //nrf_gpio_pin_toggle(LED1);
                break;
            case 4:
                nrf_gpio_pin_clear(LED4);
                //nrf_gpio_pin_toggle(LED1);
                break;
            default:
                nrf_gpio_pin_toggle(LED1);
                nrf_gpio_pin_toggle(LED2);
                nrf_gpio_pin_toggle(LED3);
                nrf_gpio_pin_toggle(LED4);
                break;
        }

        nrf_drv_timer_resume(&TIMER1);
        //nrf_drv_gpiote_in_event_enable(input_pin, false);
        message_received = false;

        nrf_gpio_pin_set(LED1);
        nrf_gpio_pin_set(LED2);
        nrf_gpio_pin_set(LED3);
        nrf_gpio_pin_set(LED4);
    }
}

void gpiote_init(void)
{
    uint32_t err_code;

    err_code = nrf_drv_gpiote_init();
    APP_ERROR_CHECK(err_code);

    nrf_drv_gpiote_in_config_t input_pin_cfg = GPIOTE_CONFIG_IN_SENSE_LOTOHI(true);
    input_pin_cfg.pull = NRF_GPIO_PIN_PULLDOWN; // TODO Optional, tweak pullup/down or no pull...

    err_code = nrf_drv_gpiote_in_init(input_pin, &input_pin_cfg, NULL);
    APP_ERROR_CHECK(err_code);
}

// the PPI is used to connect the pin state change(GPIOTE_IN_EVENT) on the input_pin to the timer (NRF_TIMER_TASK_START)
void ppi_init(void)
{
    ret_code_t err_code;
    uint32_t timer_task_addr;
    uint32_t gpiote_evt_addr;

    err_code = nrf_drv_ppi_init();
    APP_ERROR_CHECK(err_code);

    err_code = nrf_drv_ppi_channel_alloc(&ppi_channel_1);
    APP_ERROR_CHECK(err_code);

    timer_task_addr = nrf_drv_timer_task_address_get(&TIMER2, NRF_TIMER_TASK_START);
    gpiote_evt_addr = nrf_drv_gpiote_in_event_addr_get(input_pin);

    // starts the timer when input_pin goes hi.
    err_code = nrf_drv_ppi_channel_assign(ppi_channel_1, gpiote_evt_addr, timer_task_addr);
    APP_ERROR_CHECK(err_code);

    err_code = nrf_drv_ppi_channel_enable(ppi_channel_1);
    APP_ERROR_CHECK(err_code);

}

// for developement purposes with the nRF52DK
void leds_init(void)
{
    nrf_gpio_cfg_output(LED1);
    nrf_gpio_cfg_output(LED1);
    nrf_gpio_cfg_output(LED1);
    nrf_gpio_cfg_output(LED1);

    nrf_gpio_pin_set(LED1);
    nrf_gpio_pin_set(LED2);
    nrf_gpio_pin_set(LED3);
    nrf_gpio_pin_set(LED4);
}

/**
 * @brief Function for main application entry.
 */
int main(void)
{
    nrf_gpio_cfg_input(INPUT_PIN, NRF_GPIO_PIN_PULLUP);
    leds_init();

    timers_init();
    gpiote_init();
    ppi_init();

    while (1)
    {
        __WFE();    //TODO replace with the soft_device sleep function.
        __SEV();
        __WFE();

    }
}

/** @} */
