//
// can: initializes and provides methods to interact with the CAN peripheral
//

#include "config.h"

#ifdef C1CAN

#include "led.h"
#include "can.h"
#include "uart.h"
#include "processing.h"
#include "logging.h"

void handle_rpm(GlobalState *state, CAN_RxHeaderTypeDef rx_msg_header, uint8_t *rx_msg_data)
{
    if (rx_msg_header.DLC >= 2)
    {
        state->car.rpm = (rx_msg_data[0] * 256 + (rx_msg_data[1] & ~0x3)) / 4;
        VLOG("%d RPM:%d\n", state->board.now, state->car.rpm);
    }
}

#ifdef ENABLE_DASHBOARD
char gears[] = {'N', '1', '2', '3', '4', '5', '6', 'R', '7', '8', '9'};
uint8_t latestCCButtonEvent = 0x10; // no button pressed
uint32_t resPushedAt = 0;

void handle_torque(GlobalState *state, CAN_RxHeaderTypeDef rx_msg_header, uint8_t *rx_msg_data)
{
    if (rx_msg_header.DLC >= 4)
    {
        state->car.torque = (state->car.rpm * ((rx_msg_data[2] & 0b01111111) << 4 | ((rx_msg_data[3] >> 4) & 0b00001111))) - 500;
        VLOG("%d torque:%d\n", state->board.now, state->car.torque);
    }
}

void handle_gear(GlobalState *state, CAN_RxHeaderTypeDef rx_msg_header, uint8_t *rx_msg_data)
{
    uint8_t i = ((uint8_t)(rx_msg_data[0] & ~0xF) >> 4);
    state->car.gear = gears[i];
    VLOG("%d gear %c\n", state->board.now, state->car.gear);
}

void handle_battery(GlobalState *state, CAN_RxHeaderTypeDef rx_msg_header, uint8_t *rx_msg_data)
{
    if (rx_msg_header.DLC >= 6)
    {
        state->car.battery.chargePercent = (rx_msg_data[1] & 0b01111111);
        state->car.battery.current = (0.1f * (rx_msg_data[4] << 4 | ((rx_msg_data[5] >> 4) & 0b00001111))) - 250.0f;
        VLOG("%d Battery p:%d,c:%.1f\n", state->board.now, state->car.battery.chargePercent, state->car.battery.current);
    }
}
void handle_oil(GlobalState *state, CAN_RxHeaderTypeDef rx_msg_header, uint8_t *rx_msg_data)
{
    if (rx_msg_header.DLC >= 4)
    {
        state->car.oil.pressure = 0.1f * ((rx_msg_data[0] & 0b00000001) << 7 | ((rx_msg_data[1] >> 1) & 0b01111111));
        state->car.oil.temperature = ((rx_msg_data[2] & 0b00111111) << 2 | ((rx_msg_data[3] >> 6) & 0b00000011)) - 40;
        VLOG("%d Oil p:%.1f,t:%d\n", state->board.now, state->car.oil.pressure, state->car.oil.temperature);
    }
}

void handle_cc_buttons(GlobalState *state, CAN_RxHeaderTypeDef rx_msg_header, uint8_t *rx_msg_data)
{
    uint8_t ccButtonEvent = rx_msg_data[0];

    if (latestCCButtonEvent != ccButtonEvent)
    {
        bool knownEvent = false;
        bool sendEvent = false;
        uint8_t itemsCount = state->board.dashboardState.itemsCount;
        if (state->board.dashboardState.visible)
        {
            switch (ccButtonEvent)
            {
            case 0x20: // speed hard down
                state->board.dashboardState.currentItemIndex = (state->board.dashboardState.currentItemIndex + DASHBOARD_PAGE_SIZE) % itemsCount;
                LOG("%d speed vv:%d\n", state->board.now, state->board.dashboardState.currentItemIndex);
                knownEvent = true;
                sendEvent = true;
                break;
            case 0x00: // speed hard up
                state->board.dashboardState.currentItemIndex = (state->board.dashboardState.currentItemIndex + itemsCount - DASHBOARD_PAGE_SIZE) % itemsCount;
                LOG("%d speed ^^:%d\n", state->board.now, state->board.dashboardState.currentItemIndex);
                knownEvent = true;
                sendEvent = true;
                break;
            case 0x18: // speed down
                state->board.dashboardState.currentItemIndex = (state->board.dashboardState.currentItemIndex + 1) % itemsCount;
                LOG("%d speed v:%d\n", state->board.now, state->board.dashboardState.currentItemIndex);
                knownEvent = true;
                sendEvent = true;
                break;
            case 0x08: // speed up
                state->board.dashboardState.currentItemIndex = (state->board.dashboardState.currentItemIndex + itemsCount - 1) % itemsCount;
                LOG("%d speed ^:%d\n", state->board.now, state->board.dashboardState.currentItemIndex);
                knownEvent = true;
                sendEvent = true;
                break;
            }
        }
        switch (ccButtonEvent)
        {
        case 0x10: // no button pressed
            if (resPushedAt > 0)
            {
                if (state->board.now - resPushedAt > RES_LONG_PRESS_DURATION_MS)
                {
                    // long click
                    state->board.dashboardState.visible = !state->board.dashboardState.visible;
                    LOG("%d res ^ lc\n", state->board.now);
                    sendEvent = true;
                }
                else
                {
                    // short click
                    LOG("%d res ^ sc\n", state->board.now);
                    sendEvent = state->board.dashboardState.visible;
                }
                resPushedAt = 0;
            }
            knownEvent = true;
            break;
        case 0x90: // RES button was pressed
        case 0x50: // distance selector, used like RES, to manage the menu
            VLOG("%d res v\n", state->board.now);
            resPushedAt = state->board.now;
            knownEvent = true;
            break;
        }
        if (knownEvent)
        {
            latestCCButtonEvent = ccButtonEvent;
        }

        if (sendEvent)
        {
            send_state(state);
        }
    }
}
#endif

