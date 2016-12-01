#include <stdint.h>
#include <stdbool.h>
#include "nrf_drv_rtc.h"
#include "nrf_drv_ppi.h"
#include "nrf_drv_gpiote.h"
#include "nrf_drv_timer.h"


#define CLK                             1424        //us
#define TIMER_TICKS                     (CLK/8)     //us
#define INPUT_PIN                       4
#define OUTPUT_PIN                      29

// interrupt handler called by the rtc half-way between each clock sycle.
void timer_rc_button_evt_handler(nrf_timer_event_t event_type, void* p_context);

// function initialization and configuration of RTC driver instance.
void timer_init(void);
