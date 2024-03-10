#include "protocol.h"
#include "logging.h"
#include <string.h>
#include <time.h>

String_View string_view_from_parts(const char *src, size_t len) {
    String_View view = {.length = len, .p = src};
    return view;
}

String_View string_view_from_cstring(const char *src) {
    return string_view_from_parts(src, strlen(src));
}

String_View string_view_chop_by_delim(String_View *view, const char delim) {
    size_t i = 0;
    while (i < view->length && view->p[i] != delim) {
        i += 1;
    }

    String_View result = string_view_from_parts(view->p, i);

    if (i < view->length) {
        view->length -= i + 1;
        view->p += i + 1;
    } else {
        view->length -= i;
        view->p += i;
    }

    return result;
}

Command parse_command(const char *input) {
    Command cmd = {0};
    String_View tokens[4];
    String_View input_view = string_view_from_cstring(input);
    int i = 0;
    struct timespec tv;

    while (input_view.length > 0)
        tokens[i++] = string_view_chop_by_delim(&input_view, ' ');

    if (strncmp(tokens[0].p, "PING", 4) == 0) {
        cmd.type = PING;
    } else if (strncmp(tokens[0].p, "SELECT", 6) == 0) {
        cmd.type = SELECT;
        sscanf(tokens[2].p, "%lu", &cmd.timestamp);
    } else if (strncmp(tokens[0].p, "INSERT", 6) == 0) {
        cmd.type = INSERT;
        strncpy(cmd.metric, tokens[1].p, tokens[1].length);
        printf("token %s\n", cmd.metric);
        sscanf(tokens[2].p, "%lf", &cmd.value);
        if (i > 3) {
            sscanf(tokens[3].p, "%lu", &cmd.timestamp);
        } else {
            clock_gettime(CLOCK_REALTIME, &tv);
            cmd.timestamp = tv.tv_sec * 1e9 + tv.tv_nsec;
        }
    } else if (strncmp(tokens[0].p, "CREATE", 6) == 0) {
        cmd.type = CREATE;
        strncpy(cmd.metric, tokens[1].p, tokens[1].length);
    } else if (strncmp(tokens[0].p, "DELETE", 6) == 0) {
        cmd.type = DELETE;
        strncpy(cmd.metric, tokens[1].p, tokens[1].length);
    } else if (strncmp(tokens[0].p, "QUIT", 4) == 0) {
        cmd.type = QUIT;
    } else {
        // Invalid command type
        log_error("Invalid command");
    }

    return cmd;
}
