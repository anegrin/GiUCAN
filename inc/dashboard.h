#ifndef _DASHBOARD_H
#define _DASHBOARD_H

#include <stdbool.h>
#include "config.h"
#include "model.h"

#ifdef XCAN

typedef enum
{
    UNKNOWN_ITEM,
    FIRMWARE_ITEM,
    HP_ITEM,
    TORQUE_ITEM,
    DPF_CLOG_ITEM,
    DPF_TEMP_ITEM,
    DPF_REG_ITEM,
    DPF_DIST_ITEM,
    DPF_COUNT_ITEM,
    DPF_MEAN_DIST_ITEM,
    DPF_MEAN_DURATION_ITEM,
    BATTERY_V_ITEM,
    BATTERY_P_ITEM,
    BATTERY_A_ITEM,
    OIL_QUALITY_ITEM,
    OIL_TEMP_ITEM,
    OIL_PRESS_ITEM,
    AIR_IN_ITEM,
    GEAR_ITEM,
    GEARBOX_TEMP_ITEM,
    FRONT_TIRES_TEMP_ITEM,
    REAR_TIRES_TEMP,
} DashboardItemType;

const DashboardItemType type_of(uint8_t index);

#endif

#ifdef BHCAN
const char *pattern_of(DashboardItemType type);
#endif

uint8_t count_dashboard_items(void);

#define SWAP_UINT32(x) (((uint32_t)(x) >> 24) & 0x000000FF) |    \
                           (((uint32_t)(x) >> 8) & 0x0000FF00) | \
                           (((uint32_t)(x) << 8) & 0x00FF0000) | \
                           (((uint32_t)(x) << 24) & 0xFF000000)

#define A(x) x[4]
#define B(x) x[5]
#define C(x) x[6]
#define D(x) x[7]

typedef float (*ExtractionFuncPtr)(GlobalState *state, uint8_t *rx_msg_data);

#ifdef C1CAN
typedef struct
{
    uint32_t reqId;
    uint32_t reqData;
    uint32_t replyId;
} CANQuery;

typedef struct
{
    bool needsQuery;
    CANQuery query;
    ExtractionFuncPtr extract;
} CarValueExtractor;

typedef struct
{
    bool hasV0;
    bool hasV1;
    CarValueExtractor forV0;
    CarValueExtractor forV1;

} CarValueExtractors;

CarValueExtractors extractor_of(DashboardItemType type, GlobalState *state);
#endif

#endif // _DASHBOARD_H
