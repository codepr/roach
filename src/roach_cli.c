#include "client.h"
#include "protocol.h"
#include <errno.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define LOCALHOST    "127.0.0.1"
#define DEFAULT_PORT 17678

static const char *cmd_usage(const char *cmd)
{
    if (strncasecmp(cmd, "create", 6) == 0)
        return "CREATE <database-name>|<timeseries-name> [INTO <database "
               "name>] [retention] [dup policy]";
    if (strncasecmp(cmd, "delete", 6) == 0)
        return "DELETE <database-name>|<timeseries-name>";
    if (strncasecmp(cmd, "insert", 3) == 0)
        return "INSERT <timeseries-name> INTO <database-name> timestamp|* "
               "value, ..";
    if (strncasecmp(cmd, "select", 5) == 0)
        return "SELECT <timeseries-name> FROM <database-name> [RANGE|AT] "
               "start_timestamp [end_timestamp] [WHERE] <identifier> "
               "[<|>|<=|>=|=|!=] [AGGREGATE] [MIN|MAX|AVG] [BY literal]";
    return NULL;
}

static double timespec_seconds(struct timespec *ts)
{
    return (double)ts->tv_sec + (double)ts->tv_nsec * 1.0e-9;
}

static void prompt(Client *c)
{
    if (c->opts->s_family == AF_INET)
        printf("%s:%i> ", c->opts->s_addr, c->opts->s_port);
    else if (c->opts->s_family == AF_UNIX)
        printf("%s> ", c->opts->s_addr);
}

static void print_response(const Response *rs)
{
    if (rs->type == STRING_RSP) {
        printf("%s\n", rs->string_response.message);
    } else {
        for (size_t i = 0; i < rs->array_response.length; ++i)
            printf("%" PRIu64 " %.6f\n",
                   rs->array_response.records[i].timestamp,
                   rs->array_response.records[i].value);
    }
}

int main(void)
{
    int port = DEFAULT_PORT, mode = AF_INET;
    char *host      = LOCALHOST;
    size_t line_len = 0LL;
    char *line      = NULL;
    Response rs;
    double delta = 0.0;

    Client c;
    struct connect_options conn_opts;
    memset(&conn_opts, 0x00, sizeof(conn_opts));
    conn_opts.s_family = mode;
    conn_opts.s_addr   = host;
    conn_opts.s_port   = port;
    client_init(&c, &conn_opts);
    if (client_connect(&c) < 0)
        exit(EXIT_FAILURE);
    int err                    = 0;
    struct timespec start_time = {0}, end_time = {0};
    while (1) {
        prompt(&c);
        getline(&line, &line_len, stdin);
        (void)clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start_time);
        err = client_send_command(&c, line);
        if (err <= 0) {
            if (err == CLIENT_SUCCESS) {
                client_disconnect(&c);
                break;
            } else if (err == CLIENT_UNKNOWN_CMD) {
                printf("Unknown command or malformed one\n");
                const char *usage = cmd_usage(line);
                if (usage)
                    printf("\nSuggesed usage: %s\n\n", usage);
            } else if (err == CLIENT_FAILURE) {
                printf("Couldn't send the command: %s\n", strerror(errno));
            }
            continue;
        }
        client_recv_response(&c, &rs);
        (void)clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end_time);
        print_response(&rs);
        if (rs.type == ARRAY_RSP) {
            delta = timespec_seconds(&end_time) - timespec_seconds(&end_time);
            printf("%lu results in %lf seconds.\n", rs.array_response.length,
                   delta);
        }
    }
    client_disconnect(&c);
    free(line);
    return 0;
}
