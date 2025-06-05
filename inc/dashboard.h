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
    DPF_STATUS_ITEM,
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

#ifdef XCAN
#ifndef DASHBOARD_ITEMS
#define DASHBOARD_ITEMS                            \
    X(FIRMWARE_ITEM, "GiUCAN " GIUCAN_VERSION)     \
    X(HP_ITEM, "Power: %.1fhp")                    \
    X(TORQUE_ITEM, "Torque: %.0fnm")               \
    X(DPF_STATUS_ITEM, "DPF status: %s")           \
    X(DPF_CLOG_ITEM, "DPF clog: %.0f%%")           \
    X(DPF_TEMP_ITEM, "DPF temp: %.0f"              \
                     "\xB0"                        \
                     "C")                          \
    X(DPF_REG_ITEM, "DPF reg: %.0f%%")             \
    X(DPF_DIST_ITEM, "DPF dist: %.0fkm")           \
    X(DPF_COUNT_ITEM, "DPF count: %.0f")           \
    X(DPF_MEAN_DIST_ITEM, "DPF mean: %.0fkm")      \
    X(DPF_MEAN_DURATION_ITEM, "DPF mean: %.0fmin") \
    X(BATTERY_V_ITEM, "Battery: %.1fV")            \
    X(BATTERY_P_ITEM, "Battery: %.0f%%")           \
    X(BATTERY_A_ITEM, "Battery: %.2fA")            \
    X(OIL_QUALITY_ITEM, "Oil qlt: %.0f%%")         \
    X(OIL_TEMP_ITEM, "Oil temp: %.0f"              \
                     "\xB0"                        \
                     "C")                          \
    X(OIL_PRESS_ITEM, "Oil press: %.1fbar")        \
    X(AIR_IN_ITEM, "Air in temp: %.0f"             \
                   "\xB0"                          \
                   "C")                            \
    X(GEAR_ITEM, "Current gear: %c")               \
    X(GEARBOX_TEMP_ITEM, "Gearbox: %.0f"           \
                         "\xB0"                    \
                         "C")                      \
    X(FRONT_TIRES_TEMP_ITEM, "%.0f"                \
                             "\xB0"                \
                             "C F.T. %.0f"         \
                             "\xB0"                \
                             "C")                  \
    X(REAR_TIRES_TEMP, "%.0f"                      \
                       "\xB0"                      \
                       "C R.T. %.0f"               \
                       "\xB0"                      \
                       "C")
#endif
#endif
#endif // _DASHBOARD_H
