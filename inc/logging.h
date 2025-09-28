#ifndef _LOGGING_H
#define _LOGGING_H

#include "config.h"

#ifdef DEBUG_MODE
#ifdef SLCAN
#include "uart.h"
#define LOGS(message) print_to_uart(message)
#define LOG(format, ...) printf_to_uart(format, ##__VA_ARGS__)
#ifdef VERBOSE
#define VLOG(format, ...) printf_to_uart(format, ##__VA_ARGS__)
#else
#define VLOG(...)
#endif
#endif
#ifdef XCAN
#include "usbd_cdc_if.h"
#define LOGS(message) print_to_usb(message)
#define LOG(format, ...) printf_to_usb(format, ##__VA_ARGS__)
#ifdef VERBOSE
#define VLOG(format, ...) printf_to_usb(format, ##__VA_ARGS__)
#else
#define VLOG(...)
#endif
#endif
#else
#define LOG(...)
#define LOGS(message)
#define VLOG(...)
#endif

#endif
