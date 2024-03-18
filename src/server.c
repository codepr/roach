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

static Response execute_statement(const Statement *statement) {
    Response rs;
    Record r = {0};
    Timeseries *ts = NULL;
    struct timespec tv;

    switch (statement->type) {
    case STATEMENT_CREATE:
        if (statement->create.mask == 0)
            db = tsdb_init(statement->create.db_name);
        else
            (void)ts_create(db, statement->create.ts_name, 0, DP_IGNORE);
        rs.type = STRING_RSP;
        rs.string_response.rc = 0;
        strncpy(rs.string_response.message, "Ok", 3);
        rs.string_response.length = 3;
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
        rs.string_response.rc = 0;
        strncpy(rs.string_response.message, "Ok", 3);
        rs.string_response.length = 3;
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
            rs.array_response.length = 1;
            rs.array_response.records =
                calloc(1, sizeof(*rs.array_response.records));
            rs.array_response.records[0].timestamp = r.timestamp;
            rs.array_response.records[0].value = r.value;
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
            rs.array_response.length = vec_size(coll);
            for (size_t i = 0; i < vec_size(coll); ++i) {
                rs.array_response.records[i].timestamp = r.timestamp;
                rs.array_response.records[i].value = r.value;
            }
        }
        break;
    default:
        log_error("Unknown command");
        break;
    }

    if (ts)
        ts_close(ts);

    return rs;
}

static void on_close(ev_tcp_handle *client, int err) {
    (void)client;
    if (err == EV_TCP_SUCCESS)
        log_info("Closed connection with %s:%i", client->addr, client->port);
    else
        log_info("Connection closed: %s", ev_tcp_err(err));
    free(client);
}

static void on_write(ev_tcp_handle *client) {
    log_info("Written %lu bytes to %s:%i", client->to_write, client->addr,
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
        Statement statement = parse(rq.query);
        // Execute it
        rs = execute_statement(&statement);
    }

    (void)encode_response(&rs, (uint8_t *)client->buffer.buf);

    ev_tcp_queue_write(client);
}

static void on_connection(ev_tcp_handle *server) {
    int err = 0;
    ev_tcp_handle *client = malloc(sizeof(*client));
    if ((err = ev_tcp_server_accept(server, client, on_data, on_write)) < 0) {
        log_error("Error occured: %s",
                  err == -1 ? strerror(errno) : ev_tcp_err(err));
        free(client);
    } else {
        log_info("New connection from %s:%i", client->addr, client->port);
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
            log_error("Error occured: %s", strerror(errno));
        else
            log_error("Error occured: %s", ev_tcp_err(err));
    }

    log_info("Listening on %s:%i", host, port);

    // Blocking call
    ev_tcp_server_run(&server);

    // This could be registered to a SIGINT|SIGTERM signal notification
    // to stop the server with Ctrl+C
    ev_tcp_server_stop(&server);

    tsdb_close(db);

    return 0;
}
