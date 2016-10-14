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

const uint16_t DATA_H =                 0xE;        //from EV1527 OTP Encoder protocol spec
const uint16_t DATA_L =                 0x8;        //from EV1527 OTP Encoder protocol spec

const nrf_drv_rtc_t RTC_RC_BUTTON =     NRF_DRV_RTC_INSTANCE(2); /**< Declaring an instance of nrf_drv_rtc for RTC2. */
nrf_drv_gpiote_pin_t rc_button_pin =    13;         // input pin

struct buffer_t buffer;
bool message[24];                                   //boolean array containing the Control and Data bits

uint32_t control_msk[20] =  {0};                    //bitmasks used to extract control bits C0:C19
uint32_t data_msk[4] =      {0};                    //bitmasks used to extract data bits D0:D3

volatile bool transfer_done =           false;      // check to signal that the transfer buffers are filled


uint32_t us_to_ticks_convert(uint32_t time_us)
{
   uint32_t u1, u2, t1, t2;

   t1 = time_us / 15625;
   u1 = t1 * 512;
   t2 = time_us - t1 * 15625;
   u2 = (t2 << 9) / 15625;

   return (u1 + u2);
}

void rc_button_init(void)
{
    ret_code_t err_code;
    uint32_t rtc_task_addr;
    uint32_t gpiote_evt_addr;
    nrf_ppi_channel_t ppi_channel;

    nrf_drv_gpiote_in_config_t rc_button_pin_cfg = GPIOTE_CONFIG_IN_SENSE_LOTOHI(true);
    rc_button_pin_cfg.pull = NRF_GPIO_PIN_PULLDOWN;

    err_code = nrf_drv_gpiote_in_init(rc_button_pin, &rc_button_pin_cfg, NULL);
    APP_ERROR_CHECK(err_code);

    err_code = nrf_drv_ppi_channel_alloc(&ppi_channel);
    APP_ERROR_CHECK(err_code);

    rtc_task_addr = nrf_drv_rtc_task_address_get(&RTC_RC_BUTTON, NRF_RTC_TASK_START);
    gpiote_evt_addr = nrf_drv_gpiote_in_event_addr_get(rc_button_pin);

    err_code = nrf_drv_ppi_channel_assign(ppi_channel, gpiote_evt_addr, rtc_task_addr);
    APP_ERROR_CHECK(err_code);

    err_code = nrf_drv_ppi_channel_enable(ppi_channel);
    APP_ERROR_CHECK(err_code);

    nrf_drv_gpiote_in_event_enable(rc_button_pin, false);
}


void buffer_sort(nrf_drv_rtc_int_type_t int_type, struct buffer_t * buffer_p){
    uint32_t pin =              0;
    uint16_t bitmask =          0;
    static volatile uint8_t i = 1;

    switch (int_type)
    {
        case NRF_DRV_RTC_INT_COMPARE0:

            pin = nrf_gpio_pin_read(rc_button_pin);

            if(i <= PREAMBLE_LENGTH){                                                                                   // Preamble
                bitmask = ~(1 << i);
                buffer.preamble &= (pin | bitmask);
            }
            if((i > PREAMBLE_LENGTH) && (i <= (PREAMBLE_LENGTH + CONTROL_LENGTH_0))){                                   // Control bits 0:7
                bitmask = ~(1 << (i - PREAMBLE_LENGTH));
                buffer.control_0 &= (pin | bitmask);
            }
            if((i > PREAMBLE_LENGTH) && (i <= (PREAMBLE_LENGTH + CONTROL_LENGTH_0 + CONTROL_LENGTH_1))){                // Control bits 8:15
                bitmask = ~(1 << (i - (PREAMBLE_LENGTH + CONTROL_LENGTH_0)));
                buffer.control_1 &= (pin | bitmask);
            }
            if((i > PREAMBLE_LENGTH) && (i <= (PREAMBLE_LENGTH + CONTROL_LENGTH))){                                     // Control bits 16:19
                bitmask = ~(1 << (i - (PREAMBLE_LENGTH + CONTROL_LENGTH_0 + CONTROL_LENGTH_1)));
                buffer.control_2 &= (pin | bitmask);
            }
            if((i > (PREAMBLE_LENGTH + CONTROL_LENGTH)) && (i <= (PREAMBLE_LENGTH + CONTROL_LENGTH + DATA_LENGTH))){    // Data bits 0:3
                bitmask = ~(1 << (i - (PREAMBLE_LENGTH + CONTROL_LENGTH)));
                buffer.data &= (pin | bitmask);
            }
            if(i > (PREAMBLE_LENGTH + CONTROL_LENGTH + DATA_LENGTH)){
                i = 1;
                NRF_RTC2->TASKS_STOP = 1;
                transfer_done = true;
                bitmask =  0;
            }
            i++;
            break;

        case NRF_DRV_RTC_INT_COMPARE1:
            nrf_drv_rtc_counter_clear(&RTC_RC_BUTTON);
            if(i > (PREAMBLE_LENGTH + CONTROL_LENGTH + DATA_LENGTH)){
                i = 0;
                NRF_RTC2->TASKS_STOP = 1;
                nrf_drv_rtc_counter_clear(&RTC_RC_BUTTON);
                transfer_done = true;
            }
            break;

        default:
            //Do nothing.
            break;
    }
}


