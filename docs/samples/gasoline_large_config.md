```c
#define DISABLE_DPF_REGEN_NOTIFICATIION

#define DASHBOARD_ITEMS                              \
    X(FIRMWARE_ITEM, "GiUCAN " GIUCAN_VERSION)       \
    X(UPTIME_ITEM, "Uptime: %.0fmin/%.0fmin")        \
    X(HP_NM_ITEM, "Power: %.1fhp/%.0fnm")            \
    X(BATTERY_V_A_ITEM, "Battery: %.1fV/%.2fA")      \
    X(BATTERY_P_ITEM, "Battery charge: %.0f%%")      \
    X(OIL_TEMP_ITEM, "Oil temperature: %.0f"         \
                     "\xB0"                          \
                     "C")                            \
    X(COOLANT_TEMP_ITEM, "Coolant temperature: %.0f" \
                         "\xB0"                      \
                         "C")                        \
    X(AIR_IN_ITEM, "Air in temperature: %.0f"        \
                   "\xB0"                            \
                   "C")                              \
    X(GEAR_ITEM, "Current gear: %c")                 \
    X(GEARBOX_TEMP_ITEM, "Gearbox temperature: %.0f" \
                         "\xB0"                      \
                         "C")                        \
    X(STEERING_ITEM, "Steering angle: %.1f"          \
                     "\xB0")
#define CONVERTERS \
    X(GEAR_ITEM, char, ((unsigned char)value), bool, false)

#define EXTRACTION_FUNCTIONS                                                      \
    X(extractTempCommon, ((float)(((A(r) * 256) + B(r))) * 0.02f) - 40.0f)        \
    X(extractCarUptime, ((float)((A(r) * 256) + B(r)) / 4.0f))                    \
    X(extractBoardUptime, (((float)s->board.now) / 60000.0f))                     \
    X(extractHP, ((float)s->car.torque - 500) * (float)s->car.rpm * 0.000142378f) \
    X(extractNM, (float)s->car.torque - 500)                                      \
    X(extractBatteryVolt, (float)((A(r) * 256) + B(r)) * 0.0005f)                 \
    X(extractBatteryPerc, (float)s->car.battery.chargePercent)                    \
    X(extractBatteryApere, (float)s->car.battery.current)                         \
    X(extractOilTemp, (float)s->car.oil.temperature)                              \
    X(extractGearboxTemp, (float)A(r) - 40.0f)                                    \
    X(extractGear, (float)s->car.gear)                                            \
    X(extractSteeringAngle, ((float)((((int8_t)A(r)) * 256) + B(r))) / 16.0f)

#define EXTRACTORS                                                                                                                     \
    X(UPTIME_ITEM, true, true, 0x18DA10F1, 0x03221009, 0x18DAF110, extractCarUptime, true, false, 0, 0, 0, extractBoardUptime)         \
    X(HP_NM_ITEM, true, false, 0, 0, 0, extractHP, true, false, 0, 0, 0, extractNM)                                                    \
    X(BATTERY_V_A_ITEM, true, true, 0x18DA10F1, 0x03221955, 0x18DAF110, extractBatteryVolt, true, false, 0, 0, 0, extractBatteryApere) \
    X(BATTERY_P_ITEM, true, false, 0, 0, 0, extractBatteryPerc, false, false, 0, 0, 0, noop_extract)                                   \
    X(OIL_TEMP_ITEM, true, false, 0, 0, 0, extractOilTemp, false, false, 0, 0, 0, noop_extract)                                        \
    X(COOLANT_TEMP_ITEM, true, true, 0x18DA10F1, 0x03221003, 0x18DAF110, extractTempCommon, false, false, 0, 0, 0, noop_extract)       \
    X(AIR_IN_ITEM, true, true, 0x18DA10F1, 0x03221935, 0x18DAF110, extractTempCommon, false, false, 0, 0, 0, noop_extract)             \
    X(GEAR_ITEM, true, false, 0, 0, 0, extractGear, false, false, 0, 0, 0, noop_extract)                                               \
    X(GEARBOX_TEMP_ITEM, true, true, 0x18DA18F1, 0x032204FE, 0x18DAF118, extractGearboxTemp, false, false, 0, 0, 0, noop_extract)      \
    X(STEERING_ITEM, true, true, 0x18DA2AF1, 0x0322083C, 0x18DAF12A, extractSteeringAngle, false, false, 0, 0, 0, noop_extract)
```
