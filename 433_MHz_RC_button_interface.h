#include <stdint.h>
#include <stdbool.h>
#include "nrf_drv_rtc.h"
#include "nrf_drv_ppi.h"
#include "nrf_drv_gpiote.h"

#define CLK                             1480        //us
#define bit_sample_offset               (CLK/2)      //us
#define INPUT_PIN                       29

#define RC_BUTTON_PREAMBLE              0x80000000  //from EV1527 OTP Encoder protocol spec
#define PREAMBLE_LENGTH                 32          //from EV1527 OTP Encoder protocol spec
#define CONTROL_LENGTH                  80          //from EV1527 OTP Encoder protocol spec
#define DATA_LENGTH                     16          //from EV1527 OTP Encoder protocol spec
#define PACKET_LENGTH                   (PREAMBLE_LENGTH + CONTROL_LENGTH + DATA_LENGTH)

#define DATA_H                          0xE        //from EV1527 OTP Encoder protocol spec
#define DATA_L                          0x8        //from EV1527 OTP Encoder protocol spec

uint8_t is_odd(uint8_t x);

typedef struct {
    uint32_t C0;
    uint32_t C1;
    uint16_t C2;
}control4_t;

typedef struct{
    uint32_t preamble;
    control4_t control;
    uint16_t data;
}buffer4_t;

// function converts ms to rtc tick units
uint32_t us_to_ticks_convert(uint32_t);

//function that initlizes the GPIOTE to start the RTC when rc_button_pin is pulled high
void rc_button_init(void);

// function collects the bit stream and places it into their respective buffers
void buffer_sort(nrf_drv_rtc_int_type_t int_type, volatile uint32_t * buffer_p);

// interrupt handler called by the rtc half-way between each clock sycle.
void rtc_rc_button_int_handler(nrf_drv_rtc_int_type_t int_type);

// function decodes bits from the 4bit worded buffer and places them in a uint32_t
void bit_decode(volatile buffer4_t * buffer4_p, volatile uint32_t * buffer_p);

// function initialization and configuration of RTC driver instance.
void rtc_init(void);

// function creates bitmaks for decoding the bit stream
void bitmasks_init(void);

// function that extracts individual bits from their respective buffer and places them into a boolean array of bits
void rc_button_handler(bool * message_p);

static inline uint8_t is_odd(uint8_t x) { return x & 1; }

