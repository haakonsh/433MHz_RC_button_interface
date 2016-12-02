#include <stdint.h>
#include <stdbool.h>
#include "nrf_drv_ppi.h"
#include "nrf_drv_gpiote.h"
#include "nrf_drv_timer.h"

#define CLK                             1480        //us
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

extern volatile buffer4_t buffer_4;
extern volatile uint32_t buffer;
extern volatile bool message_received;

// funcion samples the datastream and places the bits in a buffer
void rx_to_buffer(nrf_timer_event_t evt_type, volatile buffer4_t * buffer4_p);


// function decodes bits from the 4bit worded buffer and places them in a uint32_t
void bit_decode(volatile buffer4_t * buffer4_p, volatile uint32_t * buffer_p);
=======
>>>>>>> preamble_sense
