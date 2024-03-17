#include <stdio.h>
#define EV_SOURCE
#define EV_TCP_SOURCE
#include "ev_tcp.h"
#include "logging.h"
#include "parser.h"
#include "protocol.h"
#include "server.h"
#include "timeseries.h"

#define BACKLOG 128

// testing dummy
static Timeseries_DB *db;

static void execute_statement(Statement *statement) {
    Record r = {0};
    Timeseries *ts = NULL;
    struct timespec tv;

    switch (statement->type) {
    case STATEMENT_CREATE:
        if (statement->create.mask == 0)
            db = tsdb_init(statement->create.db_name);
        else
            (void)ts_create(db, statement->create.ts_name, 0, DP_IGNORE);
        break;
    case STATEMENT_INSERT:
        ts = ts_get(db, statement->insert.ts_name);
        uint64_t timestamp = 0;
        for (size_t i = 0; i < statement->insert.record_len; ++i) {
            if (statement->insert.records[i].timestamp == -1) {
                clock_gettime(CLOCK_REALTIME, &tv);
                timestamp = tv.tv_sec * 1e9 + tv.tv_nsec;
            } else {
                timestamp = statement->insert.records[i].timestamp;
            }
            ts_insert(ts, timestamp, statement->insert.records[i].value);
        }
        break;
    case STATEMENT_SELECT:
        ts = ts_get(db, statement->select.ts_name);
        int err = 0;
        Points coll;
        vec_new(coll);

        if (statement->select.mask & SM_SINGLE) {
            err = ts_find(ts, statement->select.start_time, &r);
            if (err < 0)
                log_error("Couldn't find the record %lu",
                          statement->select.start_time);
            else
                log_info("Record found: %lu %.2lf", r.timestamp, r.value);
        } else if (statement->select.mask & SM_RANGE) {
            err = ts_range(ts, statement->select.start_time,
                           statement->select.end_time, &coll);
            if (err < 0) {
                log_error("Couldn't find the record %lu",
                          statement->select.start_time);
            } else {
                for (size_t i = 0; i < vec_size(coll); i++) {
                    Record r = vec_at(coll, i);
                    log_info(" %lu {.sec: %lu, .nsec: %lu .value: %.02f }",
                             r.timestamp, r.tv.tv_sec, r.tv.tv_nsec, r.value);
                }
            }
        }
        break;
    default:
        log_error("Unknown command");
        break;
    }

    if (ts)
        ts_close(ts);
}

static void on_close(ev_tcp_handle *client, int err) {
    (void)client;
    if (err == EV_TCP_SUCCESS)
        printf("Closed connection with %s:%i\n", client->addr, client->port);
    else
        printf("Connection closed: %s\n", ev_tcp_err(err));
    free(client);
}

static void on_write(ev_tcp_handle *client) {
    printf("Written %lu bytes to %s:%i\n", client->to_write, client->addr,
           client->port);
}

static void on_data(ev_tcp_handle *client) {
    if (client->buffer.size == 0)
        return;
    log_info("Data: %s", client->buffer.buf);
    Request rq;
    Response rs;
    ssize_t n = decode_request((const uint8_t *)client->buffer.buf, &rq);
    if (n < 0) {
        log_error("Can't decode a request from data");
        rs.type = STRING_RSP;
        rs.string_response.rc = 1;
        strncpy(rs.string_response.message, "Err", 4);
        rs.string_response.length = 4;
    } else {
        // Parse into Statement
        size_t total_tokens;
        Token *tokens = tokenize(rq.query, &total_tokens);
        Statement statement = parse(tokens, total_tokens);
        // Execute it
        execute_statement(&statement);
        rs.type = STRING_RSP;
        rs.string_response.rc = 0;
        strncpy(rs.string_response.message, "Ok", 3);
        rs.string_response.length = 3;
    }

    (void)encode_response(&rs, (uint8_t *)client->buffer.buf);
    /* line_start = line_end + 2; */

    /* char *line_start = client->buffer.buf; */
    /* char *line_end; */

    /* while ((line_end = strstr(line_start, "\r\n")) != NULL) { */
    /*     *line_end = '\0'; */
    /*     log_info("Line received %s", line_start); */
    /*     // Line gets processed here */
    /* } */
    /* } */

    ev_tcp_queue_write(client);
}

static void on_connection(ev_tcp_handle *server) {
    int err = 0;
    ev_tcp_handle *client = malloc(sizeof(*client));
    if ((err = ev_tcp_server_accept(server, client, on_data, on_write)) < 0) {
        fprintf(stderr, "Error occured: %s\n",
                err == -1 ? strerror(errno) : ev_tcp_err(err));
        free(client);
    } else {
        printf("New connection from %s:%i\n", client->addr, client->port);
        ev_tcp_handle_set_on_close(client, on_close);
    }
}

int roachdb_server_run(const char *host, int port) {
    db = tsdb_init("testdb");
    ev_context *ctx = ev_get_context();
    ev_tcp_server server;
    ev_tcp_server_init(&server, ctx, BACKLOG);
    int err = ev_tcp_server_listen(&server, host, port, on_connection);
    if (err < 0) {
        if (err == -1)
            fprintf(stderr, "Error occured: %s\n", strerror(errno));
        else
            fprintf(stderr, "Error occured: %s\n", ev_tcp_err(err));
    }

    printf("Listening on %s:%i\n", host, port);

    // Blocking call
    ev_tcp_server_run(&server);

    // This could be registered to a SIGINT|SIGTERM signal notification
    // to stop the server with Ctrl+C
    ev_tcp_server_stop(&server);

    tsdb_close(db);

    return 0;
}
