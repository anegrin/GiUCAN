//
// can: initializes and provides methods to interact with the CAN peripheral
//

#include "config.h"

#ifdef C1CAN

#include "led.h"
#include "can.h"
#include "processing.h"
#include "logging.h"

CAN_TxHeaderTypeDef disableSNSHeader = {.IDE = CAN_ID_STD, .RTR = CAN_RTR_DATA, .StdId = 0x4B1, .DLC = 8};
uint8_t disableSNSFrame[8] = {0x04, 0x00, 0x00, 0x10, 0xA0, 0x08, 0x08, 0x00}; // byte 5 shall be set to 0x08
char gears[] = {'N', '1', '2', '3', '4', '5', '6', 'R', '7', '8', '9'};

void handle_torque(GlobalState *state, CAN_RxHeaderTypeDef rx_msg_header, uint8_t *rx_msg_data)
{
    if (rx_msg_header.DLC >= 4)
    {
        state->car.torque = (state->car.rpm * ((rx_msg_data[2] & 0b01111111) << 4 | ((rx_msg_data[3] >> 4) & 0b00001111))) - 500;
        VLOG("%d torque:%d\r\n", state->board.now, state->car.torque);
    }
}
void handle_rpm(GlobalState *state, CAN_RxHeaderTypeDef rx_msg_header, uint8_t *rx_msg_data)
{
    if (rx_msg_header.DLC >= 2)
    {
        state->car.rpm = (rx_msg_data[0] * 256 + (rx_msg_data[1] & ~0x3)) / 4;
        VLOG("%d RPM:%d\r\n", state->board.now, state->car.rpm);
    }
}

void handle_sns_status(GlobalState *state, CAN_RxHeaderTypeDef rx_msg_header, uint8_t *rx_msg_data)
{
    if (state->car.sns.snsOffAt == 0 && state->board.snsRequestOffAt == 0 && rx_msg_header.DLC >= 3)
    {
        state->car.sns.active = !(((rx_msg_data[2] >> 2) & 0x03) == 0x01);
        LOG("%d SNS status:%d\r\n", state->board.now, state->car.sns.active);
    }
}

void handle_gear(GlobalState *state, CAN_RxHeaderTypeDef rx_msg_header, uint8_t *rx_msg_data)
{
    uint8_t i = ((uint8_t)(rx_msg_data[0] & ~0xF) >> 4);
    state->car.gear = gears[i];
    VLOG("%d gear %c\r\n", state->board.now, state->car.gear);
}

void handle_battery(GlobalState *state, CAN_RxHeaderTypeDef rx_msg_header, uint8_t *rx_msg_data)
{
    if (rx_msg_header.DLC >= 6)
    {
        state->car.battery.chargePercent = (rx_msg_data[1] & 0b01111111);
        state->car.battery.current = (0.1f * (rx_msg_data[4] << 4 | ((rx_msg_data[5] >> 4) & 0b00001111))) - 250.0f;
        VLOG("%d Battery p:%d,c:%.1f\r\n", state->board.now, state->car.battery.chargePercent, state->car.battery.current);
    }
}

void handle_sns_request(GlobalState *state, CAN_RxHeaderTypeDef rx_msg_header, uint8_t *rx_msg_data)
{
    bool shouldDisableSNS = state->car.sns.snsOffAt == 0 && state->car.sns.active && state->board.snsRequestOffAt > 0;
    if (shouldDisableSNS)
    {
        LOG("%d disable SNS\r\n", state->board.now);
        memcpy(&disableSNSFrame, &rx_msg_data, rx_msg_header.DLC);
        disableSNSHeader.DLC = rx_msg_header.DLC;
        disableSNSFrame[5] = (disableSNSFrame[5] & 0b11000111) | (0x01 << 3);
        can_tx(&disableSNSHeader, disableSNSFrame);
        state->car.sns.snsOffAt = state->board.now;
        led_tx_on();
        LOG("%d SNS disabled\r\n", state->board.now);
    }
}
void handle_oil(GlobalState *state, CAN_RxHeaderTypeDef rx_msg_header, uint8_t *rx_msg_data)
{
    if (rx_msg_header.DLC >= 4)
    {
        state->car.oil.pressure = 0.1f * ((rx_msg_data[0] & 0b00000001) << 7 | ((rx_msg_data[1] >> 1) & 0b01111111));
        state->car.oil.temperature = ((rx_msg_data[2] & 0b00111111) << 2 | ((rx_msg_data[3] >> 6) & 0b00000011)) - 40;
        VLOG("%d Oil p:%.1f,t:%d\r\n", state->board.now, state->car.oil.pressure, state->car.oil.temperature);
    }
}

void handle_standard_frame(GlobalState *state, CAN_RxHeaderTypeDef rx_msg_header, uint8_t *rx_msg_data)
{
    switch (rx_msg_header.StdId)
    {
    case 0x000000FB:
        handle_torque(state, rx_msg_header, rx_msg_data);
        break;
    case 0x000000FC:
        handle_rpm(state, rx_msg_header, rx_msg_data);
        break;
#ifdef ENABLE_SNS_AUTO_OFF
    case 0x00000226:
        handle_sns_status(state, rx_msg_header, rx_msg_data);
        break;
#endif
    case 0x000002EF:
        handle_gear(state, rx_msg_header, rx_msg_data);
        break;
    case 0x000002FA:
        // processingMessage0x000002FA();
        break;
    case 0x0000041A:
        handle_battery(state, rx_msg_header, rx_msg_data);
        break;
#ifdef ENABLE_SNS_AUTO_OFF
    case 0x000004B1:
        handle_sns_request(state, rx_msg_header, rx_msg_data);
        break;
#endif
    case 0x000004B2:
        handle_oil(state, rx_msg_header, rx_msg_data);
        break;
    case 0x000005A5:
        state->car.ccActive = ((rx_msg_data[0] >> 7) == 1);
        VLOG("%d CC active:%d\r\n", state->board.now, state->car.ccActive);
        break;
    case 0x000005AE:
        // uint8_t dieselEngineRegenerationMode = (rx_msg_data[5]>>2 ) & 0b00000111;
        break;
    default:
    }
}

void handle_extended_frame(GlobalState *state, CAN_RxHeaderTypeDef rx_msg_header, uint8_t *rx_msg_data)
{
}
#endif
