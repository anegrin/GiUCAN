//
// can: initializes and provides methods to interact with the CAN peripheral
//

#include <stdbool.h>
#include "loops.h"
#include "logging.h"

#ifdef C1CAN
void c1_loop(GlobalState *state)
{
    bool shouldCheckSNS = state->car.sns.snsOffAt == 0 && state->board.now > SNS_AUTO_OFF_DELAY_MS && state->car.rpm > SNS_AUTO_OFF_MIN_RPM && state->car.sns.available;

    if (shouldCheckSNS)
    {
        state->board.snsRequestOffAt = state->board.now;
    }
}
#endif
