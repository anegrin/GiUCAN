//
// can: initializes and provides methods to interact with the CAN peripheral
//

#include <stdbool.h>
#include "loops.h"
#include "logging.h"

#ifdef C1CAN
void c1_loop(GlobalState *state)
{
    bool shouldHandleSNSAutoOff = state->car.sns.snsOffAt == 0 && state->board.snsRequestOffAt == 0 && state->board.now > SNS_AUTO_OFF_DELAY_MS && state->car.rpm > SNS_AUTO_OFF_MIN_RPM;

    if (shouldHandleSNSAutoOff)
    {
        if (state->car.sns.active) {
            //sns was on, we request for off
            LOG("%d SNS req off\r\n", state->board.now);
            state->board.snsRequestOffAt = state->board.now;
        } else {
            //sns was off (user action), we will consider this done
            state->car.sns.snsOffAt = state->board.now;
        }
    }
}
#endif

#ifdef BHCAN
void bh_loop(GlobalState *state)
{
}
#endif
