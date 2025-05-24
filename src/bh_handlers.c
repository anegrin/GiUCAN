//
// can: initializes and provides methods to interact with the CAN peripheral
//

#include "config.h"

#ifdef BHCAN

#include "led.h"
#include "can.h"
#include "processing.h"
#include "logging.h"

void handle_standard_frame(GlobalState *state, CAN_RxHeaderTypeDef rx_msg_header, uint8_t *rx_msg_data)
{
}

void handle_extended_frame(GlobalState *state, CAN_RxHeaderTypeDef rx_msg_header, uint8_t *rx_msg_data)
{
}
#endif
