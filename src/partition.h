#ifndef PARTITION_H
#define PARTITION_H

#include "commit_log.h"
#include "persistent_index.h"

typedef struct timeseries_chunk Timeseries_Chunk;

typedef struct partition {
    Commit_Log clog;
    Persistent_Index index;
} Partition;

int partition_dump_timeseries_chunk(Partition *p, const Timeseries_Chunk *tc);

int partition_find(const Partition *p, uint8_t *dst, uint64_t timestamp);

#endif
