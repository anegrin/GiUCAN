```c
#define DASHBOARD_MESSAGE_MAX_LENGTH 18

#define DASHBOARD_ITEMS                            \
    X(FIRMWARE_ITEM, "GiUCAN " GIUCAN_VERSION)     \
    X(UPTIME_ITEM, "Uptime: %.0fmin")              \
    X(HP_ITEM, "Power: %.1fhp")                    \
    X(NM_ITEM, "Torque: %.0fnm")                   \
    X(DPF_STATUS_ITEM, "DPF status: %s")           \
    X(DPF_CLOG_ITEM, "DPF clog: %.0f%%")           \
    X(DPF_TEMP_ITEM, "DPF temp: %.0f"              \
                     "\xB0"                        \
                     "C")                          \
    X(DPF_REG_ITEM, "DPF ref: %.0f%%")             \
    X(DPF_DIST_ITEM, "DPF dist: %.0fkm")           \
    X(DPF_COUNT_ITEM, "DPF count: %.0f")           \
    X(DPF_MEAN_DIST_ITEM, "DPF mean: %.0fkm")      \
    X(DPF_MEAN_DURATION_ITEM, "DPF mean: %.0fmin") \
    X(BATTERY_V_ITEM, "Battery: %.1fV")            \
    X(BATTERY_A_ITEM, "Battery: %.2fA")            \
    X(BATTERY_P_ITEM, "Battery: %.0f%%")           \
    X(OIL_PRESS_ITEM, "Oil press: %.1fbar")        \
    X(OIL_QUALITY_ITEM, "Oil q: %.0f%%")           \
    X(OIL_TEMP_ITEM, "Oil temp: %.0f"              \
                     "\xB0"                        \
                     "C")                          \
    X(COOLANT_TEMP_ITEM, "Coolant temp: %.0f"      \
                         "\xB0"                    \
                         "C")                      \
    X(AIR_IN_ITEM, "Air in temp: %.0f"             \
                   "\xB0"                          \
                   "C")                            \
    X(GEAR_ITEM, "Gear: %c")                       \
    X(GEARBOX_TEMP_ITEM, "Gbox temp: %.0f"         \
                         "\xB0"                    \
                         "C")                      \
    X(STEERING_ITEM, "Steering: %.1f"              \
                     "\xB0")
#define CONVERTERS                                                             \
    X(DPF_STATUS_ITEM, const char *, dpf_status_as_string(value), bool, false) \
    X(GEAR_ITEM, char, ((unsigned char)value), bool, false)

#define EXTRACTION_FUNCTIONS                                                      \
    X(extractTempCommon, ((float)(((A(r) * 256) + B(r))) * 0.02f) - 40.0f)        \
    X(extractCarUptime, ((float)((A(r) * 256) + B(r)) / 4.0f))                    \
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
    X(extractSteeringAngle, ((float)((((int8_t)A(r)) * 256) + B(r))) / 16.0f)

#define EXTRACTORS                                                                                                          \
    X(UPTIME_ITEM, true, true, 0x18DA10F1, 0x03221009, extractCarUptime, false, false, 0, 0, noop_extract)                  \
    X(HP_ITEM, true, false, 0, 0, extractHP, false, false, 0, 0, noop_extract)                                              \
    X(NM_ITEM, true, false, 0, 0, extractNM, false, false, 0, 0, noop_extract)                                              \
    X(DPF_STATUS_ITEM, true, false, 0, 0, extractDpfStatus, false, false, 0, 0, noop_extract)                               \
    X(DPF_CLOG_ITEM, true, true, 0x18DA10F1, 0x032218E4, extractDpfClog, false, false, 0, 0, noop_extract)                  \
    X(DPF_TEMP_ITEM, true, true, 0x18DA10F1, 0x032218DE, extractTempCommon, false, false, 0, 0, noop_extract)               \
    X(DPF_REG_ITEM, true, true, 0x18DA10F1, 0x0322380B, extractDpfReg, false, false, 0, 0, noop_extract)                    \
    X(DPF_DIST_ITEM, true, true, 0x18DA10F1, 0x03223807, extractDpfDist, false, false, 0, 0, noop_extract)                  \
    X(DPF_COUNT_ITEM, true, true, 0x18DA10F1, 0x032218A4, extractDpfCount, false, false, 0, 0, noop_extract)                \
    X(DPF_MEAN_DIST_ITEM, true, true, 0x18DA10F1, 0x03223809, extractDpfMeanDist, false, false, 0, 0, noop_extract)         \
    X(DPF_MEAN_DURATION_ITEM, true, true, 0x18DA10F1, 0x0322380A, extractDpfMeanDuration, false, false, 0, 0, noop_extract) \
    X(BATTERY_V_ITEM, true, true, 0x18DA10F1, 0x03221955, extractBatteryVolt, false, false, 0, 0, noop_extract)             \
    X(BATTERY_A_ITEM, true, false, 0, 0, extractBatteryAmpere, false, false, 0, 0, noop_extract)                            \
    X(BATTERY_P_ITEM, true, false, 0, 0, extractBatteryPerc, false, false, 0, 0, noop_extract)                              \
    X(OIL_PRESS_ITEM, true, false, 0, 0, extractOilPressure, false, false, 0, 0, noop_extract)                              \
    X(OIL_QUALITY_ITEM, true, true, 0x18DA10F1, 0x03223813, extractOilQuality, false, false, 0, 0, noop_extract)            \
    X(OIL_TEMP_ITEM, true, false, 0, 0, extractOilTemp, false, false, 0, 0, noop_extract)                                   \
    X(COOLANT_TEMP_ITEM, true, true, 0x18DA10F1, 0x03221003, extractTempCommon, false, false, 0, 0, noop_extract)           \
    X(AIR_IN_ITEM, true, true, 0x18DA10F1, 0x03221935, extractTempCommon, false, false, 0, 0, noop_extract)                 \
    X(GEAR_ITEM, true, false, 0, 0, extractGear, false, false, 0, 0, noop_extract)                                          \
    X(GEARBOX_TEMP_ITEM, true, true, 0x18DA18F1, 0x032204FE, extractGearboxTemp, false, false, 0, 0, noop_extract)          \
    X(STEERING_ITEM, true, true, 0x18DA2AF1, 0x0322083C, extractSteeringAngle, false, false, 0, 0, noop_extract)
```
