#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nrf.h"
#include "nrf_gpio.h"
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

volatile bool transfer_done =       false;          // check to signal that the transfer buffers are filled

// funcion samples the datastream and places the bits in a buffer
void rx_to_buffer(nrf_timer_event_t evt_type, volatile buffer4_t * buffer4_p){

    uint32_t pin =                                  0;  // Temporary variable used to hold the input_pin state
    static volatile uint8_t bitcounter =            0;  // Tracks the number of bits that have been read
    static volatile uint8_t bitcounter_control =    31; // Tracks the number of bits in the 'control' bitfield
    static volatile uint8_t wordcounter_control =   0;  // Tracks the number of words in the 'control' bitfield
    static volatile uint8_t bitcounter_data =       15; // Tracks the number of bits in the 'data' bitfield

    if(bitcounter == 0){    //reset the buffers
        buffer4_p->control.C0   = 0;
        buffer4_p->control.C1   = 0;
        buffer4_p->control.C2   = 0;
        buffer4_p->data         = 0;
    }

    switch (evt_type)
    {
        case NRF_TIMER_EVENT_COMPARE0:
            pin = nrf_gpio_pin_read(input_pin);     //Sample the input_pin

            if(bitcounter <=  CONTROL_LENGTH){      // fill control buffer (big-endian)

                // the control buffer consits of two uint32_t and one uint16_t, we
                switch(wordcounter_control)
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
                        // TODO ERROR handling
                        break;
                }

                // decrement bitcounter_control from 31 to 0, then increment wordcounter_control by 1. Big endian.
                if(bitcounter_control == 0){
                    bitcounter_control = 31;
                    wordcounter_control++;
                }
                else{
                bitcounter_control--;
                }
                break;
            }
            // fill data buffer (big-endian)
            if(bitcounter > CONTROL_LENGTH){
                buffer4_p->data |= (pin << bitcounter_data);
            }
            bitcounter_data--;
            if(bitcounter_data == 0){
                bitcounter_data = 15;
            }
            break;

        case NRF_TIMER_EVENT_COMPARE1:
            // Do nothing, compare1 resets TIMER2.
            break;

        default:
            //Do nothing.
            break;
    }
    // increment the bitcounter
    bitcounter++;

    // prep for next transmission
    if(bitcounter >= PACKET_LENGTH){
        bitcounter = 0;

        bitcounter_control  = 31;
        wordcounter_control = 0;
        bitcounter_data     = 3;

        nrf_drv_timer_pause(&TIMER2);
        nrf_drv_timer_clear(&TIMER2);

        message_received = true;
    }
}

// function decodes bits from the 4bit worded buffer and places them in a uint32_t
void bit_decode(volatile buffer4_t * buffer4_p, volatile uint32_t * buffer_p){
    uint8_t wordcounter   = 0;
    uint32_t placeholder = 0;

    buffer_p = 0;

    /* The following logic is a bit hard to follow. We are extracting one 4 bit word at a time, then we compare
    the word with DATA_H which is the word that corresponds to a logic 1, if it evaluates to true, it will set a 1 in the
    the correct position of a uint32_t buffer. Our msb is at bit 23*/
    for(wordcounter = 0;wordcounter <= 7; wordcounter++){ // 8 * 4 bits per uint32_t.
        /* Here we extract the 4 bit word. buffer4_p is a pointer to a struct which contains control.C0. We bit shift the
        entire register right a number of place to extract the word, starting with bits 31-27
        31 30 29 28 27 26 25 24 23 22 21 .. .. .. .. --> .. .. .. .. 0 0 0 0 0 31 30 29 28.
        .. .. .. .. 0 0 0 0 0 31 30 29 28 is then compared to DATA_H (0xE / 0b1110). & 0xF is just to remove any unwanted bits
        over 0xF  */
        placeholder = buffer4_p->control.C0 >> (28 - (wordcounter*4)) & 0xF;
        // needs the placeholder variable because the if statement evaluates '==' before '&'
        if(placeholder ==  DATA_H){
            /* Here we OR a 1 into the buffer, Example:
                001100101100100101001101110101010
            or  000000010000000000000000000000000
            =   001100111100100101001101110101010  */
            *buffer_p |= (1 << (23 - wordcounter));  // wordcounter = 0 -> msb
        }
        else if(!(placeholder ==  DATA_H) && !(placeholder ==  DATA_H) ){
            //TODO ERROR invalid word, buffer is misaligned or erroneous bitstream. This is a good place for a breakpoint
        }

        placeholder = buffer4_p->control.C1 >> (28 - (wordcounter*4)) & 0xF;
        if(placeholder ==  DATA_H){
            *buffer_p |= (1 << (15 - wordcounter));  // wordcounter = 0 -> msb - 8 bits
        }
        else if(!(placeholder ==  DATA_H) && !(placeholder ==  DATA_H) ){
            //TODO ERROR invalid word, buffer is misaligned or erroneous bitstream. This is a good place for a breakpoint
        }

        if(wordcounter <= 3){
            placeholder = buffer4_p->control.C2 >> (12 - (wordcounter*4)) & 0xF; // control.C2 is only 16bit
            if(placeholder ==  DATA_H){
                *buffer_p |= (1 << (7 - wordcounter));   // wordcounter = 0 -> msb - 16 bits
            }
            else if(!(placeholder ==  DATA_H) && !(placeholder ==  DATA_H) ){
                //TODO ERROR invalid word, buffer is misaligned or erroneous bitstream. This is a good place for a breakpoint
            }

            placeholder = buffer4_p->data >> (12 - (wordcounter*4)) & 0xF;        // data is only 16bit
            if(placeholder ==  DATA_H){
                *buffer_p |= (1 << (3 - wordcounter));   // wordcounter = 0 -> msb - 20 bits
            }
            else if(!(placeholder ==  DATA_H) && !(placeholder ==  DATA_H) ){
                //TODO ERROR invalid word, buffer is misaligned or erroneous bitstream. This is a good place for a breakpoint
            }
        }
    }
}
