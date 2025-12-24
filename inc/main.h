#ifndef _MAIN_H
#define _MAIN_H

/* main.h is just a wrapper for IDE generated code */
#include "config.h"
#include "error.h"

#ifndef UART_PIN
//UART_PIN on GPIOA, default is SWDCLK one
#define UART_PIN GPIO_PIN_14
#endif

#ifdef C1CAN
//RESET_PIN on GPIOA, default is SWDIO one
#define RESET_CMD_PIN GPIO_PIN_13
#endif

//CAN_S_PIN on GPIOC
#define CAN_S_PIN GPIO_PIN_13

#endif
