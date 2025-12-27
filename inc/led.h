#ifndef _LED_H
#define _LED_H

#include "config.h"
#include <stdint.h>

//LED_TX_Pin on GPIOA
#define LED_TX_Port GPIOA
#define LED_TX LED_TX_Port , LED_TX_Pin

//LED_RX_Pin on GPIOA
#define LED_RX_Port GPIOA
#define LED_RX LED_RX_Port , LED_RX_Pin

#define LED_DURATION 25 

void leds_on_error(void);
void leds_blink(uint8_t count, uint32_t delay);
void led_rx_on(void);
void led_tx_on(void);
void led_process(void);

#endif
