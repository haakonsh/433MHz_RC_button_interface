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
#define CONTROL_LENGTH_0                32          //lowest control buffer length
#define CONTROL_LENGTH_1                32          //middle control buffer length
#define CONTROL_LENGTH_2                16          //highest control buffer length
#define CONTROL_LENGTH                  80          //from EV1527 OTP Encoder protocol spec
#define DATA_LENGTH                     16          //from EV1527 OTP Encoder protocol spec


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

//this buffer stores the decoded information in boolean arrays
struct buffer_t{
    uint32_t preamble;
    uint32_t control_0;          // control bits 0:7
    uint32_t control_1;          // control bits 8:15
    uint16_t control_2;          // control bits 16:19
    uint16_t data;               // data bits 0:3
 };

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
