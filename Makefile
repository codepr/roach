CC = gcc
CFLAGS = -Wall -Wextra -Werror -Wunused -std=c11 -pedantic -ggdb -fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer -pg -D_DEFAULT_SOURCE=200809L -Iinclude -Isrc
LDFLAGS = -L. -ltimeseries -fsanitize=address -fsanitize=undefined
LDFLAGS_CLI = -fsanitize=address -fsanitize=undefined

LIB_SOURCES = src/timeseries.c src/partition.c src/wal.c src/disk_io.c src/binary.c src/logging.c src/persistent_index.c src/commit_log.c
LIB_OBJECTS = $(LIB_SOURCES:.c=.o)
LIB_PERSISTENCE = logdata

SERVER_SOURCES = src/main.c src/parser.c src/protocol.c src/server.c src/ev.h src/ev_tcp.h
SERVER_OBJECTS = $(SERVER_SOURCES:.c=.o)
SERVER_EXECUTABLE = roach-server

CLI_SOURCES = src/client.c src/roach_cli.c src/protocol.c src/ev.h src/ev_tcp.h
CLI_OBJECTS = $(CLI_SOURCES:.c=.o)
CLI_EXECUTABLE = roach-cli

all: libtimeseries.so $(SERVER_EXECUTABLE) $(CLI_EXECUTABLE)

libtimeseries.so: $(LIB_OBJECTS)
	$(CC) -shared -o $@ $(LIB_OBJECTS)

$(SERVER_EXECUTABLE): $(SERVER_OBJECTS) libtimeseries.so
	$(CC) -o $@ $(SERVER_OBJECTS) $(LDFLAGS)

$(CLI_EXECUTABLE): $(CLI_OBJECTS)
	$(CC) -o $@ $(CLI_OBJECTS) $(LDFLAGS_CLI)

%.o: %.c
	$(CC) $(CFLAGS) -fPIC -c $< -o $@

clean:
	@rm -f $(LIB_OBJECTS) $(SERVER_OBJECTS) libtimeseries.so $(SERVER_EXECUTABLE)
	@rm -rf $(LIB_PERSISTENCE) 2> /dev/null
