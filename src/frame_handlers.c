//
// can: initializes and provides methods to interact with the CAN peripheral
//

#include "config.h"
#include "can.h"
#include "frame_handlers.h"
#include "logging.h"

#ifdef C1CAN

CAN_TxHeaderTypeDef disableSNSHeader = {.IDE = CAN_ID_STD, .RTR = CAN_RTR_DATA, .StdId = 0x4B1, .DLC = 8};
uint8_t disableSNSFrame[8] = {0x04, 0x00, 0x00, 0x10, 0xA0, 0x08, 0x08, 0x00};
char gears[]={'N','1','2','3','4','5','6','R','7','8','9'};

void handle_c1_standard_frame(GlobalState *state, CAN_RxHeaderTypeDef rx_msg_header, uint8_t *rx_msg_data)
{
    switch (rx_msg_header.StdId)
    {
    case 0x000000FC:
        if (rx_msg_header.DLC >= 2)
        {
            state->car.rpm = (rx_msg_data[0] * 256 + (rx_msg_data[1] & ~0x3)) / 4;
            VLOG("%d RPM:%f\r\n", state->board.now, state->car.rpm);
        }
        break;
    case 0x00000226:
        if (!state->car.sns.available && rx_msg_header.DLC >= 3)
        {
            state->car.sns.available = (((rx_msg_data[2] >> 2) & 0x03) == 0x01);
            VLOG("%d SNS s:%d\r\n", state->board.now, state->car.sns.available);
        }
        break;
    case 0x000002EF:
        uint8_t i = ((uint8_t)(rx_msg_data[0] & ~0xF) >> 4);
        state->car.gear=gears[i];
        VLOG("%d gear %c\r\n", state->board.now, state->car.gear);
        break;
    case 0x000004B1:
        bool shouldDisableSNS = state->car.sns.available && state->car.sns.snsOffAt == 0 && state->board.snsRequestOffAt > 0;
        if (shouldDisableSNS)
        {
            memcpy(&disableSNSFrame, &rx_msg_data, rx_msg_header.DLC);
            disableSNSHeader.DLC = rx_msg_header.DLC;
            disableSNSFrame[5] = (disableSNSFrame[5] & 0b11000111) | (0x01 << 3);
            can_tx(&disableSNSHeader, disableSNSFrame);
            state->car.sns.snsOffAt = state->board.now;
            VLOG("%d dis SNS\r\n", state->board.now);
        }
        break;
    case 0x000004B2:
        if (rx_msg_header.DLC >= 4)
        {
            state->car.oil.pressure = ((rx_msg_data[0] & 0b00000001) << 7 | ((rx_msg_data[1] >> 1) & 0b01111111)) * 0.1f;
            state->car.oil.temperature = ((rx_msg_data[2] & 0b00111111) << 2 | ((rx_msg_data[3] >> 6) & 0b00000011)) - 40;
            VLOG("%d Oil p:%.1f,t:%.0f\r\n", state->board.now, state->car.oil.pressure, state->car.oil.temperature);
        }
        break;
    default:
    }
}

void handle_c1_frame(GlobalState *state, CAN_RxHeaderTypeDef rx_msg_header, uint8_t *rx_msg_data)
{
    if (rx_msg_header.RTR == CAN_RTR_DATA)
    {
        switch (rx_msg_header.IDE)
        {
        case CAN_ID_STD:
            handle_c1_standard_frame(state, rx_msg_header, rx_msg_data);
            break;
        case CAN_ID_EXT:
        default:
        }
    }
}
#endif
