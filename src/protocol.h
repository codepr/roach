#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <math.h>
#include <stdint.h>
#include <stdio.h>

#define METRIC_MAX_LEN 1 << 9

// String view APIs definition
// A string view is merely a pointer to an existing string, or better, to a
// slice of an existing string (which may be of entire target string length),
// thus it's not nul-terminated

typedef struct {
    size_t length;
    const char *p;
} String_View;

String_View string_view_from_parts(const char *src, size_t len);

String_View string_view_from_cstring(const char *src);

String_View string_view_chop_by_delim(String_View *view, const char delim);

typedef enum { PING, CREATE, INSERT, DELETE, SELECT, UNKNOWN } Command_Type;

// CREATE, INSERT, SELECT
typedef struct {
    Command_Type type;
    char metric[METRIC_MAX_LEN];
    uint64_t timestamp;
    double_t value;
} Command;

Command parse_command(const char *input);

#endif
