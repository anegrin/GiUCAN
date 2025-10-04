#ifndef _LOOPS_H
#define _LOOPS_H

#include "config.h"
#include "model.h"

void state_process(GlobalState *state, Settings *settings);
void handle_standard_frame(GlobalState *state, Settings *settings, CAN_RxHeaderTypeDef rx_msg_header, uint8_t *rx_msg_data);
void handle_extended_frame(GlobalState *state, Settings *settings, CAN_RxHeaderTypeDef rx_msg_header, uint8_t *rx_msg_data);

#endif // _LOOPS_H
