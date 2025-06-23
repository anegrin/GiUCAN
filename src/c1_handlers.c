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
#include "dashboard.h"

void handle_rpm(GlobalState *state, CAN_RxHeaderTypeDef rx_msg_header, uint8_t *rx_msg_data)
{
    if (rx_msg_header.DLC >= 2)
    {
        state->car.rpm = (rx_msg_data[0] * 256 + (rx_msg_data[1] & ~0x03)) / 4;
        if (state->car.rpm > CAR_IS_ON_MIN_RPM)
        {
            state->board.latestMessageReceivedAt = state->board.now;
        }
    }
}

#ifdef ENABLE_DASHBOARD
uint8_t latestCCButtonEvent = 0x10; // no button pressed
uint32_t resPushedAt = 0;

void handle_torque(GlobalState *state, CAN_RxHeaderTypeDef rx_msg_header, uint8_t *rx_msg_data)
{
    if (rx_msg_header.DLC >= 4)
    {
        state->car.torque = (rx_msg_data[2] & 0b01111111) << 4 | ((rx_msg_data[3] >> 4) & 0b00001111);
    }
}

static const char gears[] = {'N', '1', '2', '3', '4', '5', '6', 'R', '7', '8', '9'};
void handle_gear(GlobalState *state, CAN_RxHeaderTypeDef rx_msg_header, uint8_t *rx_msg_data)
{
    uint8_t i = ((uint8_t)(rx_msg_data[0] & ~0x0F) >> 4);
    if (i < sizeof(gears))
    {
        state->car.gear = gears[i];
    }
}

void handle_battery(GlobalState *state, CAN_RxHeaderTypeDef rx_msg_header, uint8_t *rx_msg_data)
{
    if (rx_msg_header.DLC >= 6)
    {
        state->car.battery.chargePercent = (rx_msg_data[1] & 0b01111111);
        state->car.battery.current = ((float)(rx_msg_data[4] << 4 | ((rx_msg_data[5] >> 4) & 0b00001111))) * 0.1f - 250.0f;
    }
}
void handle_oil(GlobalState *state, CAN_RxHeaderTypeDef rx_msg_header, uint8_t *rx_msg_data)
{
    if (rx_msg_header.DLC >= 4)
    {
        state->car.oil.pressure = 0.1f * ((rx_msg_data[0] & 0b00000001) << 7 | ((rx_msg_data[1] >> 1) & 0b01111111));
        state->car.oil.temperature = ((rx_msg_data[2] & 0b00111111) << 2 | ((rx_msg_data[3] >> 6) & 0b00000011)) - 40;
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
            case 0x00: // speed hard up
                state->board.dashboardState.currentItemIndex = (state->board.dashboardState.currentItemIndex + itemsCount - DASHBOARD_PAGE_SIZE) % itemsCount;
                LOG("%d speed ^^:%d\n", state->board.now, state->board.dashboardState.currentItemIndex);
                knownEvent = true;
                sendEvent = true;
                break;
            case 0x20: // speed hard down
                state->board.dashboardState.currentItemIndex = (state->board.dashboardState.currentItemIndex + DASHBOARD_PAGE_SIZE) % itemsCount;
                LOG("%d speed vv:%d\n", state->board.now, state->board.dashboardState.currentItemIndex);
                knownEvent = true;
                sendEvent = true;
                break;
            case 0x08: // speed up
                state->board.dashboardState.currentItemIndex = (state->board.dashboardState.currentItemIndex + itemsCount - 1) % itemsCount;
                LOG("%d speed ^:%d\n", state->board.now, state->board.dashboardState.currentItemIndex);
                knownEvent = true;
                sendEvent = true;
                break;
            case 0x18: // speed down
                state->board.dashboardState.currentItemIndex = (state->board.dashboardState.currentItemIndex + 1) % itemsCount;
                LOG("%d speed v:%d\n", state->board.now, state->board.dashboardState.currentItemIndex);
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
            LOG("%d res v\n", state->board.now);
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
            state->board.dashboardState.values[0] = 0.0f;
            state->board.dashboardState.values[1] = 0.0f;
            send_state(state);
        }
    }
}
#endif

