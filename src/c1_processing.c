#include "config.h"
#ifdef C1CAN
#include <stdbool.h>
#include <string.h>
#include "processing.h"
#include "logging.h"
#include "dashboard.h"
#include "can.h"
#include "uart.h"

static uint8_t *no_rx_msg_data = {0};

void extract_or_send_request(CarValueExtractor extractor, GlobalState *state, uint8_t valueIndex)
{
    if (extractor.needsQuery)
    {
        CAN_TxHeaderTypeDef tx_msg_header = {.IDE = CAN_ID_EXT, .RTR = CAN_RTR_DATA, .DLC = 4};
        tx_msg_header.ExtId = extractor.query.reqId;
        uint8_t tx_msg_data[8] = {0};
        memcpy(&tx_msg_data[0], &extractor.query.reqData, 4);
        VLOG("%d req %02x%02x%02x%02x\n", state->board.now, tx_msg_data[0], tx_msg_data[1], tx_msg_data[2], tx_msg_data[3])
        can_tx(&tx_msg_header, tx_msg_data);
    }
    else
    {
        float value = extractor.extract(state, no_rx_msg_data);
        if (state->board.dashboardState.values[valueIndex] != value)
        {
            state->board.dashboardState.values[valueIndex] = value;
            send_state(state);
        }
    }
}

static uint32_t valuesUpdatedAt = 0;
static int valueToExtract = 0;
void state_process(GlobalState *state)
{
#ifdef ENABLE_SNS_AUTO_OFF
    bool shouldHandleSNSAutoOff = state->car.sns.snsOffAt == 0 && state->board.snsRequestOffAt == 0 && state->board.now > SNS_AUTO_OFF_DELAY_MS && state->car.rpm > SNS_AUTO_OFF_MIN_RPM;

    if (shouldHandleSNSAutoOff)
    {
        if (state->car.sns.active)
        {
            // sns was on, we request for off
            LOG("%d SNS req off\n", state->board.now);
            state->board.snsRequestOffAt = state->board.now;
        }
        else
        {
            // sns was off (user action), we will consider this done
            state->car.sns.snsOffAt = state->board.now;
        }
    }
#endif
#ifdef ENABLE_DASHBOARD

    if (state->board.dashboardState.visible)
    {
        if (valuesUpdatedAt + VALUES_REFRESH_MS < state->board.now)
        {

            valuesUpdatedAt = state->board.now;
            DashboardItemType type = type_of(state->board.dashboardState.currentItemIndex);
            CarValueExtractors extractors = extractor_of(type, state);
            if (extractors.hasV0 && extractors.hasV1)
            {
                if (valueToExtract == -1)
                {
                    valueToExtract = 0;
                }
                else
                {
                    valueToExtract = (valueToExtract + 1) % 2;
                }
            }
            else if (extractors.hasV0)
            {
                valueToExtract = 0;
            }
            else if (extractors.hasV1)
            {
                valueToExtract = 1;
            }
            else
            {
                valueToExtract = -1;
            }

            if (valueToExtract == 0)
            {
                extract_or_send_request(extractors.forV0, state, 0);
            }
            if (valueToExtract == 1)
            {
                extract_or_send_request(extractors.forV1, state, 1);
            }
        }
    }
#endif
}
#endif
