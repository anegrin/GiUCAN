#ifndef _UART_H
#define _UART_H

#include <stdbool.h>
#include "config.h"
#include "stm32f0xx_hal.h"
#include "model.h"

#ifdef SLCAN
#define MESSAGE_SIZE 128
#endif

#ifdef XCAN
#define MESSAGE_SIZE 13
#endif

#ifdef C1CAN
bool send_state(GlobalState *state);
#endif
#ifdef SLCAN
#ifdef DEBUG_MODE
uint8_t print_to_uart(char* message);
uint8_t printf_to_uart(const char* format, ...);
#endif
#endif
void uart_init(void);
void uart_process(GlobalState *state);

#endif // _UART_H
