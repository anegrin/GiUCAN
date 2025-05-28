#ifndef _LOOPS_H
#define _LOOPS_H

#include "config.h"
#include "model.h"

void state_process(GlobalState *state);
void handle_standard_frame(GlobalState *state, CAN_RxHeaderTypeDef rx_msg_header, uint8_t *rx_msg_data);
void handle_extended_frame(GlobalState *state, CAN_RxHeaderTypeDef rx_msg_header, uint8_t *rx_msg_data);

#endif // _LOOPS_H
