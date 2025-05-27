#include "config.h"

#include <string.h>
#include "crc.h"
#include "logging.h"
#include "uart.h"
#include "led.h"
#include "error.h"

#define QUEUE_SIZE 16

const uint8_t MESSAGE_TYPE_DASHBORAD_STATE = 0xFE; // rare first byte for floats, Large negative NaNs
const uint8_t MESSAGE_TYPE_SNS_STATE = 0xF0;       // rare first byte for floats, High magnitude negatives

const int queuePollingInterval = 1000 / (USART2_BAUD_RATE / (10 * MESSAGE_SIZE));
uint32_t queuePolledAt = 0;

bool tx_done = true;

UART_HandleTypeDef huart2;
#ifdef BHCAN
uint8_t rx_buffer[MESSAGE_SIZE];
#endif

typedef struct
{
    uint8_t buffer[QUEUE_SIZE][MESSAGE_SIZE];
    uint8_t head;
    uint8_t tail;
    uint8_t count;
} Queue;

Queue queue = {0};
Queue *rx_tx_queue = &queue;

void fill_buffer(uint8_t *buffer, uint8_t bufferLength, const uint8_t *data, uint8_t dataLength)
{
    if (dataLength > bufferLength)
        dataLength = bufferLength;
    memcpy(buffer, data, dataLength);
    if (bufferLength > dataLength + 1)
    {
        memset(&buffer[dataLength + 1], 0x00, bufferLength - dataLength - 1);
    }
}

bool uart_enqueue(const uint8_t *data, uint8_t size)
{
    if (rx_tx_queue->count < QUEUE_SIZE)
    {
        fill_buffer(rx_tx_queue->buffer[rx_tx_queue->tail], MESSAGE_SIZE, data, size);
        rx_tx_queue->tail = (rx_tx_queue->tail + 1) % QUEUE_SIZE;
        rx_tx_queue->count++;
        return true;
    }

    return false;
}

bool send_state(GlobalState *state)
{
    float v0 = state->board.dashboardState.values[0];
    float v1 = state->board.dashboardState.values[1];
    uint8_t buffer[MESSAGE_SIZE];
    uint8_t type = MESSAGE_TYPE_DASHBORAD_STATE;
    buffer[0] = type;
    bool visible = state->board.dashboardState.visible;
    buffer[1] = visible;
    uint8_t currentItemIndex = state->board.dashboardState.currentItemIndex;
    buffer[2] = currentItemIndex;
    memcpy(&buffer[3], &v0, 4);
    memcpy(&buffer[7], &v1, 4);
    bool regenerating = state->car.dpf.regenerating;
    buffer[11] = regenerating;
    uint8_t crc = calculate_crc8(buffer, MESSAGE_SIZE - 1);
    buffer[MESSAGE_SIZE - 1] = crc;
#ifdef UART_DEBUG_MODE
    uint8_t sb[MESSAGE_SIZE];
    bool regenerating = state->car.dpf.regenerating;
    int s = snprintf((char *)sb, sizeof(sb), "t:%02X v:%d ci:%d v0:%.2f v1:%.2f r:%d c:%d\n", type, visible, currentItemIndex, v0, v1, , crc);
    return uart_enqueue(sb, s);
#else
    return uart_enqueue(buffer, MESSAGE_SIZE);
#endif
}

void update_state(GlobalState *state, bool visible, uint8_t currentItemIndex, float v0, float v1, bool regenerating)
{
    VLOG("v:%d ci:%d", visible, currentItemIndex);
    VLOG(" v0:%.2f v1:%.2f", v0, v1);
    VLOG(" r:%d\n", regenerating);
    state->board.dashboardState.visible = visible;
    state->board.dashboardState.currentItemIndex = currentItemIndex;
    state->board.dashboardState.values[0] = v0;
    state->board.dashboardState.values[1] = v1;
    state->car.dpf.regenerating = regenerating;
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    tx_done = true;
}

#ifdef C1CAN
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2)
    {
        tx_done = true;
        led_tx_on();
    }
}
#endif

#ifdef BHCAN
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    uart_enqueue(rx_buffer, MESSAGE_SIZE);
    led_rx_on();
    HAL_UART_Receive_IT(&huart2, rx_buffer, MESSAGE_SIZE);
}
#endif

void uart_init(void)
{
    huart2.Instance = USART2;
    huart2.Init.BaudRate = USART2_BAUD_RATE;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;

    if (HAL_HalfDuplex_Init(&huart2) != HAL_OK)
    {
        Error_Handler();
    }
#ifdef C1CAN
    HAL_HalfDuplex_EnableTransmitter(&huart2);
#endif
#ifdef BHCAN
    HAL_HalfDuplex_EnableReceiver(&huart2);
    HAL_UART_Receive_IT(&huart2, rx_buffer, MESSAGE_SIZE);
#endif
}

#ifdef C1CAN
bool uart_tx(const uint8_t *data, uint8_t size)
{
    if (tx_done)
    {
        tx_done = false;
        HAL_StatusTypeDef status = HAL_UART_Transmit_DMA(&huart2, (const uint8_t *)data, size);
        return status == HAL_OK;
    }

    return false;
}
#endif

#ifdef BHCAN
bool dashboard_tx(GlobalState *state, const uint8_t *data, uint8_t size)
{
    float v0 = 0;
    float v1 = 0;
    uint8_t type = data[0];
    bool visible = data[1];
    uint8_t currentItemIndex = data[2];
    memcpy(&v0, &data[3], 4);
    memcpy(&v1, &data[7], 4);
    bool regenerating = data[12];
    uint8_t crc_check = calculate_crc8(data, MESSAGE_SIZE - 1);
    uint8_t crc = data[MESSAGE_SIZE - 1];

    if (type == MESSAGE_TYPE_DASHBORAD_STATE && crc_check == crc)
    {
        update_state(state, visible, currentItemIndex, v0, v1, regenerating);
        return true;
    }

    return false;
}
#endif

void uart_process(GlobalState *state)
{
    if (rx_tx_queue->count == 0)
    {
        return;
    }

    if (state->board.now - queuePolledAt > queuePollingInterval)
    {
#ifdef C1CAN
        if (uart_tx(rx_tx_queue->buffer[rx_tx_queue->head], MESSAGE_SIZE))
#endif
#ifdef BHCAN
            if (dashboard_tx(state, rx_tx_queue->buffer[rx_tx_queue->head], MESSAGE_SIZE))
#endif
            {
                rx_tx_queue->head = (rx_tx_queue->head + 1) % QUEUE_SIZE;
                rx_tx_queue->count--;
                queuePolledAt = state->board.now;
            }
    }
}
