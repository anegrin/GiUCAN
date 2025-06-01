#ifndef __MODEL_H
#define __MODEL_H

#include <stdint.h>
#include <stdbool.h>

typedef struct
{
    bool active;
    uint32_t snsOffAt;
} SNSState;

typedef struct
{
    uint8_t chargePercent;
    float current;
} Battery;

typedef struct
{
    float pressure;
    int16_t temperature;
} Oil;

typedef struct
{
    uint8_t regenMode;
    bool regenerating;
} DPF;

typedef struct
{
    int hp;
    int nm;
} Power;

typedef struct
{
    uint32_t rpm;
    uint16_t torque;
    Power power;
    char gear;
    bool ccActive;
    DPF dpf;
    Oil oil;
    Battery battery;
    SNSState sns;
} CarState;

typedef struct
{
    bool visible;
    uint8_t itemsCount;
    uint8_t currentItemIndex;
    float values[2];
} DashboardState;

typedef struct
{
    uint32_t now;
    uint32_t snsRequestOffAt;
    uint32_t dpfRegenNotificationRequestOffAt;
    DashboardState dashboardState;
} BoardState;

typedef struct
{
    BoardState board;
    CarState car;
} GlobalState;

#endif /* __MODEL_H */
