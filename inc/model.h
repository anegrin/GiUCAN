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
    uint32_t canIsOnAt;
    uint32_t engineIsOnAt;
    uint32_t rpm;
    uint16_t torque;
    uint8_t gear;//char
    bool ccActive;
    DPF dpf;
    Oil oil;
    Battery battery;
    SNSState sns;
} CarState;

typedef struct
{
    float values[2];
    uint32_t carouselShowNextItemAt;
    uint8_t itemsCount;
    uint8_t currentItemIndex;
    bool visible;
} DashboardState;

typedef struct
{
    uint32_t now;
    uint32_t snsRequestOffAt;
    uint32_t dpfRegenNotificationRequestAt;
    uint32_t dashboardExternallyUpdatedAt;
    uint32_t goingToBedAt;
    DashboardState dashboardState;
    int8_t collectingMultiframeResponse;
    bool sleeping;
} BoardState;

typedef struct
{
    BoardState board;
    CarState car;
} GlobalState;

typedef struct
{
    uint32_t bootCarouselDelay;
    uint32_t bootCarouselInterval;
    uint8_t bootCarouselLoops;
    bool bootCarouselEnabled;
    uint8_t favoriteItemsCount;
    uint8_t *favoriteItems;
} Settings;

#endif /* __MODEL_H */
