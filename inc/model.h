#ifndef __MODEL_H
#define __MODEL_H

#include <stdint.h>
#include <stdbool.h>

typedef struct
{
    uint32_t snsOffAt;
    bool active;
} SNSState;

typedef struct
{
    float current;
    uint8_t chargePercent;
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
    DPF dpf;
    Oil oil;
    Battery battery;
    SNSState sns;
    uint16_t torque;
    uint8_t gear;
    bool ccActive;
} CarState;

typedef struct
{
    float values[2];
    uint8_t itemsCount;
    uint8_t currentItemIndex;
    bool visible;
} DashboardState;

typedef struct
{
    uint32_t now;
    uint32_t snsRequestOffAt;
    uint32_t dpfRegenNotificationRequestAt;
    uint32_t latestMessageReceivedAt;
    uint32_t dashboardExternallyUpdatedAt;
    DashboardState dashboardState;
    int8_t collectingMultiframeResponse;
} BoardState;

typedef struct
{
    BoardState board;
    CarState car;
} GlobalState;

#endif /* __MODEL_H */
