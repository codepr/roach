CFLAGS=-Wall -Wextra -std=c11 -pedantic -ggdb -D_DEFAULT_SOURCE=200809L

all: roach

roach: src/*.c
	$(CC) $(CFLAGS) -o roach src/*.c

clean:
	rm -f roach
