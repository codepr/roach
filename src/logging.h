#ifndef LOGGING_H
#define LOGGING_H

// Basic log utility function

typedef enum {
    R_INFO,
    R_WARNING,
    R_ERROR,
} rr_log_level;

#define r_log(...)     rr_log(__VA_ARGS__)
#define log_info(...)  r_log(R_INFO, __VA_ARGS__)
#define log_warn(...)  r_log(R_WARNING, __VA_ARGS__)
#define log_error(...) r_log(R_ERROR, __VA_ARGS__)

void rr_log(rr_log_level level, const char *fmt, ...);

#endif
