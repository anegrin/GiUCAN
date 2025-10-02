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
#ifdef SLCAN
#include "usbd_cdc_if.h"
#endif
#include "storage.h"
#include "logging.h"

DMA_HandleTypeDef hdma_usart2_tx;
DMA_HandleTypeDef hdma_usart2_rx;
DMA_HandleTypeDef hdma_memtomem_dma1_channel1;

void state_init(GlobalState *state, Settings *settings);
void SystemClock_Config(void);
static void MX_DMA_Init(void);

int main(void)
{
    CAN_RxHeaderTypeDef rx_msg_header; // msg header
    uint8_t rx_msg_data[8] = {
        0,
    }; // msg data
    uint8_t msg_buf[SLCAN_MTU];

    HAL_Init();
    SystemClock_Config();
    led_init();
    MX_DMA_Init();
    uart_init();

    Settings settings = {};
    storage_init();
    load_settings(&settings);

    MX_USB_DEVICE_Init();

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

    GlobalState state = {};
    state_init(&state, &settings);

    while (1)
    {
        state.board.now = HAL_GetTick();

#ifdef SLCAN
        cdc_process();
#endif
        led_process();
        can_process();

#ifdef XCAN
        state_process(&state, &settings);
#endif
        uart_process(&state);
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

void state_init(GlobalState *state, Settings *settings)
{
    uint32_t now = HAL_GetTick();
    state->board.collectingMultiframeResponse = -1;
    state->board.dashboardExternallyUpdatedAt = 0;
    state->board.dashboardState.itemsCount = DASHBOARD_ITEMS_COUNT;
    state->board.dashboardState.currentItemIndex = 0;
    state->board.dashboardState.values[0] = -1.0f;
    state->board.dashboardState.values[1] = -1.0f;
    state->board.dashboardState.carouselShowNextItemAt = settings->bootCarouselEnabled ? now + settings->bootCarouselDelay : 0;
    state->board.dpfRegenNotificationRequestAt = 0;
    state->board.latestMessageReceivedAt = 0;
    state->board.snsRequestOffAt = 0;

    state->car.battery.chargePercent = 0;
    state->car.battery.current = 0.0f;
    state->car.ccActive = false;
    state->car.dpf.regenerating = false;
    state->car.dpf.regenMode = 0;
    state->car.gear = '-';
    state->car.oil.pressure = 0.0f;
    state->car.oil.temperature = 0;
    state->car.rpm = 0;
    state->car.sns.active = true;
    state->car.sns.snsOffAt = 0;
    state->car.torque = 0;
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
#ifdef ENABLE_EXTERNAL_OSCILLATOR
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL6;
    RCC_OscInitStruct.PLL.PREDIV = RCC_PREDIV_DIV1;
#else
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI48;
    RCC_OscInitStruct.HSI48State = RCC_HSI48_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
#endif
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }

    /** Initializes the CPU, AHB and APB buses clocks
     */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1;
#ifdef ENABLE_EXTERNAL_OSCILLATOR
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
#else
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI48;
#endif
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
    {
        Error_Handler();
    }
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USB | RCC_PERIPHCLK_USART2;
#ifdef ENABLE_EXTERNAL_OSCILLATOR
    PeriphClkInit.Usart2ClockSelection = RCC_USART2CLKSOURCE_PCLK1;
    PeriphClkInit.UsbClockSelection = RCC_USBCLKSOURCE_PLL;
#else
    PeriphClkInit.Usart2ClockSelection = RCC_USART2CLKSOURCE_SYSCLK;
    PeriphClkInit.UsbClockSelection = RCC_USBCLKSOURCE_HSI48;
#endif

    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
    {
        Error_Handler();
    }
}

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
