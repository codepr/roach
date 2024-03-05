#ifndef WAL_H
#define WAL_H

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define WAL_PATH_SIZE 512

typedef struct wal {
    FILE *fp;
    char path[WAL_PATH_SIZE];
    size_t size;
} Wal;

int wal_init(Wal *w, const char *path, uint64_t base_timestamp, int main);

int wal_from_disk(Wal *w, const char *path, uint64_t base_timestamp, int main);

int wal_delete(Wal *w);

int wal_append_record(Wal *wal, uint64_t ts, double_t value);

size_t wal_size(const Wal *wal);

#endif
