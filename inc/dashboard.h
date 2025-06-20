#ifndef _DASHBOARD_H
#define _DASHBOARD_H

#include <stdbool.h>
#include <string.h>
#include "config.h"
#include "model.h"

#define A(x) x[4]
#define B(x) x[5]
#define C(x) x[6]
#define D(x) x[7]
#define E(x) x[8]

typedef float (*ExtractionFuncPtr)(GlobalState *state, uint8_t *rx_msg_data);

float noop_extract(GlobalState *state, uint8_t *rx_msg_data);

#ifdef XCAN
#ifndef DASHBOARD_ITEMS
/* item_type, pattern */
#define DASHBOARD_ITEMS                                        \
    X(FIRMWARE_ITEM, "GiUCAN " GIUCAN_VERSION)                 \
    X(UPTIME_ITEM, "Uptime: %.0fmin/%.0fmin")                  \
    X(HP_NM_ITEM, "Power: %.1fhp/%.0fnm")                      \
    X(DPF_STATUS_ITEM, "DPF status: %s")                       \
    X(DPF_CLOG_ITEM, "DPF clogging: %.0f%%")                   \
    X(DPF_TEMP_ITEM, "DPF temperature: %.0f"                   \
                     "\xB0"                                    \
                     "C")                                      \
    X(DPF_REG_ITEM, "DPF regeneration: %.0f%%")                \
    X(DPF_DIST_ITEM, "DPF distance: %.0fkm")                   \
    X(DPF_COUNT_ITEM, "DPF count: %.0f")                       \
    X(DPF_MEAN_DIST_DURATION_ITEM, "DPF mean: %.0fkm/%.0fmin") \
    X(BATTERY_V_A_ITEM, "Battery: %.1fV/%.2fA")                \
    X(BATTERY_P_ITEM, "Battery charge: %.0f%%")                \
    X(OIL_PRESS_ITEM, "Oil pressure: %.1fbar")                 \
    X(OIL_QUALITY_ITEM, "Oil quality: %.0f%%")                 \
    X(OIL_TEMP_ITEM, "Oil temperature: %.0f"                   \
                     "\xB0"                                    \
                     "C")                                      \
    X(COOLANT_TEMP_ITEM, "Coolant temperature: %.0f"           \
                         "\xB0"                                \
                         "C")                                  \
    X(AIR_IN_ITEM, "Air in temperature: %.0f"                  \
                   "\xB0"                                      \
                   "C")                                        \
    X(GEAR_ITEM, "Current gear: %c")                           \
    X(GEARBOX_TEMP_ITEM, "Gearbox temperature: %.0f"           \
                         "\xB0"                                \
                         "C")                                  \
    X(STEERING_ITEM, "Steering angle: %.1f"                    \
                     "\xB0")                                   \
    X(TIRES_TEMP_FRONT_ITEM, "%.0f"                            \
                             "\xB0"                            \
                             "C F.T. temp %.0f"                \
                             "\xB0"                            \
                             "C")                              \
    X(TIRES_TEMP_REAR_ITEM, "%.0f"                             \
                            "\xB0"                             \
                            "C R.T. temp %.0f"                 \
                            "\xB0"                             \
                            "C")
#endif

typedef enum
{
#define X(name, str) name,
    DASHBOARD_ITEMS
#undef X
        DASHBOARD_ITEMS_COUNT
} DashboardItemType;
#else
#define DASHBOARD_ITEMS_COUNT 0
#endif

#ifdef BHCAN

#define DASHBOARD_BUFFER_SIZE DASHBOARD_MESSAGE_MAX_LENGTH + 1

const char *pattern_of(DashboardItemType type);
void render_message(char *buffer, GlobalState *state);
// utility fn to optimize crazy nested ternary
const char *dpf_status_as_string(float value);

#ifndef CONVERTERS
/*
item_type, forV0_return_type, forV0_convert_function_code, forV1_return_type, forV1_convert_function_code

renders to

forV0_return_type item_type_V0Converter(float value)  {return forV0_convert_function_code;}
forV1_return_type item_type_V1Converter(float value)  {return forV1_convert_function_code;}

function render_message in bh_processing.c will call the function if needed
*/
#define CONVERTERS                                                             \
    X(DPF_STATUS_ITEM, const char *, dpf_status_as_string(value), bool, false) \
    X(GEAR_ITEM, char, ((unsigned char)value), bool, false)
#endif

#endif

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

