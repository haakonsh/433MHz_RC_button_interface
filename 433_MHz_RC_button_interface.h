#include <stdint.h>
#include <stdbool.h>
#include "nrf_drv_rtc.h"
#include "nrf_drv_ppi.h"
#include "nrf_drv_gpiote.h"

#define CLK                             1424        //us
#define bit_sample_offset               (CLK/8)     //us
#define INPUT_PIN                       29

#define TIMER_TICKS                     178        // 1/8th of a clock cycle, 1424uS

// interrupt handler called by the rtc half-way between each clock sycle.
void rtc_rc_button_int_handler(nrf_drv_rtc_int_type_t int_type);

// function initialization and configuration of RTC driver instance.
void timer_init(void);
