#include "config.h"
#ifdef BHCAN
#include <stdbool.h>
#include "processing.h"
#include "led.h"
#include "logging.h"
#include "dashboard.h"
#include "can.h"

#define SEND_DASHBOARD_FRAME_DELAY 29

static bool localStateSet = false;
static DashboardState dashboardLocalState;
static DPF dpfLocalState;
static uint32_t queuePolledAt = 0;

#define FRAME_QUEUE_POLLING_INTERVAL 29
#define FRAME_QUEUE_SIZE 128

typedef struct
{
    uint8_t partsCount;
    uint8_t part;
    uint8_t byte3;
    uint8_t byte5;
    uint8_t byte7;
    uint8_t infoCode;
} DashboardFrame;

typedef struct
{
    DashboardFrame buffer[FRAME_QUEUE_SIZE];
    uint8_t head;
    uint8_t tail;
    uint8_t count;
} FrameQueue;

static FrameQueue queue = {0};
static FrameQueue *tx_queue = &queue;

static uint8_t previousPart = 0;

bool buffer_dashboard_text(uint8_t partsCount, uint8_t part, char *buffer, uint8_t offset, uint8_t infoCode)
{
    if (part == 0 || previousPart < part)
    {

        if (tx_queue->count < FRAME_QUEUE_SIZE)
        {
            previousPart = part;
            tx_queue->buffer[tx_queue->tail].partsCount = partsCount;
            tx_queue->buffer[tx_queue->tail].part = part;
            tx_queue->buffer[tx_queue->tail].byte3 = buffer[offset];
            tx_queue->buffer[tx_queue->tail].byte5 = buffer[offset + 1];
            tx_queue->buffer[tx_queue->tail].byte7 = buffer[offset + 2];
            tx_queue->buffer[tx_queue->tail].infoCode = infoCode;
            tx_queue->tail = (tx_queue->tail + 1) % FRAME_QUEUE_SIZE;
            tx_queue->count++;
            return true;
        }
    }

    return false;
}

void send_dashboard_text(DashboardFrame *frame)
{
    uint8_t tx_msg_data[8] = {0};

    // Num frames - 1, byte[0] bit[7..3]
    tx_msg_data[0] = ((frame->partsCount - 1) << 3) & 0b11111000;

    // InfoCode, byte[1] bit[5..0]
    tx_msg_data[1] = frame->infoCode & 0b00111111;

    // Current frame, byte[0] bit[2..0] and byte[1] bit[7..6]
    tx_msg_data[0] |= (frame->part >> 2) & 0b00000111;
    tx_msg_data[1] |= (frame->part << 6) & 0b11000000;

    tx_msg_data[2] = 0;
    tx_msg_data[3] = frame->byte3;
    tx_msg_data[4] = 0;
    tx_msg_data[5] = frame->byte5;
    tx_msg_data[6] = 0;
    tx_msg_data[7] = frame->byte7;

    CAN_TxHeaderTypeDef tx_msg_header = {.IDE = CAN_ID_STD, .RTR = CAN_RTR_DATA, .StdId = DASHBOARD_FRAME_STD_ID, .DLC = 8};

    if (can_tx(&tx_msg_header, tx_msg_data) == HAL_OK)
    {
        led_tx_on();
    }
}

void state_process(GlobalState *state)
{
    bool updateDashboard = false;
    if (localStateSet)
    {
        if (state->board.dashboardState.visible != dashboardLocalState.visible)
        {
            updateDashboard = true;
        }

        if (state->board.dashboardState.visible)
        {
            if (state->board.dashboardState.currentItemIndex != dashboardLocalState.currentItemIndex)
            {
                updateDashboard = true;
            }

            if (state->board.dashboardState.values[0] != dashboardLocalState.values[0])
            {
                updateDashboard = true;
            }
            if (state->board.dashboardState.values[1] != dashboardLocalState.values[1])
            {
                updateDashboard = true;
            }
        }

        if (state->car.dpf.regenerating != dpfLocalState.regenerating)
        {
            LOGS(state->car.dpf.regenerating ? "regen started\n" : "regen ended\n");
            state->board.dpfRegenNotificationRequestAt = state->car.dpf.regenerating ? state->board.now : 0;
        }
    }

    dashboardLocalState.visible = state->board.dashboardState.visible;
    dashboardLocalState.currentItemIndex = state->board.dashboardState.currentItemIndex;
    dashboardLocalState.values[0] = state->board.dashboardState.values[0];
    dashboardLocalState.values[1] = state->board.dashboardState.values[1];
    dpfLocalState.regenerating = state->car.dpf.regenerating;

    localStateSet = true;

    if (state->board.dashboardExternallyUpdatedAt + DASHBOARD_FORCED_REFRESH_MS < state->board.now)
    {
        if (tx_queue->count != 0)
        {
            if (state->board.now - queuePolledAt > FRAME_QUEUE_POLLING_INTERVAL)
            {
                DashboardFrame frame = tx_queue->buffer[tx_queue->head];

                send_dashboard_text(&frame);

                tx_queue->head = (tx_queue->head + 1) % FRAME_QUEUE_SIZE;
                tx_queue->count--;
                queuePolledAt = state->board.now;
            }
        }
    } else {
        dashboardLocalState.visible = false;
        updateDashboard = false;
    }

    if (updateDashboard)
    {
        if (dashboardLocalState.visible)
        {
            char buffer[DASHBOARD_BUFFER_SIZE];
            render_message(buffer, state);
            int partsCount = DASHBOARD_MESSAGE_MAX_LENGTH / 3;
            int offset = 0;
            int part = 0;
            while (offset < DASHBOARD_MESSAGE_MAX_LENGTH && buffer_dashboard_text(partsCount, part, buffer, offset, DISPLAY_INFO_CODE))
            {
                offset += 3;
                part += 1;
            }
        }
        else
        {
            // 00 01 00 00 00 00 00 00
            buffer_dashboard_text(1, 0, "\0\0\0", 0, 0x01);
        }
    }
}
#endif
