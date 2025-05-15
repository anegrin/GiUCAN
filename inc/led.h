#ifndef _LED_H
#define _LED_H


#define LED_TX_Pin GPIO_PIN_1
#define LED_TX_Port GPIOA
#define LED_TX LED_TX_Port , LED_TX_Pin

#define LED_RX_Pin GPIO_PIN_0
#define LED_RX_Port GPIOA
#define LED_RX LED_RX_Port , LED_RX_Pin

#define LED_DURATION 25 

void led_init();
void leds_on_error(void);
void led_rx_on(void);
void led_tx_on(void);
void led_process(void);

#endif
