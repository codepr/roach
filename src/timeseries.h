#ifndef TIMESERIES_H
#define TIMESERIES_H

#include <math.h>
#include <stdint.h>
#include <stdlib.h>

#define TS_NAME_MAX_LENGTH 1 << 9
#define TS_CHUNK_SIZE 3600 // 1 hr

/*
 * Simple record struct, wrap around a column inside the database, defined as a
 * key-val couple alike, though it's used only to describe the value of each
 * column
 */
typedef struct record {
    uint64_t timestamp;
    double_t value;
} Record;

/*
 * Time series, main data structure to handle the time-series, it carries some
 * basic informations like the name of the series and the data. Data are stored
 * as two paired arrays, one indexing the timestamp of each row, the other
 * being an array of arrays of `tts_record`, this way we can easily store
 * different number of columns for each row, depending on the presence of the
 * data during the insertion.
 * A third array is used to store the fields name, each row (index in the
 * columns array) will be paired with the fields array to retrieve what field
 * it refers to, if present.
 */

typedef struct timeseries_chunk {
    int64_t retention;
    uint16_t base_offset;
    char name[TS_NAME_MAX_LENGTH];
    Record columns[TS_CHUNK_SIZE];
} Timeseries_Chunk;

Timeseries_Chunk ts_chunk_new(const char *name, int64_t retention);

int ts_chunk_set_record(Timeseries_Chunk *ts_chunk, uint64_t ts,
                        double_t value);

#endif
