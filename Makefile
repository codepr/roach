CFLAGS=-Wall -Wextra -std=c11 -pedantic -ggdb

all: roach

roach: src/*.c
	$(CC) $(CFLAGS) -o roach src/*.c

clean:
	rm -f roach
