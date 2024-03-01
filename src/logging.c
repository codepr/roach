#include "logging.h"
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void rr_log(rr_log_level level, const char *fmt, ...) {
    switch (level) {
    case R_INFO:
        fprintf(stderr, "[INFO] ");
        break;
    case R_WARNING:
        fprintf(stderr, "[WARNING] ");
        break;
    case R_ERROR:
        fprintf(stderr, "[ERROR] ");
        break;
    default:
        assert(0 && "unreachable");
    }

    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");
}
