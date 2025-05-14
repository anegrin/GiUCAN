#ifndef _FRAME_HANDLERS_H
#define _FRAME_HANDLERS_H

#include "config.h"
#include "model.h"

#ifdef C1CAN
void handle_c1_frame(GlobalState *state, CAN_RxHeaderTypeDef rx_msg_header, uint8_t *rx_msg_data);
#endif

#endif // _FRAME_HANDLERS_H
