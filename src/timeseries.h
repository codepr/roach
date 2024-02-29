#ifndef TIMESERIES_H
#define TIMESERIES_H

#include "vec.h"
#include "wal.h"
#include <math.h>
#include <stdint.h>
#include <stdlib.h>

#define TS_NAME_MAX_LENGTH 1 << 9
#define TS_CHUNK_SIZE 900 // 15 min

/*
 * Simple record struct, wrap around a column inside the database, defined as a
 * key-val couple alike, though it's used only to describe the value of each
 * column
 */
typedef struct record {
    uint64_t timestamp;
    struct timespec tv;
    double_t value;
    int is_set;
} Record;

typedef VEC(Record) Points;

/*
 * Time series chunk, main data structure to handle the time-series, it carries
 * some a base offset which represents the 1st timestamp inserted and the
 * columns data. Data are stored in a single array, using a base_offset as a
 * strating timestamp, resulting in the timestamps fitting in the allocated
 * space.
 */
typedef struct timeseries_chunk {
    Wal wal;
    uint64_t base_offset;
    Points points[TS_CHUNK_SIZE];
} Timeseries_Chunk;

/*
 * Time series, main data structure to handle the time-series, it carries some
 * basic informations like the name of the series and the retention time. Data
 * are stored in 2 Timeseries_Chunk, a current and latest timestamp one and one
 * to account for out of order points that will be merged later when flushing
 * on disk.
 */
typedef struct timeseries {
    int64_t retention;
    char name[TS_NAME_MAX_LENGTH];
    Timeseries_Chunk head;
    Timeseries_Chunk prev;
} Timeseries;

int ts_chunk_set_record(Timeseries_Chunk *ts_chunk, uint64_t sec, uint64_t nsec,
                        double_t value);

Timeseries ts_new(const char *name, uint64_t retention);

int ts_init(Timeseries *ts);

void ts_destroy(Timeseries *ts);

int ts_set_record(Timeseries *ts, uint64_t timestamp, double_t value);

int ts_find_record(const Timeseries *ts, uint64_t timestamp, Record *r);

int ts_range(const Timeseries *ts, uint64_t t0, uint64_t t1, Points *p);

void ts_print(const Timeseries *ts);

#endif
