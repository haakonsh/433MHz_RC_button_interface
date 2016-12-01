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

// // TODO: 1. Detect the preamble accurately, and signal the user.
//
// // TODO: 2. Port the RTC + pin sampler to TIMER + pin sampler.
//
// // TODO: 3. Merge 1. and 2.
//
// // TODO: 4. Read DATA bits and turn on DK LED's.
//
// // TODO: 5. Celebrate.

/**
 * @brief Function for main application entry.
 */
int main(void)
{
//    uint32_t err_code = NRF_SUCCESS;

    nrf_gpio_cfg_input(INPUT_PIN, NRF_GPIO_PIN_PULLDOWN);
    nrf_gpio_cfg_output(OUTPUT_PIN);

    timer_init();

    while (1)
    {
        //__WFE();    //replace with the soft_device sleep function.
        //__SEV();
        //__WFE();
    }
}

/** @} */
