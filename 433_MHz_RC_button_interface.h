#include <stdint.h>
#include <stdbool.h>
#include "nrf_drv_ppi.h"
#include "nrf_drv_gpiote.h"

#define CLK                             1480        //us
#define bit_sample_offset               (CLK/2)      //us
#define INPUT_PIN                       29

#define CONTROL_LENGTH                  80          //from EV1527 OTP Encoder protocol spec
#define DATA_LENGTH                     16          //from EV1527 OTP Encoder protocol spec
#define PACKET_LENGTH                   (CONTROL_LENGTH + DATA_LENGTH)

#define DATA_H                          0xE        //from EV1527 OTP Encoder protocol spec
#define DATA_L                          0x8        //from EV1527 OTP Encoder protocol spec

typedef struct {
    uint32_t C0;
    uint32_t C1;
    uint16_t C2;
}control4_t;

typedef struct{
    control4_t control;
    uint16_t data;
}buffer4_t;

// funcion samples the datastream and places the bits in a buffer
void rx_to_buffer(nrf_drv_rtc_int_type_t int_type, volatile buffer4_t * buffer4_p);

// function collects the bit stream and places it into their respective buffers
void buffer_sort(nrf_drv_rtc_int_type_t int_type, volatile uint32_t * buffer_p);

// interrupt handler called by the rtc half-way between each clock sycle.
void TIMER_int_handler(nrf_drv_rtc_int_type_t int_type);

// function decodes bits from the 4bit worded buffer and places them in a uint32_t
void bit_decode(volatile buffer4_t * buffer4_p, volatile uint32_t * buffer_p);
