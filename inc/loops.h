#ifndef _LOOPS_H
#define _LOOPS_H

#include "config.h"
#include "model.h"

#ifdef C1CAN
void c1_loop(GlobalState *state);
#endif

#ifdef BHCAN
void bh_loop(GlobalState *state);
#endif

#endif // _LOOPS_H
