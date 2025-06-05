#include "config.h"
#ifdef BHCAN
#include <stdbool.h>
#include "printf.h"
#include "processing.h"
#include "led.h"
#include "logging.h"
#include "dashboard.h"
#include "can.h"

#define DASHBOARD_BUFFER_SIZE DASHBOARD_MESSAGE_MAX_LENGTH + 1

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

    VLOG("%c%c%c\n", tx_msg_data[3], tx_msg_data[5], tx_msg_data[7]);

    CAN_TxHeaderTypeDef tx_msg_header = {.IDE = CAN_ID_STD, .RTR = CAN_RTR_DATA, .StdId = 0x090, .DLC = 8};

    if (can_tx(&tx_msg_header, tx_msg_data) == HAL_OK)
    {
        led_tx_on();
    }
}

const char *dpf_state_as_string(uint8_t state)
{
    switch (state)
    {
    case 1:
        return "DPF LO";
    case 2:
        return "DPF HI";
    case 3:
        return "NSC De-NOx";
    case 4:
        return "NSC NSC De-SOx";
    case 5:
        return "SCR HeatUp";
    default:
        return "NONE";
    }
}

void render_message(char *buffer, GlobalState *state)
{
    DashboardItemType type = type_of(state->board.dashboardState.currentItemIndex);
    const char *pattern = pattern_of(type);

    int written = -1;
    if (type == GEAR_ITEM)
    {
        written = snprintf_(buffer, DASHBOARD_BUFFER_SIZE, pattern, (unsigned char)state->board.dashboardState.values[0]);
    }
    else if (type == DPF_STATUS_ITEM)
    {
        written = snprintf_(buffer, DASHBOARD_BUFFER_SIZE, pattern, dpf_state_as_string((uint8_t)state->board.dashboardState.values[0]));
    }
    else
    {
        written = snprintf_(buffer, DASHBOARD_BUFFER_SIZE, pattern, state->board.dashboardState.values[0], state->board.dashboardState.values[1]);
    }

    if (written >= 0 && written < DASHBOARD_MESSAGE_MAX_LENGTH)
    {
        memset(buffer + written, ' ', DASHBOARD_MESSAGE_MAX_LENGTH - written);
    }
    buffer[DASHBOARD_MESSAGE_MAX_LENGTH] = 0x00;
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

#ifdef DASHBOARD_FORCED_REFRESH
    if (!updateDashboard > 0 && dashboardLocalState.visible && dashboardRefreshedAt + DASHBOARD_FORCED_REFRESH_MS < state->board.now)
    {
        updateDashboard = true;
    }
#endif

    if (updateDashboard)
    {
        dashboardRefreshedAt = state->board.now;
        if (dashboardLocalState.visible)
        {
            char buffer[DASHBOARD_BUFFER_SIZE];
            render_message(buffer, state);

            int partsCount = DASHBOARD_MESSAGE_MAX_LENGTH / 3;
            int offset = 0;
            int part = 0;
            while (offset < DASHBOARD_MESSAGE_MAX_LENGTH)
            {
                send_dashboard_text(partsCount, part, buffer, offset, DISPLAY_INFO_CODE);
                offset += 3;
                part += 1;
            }
        }
        else
        {
            send_dashboard_text(1, 0, "   ", 0, 0x11); // TODO clear icon too
        }
    }

    localStateSet = true;
}
#endif
