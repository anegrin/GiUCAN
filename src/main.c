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
#ifdef C1CAN
#include "usbd_storage_if.h"
#endif
#include "storage.h"

DMA_HandleTypeDef hdma_usart2_tx;
DMA_HandleTypeDef hdma_usart2_rx;
DMA_HandleTypeDef hdma_memtomem_dma1_channel1;

void state_init(GlobalState *state, Settings *settings);
void SystemClock_Config(void);
void GPIO_Init(void);
static void MX_DMA_Init(void);
#ifdef C1CAN
static void MX_DMA_DeInit(void);
#endif

static const uint32_t goToBedDelay = 5000;
static uint32_t nextBlinkBeforeSleeping = 0;

static GlobalState state = {};
static Settings settings = {};
static bool sleeping = false;

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    GPIO_Init();
    MX_DMA_Init();
    uart_init();

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

    state_init(&state, &settings);

#ifdef C1CAN
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_13, GPIO_PIN_SET);
#endif

    while (1)
    {
        state.board.now = HAL_GetTick();

#ifdef C1CAN
        if (!STORAGE_Accessed_FS())
        {
            if (state.board.goingToBedAt == 0 && state.board.latestMessageReceivedAt + STANDBY_MS < state.board.now)
            {
                state.board.goingToBedAt = state.board.now + goToBedDelay;
                nextBlinkBeforeSleeping = state.board.now + (goToBedDelay / 4);
                send_state(&state);
            }

            if (!sleeping && state.board.goingToBedAt != 0 && state.board.goingToBedAt < state.board.now)
            {
                sleeping = true;
                //do not de init USB or it will mess up with interrupts, just stop it
                MX_USB_DEVICE_Stop();
                uart_deinit();
                MX_DMA_DeInit();
                HAL_SuspendTick();
                HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);
                HAL_ResumeTick();
            }

            if (!sleeping && nextBlinkBeforeSleeping != 0 && nextBlinkBeforeSleeping < state.board.now) {
                led_tx_on();
                led_rx_on();
                nextBlinkBeforeSleeping = state.board.now + (goToBedDelay / 4);
            }
        }
#endif

#ifdef SLCAN
        cdc_process();
#endif
        led_process();
        can_process();

#ifdef XCAN
        state_process(&state, &settings);
#endif
        uart_process(&state);
    }
}

CAN_RxHeaderTypeDef rx_msg_header; // msg header
uint8_t rx_msg_data[8] = {
    0,
}; // msg data
uint8_t msg_buf[SLCAN_MTU];
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    if (sleeping)
    {
        HAL_NVIC_SystemReset();
    }

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
                    handle_standard_frame(&state, &settings, rx_msg_header, rx_msg_data);
                    break;
                case CAN_ID_EXT:
                    handle_extended_frame(&state, &settings, rx_msg_header, rx_msg_data);
                    break;
                default:
                }
            }
#endif
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
    state->board.goingToBedAt = 0;

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

void GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOF_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
#ifdef C1CAN
    uint16_t GPIO_Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_13;
#else
    uint16_t GPIO_Pin = GPIO_PIN_0 | GPIO_PIN_1;
#endif

    HAL_GPIO_WritePin(GPIOA, GPIO_Pin, GPIO_PIN_RESET);

    GPIO_InitStruct.Pin = GPIO_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
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
    HAL_NVIC_SetPriority(DMA1_Channel4_5_6_7_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(DMA1_Channel4_5_6_7_IRQn);
}

#ifdef C1CAN
static void MX_DMA_DeInit(void)
{

    /* DMA interrupt deinit */
    HAL_NVIC_DisableIRQ(DMA1_Channel4_5_6_7_IRQn);
    /* DMA controller clock enable */
    __HAL_RCC_DMA1_CLK_DISABLE();
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
