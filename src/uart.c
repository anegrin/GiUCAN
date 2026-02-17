#include "config.h"

#include <string.h>
#include "crc.h"
#include "logging.h"
#include "uart.h"
#include "led.h"
#include "error.h"
#include "printf.h"

const uint8_t MSG_START = 0xFE; // rare first byte for floats, Large negative NaNs

static bool tx_done = true;

UART_HandleTypeDef huart2;
#ifdef BHCAN
typedef enum
{
    UART_SYNC_WAIT_START,
    UART_SYNC_RECEIVING
} UART_RxState;
static UART_RxState rx_state = UART_SYNC_WAIT_START;
static uint8_t rx_sync_byte;
static uint8_t rx_buffer[MESSAGE_SIZE];
static size_t rx_index = 0;
#endif

typedef struct
{
    uint8_t head;
    uint8_t tail;
    uint8_t count;
    uint8_t _pad;
    uint8_t buffer[UART_QUEUE_SIZE][MESSAGE_SIZE];
} UARTQueue;

static UARTQueue queue = {0};
static UARTQueue *rx_tx_queue = &queue;

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

bool uart_enqueue(const uint8_t *data, uint8_t size, bool mandatory)
{
    if (rx_tx_queue->count < UART_QUEUE_SIZE)
    {
        fill_buffer(rx_tx_queue->buffer[rx_tx_queue->tail], MESSAGE_SIZE, data, size);
        rx_tx_queue->tail = (rx_tx_queue->tail + 1) % UART_QUEUE_SIZE;
        rx_tx_queue->count++;
        return true;
    } else if (mandatory) {
        fill_buffer(rx_tx_queue->buffer[UART_QUEUE_SIZE - 1], MESSAGE_SIZE, data, size);
        return true;
    }

    return false;
}

typedef struct {
    bool visible : 1;
    bool goingToBed : 1;
    bool regenerating : 1;
    bool dpfNotify : 1;
    uint8_t _padding : 4;
} UartMessageFlags;

#ifdef C1CAN
uint8_t serialize_flags(UartMessageFlags flags) {
    uint8_t out = 0;
    out |= (flags.visible & 0x01) << 0;
    out |= (flags.goingToBed & 0x01) << 1;
    out |= (flags.regenerating & 0x01) << 2;
    out |= (flags.dpfNotify & 0x01) << 3;
    // Padding is ignored during serialization
    return out;
}
#endif

#ifdef BHCAN
UartMessageFlags deserialize_flags(uint8_t byte) {
    UartMessageFlags flags;
    flags.visible      = (byte >> 0) & 0x01;
    flags.goingToBed   = (byte >> 1) & 0x01;
    flags.regenerating = (byte >> 2) & 0x01;
    flags.dpfNotify    = (byte >> 3) & 0x01;
    flags._padding     = 0; // Clean the padding
    return flags;
}
#endif

#ifdef C1CAN
bool send_state(GlobalState *state)
{
    float v0 = state->board.dashboardState.values[0];
    float v1 = state->board.dashboardState.values[1];
    uint8_t buffer[MESSAGE_SIZE];
    uint8_t type = MSG_START;
    buffer[0] = type;
    UartMessageFlags flags = {
        .visible = state->board.dashboardState.visible,
        .goingToBed = state->board.goingToBedAt != 0,
        .regenerating = state->car.dpf.regenerating,
        .dpfNotify = state->board.dpfRegenSoundNotificationRequestAt != 0,
        ._padding = 0,
    };
    buffer[1] = serialize_flags(flags);
    uint8_t currentItemIndex = state->board.dashboardState.currentItemIndex;
    buffer[2] = currentItemIndex;
    memcpy(&buffer[3], &v0, 4);
    memcpy(&buffer[7], &v1, 4);
    uint8_t crc = calculate_crc8(buffer, MESSAGE_SIZE - 1);
    buffer[MESSAGE_SIZE - 1] = crc;
    bool mandatory = !flags.visible || flags.dpfNotify || flags.goingToBed;
    return uart_enqueue(buffer, MESSAGE_SIZE, mandatory);
}
#endif

