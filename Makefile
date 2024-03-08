CFLAGS=-Wall -Wextra -Werror -Wunused -std=c11 -pedantic -ggdb -fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer -pg -D_DEFAULT_SOURCE=200809L

all: roach

roach: src/*.c
	$(CC) $(CFLAGS) -o roach src/*.c

clean:
	rm -f roach logdata/tsdata/*.log logdata/tsdata/*.index logdata/tsdata/timestamps.ts
