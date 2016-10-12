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
#include "nrf_drv_timer.h"
#include "nrf_gpiote.h"
#include "app_error.h"
#include "nrf_delay.h"
#include "nrf_ppi.h"
#include "nrf_drv_ppi.h"
#include "nrf_drv_gpiote.h"


#define DATA0_pin   11
#define DATA1_pin   12
#define Tpw         50  //us
#define Tpi         500 //us

const nrf_drv_timer_t TIMER_WIEGAND = NRF_DRV_TIMER_INSTANCE(0);

volatile uint8_t i,j = 0;
uint8_t read_buffer[5] = {0};
uint8_t * read_message[5] = {0};

nrf_drv_gpiote_pin_t read_pin = 13;

// nrf_drv_gpiote_evt_handler_t read_pin_evt_handler(read_pin, NRF_GPIOTE_POLARITY_HITOLO)
// {
//
// }

void wiegand_read_init(void)
{
    ret_code_t err_code;
    uint32_t timer_task_addr;
    uint32_t gpiote_evt_addr;
    nrf_ppi_channel_t ppi_channel;
    
    nrf_drv_gpiote_in_config_t read_pin_cfg = GPIOTE_CONFIG_IN_SENSE_HITOLO(true);
    //read_pin_cfg.pull = NRF_GPIO_PIN_PULLUP;

    err_code = nrf_drv_gpiote_in_init(read_pin, &read_pin_cfg, NULL);
    APP_ERROR_CHECK(err_code);

    err_code = nrf_drv_ppi_channel_alloc(&ppi_channel);
    APP_ERROR_CHECK(err_code);

    timer_task_addr = nrf_drv_timer_task_address_get(&TIMER_WIEGAND, NRF_TIMER_TASK_START);
    gpiote_evt_addr = nrf_drv_gpiote_in_event_addr_get(read_pin);

    err_code = nrf_drv_ppi_channel_assign(ppi_channel, gpiote_evt_addr, timer_task_addr);
    APP_ERROR_CHECK(err_code);

    err_code = nrf_drv_ppi_channel_enable(ppi_channel);
    APP_ERROR_CHECK(err_code);

    nrf_drv_gpiote_in_event_enable(read_pin, false);
}


void timer_wiegand_event_handler(nrf_timer_event_t event_type, void* p_context)
{
    bool pin = nrf_gpio_pin_read(read_pin);
    switch (event_type)
    {
        case NRF_TIMER_EVENT_COMPARE0:
            // if 1, write 1 into buffer
            if(pin)
            {
               read_buffer[i] = (read_buffer[i] & (~(1 << j))) & (1 << j);
            }
            if((j == 7) && (i == 5))
            {
                i = 0;
                j = 0;
                break;
            }
            else if(j == 7)
            {
                j = 0; 
                i++;
                break;
            }
            j++;   
            break;
            
        case NRF_TIMER_EVENT_COMPARE1:

            break;
        
        case NRF_TIMER_EVENT_COMPARE2:
            //if 0, write 0 into buffer
            if(!pin)
            {
               read_buffer[i] &= ~(1 << j);
            }
            if((j == 7) && (i == 5))
            {
                i = 0;
                j = 0;
                break;
            }
            else if(j == 7)
            {
                j = 0; 
                i++;
                break;
            }
            j++;   
            break;
            
        case NRF_TIMER_EVENT_COMPARE3:
            // Read buffer and erase
            if((i == 0) && (j == 0))
            {
                uint8_t * p = malloc(5 * sizeof(int));
                memcpy(p, &read_buffer[0], 5 * sizeof(int));
                read_message[0] = p;
                memset(read_buffer, 0, sizeof(read_buffer));
            }     
            break;
        default:
            //Do nothing.
            break;
    }
}


void timer_init(void)
{
    uint32_t time_us_compare_0 = (Tpw/2);
    uint32_t time_ticks_0;
    uint32_t time_us_compare_1 = Tpw+Tpi;
    uint32_t time_ticks_1;
    uint32_t time_us_compare_2 = (Tpw/2)+Tpw+Tpi;
    uint32_t time_ticks_2;
    uint32_t time_us_compare_3 = 2*(Tpw+Tpi);
    uint32_t time_ticks_3;

    nrf_drv_timer_config_t timer_cfg = NRF_DRV_TIMER_DEFAULT_CONFIG;


    APP_ERROR_CHECK(nrf_drv_timer_init(&TIMER_WIEGAND, &timer_cfg, timer_wiegand_event_handler));

    time_ticks_0 = nrf_drv_timer_us_to_ticks(&TIMER_WIEGAND, time_us_compare_0);
    time_ticks_1 = nrf_drv_timer_us_to_ticks(&TIMER_WIEGAND, time_us_compare_1);
    time_ticks_2 = nrf_drv_timer_us_to_ticks(&TIMER_WIEGAND, time_us_compare_2);
    time_ticks_3 = nrf_drv_timer_us_to_ticks(&TIMER_WIEGAND, time_us_compare_3);

    // extanded compare stops the timer on compare 3
    nrf_drv_timer_extended_compare(&TIMER_WIEGAND, NRF_TIMER_CC_CHANNEL0, time_ticks_0, NRF_TIMER_SHORT_COMPARE3_CLEAR_MASK, true);
    nrf_drv_timer_compare(&TIMER_WIEGAND,NRF_TIMER_CC_CHANNEL1,time_ticks_1, true);
    nrf_drv_timer_compare(&TIMER_WIEGAND,NRF_TIMER_CC_CHANNEL2,time_ticks_2, true);
    nrf_drv_timer_compare(&TIMER_WIEGAND,NRF_TIMER_CC_CHANNEL3,time_ticks_3, true);
}

/**
 * @brief Function for main application entry.
 */
int main(void)
{
    uint32_t err_code = NRF_SUCCESS;

    err_code = nrf_drv_ppi_init();
    APP_ERROR_CHECK(err_code);

    err_code = nrf_drv_gpiote_init();
    APP_ERROR_CHECK(err_code);

    // set up DATA pins
    nrf_gpio_cfg_output(DATA0_pin);
    nrf_gpio_cfg_output(DATA1_pin);

    timer_init();
    wiegand_read_init();

    while (1)
    {
        __WFE();
        __SEV();
        __WFE();
    }
}

/** @} */
