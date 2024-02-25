#include "timeseries.h"
#include <assert.h>
#include <stdio.h>

/*
 * Set a record in the chunk at a relative index based on the first timestamp
 * stored e.g.
 *
 * Base 1782999282
 * Timestamp 1782999288
 * Index 6
 *
 * The information stored is initially formed by a timestamp and a long double
 * value
 */
int ts_chunk_set_record(Timeseries_Chunk *ts_chunk, uint64_t ts,
                        double_t value) {
    // If the chunk is empty, it also means the base offset is 0, we set it
    // here with the first record inserted
    if (ts_chunk->base_offset == 0) {
        ts_chunk->base_offset = ts;
    }

    // Relative offset inside the 2 arrays
    size_t index = ts - ts_chunk->base_offset;
    assert(index < TS_CHUNK_SIZE);

    // Append to the last record in this timestamp bucket
    Record *cursor = &ts_chunk->columns[index];

    while (cursor->next)
        cursor = cursor->next;

    cursor->value = value;
    cursor->timestamp = ts;

    return 0;
}

Timeseries ts_new(const char *name, uint64_t retention) {
    Timeseries ts;
    ts.retention = retention;
    snprintf(ts.name, TS_NAME_MAX_LENGTH, "%s", name);
    return ts;
}