#ifdef ENABLE_SNS_AUTO_OFF
CAN_TxHeaderTypeDef disableSNSHeader = {.IDE = CAN_ID_STD, .RTR = CAN_RTR_DATA, .StdId = 0x4B1, .DLC = 8};
uint8_t disableSNSFrame[8] = {0x04, 0x00, 0x00, 0x10, 0xA0, 0x08, 0x08, 0x00}; // byte 5 shall be set to 0x08

void handle_sns_status(GlobalState *state, CAN_RxHeaderTypeDef rx_msg_header, uint8_t *rx_msg_data)
{
    if (state->car.sns.snsOffAt == 0 && state->board.snsRequestOffAt == 0 && rx_msg_header.DLC >= 3)
    {
        state->car.sns.active = !(((rx_msg_data[2] >> 2) & 0x03) == 0x01);
        VLOG("%d SNS status:%d\n", state->board.now, state->car.sns.active);
    }
}

void handle_sns_request(GlobalState *state, CAN_RxHeaderTypeDef rx_msg_header, uint8_t *rx_msg_data)
{
    bool shouldDisableSNS = state->car.sns.snsOffAt == 0 && state->car.sns.active && state->board.snsRequestOffAt > 0;
    if (shouldDisableSNS)
    {
        LOG("%d disable SNS\n", state->board.now);
        memcpy(&disableSNSFrame, &rx_msg_data, rx_msg_header.DLC);
        disableSNSHeader.DLC = rx_msg_header.DLC;
        disableSNSFrame[5] = (disableSNSFrame[5] & 0b11000111) | (0x01 << 3);
        can_tx(&disableSNSHeader, disableSNSFrame);
        state->car.sns.snsOffAt = state->board.now;
        led_tx_on();
    }
}
#endif

#ifdef ENABLE_DPF_REGEN_NOTIFICATIION
void handle_dpf_regeneration(GlobalState *state, CAN_RxHeaderTypeDef rx_msg_header, uint8_t *rx_msg_data)
{
    if (rx_msg_header.DLC >= 6)
    {
        state->car.dpf.regenMode = (rx_msg_data[5] >> 2) & 0b00000111;

        if (state->car.dpf.regenMode == 2 && state->car.dpf.regenerating == 0)
        {
            state->car.dpf.regenerating = 1;
            LOG("%d start DPF r\n", state->board.now);
        }

        if (state->car.dpf.regenMode == 0 && state->car.dpf.regenerating == 1)
        {
            state->car.dpf.regenerating = 0;
            LOG("%d end DPF r\n", state->board.now);
        }
    }
}
#endif

void handle_standard_frame(GlobalState *state, CAN_RxHeaderTypeDef rx_msg_header, uint8_t *rx_msg_data)
{
    switch (rx_msg_header.StdId)
    {
    case 0x000000FC:
        handle_rpm(state, rx_msg_header, rx_msg_data);
        break;
#ifdef ENABLE_SNS_AUTO_OFF
    case 0x00000226:
        handle_sns_status(state, rx_msg_header, rx_msg_data);
        break;
#endif
#ifdef ENABLE_DASHBOARD
    case 0x000000FB:
        handle_torque(state, rx_msg_header, rx_msg_data);
        break;
    case 0x000002EF:
        handle_gear(state, rx_msg_header, rx_msg_data);
        break;
    case 0x000002FA:
        handle_cc_buttons(state, rx_msg_header, rx_msg_data);
        break;
    case 0x000005A5:
        state->car.ccActive = ((rx_msg_data[0] >> 7) == 1);
        VLOG("%d CC active:%d\n", state->board.now, state->car.ccActive);
        break;
    case 0x0000041A:
        handle_battery(state, rx_msg_header, rx_msg_data);
        break;
    case 0x000004B2:
        handle_oil(state, rx_msg_header, rx_msg_data);
        break;
#endif
#ifdef ENABLE_SNS_AUTO_OFF
    case 0x000004B1:
        handle_sns_request(state, rx_msg_header, rx_msg_data);
        break;
#endif
#ifdef ENABLE_DPF_REGEN_NOTIFICATIION
    case 0x000005AE:
        handle_dpf_regeneration(state, rx_msg_header, rx_msg_data);
        break;
#endif
    default:
    }
}

void handle_extended_frame(GlobalState *state, CAN_RxHeaderTypeDef rx_msg_header, uint8_t *rx_msg_data)
{
}
#endif
