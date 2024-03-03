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
    uint64_t base_timestamp;
} Persistent_Index;

/*
 * Represents an offset range containing the requested point(s), with a
 * start and end bounds.
 */
typedef struct range {
    int64_t start;
    int64_t end;
} Range;

int p_index_init(Persistent_Index *pi, const char *path, uint64_t base);

int p_index_close(Persistent_Index *pi);

int p_index_from_disk(Persistent_Index *pi, const char *path, uint64_t base);

int p_index_append_offset(Persistent_Index *pi, uint64_t ts, uint64_t offset);

int p_index_find_offset(const Persistent_Index *pi, uint64_t ts, Range *r);

void p_index_print(const Persistent_Index *pi);

#endif
