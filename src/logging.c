#include "logging.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static const char *llevels = "+#!";

void rr_log(rr_log_level level, const char *fmt, ...)
{
    FILE *fp = level > R_INFO ? stderr : stdout;
    fprintf(fp, "%lu %c ", time(NULL), llevels[level]);
    va_list args;
    va_start(args, fmt);
    vfprintf(fp, fmt, args);
    va_end(args);
    fprintf(fp, "\n");
}
