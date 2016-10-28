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

const nrf_drv_rtc_t RTC_RC_BUTTON =     NRF_DRV_RTC_INSTANCE(1); /**< Declaring an instance of nrf_drv_rtc for RTC1. */
nrf_drv_gpiote_pin_t rc_button_pin =    INPUT_PIN;  // input pin

volatile buffer4_t buffer_4 = {0};
volatile uint32_t buffer    = 0;

volatile bool message_received = false;

uint32_t control_msk[20];                           //bitmasks used to extract control bits C0:C19
uint32_t data_msk[4];                               //bitmasks used to extract data bits D0:D3

volatile bool transfer_done =           false;      // check to signal that the transfer buffers are filled

nrf_ppi_channel_t ppi_channel_1, ppi_channel_2;

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

void rx_to_buffer(nrf_drv_rtc_int_type_t int_type, volatile buffer4_t * buffer4_p){
    
    uint32_t pin =                                  0;
    static volatile uint8_t i =                     0;
    static volatile uint8_t bitcounter_preamble =   31;
    static volatile uint8_t bitcounter_control =    31;
    static volatile uint8_t bytecounter_control =   0;
    static volatile uint8_t bitcounter_data =       15;

    if(i == 0){    //reset the buffers
        buffer4_p->preamble     = 0;
        buffer4_p->control.C0   = 0;
        buffer4_p->control.C1   = 0;
        buffer4_p->control.C2   = 0;        
        buffer4_p->data         = 0;
    }

    switch (int_type)
    {
        case NRF_DRV_RTC_INT_COMPARE0:
            pin = nrf_gpio_pin_read(rc_button_pin);
            
            if(i <= PREAMBLE_LENGTH){  // fill preamble buffer (big-endian)
                buffer4_p->preamble = buffer4_p->preamble | (pin << bitcounter_preamble);                
                bitcounter_preamble--;                
                break;
            }
            if(((i - 1) >= PREAMBLE_LENGTH) && (i <= (PREAMBLE_LENGTH + CONTROL_LENGTH))){       // fill control buffer (big-endian)
                
                switch(bytecounter_control)
                {
                    case 0:
                        buffer4_p->control.C0 =  (pin << bitcounter_control);
                        break;
                    case 1:
                        buffer4_p->control.C1 =  (pin << bitcounter_control);
                        break;
                    case 2:
                        bitcounter_control -= 16; // C2 is only 16 bit
                        buffer4_p->control.C2 =  (pin << bitcounter_control);
                        break;
                    default:
                        // ERROR handling
                        break;
                }
                 
                if(bitcounter_control == 0){
                    bitcounter_control = 31;
                    bytecounter_control++;
                }
                bitcounter_control--;
                
                break;            
            }
            if(i >= (PREAMBLE_LENGTH + CONTROL_LENGTH)){           // fill data buffer (big-endian)      
                buffer4_p->data |= (pin << bitcounter_data);
                bitcounter_data--;
                
                if(bitcounter_data == 0){                    
                    bitcounter_data = 15;
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
    if(i >= PACKET_LENGTH){     // prep for next transmission
        i = 0;          
        bitcounter_preamble = 31; 
        bitcounter_control  = 31;
        bytecounter_control = 0;
        bitcounter_data     = 3;        
        
        nrf_rtc_task_trigger(RTC_RC_BUTTON.p_reg, NRF_RTC_TASK_STOP);
        nrf_rtc_task_trigger(RTC_RC_BUTTON.p_reg, NRF_RTC_TASK_CLEAR);
        
        message_received = true;
    }
    
}

void rtc_rc_button_int_handler(nrf_drv_rtc_int_type_t int_type)
{
    rx_to_buffer(int_type, &buffer_4);
    if(message_received){
        if(buffer_4.preamble != RC_BUTTON_PREAMBLE){
        // Error handling
        }        
        bit_decode(&buffer_4, &buffer);
        // send 'buffer' to UART
        message_received = false;
    }
}

void bit_decode(volatile buffer4_t * buffer4_p, volatile uint32_t * buffer_p){
    uint8_t i   = 0;
    uint8_t j   = 0;
    uint32_t placeholder = 0;

    for(i = 0;i <= 7; i++){ // 4 * 8 bits per 32 bit uint.
        placeholder = buffer4_p->control.C0 >> (28 - (j*4)) & 0xF; // need a placeholder because the if statement evaluates '==' before '&'
        if(placeholder ==  DATA_H){ 
            buffer |= (1 << (23 - i));  // i = 0 -> MSB
        }
        else{
            buffer |= (0 << (23 - i));
        }
        placeholder = buffer4_p->control.C1 >> (28 - (j*4)) & 0xF;
        if(placeholder ==  DATA_H){ 
            buffer |= (1 << (15 - i));  // i = 0 -> LSB + 16 bits
        }
        else{
            buffer |= (0 << (15 - i));
        }
        if(i <= 3){
            placeholder = buffer4_p->control.C2 >> (12 - (j*4)) & 0xF; // .control.C2 is only 16bit
            if(placeholder ==  DATA_H){ 
                buffer |= (1 << (7 - i));   // i = 0 -> LSB + 8 bits
            }
            else{
                buffer |= (0 << (7 - i));
            }
        
            placeholder = buffer4_p->data >> (12 - (j*4)) & 0xF;        // .data is only 16bit
            if(placeholder ==  DATA_H){ 
                buffer |= (1 << (3 - i));   // i = 0 -> LSB + 4 bits
            }
            else{
                buffer |= (0 << (3 - i));
            }
        }
    }

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
void rtc_init(void)
{
    uint32_t err_code, rtc_ticks;

    //Initialize RTC instance
    nrf_drv_rtc_config_t config = NRF_DRV_RTC_DEFAULT_CONFIG;
    err_code = nrf_drv_rtc_init(&RTC_RC_BUTTON, &config, rtc_rc_button_int_handler);
    APP_ERROR_CHECK(err_code);

    //Enable tick event & interrupt
    nrf_drv_rtc_tick_enable(&RTC_RC_BUTTON, true);
    nrf_drv_rtc_tick_disable(&RTC_RC_BUTTON);

    // Set compare channel to trigger interrupts
    rtc_ticks = us_to_ticks_convert(bit_sample_offset);
    err_code = nrf_drv_rtc_cc_set(&RTC_RC_BUTTON, 0, rtc_ticks, true);
    APP_ERROR_CHECK(err_code);

    rtc_ticks = us_to_ticks_convert(CLK);
    err_code = nrf_drv_rtc_cc_set(&RTC_RC_BUTTON, 1, rtc_ticks, true);
    APP_ERROR_CHECK(err_code);

    //Power on RTC instance
    //nrf_drv_rtc_enable(&RTC_RC_BUTTON);
}
