#ifndef WAL_H
#define WAL_H

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct wal {
    FILE *fp;
    size_t size;
} Wal;

int wal_init(Wal *w, const char *path, uint64_t base_timestamp, int main);

int wal_from_disk(Wal *w, const char *path, uint64_t base_timestamp, int main);

int wal_append_record(Wal *wal, uint64_t ts, double_t value);

#endif
