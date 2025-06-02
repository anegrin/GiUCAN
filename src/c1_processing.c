#include "config.h"
#ifdef C1CAN
#include <stdbool.h>
#include "processing.h"
#include "logging.h"
#include "dashboard.h"
#include "can.h"

static uint32_t valuesUpdatedAt = 0;
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
            CarValueExtractors extractors = extractor_of(state->board.dashboardState.currentItemIndex, state);
            if (extractors.hasV0)
            {
                CarValueExtractor extractor = extractors.forV0;
                if (extractor.needsQuery)
                {
                    // send query, will be handled in handle_extended_frame
                }
                else
                {
                    state->board.dashboardState.values[0] = extractor.value;
                }
            }
            if (extractors.hasV1)
            {
                CarValueExtractor extractor = extractors.forV1;
                if (extractor.needsQuery)
                {
                    // send query, will be handled in handle_extended_frame
                }
                else
                {
                    state->board.dashboardState.values[1] = extractor.value;
                }
            }
        }
    }
#endif
}
#endif
