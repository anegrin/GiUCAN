#ifndef __CONFIG_H
#define __CONFIG_H

#ifdef __cplusplus
extern "C"
{
#endif

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
#endif

#ifdef BHCAN
    #ifdef C1CAN
        #error "Can't build BHCAN+C1CAN"
    #endif
    #ifdef ECHO_MODE
        #error "Can't build BHCAN+ECHO_MODE"
    #endif
#endif

#ifdef C1CAN
    #ifdef ECHO_MODE
        #error "Can't build C1CAN+ECHO_MODE"
    #endif
#endif

#ifdef BHCAN
    #define CAN_BITRATE CAN_BITRATE_125K
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

#ifdef __cplusplus
}
#endif

#endif /* __CONFIG_H */
