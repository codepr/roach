#ifndef CLIENT_H
#define CLIENT_H

#include <netdb.h>
#include <stdio.h>

#define CLIENT_SUCCESS 0
#define CLIENT_FAILURE -1
#define CLIENT_UNKNOWN_CMD -2

typedef struct response Response;

typedef struct client Client;

/*
 * Connection options, use this structure to specify connection related opts
 * like socket family, host port and timeout for communication
 */
struct connect_options {
    int timeout;
    int s_family;
    int s_port;
    char *s_addr;
};

/*
 * Pretty basic connection wrapper, just a FD with a buffer tracking bytes and
 * some options for connection
 */
struct client {
    int fd;
    const struct connect_options *opts;
};

void client_init(Client *c, const struct connect_options *opts);

int client_connect(Client *c);

void client_disconnect(Client *c);

int client_send_command(Client *c, char *buf);

int client_recv_response(Client *c, Response *rs);

#endif // CLIENT_H
