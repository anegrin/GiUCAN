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
static uint8_t localCurrentDashboardItemIndex = 0;
static uint32_t refreshOperations = 0;
void state_process(GlobalState *state)
{
#ifdef ENABLE_SNS_AUTO_OFF
    bool shouldHandleSNSAutoOff = state->car.sns.snsOffAt == 0 && state->board.snsRequestOffAt == 0 && state->board.now > SNS_AUTO_OFF_DELAY_MS && state->car.rpm > CAR_IS_ON_MIN_RPM;

    if (shouldHandleSNSAutoOff)
    {
        if (state->car.sns.active)
        {
            state->board.snsRequestOffAt = state->board.now;
            // sns was on, we request for off
            LOG("%d SNS req off\n", state->board.now);
        }
        else
        {
            // sns was off (user action), we will consider this done
            state->car.sns.snsOffAt = state->board.now;
        }
    }
#endif
#ifdef ENABLE_DASHBOARD

    if (state->board.dashboardState.visible && state->board.latestMessageReceivedAt + VALUES_TIMEOUT_MS < state->board.now)
    {
        VLOG("%d no msg", state->board.now);
        state->board.dashboardState.visible = false;
        send_state(state);
    }

    if (state->board.dashboardState.visible && state->board.collectingMultiframeResponse == -1)
    {
        DashboardItemType type = state->board.dashboardState.currentItemIndex;

        if (localCurrentDashboardItemIndex != type)
        {
            valueToExtract = -1;
            refreshOperations = 0;
            localCurrentDashboardItemIndex = type;
        }

        uint32_t values_refresh_ms = values_refresh_rate_of(type);

        //first 2 iterations for an item (v0 and optional v1)
        //must be done ASAP, then we'll honor values_refresh_rate_of
        if (refreshOperations < 2)
        {
            values_refresh_ms = DEFAULT_VALUES_REFRESH_MS / 5;
        }

        if (valuesUpdatedAt + values_refresh_ms < state->board.now)
        {
            refreshOperations++;
            valuesUpdatedAt = state->board.now;
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
