#include "nrf_drv_timer.h"
#include "nrf_drv_gpiote.h"

#define CLK                             1480        //us
#define INPUT_PIN                       4
#define OUTPUT_PIN                      17

const extern nrf_drv_timer_t TIMER2;
void timer2_evt_handler(nrf_timer_event_t evt_type, void* p_context);
extern nrf_drv_gpiote_pin_t input_pin;