#ifndef EXTRACTION_FUNCTIONS
/*
function_name, code

renders to

float function_name(GlobalState *s, uint8_t *r) { return code; }
*/
#define EXTRACTION_FUNCTIONS                                                      \
    X(extractTempCommon, ((float)(((A(r) * 256) + B(r))) * 0.02f) - 40.0f)        \
    X(extractCarUptime, ((float)((A(r) * 256) + B(r)) / 4.0f))                    \
    X(extractBoardUptime, (((float)s->board.now) / 60000.0f))                     \
    X(extractHP, ((float)s->car.torque - 500) * (float)s->car.rpm * 0.000142378f) \
    X(extractNM, (float)s->car.torque - 500)                                      \
    X(extractDpfStatus, (float)s->car.dpf.regenMode)                              \
    X(extractDpfClog, ((float)((A(r) * 256) + B(r))) * 0.01525902f)               \
    X(extractDpfReg, ((float)((A(r) * 256) + B(r))) * 0.001525902f)               \
    X(extractDpfDist, ((float)((A(r) * 65536) + (B(r) * 256) + C(r))) * 0.1)      \
    X(extractDpfCount, (float)((A(r) * 256) + B(r)))                              \
    X(extractDpfMeanDist, (float)((A(r) * 256) + B(r)))                           \
    X(extractDpfMeanDuration, (float)((A(r) * 256) + B(r)) / 60.0f)               \
    X(extractBatteryVolt, (float)((A(r) * 256) + B(r)) * 0.0005f)                 \
    X(extractBatteryPerc, (float)s->car.battery.chargePercent)                    \
    X(extractBatteryAmpere, (float)s->car.battery.current)                        \
    X(extractOilPressure, s->car.oil.pressure)                                    \
    X(extractOilQuality, ((float)((A(r) * 256) + B(r))) * 0.001525902f)           \
    X(extractOilTemp, (float)s->car.oil.temperature)                              \
    X(extractGearboxTemp, (float)A(r) - 40.0f)                                    \
    X(extractGear, (float)s->car.gear)                                            \
    X(extractSteeringAngle, ((float)((((int8_t)A(r)) * 256) + B(r))) / 16.0f)     \
    X(extractTireTemp, ((float)E(r) - 50.0f))
#endif

#ifndef EXTRACTORS
/*
item_type,
hasV0,
forV0_needsQuery,
forV0_query_reqId,
forV0_query_reqData,
forV0_extraction_function
hasV1,
forV1_needsQuery,
forV1_query_reqId,
forV1_query_reqData,
forV1_extraction_function
*/
#define EXTRACTORS                                                                                                                                     \
    X(UPTIME_ITEM, true, true, 0x18DA10F1, 0x03221009, extractCarUptime, true, false, 0, 0, extractBoardUptime)                                        \
    X(HP_NM_ITEM, true, false, 0, 0, extractHP, true, false, 0, 0, extractNM)                                                                          \
    X(DPF_STATUS_ITEM, true, false, 0, 0, extractDpfStatus, false, false, 0, 0, noop_extract)                                                          \
    X(DPF_CLOG_ITEM, true, true, 0x18DA10F1, 0x032218E4, extractDpfClog, false, false, 0, 0, noop_extract)                                             \
    X(DPF_TEMP_ITEM, true, true, 0x18DA10F1, 0x032218DE, extractTempCommon, false, false, 0, 0, noop_extract)                                          \
    X(DPF_REG_ITEM, true, true, 0x18DA10F1, 0x0322380B, extractDpfReg, false, false, 0, 0, noop_extract)                                               \
    X(DPF_DIST_ITEM, true, true, 0x18DA10F1, 0x03223807, extractDpfDist, false, false, 0, 0, noop_extract)                                             \
    X(DPF_COUNT_ITEM, true, true, 0x18DA10F1, 0x032218A4, extractDpfCount, false, false, 0, 0, noop_extract)                                           \
    X(DPF_MEAN_DIST_DURATION_ITEM, true, true, 0x18DA10F1, 0x03223809, extractDpfMeanDist, true, true, 0x18DA10F1, 0x0322380A, extractDpfMeanDuration) \
    X(BATTERY_V_A_ITEM, true, true, 0x18DA10F1, 0x03221955, extractBatteryVolt, true, false, 0, 0, extractBatteryAmpere)                               \
    X(BATTERY_P_ITEM, true, false, 0, 0, extractBatteryPerc, false, false, 0, 0, noop_extract)                                                         \
    X(OIL_PRESS_ITEM, true, false, 0, 0, extractOilPressure, false, false, 0, 0, noop_extract)                                                         \
    X(OIL_QUALITY_ITEM, true, true, 0x18DA10F1, 0x03223813, extractOilQuality, false, false, 0, 0, noop_extract)                                       \
    X(OIL_TEMP_ITEM, true, false, 0, 0, extractOilTemp, false, false, 0, 0, noop_extract)                                                              \
    X(COOLANT_TEMP_ITEM, true, true, 0x18DA10F1, 0x03221003, extractTempCommon, false, false, 0, 0, noop_extract)                                      \
    X(AIR_IN_ITEM, true, true, 0x18DA10F1, 0x03221935, extractTempCommon, false, false, 0, 0, noop_extract)                                            \
    X(GEAR_ITEM, true, false, 0, 0, extractGear, false, false, 0, 0, noop_extract)                                                                     \
    X(GEARBOX_TEMP_ITEM, true, true, 0x18DA18F1, 0x032204FE, extractGearboxTemp, false, false, 0, 0, noop_extract)                                     \
    X(STEERING_ITEM, true, true, 0x18DA2AF1, 0x0322083C, extractSteeringAngle, false, false, 0, 0, noop_extract)                                       \
    X(TIRES_TEMP_FRONT_ITEM, true, true, 0x18DAC7F1, 0x032240B1, extractTireTemp, true, true, 0x18DAC7F1, 0x032240B2, extractTireTemp)                 \
    X(TIRES_TEMP_REAR_ITEM, true, true, 0x18DAC7F1, 0x032240B3, extractTireTemp, true, true, 0x18DAC7F1, 0x032240B5, extractTireTemp)
#endif
#endif

#endif // _DASHBOARD_H
