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


#define C0                              0           // bit index in the boolean array
#define C1                              1
#define C2                              2
#define C3                              3
#define C4                              4
#define C5                              5
#define C6                              6
#define C7                              7
#define C8                              8
#define C9                              9
#define C10                             10
#define C11                             11
#define C12                             12
#define C13                             13
#define C14                             14
#define C15                             15
#define C16                             16
#define C17                             17
#define C18                             18
#define C19                             19
#define D0                              20
#define D1                              21
#define D2                              22
#define D3                              23

uint8_t is_odd(uint8_t x);

typedef struct {
    uint8_t C1  :1;
    uint8_t C2  :1;
    uint8_t C3  :1;
    uint8_t C4  :1;
    uint8_t C5  :1;
    uint8_t C6  :1;
    uint8_t C7  :1;
    uint8_t C8  :1;
    uint8_t C9  :1;
    uint8_t C10 :1;
    uint8_t C11 :1;
    uint8_t C12 :1;
    uint8_t C13 :1;
    uint8_t C14 :1;
    uint8_t C15 :1;
    uint8_t C16 :1;
    uint8_t C17 :1;
    uint8_t C18 :1;
    uint8_t C19 :1;
}control_t;

typedef struct{
    uint8_t D0 : 1;
    uint8_t D1 : 1;
    uint8_t D2 : 1;
    uint8_t D3 : 1;
}data_t;

typedef struct {
    control_t control;
    data_t data;
}buffer_bitfields_t;

typedef struct {
    uint8_t C0  :4;
    uint8_t C1  :4;
    uint8_t C2  :4;
    uint8_t C3  :4;
    uint8_t C4  :4;
    uint8_t C5  :4;
    uint8_t C6  :4;
    uint8_t C7  :4;
    uint8_t C8  :4;
    uint8_t C9  :4;
    uint8_t C10 :4;
    uint8_t C11 :4;
    uint8_t C12 :4;
    uint8_t C13 :4;
    uint8_t C14 :4;
    uint8_t C15 :4;
    uint8_t C16 :4;
    uint8_t C17 :4;
    uint8_t C18 :4;
    uint8_t C19 :4;
}control4_t;

typedef struct{
    uint8_t D0 :4;
    uint8_t D1 :4;
    uint8_t D2 :4;
    uint8_t D3 :4;
}data4_t;

typedef struct{
    uint32_t preamble;
    control4_t control;
    data4_t data;
}buffer_bitfields4_t;

// function converts ms to rtc tick units
uint32_t us_to_ticks_convert(uint32_t);

//function that initlizes the GPIOTE to start the RTC when rc_button_pin is pulled high
void rc_button_init(void);

// function collects the bit stream and places it into their respective buffers
void buffer_sort(nrf_drv_rtc_int_type_t int_type, volatile struct buffer_t * buffer_p);

// interrupt handler called by the rtc half-way between each clock sycle.
void rtc_rc_button_int_handler(nrf_drv_rtc_int_type_t int_type);


// function initialization and configuration of RTC driver instance.
void rtc_init(void);

// function creates bitmaks for decoding the bit stream
void bitmasks_init(void);

// function that extracts individual bits from their respective buffer and places them into a boolean array of bits
void rc_button_handler(bool * message_p);

static inline uint8_t is_odd(uint8_t x) { return x & 1; }