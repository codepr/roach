#ifndef PERSISTENT_INDEX_H
#define PERSISTENT_INDEX_H

#include <stdint.h>
#include <stdio.h>

/*
 * Keeps the state for an index file on disk, updated every interval values
 * to make it easier to read data efficiently from the main segment storage
 * on disk.
 */
typedef struct persistent_index {
    FILE *fp;
    size_t size;
    uint32_t interval;
    uint64_t base_timestamp;
} Persistent_Index;

/*
 * Represents an offset range containing the requested point(s), with a
 * start and end bounds.
 */
typedef struct range {
    uint64_t start[2];
    uint64_t end[2];
} Range;

int p_index_init(Persistent_Index *pi, const char *path,
                 uint64_t base_timestamp, uint32_t interval);

int p_index_close(Persistent_Index *pi);

int p_index_from_disk(Persistent_Index *pi, const char *path,
                      uint64_t base_timestamp, uint32_t interval);

int p_index_append_offset(Persistent_Index *pi, uint64_t relative_ts,
                          uint64_t offset);

int p_index_find_offset(const Persistent_Index *pi, uint64_t timestamp,
                        Range *r);

#endif
