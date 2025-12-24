#ifndef _UART_H
#define _UART_H

#include <stdbool.h>
#include "config.h"
#include "stm32f0xx_hal.h"
#include "model.h"

#ifdef SLCAN
#ifdef DEBUG_MODE
#define UART_QUEUE_SIZE 8
#define MESSAGE_SIZE 128
#else
//minimal values as SLCAN does not use UART if no in debug mode
#define UART_QUEUE_SIZE 1
#define MESSAGE_SIZE 3
#endif
#endif

#ifdef XCAN
#define UART_QUEUE_SIZE 64
#define MESSAGE_SIZE 14
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
void uart_deinit(void);
void uart_process(GlobalState *state);

#endif // _UART_H
