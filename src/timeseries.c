#include "timeseries.h"
#include <assert.h>
#include <stdio.h>

int ts_chunk_new(Timeseries_Chunk *ts_chunk, const char *path) {
    if (wal_init(&ts_chunk->wal, path, ts_chunk->base_offset) < 0)
        return -1;
    ts_chunk->base_offset = 0;
    return 0;
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
    printf("%lu\n", index);
    assert(index < TS_CHUNK_SIZE);

    // Append to the last record in this timestamp bucket
    Record *cursor = &ts_chunk->columns[index];

    while (cursor->next)
        cursor = cursor->next;

    // Move to a vector, first quick implementation with LL is fine
    if (cursor->is_set) {
        cursor->next = malloc(sizeof(*cursor));
        cursor = cursor->next;
    }

    // Persist to disk for disaster recovery
    wal_append_record(&ts_chunk->wal, ts, value);

    // Set the record
    cursor->value = value;
    cursor->timestamp = ts;
    cursor->is_set = 1;

    return 0;
}

Timeseries ts_new(const char *name, uint64_t retention) {
    Timeseries ts;
    ts.retention = retention;
    snprintf(ts.name, TS_NAME_MAX_LENGTH, "%s", name);
    ts_chunk_new(&ts.current_chunk, "logdata");
    // TODO remove
    ts.ooo_chunk.base_offset = 10;
    ts_chunk_new(&ts.ooo_chunk, "logdata");
    return ts;
}

int ts_set_record(Timeseries *ts, uint64_t timestamp, double_t value) {
    // Let it crash for now if the timestamp is out of bounds in the ooo
    if (timestamp < ts->current_chunk.base_offset)
        return ts_chunk_set_record(&ts->ooo_chunk, timestamp, value);
    return ts_chunk_set_record(&ts->current_chunk, timestamp, value);
}
