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
    struct record *next;
} Record;

/*
 * Time series chunk, main data structure to handle the time-series, it carries
 * some basic informations like the name of the series and the data. Data are
 * stored in a single array, using a base_offset as a strating timestamp,
 * resulting in the timestamps fitting in the allocated space.
 */

typedef struct timeseries_chunk {
    int64_t retention;
    uint16_t base_offset;
    char name[TS_NAME_MAX_LENGTH];
    Record columns[TS_CHUNK_SIZE];
} Timeseries_Chunk;

typedef struct timeseries {
    Timeseries_Chunk current_chunk;
    Timeseries_Chunk ooo_chunk;
} Timeseries;

Timeseries_Chunk ts_chunk_new(const char *name, int64_t retention);

int ts_chunk_set_record(Timeseries_Chunk *ts_chunk, uint64_t ts,
                        double_t value);

#endif
