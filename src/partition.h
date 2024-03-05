#ifndef PARTITION_H
#define PARTITION_H

#include "commit_log.h"
#include "persistent_index.h"

typedef struct timeseries_chunk Timeseries_Chunk;

typedef struct partition {
    Commit_Log clog;
    Persistent_Index index;
} Partition;

int partition_init(Partition *p, const char *path, uint64_t base);

int partition_from_disk(Partition *p, const char *path, uint64_t base);

int partition_dump_ts_chunk(Partition *p, const Timeseries_Chunk *tc);

int partition_find(const Partition *p, uint8_t *dst, uint64_t timestamp);

int partition_range(const Partition *p, uint8_t *dst, uint64_t t0, uint64_t t1);

#endif
