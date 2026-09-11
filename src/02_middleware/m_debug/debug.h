#ifndef __DEBUG_H__
#define __DEBUG_H__

#include <stdio.h>
#include "app_main.h"

void debug_print(const char *fmt, ...);

#if defined(DEBUG_ENABLE) && (DEBUG_ENABLE == 1)
#define DEBUG_LOG(fmt, ...) debug_print(fmt, ##__VA_ARGS__)
#else
#define DEBUG_LOG(fmt, ...) do {} while (0)
#endif

#endif /* __DEBUG_H__ */
