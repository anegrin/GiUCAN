#ifndef __MAIN_H
#define __MAIN_H

#include "stm32f0xx_hal.h"

#define PRINTF_INCLUDE_CONFIG_H

#ifdef INCLUDE_USER_CONFIG_H
#include "user_config.h"
#endif

#ifndef GIUCAN_VERSION
#ifdef GIT_VERSION
#define GIUCAN_VERSION GIT_VERSION
#else
#define GIUCAN_VERSION "dev"
#endif
#endif

#ifdef SLCAN
#ifdef BHCAN
#error "Can't build SLCAN+BHCAN"
#endif
#ifdef C1CAN
#error "Can't build SLCAN+C1CAN"
#endif
#ifdef DEBUG_MODE
#error "Can't build SLCAN+DEBUG_MODE"
#endif

#define LEDS_ON_CAN_RX
#define LEDS_ON_CAN_TX
#endif

#ifdef BHCAN
#define XCAN
#ifdef C1CAN
#error "Can't build BHCAN+C1CAN"
#endif
#ifndef GASOLINE_ENGINE
#ifndef DISABLE_DPF_REGEN_NOTIFICATIION
#define ENABLE_DPF_REGEN_NOTIFICATIION
#endif
#endif
// 0x02=FM radio, 0x03=AM Radio, 0x05=Aux, 0x06=left USB, 0x07=Right USB,
// 0x08=Center USB, 0x09=Bluetooth, 0x12=phone connected, 0x13=phone disconnected,
// 0x15=call in progress, 0x17=call in wait, 0x18=call terminated, 0x11=clear display, ...
#ifndef DISPLAY_INFO_CODE
#define DISPLAY_INFO_CODE 0x09
#endif
// must be a multiple of 3; suggested value for 7 inch is 24, for 3.5 inch is 18
#ifndef DASHBOARD_MESSAGE_MAX_LENGTH
#define DASHBOARD_MESSAGE_MAX_LENGTH 24
#endif
#ifndef DISABLE_DASHBOARD_FORCED_REFRESH
#define DASHBOARD_FORCED_REFRESH
#ifndef DASHBOARD_FORCED_REFRESH_MS
#define DASHBOARD_FORCED_REFRESH_MS 1000
#endif
#endif
#ifndef CAN_BITRATE
#define CAN_BITRATE CAN_BITRATE_125K
#endif
#endif

#ifdef C1CAN
#define XCAN
#ifndef DISABLE_DASHBOARD
#define ENABLE_DASHBOARD
#ifndef RES_LONG_PRESS_DURATION_MS
#define RES_LONG_PRESS_DURATION_MS 1000
#endif
#ifndef DASHBOARD_PAGE_SIZE
#define DASHBOARD_PAGE_SIZE 10
#endif
#endif
#ifndef DISABLE_SNS_AUTO_OFF
#define ENABLE_SNS_AUTO_OFF
#ifndef SNS_AUTO_OFF_DELAY_MS
#define SNS_AUTO_OFF_DELAY_MS 10000
#endif
#ifndef SNS_AUTO_OFF_MIN_RPM
#define SNS_AUTO_OFF_MIN_RPM 400
#endif
#endif
#ifndef ENABLE_DASHBOARD
#ifndef ENABLE_SNS_AUTO_OFF
#error "You're building a C1CAN without any active feature :/"
#endif
#endif
#ifndef VALUES_REFRESH_MS
#define VALUES_REFRESH_MS 250
#endif
#endif

#ifndef USART2_BAUD_RATE
#define USART2_BAUD_RATE 9600
#endif

#ifndef CAN_BITRATE
#define CAN_BITRATE CAN_BITRATE_500K
#endif

#if defined(SLCAN) || defined(DEBUG_MODE)
#define ENABLE_USB_PORT
#endif

#if DEBUG_MODE
#ifdef ENABLE_DASHBOARD
#warning "DEBUG_MODE+ENABLE_DASHBOARD might lead to malfunction because of UART communication"
#endif
#ifdef BHCAN
#warning "DEBUG_MODE+BHCAN might lead to malfunction because of UART communication"
#endif
#endif

#ifdef XCAN
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
#endif

#endif /* __MAIN_H */
