#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nrf.h"
#include "nrf_ppi.h"
#include "nrf_drv_ppi.h"
#include "nrf_drv_gpiote.h"
#include "433_MHz_RC_button_interface.h"
#include "nrf_gpiote.h"
#include "app_error.h"
#include "nrf_drv_clock.h"
#include "nrf_drv_timer.h"
#include "main.h"

volatile buffer4_t buffer_4 =       {0};
volatile uint32_t buffer    =       0;

volatile bool message_received =    false;

uint32_t control_msk[20];                           //bitmasks used to extract control bits C0:C19
uint32_t data_msk[4];                               //bitmasks used to extract data bits D0:D3

volatile bool transfer_done =       false;      // check to signal that the transfer buffers are filled

void rx_to_buffer(nrf_timer_event_t evt_type, volatile buffer4_t * buffer4_p){

    uint32_t pin =                                  0;
    static volatile uint8_t i =                     0;

    static volatile uint8_t bitcounter_control =    31;
    static volatile uint8_t bytecounter_control =   0;
    static volatile uint8_t bitcounter_data =       15;

    if(i == 0){    //reset the buffers
        buffer4_p->control.C0   = 0;
        buffer4_p->control.C1   = 0;
        buffer4_p->control.C2   = 0;
        buffer4_p->data         = 0;
    }

    switch (evt_type)
    {
        case NRF_TIMER_EVENT_COMPARE0:
            pin = nrf_gpio_pin_read(input_pin);

            if(i <=  CONTROL_LENGTH){       // fill control buffer (big-endian)

                switch(bytecounter_control)
                {
                    case 0:
                        buffer4_p->control.C0 |=  (pin << bitcounter_control);
                        break;
                    case 1:
                        buffer4_p->control.C1 |=  (pin << bitcounter_control);
                        break;
                    case 2:
                        bitcounter_control -= 16; // C2 is only 16 bit
                        buffer4_p->control.C2 |=  (pin << bitcounter_control);
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
            if(i > CONTROL_LENGTH){           // fill data buffer (big-endian)
                buffer4_p->data |= (pin << bitcounter_data);
                bitcounter_data--;

                if(bitcounter_data == 0){
                    bitcounter_data = 15;
                }
            }
            break;

        case NRF_TIMER_EVENT_COMPARE1:
            // TODO what happends here?
            break;

        default:
            //Do nothing.
            break;
    }
    i++;
    if(i >= PACKET_LENGTH){     // prep for next transmission
        i = 0;
  
        bitcounter_control  = 31;
        bytecounter_control = 0;
        bitcounter_data     = 3;

        nrf_drv_timer_pause(&TIMER2);
        nrf_drv_timer_clear(&TIMER2);

        message_received = true;
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
