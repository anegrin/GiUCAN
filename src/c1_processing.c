#include "config.h"
#ifdef C1CAN
#include <stdbool.h>
#include "processing.h"
#include "logging.h"
#include "dashboard.h"
#include "can.h"


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
    }
#endif
}
#endif
