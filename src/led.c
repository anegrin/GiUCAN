//
// LED: Handles blinking of status light
//

#include "stm32f0xx_hal.h"
#include "config.h"
#include "led.h"

// Private variables
static uint32_t led_tx_laston = 0;
static uint32_t led_rx_laston = 0;
static uint32_t led_tx_lastoff = 0;
static uint32_t led_rx_lastoff = 0;

// Initialize LED GPIOs
void led_init()
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = 0;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

void leds_on_error(void)
{
    led_tx_laston = 0;
    led_rx_laston = 0;
    HAL_GPIO_WritePin(LED_TX, 1);
    HAL_GPIO_WritePin(LED_RX, 1);
}

void leds_blink(uint8_t count, uint32_t delay){
	for(uint8_t i=0; i<count; i++){
        HAL_GPIO_WritePin(LED_RX, 1);
 		HAL_Delay(delay);
        HAL_GPIO_WritePin(LED_RX, 0);
        HAL_GPIO_WritePin(LED_TX, 1);
		HAL_Delay(delay);
        HAL_GPIO_WritePin(LED_TX, 0);
	}
}

void led_rx_on(void)
{
    // Make sure the LED has been off for at least LED_DURATION before turning on again
    // This prevents a solid status LED on a busy canbus
    if (led_rx_laston == 0 && HAL_GetTick() - led_rx_lastoff > LED_DURATION)
    {
        // Invert LED
        HAL_GPIO_WritePin(LED_RX, 1);
        led_rx_laston = HAL_GetTick();
    }
}

// Attempt to turn on status LED
void led_tx_on(void)
{
    // Make sure the LED has been off for at least LED_DURATION before turning on again
    // This prevents a solid status LED on a busy canbus
    if (led_tx_laston == 0 && HAL_GetTick() - led_tx_lastoff > LED_DURATION)
    {
        HAL_GPIO_WritePin(LED_TX, 1);
        led_tx_laston = HAL_GetTick();
    }
}

// Process time-based LED events
void led_process(void)
{
    // If LED has been on for long enough, turn it off
    if (led_tx_laston > 0 && HAL_GetTick() - led_tx_laston > LED_DURATION)
    {
        HAL_GPIO_WritePin(LED_TX, 0);
        led_tx_laston = 0;
        led_tx_lastoff = HAL_GetTick();
    }

    // If LED has been on for long enough, turn it off
    if (led_rx_laston > 0 && HAL_GetTick() - led_rx_laston > LED_DURATION)
    {
        HAL_GPIO_WritePin(LED_RX, 0);
        led_rx_laston = 0;
        led_rx_lastoff = HAL_GetTick();
    }
}
