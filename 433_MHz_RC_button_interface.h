#include <stdint.h>
#include <stdbool.h>
#include "nrf_drv_ppi.h"
#include "nrf_drv_gpiote.h"

#define CLK                             1480        //us
#define INPUT_PIN                       4
#define OUTPUT_PIN                      17

#define CONTROL_LENGTH                  80          //from EV1527 OTP Encoder protocol spec
#define DATA_LENGTH                     16          //from EV1527 OTP Encoder protocol spec
#define PACKET_LENGTH                   (CONTROL_LENGTH + DATA_LENGTH)

#define DATA_H                          0xE        //from EV1527 OTP Encoder protocol spec
#define DATA_L                          0x8        //from EV1527 OTP Encoder protocol spec