void rtc_rc_button_int_handler(nrf_drv_rtc_int_type_t int_type)
{
    buffer_sort(int_type,&buffer);
}


/** @brief Function initialization and configuration of RTC driver instance.
 */
void rtc_init(void)
{
    uint32_t err_code, rtc_ticks;

    //Initialize RTC instance
    nrf_drv_rtc_config_t config = NRF_DRV_RTC_DEFAULT_CONFIG;
    err_code = nrf_drv_rtc_init(&RTC_RC_BUTTON, &config, rtc_rc_button_int_handler);
    APP_ERROR_CHECK(err_code);

    //Enable tick event & interrupt
    nrf_drv_rtc_tick_enable(&RTC_RC_BUTTON,true);

    // Set compare channel to trigger interrupts
    rtc_ticks = us_to_ticks_convert(bit_sample_offset)
    err_code = nrf_drv_rtc_cc_set(&RTC_RC_BUTTON, 0, rtc_ticks, true);
    APP_ERROR_CHECK(err_code);

    rtc_ticks = us_to_ticks_convert(CLK);
    err_code = nrf_drv_rtc_cc_set(&RTC_RC_BUTTON, 1, rtc_ticks, true);
    APP_ERROR_CHECK(err_code);

    //Power on RTC instance
    nrf_drv_rtc_enable(&RTC_RC_BUTTON);
}


void bitmasks_init(void){                       // creates bitmaks for reading 4 bits (15 = 0b1111)

    for(uint8_t j = 0; j <= 7; j++){            // bitmasks for the control_0 buffer
        control_msk[j] = (15 << (4 * j));
    }
    for(uint8_t j = 0; j <= 7; j++){             // bitmasks for the control_1 buffer
        control_msk[j+8] = (15 << (4 * j));
    }
    for(uint8_t j = 0; j <= 3; j++){
        control_msk[j+16] = (15 << (4 * j));     // bitmasks for the control_2 buffer
    }
    for(uint8_t i = 0; i <= 3;i++){
        data_msk[0] = (15 << (4 * i));           // bitmasks for the data buffer
    }
}


void rc_button_handler(bool * message_p)
{

    while(!transfer_done)           // blocking sleep mode. Can be replaced with a SWI calling rc_button_handler
    {
        __WFE();
        __SEV();
        __WFE();
    }

    if(buffer.preamble != RC_BUTTON_PREAMBLE){
        //Error handling
    }

    for(uint8_t i = 0; i <= 7; i++){
        (((buffer.control_0 && control_msk[i]) >> (4 * i)) == DATA_H) ? (message_p[i] = 1) : (message_p[i] = 0);            // puts the control bits into the message array
    }
    for(uint8_t i = 0; i <= 7; i++){
        (((buffer.control_1 && control_msk[i+8]) >> (4 * i)) == DATA_H) ? (message_p[8 + i] = 1) : (message_p[i] = 0);
    }
    for(uint8_t i = 0; i <= 3; i++){
        (((buffer.control_2 && control_msk[i+16]) >> (4 * i)) == DATA_H) ? (message_p[16 + i] = 1) : (message_p[i] = 0);
    }

    for(uint8_t i = 0; i <= 3;i++){
        ((buffer.data && data_msk[i]) == DATA_H) ? (message_p[20 + i] = 1) : (message_p[20 + i] = 0);                       // puts the data bits into the message array
    }

    buffer.preamble =   0;  // reset the buffers
    buffer.control_0 =  0;
    buffer.control_1 =  0;
    buffer.control_2 =  0;
    buffer.data =       0;

    transfer_done = false;  // enable next transfer

}
