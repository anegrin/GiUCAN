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
    uint32_t rpm;
    uint16_t torque;
    char gear;
    bool ccActive;
    Oil oil;
    Battery battery;
    SNSState sns;
} CarState;

typedef struct
{
    uint32_t now;
    uint32_t snsRequestOffAt;
} BoardState;

typedef struct
{
    BoardState board;
    CarState car;
} GlobalState;

#endif /* __MODEL_H */
