#include "config.h"
#ifdef BHCAN
#include <stdbool.h>
#include "processing.h"
#include "led.h"
#include "logging.h"
#include "dashboard.h"
#include "can.h"

static bool localStateSet = false;
static DashboardState dashboardLocalState;
static DPF dpfLocalState;

void SetDashboardTextCharacters(uint8_t numFrames, uint8_t currentFrame, char *text, uint8_t infoCode)
{
    const uint8_t indexOfLastFrame = numFrames - 1;
    const uint8_t utfCharStartIndex = 2; // First UTF character is in canData[2]

    uint8_t canData[8] = {0};

    // Num frames - 1, byte[0] bit[7..3]
    canData[0] = (indexOfLastFrame << 3) & 0b11111000;

    // InfoCode, byte[1] bit[5..0]
    canData[1] = infoCode & 0b00111111;

    // Current frame, byte[0] bit[2..0] and byte[1] bit[7..6]
    canData[0] |= (currentFrame >> 2) & 0b00000111;
    canData[1] |= (currentFrame << 6) & 0b11000000;

    // 3 UTF characters, byte[2..3], byte[4..5], byte[6..7]
    for (int i = 0; i < 3; i++)
    {
        // UTF uses two bytes per chatacter. But, we only have simple text, so the first byte is always 0
        const uint8_t canDataIndex = i * 2;
        canData[utfCharStartIndex + canDataIndex] = 0;
        canData[utfCharStartIndex + canDataIndex + 1] = text[i];
    }

    CAN_TxHeaderTypeDef tx_msg_header = {.IDE = CAN_ID_STD, .RTR = CAN_RTR_DATA, .StdId = 0x090, .DLC = 8};

    if (can_tx(&tx_msg_header, canData) == HAL_OK)
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
            state->board.dpfRegenNotificationRequestOffAt = state->board.now;
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

    if (updateDashboard)
    {
        if (dashboardLocalState.visible)
        {
            char *pattern = pattern_of(state->board.dashboardState.currentItemIndex);
            LOG("0x%02X - ", DISPLAY_INFO_CODE);
            LOG(pattern, dashboardLocalState.values[0], dashboardLocalState.values[1]);
            LOGS("\n");
            // send can message to display
            char text[3];
            sprintf(text, "%02X", dashboardLocalState.currentItemIndex);
            SetDashboardTextCharacters(1, 0, text, DISPLAY_INFO_CODE);
        }
        else
        {
            // send can message to hide
            LOG("0x%02X\n", CLEAR_DISPLAY_INFO_CODE);
            SetDashboardTextCharacters(1, 0, "   ", CLEAR_DISPLAY_INFO_CODE);
        }
    }

    localStateSet = true;
}
#endif