#ifdef ENABLE_SNS_AUTO_OFF
static CAN_TxHeaderTypeDef disableSNSHeader = {.IDE = CAN_ID_STD, .RTR = CAN_RTR_DATA, .StdId = 0x04B1, .DLC = 8};
static uint8_t disableSNSFrame[8] = {0x04, 0x00, 0x00, 0x10, 0xA0, 0x08, 0x08, 0x00}; // byte 5 shall be set to 0x08

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
        state->car.sns.snsOffAt = state->board.now;
        LOG("%d disable SNS\n", state->board.now);
        memcpy(&disableSNSFrame, rx_msg_data, rx_msg_header.DLC);
        disableSNSHeader.DLC = rx_msg_header.DLC;
        disableSNSFrame[5] = (disableSNSFrame[5] & 0b11000111) | (0x01 << 3);
        if (can_tx(&disableSNSHeader, disableSNSFrame) == HAL_OK)
        {
            led_tx_on();
        }
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
#ifdef ENABLE_DPF_REGEN_VISUAL_NOTIFICATIION
            state->board.dashboardState.currentItemIndex = DPF_REGEN_VISUAL_NOTIFICATIION_ITEM;
            state->board.dashboardState.visible = true;
#endif
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
    case 0xFC:
        handle_rpm(state, rx_msg_header, rx_msg_data);
        break;
#ifdef ENABLE_SNS_AUTO_OFF
    case 0x0226:
        handle_sns_status(state, rx_msg_header, rx_msg_data);
        break;
#endif
#ifdef ENABLE_DASHBOARD
    case 0xFB:
        handle_torque(state, rx_msg_header, rx_msg_data);
        break;
    case 0x02EF:
        handle_gear(state, rx_msg_header, rx_msg_data);
        break;
    case 0x02FA:
        if (!state->car.ccActive)
        {
            handle_cc_buttons(state, rx_msg_header, rx_msg_data);
        }
        break;
    case 0x05A5:
        state->car.ccActive = ((rx_msg_data[0] >> 7) == 1);
        VLOG("%d CC active:%d\n", state->board.now, state->car.ccActive);
        break;
    case 0x041A:
        handle_battery(state, rx_msg_header, rx_msg_data);
        break;
    case 0x04B2:
        handle_oil(state, rx_msg_header, rx_msg_data);
        break;
#endif
#ifdef ENABLE_SNS_AUTO_OFF
    case 0x04B1:
        handle_sns_request(state, rx_msg_header, rx_msg_data);
        break;
#endif
#ifdef ENABLE_DPF_REGEN_NOTIFICATIION
    case 0x05AE:
        handle_dpf_regeneration(state, rx_msg_header, rx_msg_data);
        break;
#endif
    default:
    }
}

