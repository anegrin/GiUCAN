#ifndef __MODEL_H
#define __MODEL_H

#include <stdint.h>
#include <stdbool.h>

#ifdef C1CAN
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
#endif


typedef struct
{
    bool regenerating;
    uint8_t regenMode;
} DPF;

typedef struct
{
    uint32_t canIsOnAt;
#ifdef C1CAN
    uint32_t engineIsOnAt;
    uint32_t rpm;
    uint16_t torque;
    uint8_t gear;//char
    bool ccActive;
    Oil oil;
    Battery battery;
    SNSState sns;
#endif
    DPF dpf;
} CarState;

typedef struct
{
    float values[2];
#ifdef C1CAN
    uint32_t carouselShowNextItemAt;
#endif
    uint8_t itemsCount;
    uint8_t currentItemIndex;
    bool visible;
} DashboardState;

typedef struct
{
    uint32_t now;
    DashboardState dashboardState;
    uint32_t goingToBedAt;
    uint32_t dpfRegenSoundNotificationRequestAt;
    uint32_t dashboardExternallyUpdatedAt;
#ifdef C1CAN
    uint32_t snsRequestOffAt;
    int8_t collectingMultiframeResponse;
#endif
    bool sleeping;
} BoardState;

typedef struct
{
    BoardState board;
    CarState car;
} GlobalState;

#ifdef C1CAN
typedef struct
{
    uint32_t bootCarouselDelay;
    uint32_t bootCarouselInterval;
    uint8_t bootCarouselLoops;
    bool bootCarouselEnabled;
    uint8_t favoriteItemsCount;
    uint8_t *favoriteItems;
    bool dpfNotifyWhenFinished;
} Settings;
#endif

#endif /* __MODEL_H */