#ifdef BHCAN
void update_state(GlobalState *state, UartMessageFlags flags, uint8_t currentItemIndex, float v0, float v1)
{
    if (flags.goingToBed)
    {
        state->board.goingToBedAt = 1;
        return;
    }

    state->board.dashboardState.visible = flags.visible;
    state->board.dashboardState.currentItemIndex = currentItemIndex;
    state->board.dashboardState.values[0] = v0;
    state->board.dashboardState.values[1] = v1;
    state->car.dpf.regenerating = flags.regenerating;
    if (flags.dpfNotify) {
        state->board.dpfRegenSoundNotificationRequestAt = state->board.now;
    }
}
#endif

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    tx_done = true;
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2)
    {
        tx_done = true;
        led_tx_on();
    }
}

#ifdef BHCAN
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2)
    {
        if (rx_state == UART_SYNC_WAIT_START)
        {
            if (rx_sync_byte == MSG_START)
            {
                rx_buffer[0] = MSG_START;
                rx_index = 1;
                rx_state = UART_SYNC_RECEIVING;
                HAL_UART_Receive_DMA(&huart2, &rx_buffer[rx_index], MESSAGE_SIZE - 1);
            }
            else
            {
                // Still waiting for start byte
                HAL_UART_Receive_DMA(&huart2, &rx_sync_byte, 1);
            }
        }
        else if (rx_state == UART_SYNC_RECEIVING)
        {
            rx_state = UART_SYNC_WAIT_START;
            if (uart_enqueue(rx_buffer, MESSAGE_SIZE, false))
            {
                led_rx_on();
            }
            // Start looking for next message
            HAL_UART_Receive_DMA(&huart2, &rx_sync_byte, 1);
        }
    }
}
#endif

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

#ifdef BHCAN
bool handle_msg(GlobalState *state, const uint8_t *data, uint8_t size)
{
    float v0 = 0;
    float v1 = 0;
    uint8_t type = data[0];
    UartMessageFlags flags = deserialize_flags(data[1]);
    uint8_t currentItemIndex = data[2];
    memcpy(&v0, &data[3], 4);
    memcpy(&v1, &data[7], 4);
    uint8_t crc_check = calculate_crc8(data, MESSAGE_SIZE - 1);
    uint8_t crc = data[MESSAGE_SIZE - 1];

    if (type == MSG_START && crc_check == crc)
    {
        update_state(state, flags, currentItemIndex, v0, v1);
        return true;
    }

    return false;
}
#endif

const int UART_QUEUE_POLLING_INTERVAL = 5 * (1000 / (USART2_BAUD_RATE / (10 * MESSAGE_SIZE)));
static uint32_t queuePolledAt = 0;
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
    HAL_HalfDuplex_EnableTransmitter(&huart2);
#ifdef BHCAN
    HAL_HalfDuplex_EnableReceiver(&huart2);
    rx_state = UART_SYNC_WAIT_START;
    HAL_UART_Receive_DMA(&huart2, &rx_sync_byte, 1);
#endif
}

void uart_deinit(void)
{
    HAL_UART_MspDeInit(&huart2);
}

void uart_process(GlobalState *state)
{
    if (rx_tx_queue->count == 0)
    {
        return;
    }

    if (state->board.now - queuePolledAt > UART_QUEUE_POLLING_INTERVAL)
    {
#ifndef BHCAN
        bool success = uart_tx(rx_tx_queue->buffer[rx_tx_queue->head], MESSAGE_SIZE);
#endif
#ifdef BHCAN
        bool success = handle_msg(state, rx_tx_queue->buffer[rx_tx_queue->head], MESSAGE_SIZE);
#endif
        if (success)
        {
            rx_tx_queue->head = (rx_tx_queue->head + 1) % UART_QUEUE_SIZE;
            rx_tx_queue->count--;
            queuePolledAt = state->board.now;
        }
    }
}

#ifdef SLCAN
#ifdef DEBUG_MODE
uint8_t print_to_uart(char *message)
{
    static uint8_t buffer[MESSAGE_SIZE];
    uint16_t msg_len = strlen(message);
    msg_len = msg_len < MESSAGE_SIZE ? msg_len : MESSAGE_SIZE;
    memcpy(buffer, message, msg_len);
    return uart_enqueue(buffer, msg_len, false);
}

uint8_t printf_to_uart(const char *format, ...)
{
    static uint8_t buffer[MESSAGE_SIZE];
    va_list args;
    va_start(args, format);
    int written = vsnprintf_((char *)buffer, MESSAGE_SIZE, format, args);
    va_end(args);
    return uart_enqueue(buffer, written < MESSAGE_SIZE ? (uint8_t)written : MESSAGE_SIZE, false);
}
#endif
#endif
