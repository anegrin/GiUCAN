#include "main.h"
#include "can.h"
#include "led.h"
#include "logging.h"
#include "model.h"
#include "slcan.h"
#include "processing.h"
#include "usb_device.h"
#include "usbd_cdc_if.h"

CRC_HandleTypeDef hcrc;
DMA_HandleTypeDef hdma_memtomem_dma1_channel1;

void SystemClock_Config(void);
static void MX_DMA_Init(void);
static void MX_CRC_Init(void);
void state_init(GlobalState* state);

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
    MX_DMA_Init();
    MX_CRC_Init();
#ifdef ENABLE_USB_PORT
    MX_USB_DEVICE_Init();
#endif

#ifdef ENABLE_CAN_AT_BOOT
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
        //just for the warning
        UNUSED(state);

        #ifdef SLCAN
        cdc_process();
#endif
        led_process();
        can_process();

#if defined(C1CAN) || defined(BHCAN)
        process_state(&state);
#endif
        if (is_can_msg_pending(CAN_RX_FIFO0) > 0)
        {
            if (can_rx(&rx_msg_header, rx_msg_data) == HAL_OK)
            {
                led_rx_on();
#ifdef ECHO_MODE
                CAN_TxHeaderTypeDef echoMsgHeader;
                echoMsgHeader.IDE = rx_msg_header.IDE;
                echoMsgHeader.RTR = rx_msg_header.RTR;
                echoMsgHeader.StdId = rx_msg_header.StdId;
                echoMsgHeader.DLC = rx_msg_header.DLC;
                echoMsgHeader.ExtId = rx_msg_header.ExtId;
                can_tx(&echoMsgHeader, rx_msg_data);
#endif
                uint16_t msg_len = slcan_parse_frame((uint8_t *)&msg_buf, &rx_msg_header, rx_msg_data);
                if (msg_len)
                {
#ifdef SLCAN
                    CDC_Transmit_FS(msg_buf, msg_len);
#endif
#if defined(C1CAN) || defined(BHCAN)
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

void state_init(GlobalState* state) {
    state->car.sns.active = 1;
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
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USB;
    PeriphClkInit.UsbClockSelection = RCC_USBCLKSOURCE_PLL;

    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
    {
        Error_Handler();
    }
}

/**
 * @brief CRC Initialization Function
 * @param None
 * @retval None
 */
static void MX_CRC_Init(void)
{
    hcrc.Instance = CRC;
    hcrc.Init.DefaultPolynomialUse = DEFAULT_POLYNOMIAL_ENABLE;
    hcrc.Init.DefaultInitValueUse = DEFAULT_INIT_VALUE_ENABLE;
    hcrc.Init.InputDataInversionMode = CRC_INPUTDATA_INVERSION_NONE;
    hcrc.Init.OutputDataInversionMode = CRC_OUTPUTDATA_INVERSION_DISABLE;
    hcrc.InputDataFormat = CRC_INPUTDATA_FORMAT_BYTES;
    if (HAL_CRC_Init(&hcrc) != HAL_OK)
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

    /* Configure DMA request hdma_memtomem_dma1_channel1 on DMA1_Channel1 */
    hdma_memtomem_dma1_channel1.Instance = DMA1_Channel1;
    hdma_memtomem_dma1_channel1.Init.Direction = DMA_MEMORY_TO_MEMORY;
    hdma_memtomem_dma1_channel1.Init.PeriphInc = DMA_PINC_ENABLE;
    hdma_memtomem_dma1_channel1.Init.MemInc = DMA_MINC_ENABLE;
    hdma_memtomem_dma1_channel1.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_memtomem_dma1_channel1.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_memtomem_dma1_channel1.Init.Mode = DMA_NORMAL;
    hdma_memtomem_dma1_channel1.Init.Priority = DMA_PRIORITY_LOW;
    if (HAL_DMA_Init(&hdma_memtomem_dma1_channel1) != HAL_OK)
    {
        Error_Handler();
    }
}

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void)
{
    /* USER CODE BEGIN Error_Handler_Debug */
    /* User can add his own implementation to report the HAL error return state */
    __disable_irq();
    while (1)
    {
    }
    /* USER CODE END Error_Handler_Debug */
}
