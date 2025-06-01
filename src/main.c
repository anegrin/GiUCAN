#include "config.h"
#include "dashboard.h"
#include "can.h"
#include "uart.h"
#include "error.h"
#include "led.h"
#include "model.h"
#include "slcan.h"
#include "processing.h"
#include "usb_device.h"
#include "usbd_cdc_if.h"

DMA_HandleTypeDef hdma_usart2_tx;
DMA_HandleTypeDef hdma_usart2_rx;
DMA_HandleTypeDef hdma_memtomem_dma1_channel1;

void state_init(GlobalState *state);
void SystemClock_Config(void);
#ifdef XCAN
static void MX_DMA_Init(void);
#endif

int main(void)
{
    GlobalState state = {};
    state_init(&state);

    CAN_RxHeaderTypeDef rx_msg_header; // msg header
    uint8_t rx_msg_data[8] = {
        0,
    }; // msg data
    uint8_t msg_buf[SLCAN_MTU];

    HAL_Init();
    SystemClock_Config();
    led_init();
#ifdef XCAN
    MX_DMA_Init();
    uart_init();
#endif

#ifdef ENABLE_USB_PORT
    MX_USB_DEVICE_Init();
#endif

#ifdef XCAN
    can_set_bitrate(CAN_BITRATE);
    can_enable();
#endif

#ifdef DEBUG_MODE
    leds_blink(5, 50);
#endif
#ifdef SLCAN
    leds_blink(4, 100);
#endif
#ifdef C1CAN
    leds_blink(3, 250);
#endif
#ifdef BHCAN
    leds_blink(2, 500);
#endif

    while (1)
    {
        state.board.now = HAL_GetTick();

#ifdef SLCAN
        cdc_process();
#endif
        led_process();
        can_process();

#ifdef XCAN
        state_process(&state);
        uart_process(&state);
#endif
        if (is_can_msg_pending(CAN_RX_FIFO0) > 0)
        {
            if (can_rx(&rx_msg_header, rx_msg_data) == HAL_OK)
            {
                uint16_t msg_len = slcan_parse_frame((uint8_t *)&msg_buf, &rx_msg_header, rx_msg_data);
                if (msg_len)
                {
#ifdef SLCAN
                    CDC_Transmit_FS(msg_buf, msg_len);
#endif
#ifdef XCAN
                    if (rx_msg_header.RTR == CAN_RTR_DATA)
                    {
                        switch (rx_msg_header.IDE)
                        {
                        case CAN_ID_STD:
                            handle_standard_frame(&state, rx_msg_header, rx_msg_data);
                            break;
                        case CAN_ID_EXT:
                            handle_extended_frame(&state, rx_msg_header, rx_msg_data);
                            break;
                        default:
                        }
                    }
#endif
                }
            }
        }
    }
}

void state_init(GlobalState *state)
{
    state->car.sns.active = 1;
    state->board.snsRequestOffAt = 0;
    state->board.dpfRegenNotificationRequestOffAt = 0;
    state->board.dashboardState.itemsCount = DASHBOARD_ITEM_COUNT;
    state->board.dashboardState.currentItemIndex = 0;
    state->board.dashboardState.values[0] = 12.3f; // TODO
    state->board.dashboardState.values[1] = 45.6f; // TODO
    state->car.power.hp = 0;
    state->car.power.nm = 0;
    state->car.dpf.regenMode = 0;
    state->car.dpf.regenerating = 0;
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

    /** Initializes the RCC Oscillators according to the specified parameters
     * in the RCC_OscInitTypeDef structure.
     */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL6;
    RCC_OscInitStruct.PLL.PREDIV = RCC_PREDIV_DIV1;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }

    /** Initializes the CPU, AHB and APB buses clocks
     */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
    {
        Error_Handler();
    }
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USB | RCC_PERIPHCLK_USART1;
    PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK1;
    PeriphClkInit.UsbClockSelection = RCC_USBCLKSOURCE_PLL;

    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
    {
        Error_Handler();
    }
}

#ifdef XCAN
/**
 * Enable DMA controller clock
 * Configure DMA for memory to memory transfers
 *   hdma_memtomem_dma1_channel1
 */
static void MX_DMA_Init(void)
{

    /* DMA controller clock enable */
    __HAL_RCC_DMA1_CLK_ENABLE();

    /* DMA interrupt init */
    /* DMA1_Channel4_5_6_7_IRQn interrupt configuration */
    HAL_NVIC_SetPriority(DMA1_Channel4_5_6_7_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(DMA1_Channel4_5_6_7_IRQn);
}
#endif

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void)
{
    leds_on_error();
    /* USER CODE BEGIN Error_Handler_Debug */
    /* User can add his own implementation to report the HAL error return state */
    __disable_irq();
    while (1)
    {
    }
    /* USER CODE END Error_Handler_Debug */
}
