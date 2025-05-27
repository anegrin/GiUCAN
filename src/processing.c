//
// can: initializes and provides methods to interact with the CAN peripheral
//

#include <stdbool.h>
#include "processing.h"
#include "logging.h"

#ifdef C1CAN

void process_state(GlobalState *state)
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

#ifdef BHCAN
bool localStateSet = false;
DashboardState dashboardLocalState;
DPF dpfLocalState;

void process_state(GlobalState *state)
{
    if (localStateSet)
    {
        bool updateDashboard = false;
        if (state->board.dashboardState.visible != dashboardLocalState.visible)
        {
            updateDashboard = true;
            LOGS(state->board.dashboardState.visible ? "show dashboard\n" : "hide dashboard\n");
        }

        if (state->board.dashboardState.visible)
        {
            if (state->board.dashboardState.currentItemIndex != dashboardLocalState.currentItemIndex)
            {
            updateDashboard = true;
                LOG("show i:%d\n", state->board.dashboardState.currentItemIndex);
            }

            if (state->board.dashboardState.values[0] != dashboardLocalState.values[0])
            {
                updateDashboard = true;
                LOG("new v0:%.2f\n", state->board.dashboardState.values[0]);
            }
            if (state->board.dashboardState.values[1] != dashboardLocalState.values[1])
            {
                updateDashboard = true;
                LOG("new v1:%.2f\n", state->board.dashboardState.values[1]);
            }
        }

        if (state->car.dpf.regenerating != dpfLocalState.regenerating)
        {
            LOGS(state->car.dpf.regenerating ? "regen started\n" : "regen ended\n");
        }
    }

    dashboardLocalState.visible = state->board.dashboardState.visible;
    dashboardLocalState.currentItemIndex = state->board.dashboardState.currentItemIndex;
    dashboardLocalState.values[0] = state->board.dashboardState.values[0];
    dashboardLocalState.values[1] = state->board.dashboardState.values[1];
    dpfLocalState.regenerating = state->car.dpf.regenerating;
    localStateSet = true;
}
#endif
