#ifndef _LOOPS_H
#define _LOOPS_H

#include "main.h"
#include "model.h"

void process_state(GlobalState *state);
void handle_standard_frame(GlobalState *state, CAN_RxHeaderTypeDef rx_msg_header, uint8_t *rx_msg_data);
void handle_extended_frame(GlobalState *state, CAN_RxHeaderTypeDef rx_msg_header, uint8_t *rx_msg_data);

#endif // _LOOPS_H
