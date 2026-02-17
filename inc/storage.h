#ifndef _STORAGE_H
#define _STORAGE_H

#include "model.h"

void storage_init(void);
#ifdef C1CAN
void load_settings(Settings *settings);
#endif

#endif // _STORAGE_H
