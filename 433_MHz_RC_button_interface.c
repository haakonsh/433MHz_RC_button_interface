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

const nrf_drv_rtc_t RTC_RC_BUTTON =     NRF_DRV_RTC_INSTANCE(1); /**< Declaring an instance of nrf_drv_rtc for RTC1. */
nrf_drv_gpiote_pin_t rc_button_pin =    INPUT_PIN;  // input pin

volatile struct buffer_bitfields4_t buffer_4    = {0};
volatile struct buffer_bitfields_t buffer       = {0};
volatile bool message_received = false;

uint32_t control_msk[20];                           //bitmasks used to extract control bits C0:C19
uint32_t data_msk[4];                               //bitmasks used to extract data bits D0:D3

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
    uint32_t rtc_task_addr, rtc_evt_addr;
    uint32_t gpiote_evt_addr;
    nrf_ppi_channel_t ppi_channel_1, ppi_channel_2;

    nrf_drv_gpiote_in_config_t rc_button_pin_cfg = GPIOTE_CONFIG_IN_SENSE_LOTOHI(true);
    rc_button_pin_cfg.pull = NRF_GPIO_PIN_PULLDOWN;

    err_code = nrf_drv_gpiote_in_init(rc_button_pin, &rc_button_pin_cfg, NULL);
    APP_ERROR_CHECK(err_code);

    err_code = nrf_drv_ppi_channel_alloc(&ppi_channel_1);
    APP_ERROR_CHECK(err_code);

    err_code = nrf_drv_ppi_channel_alloc(&ppi_channel_2);
    APP_ERROR_CHECK(err_code);

    rtc_task_addr = nrf_drv_rtc_task_address_get(&RTC_RC_BUTTON, NRF_RTC_TASK_START);
    gpiote_evt_addr = nrf_drv_gpiote_in_event_addr_get(rc_button_pin);

    err_code = nrf_drv_ppi_channel_assign(ppi_channel_1, gpiote_evt_addr, rtc_task_addr);
    APP_ERROR_CHECK(err_code);

    rtc_task_addr = nrf_drv_rtc_task_address_get(&RTC_RC_BUTTON, NRF_RTC_TASK_CLEAR);
    rtc_evt_addr = nrf_drv_rtc_event_address_get(&RTC_RC_BUTTON, NRF_RTC_EVENT_COMPARE_1);

    err_code = nrf_drv_ppi_channel_assign(ppi_channel_2, rtc_evt_addr, rtc_task_addr);
    APP_ERROR_CHECK(err_code);

    err_code = nrf_drv_ppi_channel_enable(ppi_channel_1);
    APP_ERROR_CHECK(err_code);

    err_code = nrf_drv_ppi_channel_enable(ppi_channel_2);
    APP_ERROR_CHECK(err_code);

    nrf_drv_gpiote_in_event_enable(rc_button_pin, false);
}

void rx_to_buffer(nrf_drv_rtc_int_type_t int_type, volatile buffer_bitfields4_t * buffer4_p){
    
    uint8_t pin =                                  0;
    uint16_t bitmask =                              0;
    static volatile uint8_t i =                     0;
    static volatile uint8_t bitcounter_preamble =   31;
    static volatile uint8_t bitcounter_control =    7;
    static volatile uint8_t bitcounter_data =       3;

    if(i == 0){ buffer4_p = 0; } //clear the buffer

    switch (int_type)
    {
        case NRF_DRV_RTC_INT_COMPARE0:
            pin = nrf_gpio_pin_read(rc_button_pin);
            
            if(i <= PREAMBLE_LENGTH){}  // fill preamble buffer (big-endian)
                buffer4_p->preamble = buffer4_p->preamble | (pin << bitcounter_preamble);                
                bitcounter_preamble--;                
                break;
            }
            if(((i - 1) >= PREAMBLE_LENGTH) && (i <= (PREAMBLE_LENGTH + CONTROL_LENGTH))){       // fill control buffer (big-endian)
                                
                buffer4_p->control |= (pin << bitcounter_control));
                bitcounter_control--;
                
                if(bitcounter_control == 0){
                    buffer4_p += 1;     // point to next byte in control buffer
                    bitcounter_control = 7;
                }
                break;            
            }
            if(i >= (PREAMBLE_LENGTH + CONTROL_LENGTH))){           // fill data buffer (big-endian)      
                buffer4_p->data |= (pin << bitcounter_data));
                bitcounter_data--;
                
                if(bitcounter_data == 0){
                    buffer4_p.data += 1;        // point to next byte in data buffer
                    bitcounter_data = 3;
                }
                              
            }
            break;

        case NRF_DRV_RTC_INT_COMPARE1:

            break;

        default:
            //Do nothing.
            break;
    }
    i++;
    if(i >= PACKET_LENGTH){
        i = 0; // prep for next transmission
        message_received = true;
        
        bitcounter_preamble =    31; // reset counters
        bitcounter_control =     7;
        bitcounter_data =        3;
                    
        buffer4_p.control -= 10;    // reset pointers to the start of the buffers
        buffer4_p.data -= 1;
    }
    
}

void rtc_rc_button_int_handler(nrf_drv_rtc_int_type_t int_type)
{
    rx_to_buffer(int_type, &buffer_4);
    if(message_received){
        if(buffer.preamble != RC_BUTTON_PREAMBLE){
        //Error handling
        }
        
        bit_extraction(&buffer_4, &buffer);
        
        message_received = false;
    }
}

void bit_extraction(volatile buffer_bitfields4_t * buffer4_p, volatile buffer_bitfields_t * buffer_p){
    volatile static uint8_t i,j = 0;
    // fix below // Uses a pointer to iterate through the control buffer, byte by byte.
    for(i = 0;i <= CONTROL_LENGTH; i++){ 
        
        if(is_odd(i)){          
            if(buffer4_p->control.c_0 == (DATA_H << 4)){ buffer_p->control = (1 << j); }     // decodes lower nibble of the first byte of .control from four bits to one
            else{ buffer_p->control = (0 << k); }
       
            j++;
        }
        else{
            if(buffer4_p->control.C_1 == DATA_H){ buffer_p->control = (1 << j); }            // decodes lower nibble of the first byte of .control from four bits to one
            else{ buffer_p->control = (0 << k); }
        }
    }

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
    nrf_drv_rtc_tick_enable(&RTC_RC_BUTTON, true);

    // Set compare channel to trigger interrupts
    rtc_ticks = us_to_ticks_convert(bit_sample_offset);
    err_code = nrf_drv_rtc_cc_set(&RTC_RC_BUTTON, 0, rtc_ticks, true);
    APP_ERROR_CHECK(err_code);

    rtc_ticks = us_to_ticks_convert(CLK);
    err_code = nrf_drv_rtc_cc_set(&RTC_RC_BUTTON, 1, rtc_ticks, true);
    APP_ERROR_CHECK(err_code);

    //Power on RTC instance
    nrf_drv_rtc_enable(&RTC_RC_BUTTON);
}
