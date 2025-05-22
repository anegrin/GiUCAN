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
    float pressure;
    float temperature;
} Oil;

typedef struct
{
    float rpm;
    char gear;
    Oil oil;
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
