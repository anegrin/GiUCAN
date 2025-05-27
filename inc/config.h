#ifndef __MAIN_H
#define __MAIN_H

#include "stm32f0xx_hal.h"

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
    #ifdef UART_DEBUG_MODE
        #error "Can't build BHCAN+UART_DEBUG_MODE"
    #endif

    #define CAN_BITRATE CAN_BITRATE_125K
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
    #ifndef DISABLE_DPF_REGEN_NOTIFICATIION
        #define ENABLE_DPF_REGEN_NOTIFICATIION
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
    #ifndef ENABLE_DPF_REGEN_NOTIFICATIION
    #ifndef ENABLE_SNS_AUTO_OFF
        #error "You're building a C1CAN without any active feature :/"
    #endif
    #endif
    #endif
#endif

#ifndef USART2_BAUD_RATE
    #define USART2_BAUD_RATE 38400
#endif

#ifndef CAN_BITRATE
    #define CAN_BITRATE CAN_BITRATE_500K
#endif

#if defined(SLCAN) || defined(DEBUG_MODE)
    #define ENABLE_USB_PORT
#endif

#ifdef DEBUG_MODE
    #define LEDS_ON_CAN_RX
#endif

#endif /* __MAIN_H */
