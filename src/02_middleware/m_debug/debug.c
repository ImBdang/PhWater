#include "debug.h"
#include <stdarg.h>
#include <string.h>

void debug_print(const char *fmt, ...)
{
    printf("[USER] ");

    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);

    size_t len = strlen(fmt);
    if (len > 0 && fmt[len - 1] != '\n')
    {
        printf("\r\n");
    }
}
