//
// can: initializes and provides methods to interact with the CAN peripheral
//

#include "stm32f0xx_hal.h"
#include "slcan.h"
#include "usbd_cdc_if.h"
#include "can.h"
#include "led.h"
#include "error.h"

// Private variables
static CAN_HandleTypeDef can_handle;
static CAN_FilterTypeDef filter;
static uint32_t prescaler = 12; // 500k
static can_bus_state_t bus_state = OFF_BUS;
static uint8_t can_autoretransmit = ENABLE;
static can_txbuf_t txqueue = {0};

void MX_CAN_Init()
{

    can_handle.Instance = CAN;
    can_handle.Init.Prescaler = prescaler;
    can_handle.Init.Mode = CAN_MODE_NORMAL;
    can_handle.Init.SyncJumpWidth = CAN_SJW_1TQ;
    can_handle.Init.TimeSeg1 = CAN_BS1_4TQ;
    can_handle.Init.TimeSeg2 = CAN_BS2_3TQ;
    can_handle.Init.TimeTriggeredMode = DISABLE;
    can_handle.Init.AutoBusOff = ENABLE;
    can_handle.Init.AutoWakeUp = DISABLE;
    can_handle.Init.AutoRetransmission = ENABLE;
    can_handle.Init.ReceiveFifoLocked = DISABLE;
    can_handle.Init.TransmitFifoPriority = ENABLE;
    if (HAL_CAN_Init(&can_handle) != HAL_OK)
    {
        Error_Handler();
    }
}

// Start the CAN peripheral
void can_enable(void)
{
    if (bus_state == OFF_BUS)
    {
        MX_CAN_Init();
        filter.FilterIdHigh = 0;
        filter.FilterIdLow = 0;
        filter.FilterMaskIdHigh = 0;
        filter.FilterMaskIdLow = 0;
        filter.FilterFIFOAssignment = CAN_RX_FIFO0;
        filter.FilterBank = 0;
        filter.FilterMode = CAN_FILTERMODE_IDMASK;
        filter.FilterScale = CAN_FILTERSCALE_32BIT;
        filter.FilterActivation = ENABLE;
        HAL_CAN_ConfigFilter(&can_handle, &filter);
        HAL_CAN_Start(&can_handle);
        bus_state = ON_BUS;

        led_tx_on();
        led_rx_on();
    }
}

// Disable the CAN peripheral and go off-bus
void can_disable(void)
{
    if (bus_state == ON_BUS)
    {
        // Do a bxCAN reset (set RESET bit to 1)
        can_handle.Instance->MCR |= CAN_MCR_RESET;
        bus_state = OFF_BUS;

        led_tx_on();
        led_rx_on();
    }
}

// Set the bitrate of the CAN peripheral
void can_set_bitrate(enum can_bitrate bitrate)
{
    if (bus_state == ON_BUS)
    {
        // cannot set bitrate while on bus
        return;
    }

    switch (bitrate)
    {
    case CAN_BITRATE_10K:
        prescaler = 600;
        break;
    case CAN_BITRATE_20K:
        prescaler = 300;
        break;
    case CAN_BITRATE_50K:
        prescaler = 120;
        break;
    case CAN_BITRATE_100K:
        prescaler = 60;
        break;
    case CAN_BITRATE_125K:
        prescaler = 48;
        break;
    case CAN_BITRATE_250K:
        prescaler = 24;
        break;
    case CAN_BITRATE_500K:
        prescaler = 12;
        break;
    case CAN_BITRATE_750K:
        prescaler = 8;
        break;
    case CAN_BITRATE_1000K:
        prescaler = 6;
        break;
    case CAN_BITRATE_INVALID:
    default:
        prescaler = 6;
        break;
    }

    led_tx_on();
}

// Set CAN peripheral to silent mode
void can_set_silent(uint8_t silent)
{
    if (bus_state == ON_BUS)
    {
        // cannot set silent mode while on bus
        return;
    }
    if (silent)
    {
        can_handle.Init.Mode = CAN_MODE_SILENT;
    }
    else
    {
        can_handle.Init.Mode = CAN_MODE_NORMAL;
    }

    led_tx_on();
}

// Enable/disable auto-retransmission
void can_set_autoretransmit(uint8_t autoretransmit)
{
    if (bus_state == ON_BUS)
    {
        // Cannot set autoretransmission while on bus
        return;
    }
    if (autoretransmit)
    {
        can_autoretransmit = ENABLE;
    }
    else
    {
        can_autoretransmit = DISABLE;
    }

    led_tx_on();
}

// Send a message on the CAN bus
uint32_t can_tx(CAN_TxHeaderTypeDef *tx_msg_header, uint8_t *tx_msg_data)
{
    // Check if space available in the buffer (FIXME: wastes 1 item)
    if (((txqueue.head + 1) % TXQUEUE_LEN) == txqueue.tail)
    {
        error_assert(ERR_FULLBUF_CANTX);
        return HAL_ERROR;
    }

    // Copy header struct into array
    txqueue.header[txqueue.head] = *tx_msg_header;

    // Copy data into array
    for (uint8_t i = 0; i < tx_msg_header->DLC; i++)
    {
        txqueue.data[txqueue.head][i] = tx_msg_data[i];
    }

    // Increment the head pointer
    txqueue.head = (txqueue.head + 1) % TXQUEUE_LEN;

#ifdef LEDS_ON_CAN_TX
    led_tx_on();
#endif

    return HAL_OK;
}

// Process messages in the TX output queue
void can_process(void)
{
    if ((txqueue.tail != txqueue.head) && (HAL_CAN_GetTxMailboxesFreeLevel(&can_handle) > 0))
    {
        // Transmit can frame
        uint32_t mailbox_txed = 0;
        uint32_t status = HAL_CAN_AddTxMessage(&can_handle, &txqueue.header[txqueue.tail], txqueue.data[txqueue.tail], &mailbox_txed);
        txqueue.tail = (txqueue.tail + 1) % TXQUEUE_LEN;

        // This drops the packet if it fails (no retry). Failure is unlikely
        // since we check if there is a TX mailbox free.
        if (status != HAL_OK)
        {
            error_assert(ERR_CAN_TXFAIL);
        }
    }
}

// Receive message from the CAN bus RXFIFO
uint32_t can_rx(CAN_RxHeaderTypeDef *rx_msg_header, uint8_t *rx_msg_data)
{
    uint32_t status = HAL_CAN_GetRxMessage(&can_handle, CAN_RX_FIFO0, rx_msg_header, rx_msg_data);
#ifdef LEDS_ON_CAN_RX
    led_rx_on();
#endif
    return status;
}

// Check if a CAN message has been received and is waiting in the FIFO
uint8_t is_can_msg_pending(uint8_t fifo)
{
    if (bus_state == OFF_BUS)
    {
        return 0;
    }
    return (HAL_CAN_GetRxFifoFillLevel(&can_handle, CAN_RX_FIFO0) > 0);
}

// Return reference to CAN handle
CAN_HandleTypeDef *can_gethandle(void)
{
    return &can_handle;
}

// Callback for FIFO0 full
void HAL_CAN_RxFifo0FullCallback(CAN_HandleTypeDef *hcan)
{
    error_assert(ERR_CANRXFIFO_OVERFLOW);
}
