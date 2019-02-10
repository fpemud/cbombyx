/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */

#include <stdio.h>
#include <stdarg.h>
#include "util-stdio.h"

/* success returns valid pointer, failure returns NULL */
/* success:
 * 1. snprintf success
 * 2. buf has enough length
 */
char *util_snprintf(char *str, size_t size, const char *format, ...)
{
    va_list args;
    int ret;

    va_start(args, format);
    ret = vsnprintf(str, size, format, args);
    va_end(args);

    if (ret < 0 || ret >= size) {
        return NULL;
    }
    return str;
}
