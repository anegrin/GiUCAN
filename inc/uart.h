#ifndef _UART_H
#define _UART_H

#include <stdbool.h>
#include "config.h"
#include "stm32f0xx_hal.h"
#include "model.h"

#define MESSAGE_SIZE 13

#ifdef C1CAN
bool send_state(GlobalState *state);
#endif
#ifdef XCAN
void uart_init(void);
void uart_process(GlobalState *state);
#endif

#endif // _UART_H
