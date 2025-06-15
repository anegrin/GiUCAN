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
    uint32_t rpm;
    uint16_t torque;
    uint8_t gear;
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
    uint32_t dpfRegenNotificationRequestAt;
    uint32_t latestMessageReceivedAt;
    uint32_t dashboardExternallyUpdatedAt;
    DashboardState dashboardState;
} BoardState;

typedef struct
{
    BoardState board;
    CarState car;
} GlobalState;

#endif /* __MODEL_H */
