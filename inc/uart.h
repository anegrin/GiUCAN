#ifndef _UART_H
#define _UART_H

#include <stdbool.h>
#include "config.h"
#include "stm32f0xx_hal.h"
#include "model.h"

#ifdef UART_DEBUG_MODE
#define MESSAGE_SIZE 128
#else
#define MESSAGE_SIZE 13
#endif

typedef struct
{
    uint8_t id;
    char *pattern;
} DashboardItem;

#define UNKNOWN_ITEM_ID 255
#define CLEANUP_ITEM_ID 0
#define FIRMWARE_ITEM_ID 1

#ifdef BHCAN
static const DashboardItem CLEANUP_ITEM = {.id = CLEANUP_ITEM_ID};
static const DashboardItem FIRMWARE_ITEM = {.id = FIRMWARE_ITEM_ID, .pattern = "GiUCAN " GIUCAN_VERSION};
#endif

bool send_state(GlobalState* state);
void uart_init(void);
void uart_process(GlobalState* state);

#endif // _UART_H
