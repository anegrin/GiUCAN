#ifndef _LOGGING_H
#define _LOGGING_H

#include "config.h"
#include "usbd_cdc_if.h"

#ifdef DEBUG_MODE
#define LOGS(message) print_to_usb(message)
#define LOG(format, ...) printf_to_usb(format, ##__VA_ARGS__)
#ifdef VERBOSE
#define VLOG(format, ...) printf_to_usb(format, ##__VA_ARGS__)
#else
#define VLOG(...)
#endif
#else
#define LOG(...)
#define LOGS(message)
#define VLOG(...)
#endif

#endif
