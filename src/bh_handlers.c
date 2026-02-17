//
// can: initializes and provides methods to interact with the CAN peripheral
//

#include "config.h"

#ifdef BHCAN

#include "led.h"
#include "can.h"
#include "processing.h"
#include "logging.h"
#include <string.h>

#ifdef ENABLE_DPF_REGEN_SOUND_NOTIFICATION
static uint8_t dpfSoundAlertFrame[8];
static CAN_TxHeaderTypeDef dpfSoundAlertHeader = {.IDE = CAN_ID_STD, .RTR = CAN_RTR_DATA, .StdId = 0x05AC, .DLC = 8};
#endif

void handle_standard_frame(GlobalState *state, CAN_RxHeaderTypeDef rx_msg_header, uint8_t *rx_msg_data)
{
    switch (rx_msg_header.StdId)
    {
    case SOUND_FRAME_STD_ID:
        if (state->board.dpfRegenSoundNotificationRequestAt != 0)
        {
            state->board.dpfRegenSoundNotificationRequestAt = 0;

#ifdef ENABLE_DPF_REGEN_SOUND_NOTIFICATION
            memcpy(dpfSoundAlertFrame, rx_msg_data, rx_msg_header.DLC);

            dpfSoundAlertHeader.DLC = rx_msg_header.DLC;
            dpfSoundAlertFrame[0] = (dpfSoundAlertFrame[0] & 0b00111111);              // set bit 7 and 6 to zero (chime type 0)
            dpfSoundAlertFrame[1] = (dpfSoundAlertFrame[1] & 0b00111111) | 0b01000000; // byte1 bit 7 and 6 = 01 (seatbelt alarm active)
            dpfSoundAlertFrame[3] = dpfSoundAlertFrame[3] | 0xE0;                      // max volume
            if (can_tx(&dpfSoundAlertHeader, dpfSoundAlertFrame) == HAL_OK)
            {
                led_tx_on();
            }
#endif
        }
        break;
    case DASHBOARD_FRAME_STD_ID:
        // message from infotainment
        state->board.dashboardExternallyUpdatedAt = state->board.now;
        led_rx_on();
        break;
    }
}

void handle_extended_frame(GlobalState *state, CAN_RxHeaderTypeDef rx_msg_header, uint8_t *rx_msg_data)
{
}
#endif
