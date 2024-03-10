CC = gcc
CFLAGS = -Wall -Wextra -Werror -Wunused -std=c11 -pedantic -ggdb -fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer -pg -D_DEFAULT_SOURCE=200809L -Iinclude -Isrc
LDFLAGS = -L. -ltimeseries -fsanitize=address -fsanitize=undefined

LIB_SOURCES = src/timeseries.c src/partition.c src/wal.c src/disk_io.c src/binary.c src/logging.c src/persistent_index.c src/commit_log.c
LIB_OBJECTS = $(LIB_SOURCES:.c=.o)
LIB_PERSISTENCE = logdata

SERVER_SOURCES = src/main.c
# SERVER_SOURCES = src/*.c
SERVER_OBJECTS = $(SERVER_SOURCES:.c=.o)
SERVER_EXECUTABLE = server

all: libtimeseries.so $(SERVER_EXECUTABLE)

libtimeseries.so: $(LIB_OBJECTS)
	$(CC) -shared -o $@ $(LIB_OBJECTS)

$(SERVER_EXECUTABLE): $(SERVER_OBJECTS) libtimeseries.so
	$(CC) -o $@ $(SERVER_OBJECTS) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -fPIC -c $< -o $@

clean:
	rm -f $(LIB_OBJECTS) $(SERVER_OBJECTS) libtimeseries.so $(SERVER_EXECUTABLE)
	rm -rf $(LIB_PERSISTENCE) 2> /dev/null
