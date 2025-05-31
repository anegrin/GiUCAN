#ifndef _DASHBOARD_H
#define _DASHBOARD_H

#include "config.h"

#ifndef DASHBOARD_ITEMS
#define DASHBOARD_ITEMS                            \
    X(FIRMWARE_ITEM, "GiUCAN " GIUCAN_VERSION)     \
    X(HP_ITEM, "Power: %.1fhp")                    \
    X(TORQUE_ITEM, "Torque: %.0fnm")               \
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

typedef enum
{
#define X(name, str) name,
    DASHBOARD_ITEMS
#undef X
        DASHBOARD_ITEM_COUNT
} DashboardItemType;

#ifdef BHCAN
const char *pattern_of(DashboardItemType type);
#endif

#endif // _DASHBOARD_H
