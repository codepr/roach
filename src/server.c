#include <stdio.h>
#define EV_SOURCE
#define EV_TCP_SOURCE
#include "ev_tcp.h"
#include "protocol.h"
#include "server.h"

#define BACKLOG 128

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
    printf("Data received (%lu)\n", client->buffer.size);
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

    return 0;
}