#define CONSECUTIVE_FRAME_LIMIT 0x0F
static uint8_t multiframe_rx_msg_data_index = 0;
#define MULTIFRAME_RX_MSG_DATA_SIZE 127
static int8_t multiframe_rx_msg_data_pending_bytes = 0;
static uint8_t multiframe_rx_msg_data[2][MULTIFRAME_RX_MSG_DATA_SIZE];
static uint8_t flow_control_tx_msg_data[8] = {0x30, CONSECUTIVE_FRAME_LIMIT, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

bool apply_extractor(CarValueExtractor extractor, GlobalState *state, CAN_RxHeaderTypeDef *rx_msg_header, uint8_t *rx_msg_data, uint8_t valueIndex)
{
    bool extracted = false;
    if (extractor.needsQuery && extractor.query.replyId == rx_msg_header->ExtId)
    {

        bool single_frame = rx_msg_data[0] < 0x10;
        bool first_frame = rx_msg_data[0] >> 4 == 1;
        bool consecutive_frame = rx_msg_data[0] >> 4 == 2;

        uint8_t byte_and_data_offset = single_frame ? 1 : 2;

        if (consecutive_frame)
        {
            VLOG("multiframe size %d\n", multiframe_rx_msg_data_pending_bytes);
            VLOG("c_f %02X%02X%02X%02X%02X%02X%02X%02X\n", rx_msg_data[0], rx_msg_data[1], rx_msg_data[2], rx_msg_data[3], rx_msg_data[4], rx_msg_data[5], rx_msg_data[6], rx_msg_data[7]);
            for (uint8_t i = 1; i <= 7 && multiframe_rx_msg_data_pending_bytes >= 0; i++)
            {
                if (multiframe_rx_msg_data_index < MULTIFRAME_RX_MSG_DATA_SIZE)
                {
                    multiframe_rx_msg_data[valueIndex][multiframe_rx_msg_data_index] = rx_msg_data[i];
                }
                multiframe_rx_msg_data_index++;
                multiframe_rx_msg_data_pending_bytes--;
            }
            VLOG("multiframe size %d\n", multiframe_rx_msg_data_pending_bytes);

            if (multiframe_rx_msg_data_pending_bytes <= 0)
            {
                float extractedValue = extractor.extract(state, multiframe_rx_msg_data[valueIndex]);
                VLOG("v %.2f\n", extractedValue);
                VLOG("m_d1 %02X%02X%02X%02X%02X%02X%02X%02X\n", rx_msg_data[0], rx_msg_data[1], rx_msg_data[2], rx_msg_data[3], rx_msg_data[4], rx_msg_data[5], rx_msg_data[6], rx_msg_data[7]);
                VLOG("m_d2 %02X%02X%02X%02X%02X%02X%02X%02X\n", rx_msg_data[8], rx_msg_data[9], rx_msg_data[10], rx_msg_data[11], rx_msg_data[12], rx_msg_data[13], rx_msg_data[14], rx_msg_data[15]);

                if (state->board.dashboardState.values[valueIndex] != extractedValue)
                {
                    state->board.dashboardState.values[valueIndex] = extractedValue;
                    extracted = true;
                }
                state->board.collectingMultiframeResponse = -1;
            }
        }
        else if (state->board.collectingMultiframeResponse != -1)
        {
            return false;
        }
        else
        {

            uint8_t success = rx_msg_data[byte_and_data_offset] == 0x62;
            if (success)
            {
                uint16_t resData = rx_msg_data[byte_and_data_offset + 2] * 256 + rx_msg_data[byte_and_data_offset + 1];
                if (resData == (extractor.query.reqData >> 16))
                {
                    if (first_frame)
                    {
                        state->board.collectingMultiframeResponse = valueIndex;
                        VLOG("f_f %02X%02X%02X%02X%02X%02X%02X%02X\n", rx_msg_data[0], rx_msg_data[1], rx_msg_data[2], rx_msg_data[3], rx_msg_data[4], rx_msg_data[5], rx_msg_data[6], rx_msg_data[7]);
                        //2 bytes for fake init frame
                        multiframe_rx_msg_data[valueIndex][0] = 0x10;
                        multiframe_rx_msg_data[valueIndex][1] = MULTIFRAME_RX_MSG_DATA_SIZE;
                        //2 bytes for fake command
                        multiframe_rx_msg_data[valueIndex][2] = (extractor.query.reqData >> 16 & 0xff);
                        multiframe_rx_msg_data[valueIndex][3] = extractor.query.reqData >> 24;
                        //3 data bytes
                        multiframe_rx_msg_data[valueIndex][4] = rx_msg_data[5];
                        multiframe_rx_msg_data[valueIndex][5] = rx_msg_data[6];
                        multiframe_rx_msg_data[valueIndex][6] = rx_msg_data[7];
                        multiframe_rx_msg_data_index = 7;
                        CAN_TxHeaderTypeDef tx_msg_header = {.IDE = CAN_ID_EXT, .RTR = CAN_RTR_DATA, .DLC = 3};
                        tx_msg_header.ExtId = extractor.query.reqId;
                        //pending bytes is total - 2 cmd bytes - 3 data bytes
                        multiframe_rx_msg_data_pending_bytes = (rx_msg_data[0] & 0x0f) * 256 + rx_msg_data[1] - 5;
                        can_tx(&tx_msg_header, flow_control_tx_msg_data);
                    }
                    else
                    {
                        // single frame
                        float extractedValue = extractor.extract(state, rx_msg_data);
                        if (state->board.dashboardState.values[valueIndex] != extractedValue)
                        {
                            state->board.dashboardState.values[valueIndex] = extractedValue;
                            extracted = true;
                        }
                    }
                }
            }
        }
    }

    return extracted;
}

static uint8_t localCurrentDashboardItemIndex = 0;
void handle_extended_frame(GlobalState *state, CAN_RxHeaderTypeDef rx_msg_header, uint8_t *rx_msg_data)
{

    if (localCurrentDashboardItemIndex != state->board.dashboardState.currentItemIndex)
    {
        localCurrentDashboardItemIndex = state->board.dashboardState.currentItemIndex;
        state->board.collectingMultiframeResponse = -1;
    }

    if (state->board.dashboardState.visible)
    {
        bool consumingV0Multiframe = state->board.collectingMultiframeResponse == 0;
        bool consumingV1Multiframe = state->board.collectingMultiframeResponse == 1;
        bool notConsuming = state->board.collectingMultiframeResponse == -1;
        bool v0Extracted = false;
        bool v1Extracted = false;
        DashboardItemType type = state->board.dashboardState.currentItemIndex;
        CarValueExtractors extractors = extractor_of(type, state);
        if (extractors.hasV0 && (consumingV0Multiframe || notConsuming))
        {
            v0Extracted = apply_extractor(extractors.forV0, state, &rx_msg_header, rx_msg_data, 0);
        }

        if (extractors.hasV1 && (consumingV1Multiframe || notConsuming))
        {
            v1Extracted = apply_extractor(extractors.forV1, state, &rx_msg_header, rx_msg_data, 1);
        }

        if (v0Extracted || v1Extracted)
        {
            send_state(state);
        }
    }
}
#endif
