#ifndef __MAIN_H
#define __MAIN_H

#include "stm32f0xx_hal.h"

#ifdef INCLUDE_USER_CONFIG_H
    #include "user_config.h"
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
    #ifdef C1CAN
        #error "Can't build BHCAN+C1CAN"
    #endif
    #ifdef ECHO_MODE
        #error "Can't build BHCAN+ECHO_MODE"
    #endif

    #define CAN_BITRATE CAN_BITRATE_125K
#endif

#ifdef C1CAN
    #ifdef ECHO_MODE
        #error "Can't build C1CAN+ECHO_MODE"
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
#endif

#ifndef CAN_BITRATE
    #define CAN_BITRATE CAN_BITRATE_500K
#endif

#ifndef SLCAN
    #define ENABLE_CAN_AT_BOOT
#endif

#if defined(SLCAN) || defined(DEBUG_MODE)
    #define ENABLE_USB_PORT
#endif

#ifdef DEBUG_MODE
    #define LEDS_ON_CAN_RX
#endif

void Error_Handler(void);

#endif /* __MAIN_H */
