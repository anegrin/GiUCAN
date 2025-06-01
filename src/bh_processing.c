#include "config.h"
#ifdef BHCAN
#include <stdbool.h>
#include "printf.h"
#include "processing.h"
#include "led.h"
#include "logging.h"
#include "dashboard.h"
#include "can.h"

static uint32_t dashboardRefreshedAt = 0;
static bool localStateSet = false;
static DashboardState dashboardLocalState;
static DPF dpfLocalState;

void send_dashboard_text(uint8_t partsCount, uint8_t part, char *buffer, uint8_t offset, uint8_t infoCode)
{
    uint8_t tx_msg_data[8] = {0};

    // Num frames - 1, byte[0] bit[7..3]
    tx_msg_data[0] = ((partsCount - 1) << 3) & 0b11111000;

    // InfoCode, byte[1] bit[5..0]
    tx_msg_data[1] = infoCode & 0b00111111;

    // Current frame, byte[0] bit[2..0] and byte[1] bit[7..6]
    tx_msg_data[0] |= (part >> 2) & 0b00000111;
    tx_msg_data[1] |= (part << 6) & 0b11000000;

    tx_msg_data[2] = 0;
    tx_msg_data[3] = buffer[offset];
    tx_msg_data[4] = 0;
    tx_msg_data[5] = buffer[offset + 1];
    tx_msg_data[6] = 0;
    tx_msg_data[7] = buffer[offset + 2];

    CAN_TxHeaderTypeDef tx_msg_header = {.IDE = CAN_ID_STD, .RTR = CAN_RTR_DATA, .StdId = 0x090, .DLC = 8};

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
            state->board.dpfRegenNotificationRequestOffAt = state->car.dpf.regenerating ? state->board.now : 0;
        }
    }

    dashboardLocalState.visible = state->board.dashboardState.visible;
    dashboardLocalState.currentItemIndex = state->board.dashboardState.currentItemIndex;
    dashboardLocalState.values[0] = state->board.dashboardState.values[0];
    dashboardLocalState.values[1] = state->board.dashboardState.values[1];
    dpfLocalState.regenerating = state->car.dpf.regenerating;

    if (!updateDashboard && dashboardRefreshedAt + DASHBOARD_FORCED_REFRESH_MS > state->board.now) {
        updateDashboard = true;
    }

    if (updateDashboard)
    {
        dashboardRefreshedAt = state->board.now;
        if (dashboardLocalState.visible)
        {
            const char *pattern = pattern_of(state->board.dashboardState.currentItemIndex);
            char buffer[DASHBOARD_MESSAGE_MAX_LENGTH + 1];
            int written = snprintf_(buffer, DASHBOARD_MESSAGE_MAX_LENGTH + 1, pattern, state->board.dashboardState.values[0], state->board.dashboardState.values[1]);
            if (written >= 0 && written < DASHBOARD_MESSAGE_MAX_LENGTH)
            {
                memset(buffer + written, ' ', DASHBOARD_MESSAGE_MAX_LENGTH - written - 1);
            }
            buffer[DASHBOARD_MESSAGE_MAX_LENGTH - 1] = 0x00;

            int offset = 0;
            int part = 0;

            while (offset < DASHBOARD_MESSAGE_MAX_LENGTH)
            {
                send_dashboard_text(DASHBOARD_MESSAGE_MAX_LENGTH / 3, part, buffer, offset, DISPLAY_INFO_CODE);
                offset += 3;
                part += 1;
            }
        }
        else
        {
            send_dashboard_text(1, 0, "   ", 0, CLEAR_DISPLAY_INFO_CODE);
        }
    }

    localStateSet = true;
}
#endif
