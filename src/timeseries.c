#include "timeseries.h"
#include <assert.h>
#include <stdio.h>

Timeseries_Chunk ts_chunk_new(const char *name, int64_t retention) {
    Timeseries_Chunk ts_chunk;
    ts_chunk.retention = retention;
    ts_chunk.base_offset = 0;
    snprintf(ts_chunk.name, TS_NAME_MAX_LENGTH, "%s", name);
    return ts_chunk;
}

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

    ts_chunk->columns[index].value = value;
    ts_chunk->columns[index].timestamp = ts;

    return 0;
}
