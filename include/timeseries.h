#ifndef TIMESERIES_H
#define TIMESERIES_H

#include "partition.h"
#include "vec.h"
#include "wal.h"
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

#define TS_NAME_MAX_LENGTH 1 << 9
#define TS_CHUNK_SIZE      900 // 15 min
#define TS_MAX_PARTITIONS  16
#define DATA_PATH_SIZE     1 << 8

extern const size_t TS_FLUSH_SIZE;
extern const size_t TS_BATCH_OFFSET;

/*
 * Enum defining the rules to apply when a duplicate point is
 * inserted in the timeseries.
 *
 * It currently just support
 * - IGNORE drops the point, returning a failure at insert attempt
 * - INSERT just appends the point
 * - UPDATE updates the point with the new value
 */
typedef enum dup_policy { DP_IGNORE, DP_INSERT } Duplication_Policy;

/*
 * Simple record struct, wrap around a column inside the database, defined as a
 * key-val couple alike, though it's used only to describe the value of each
 * column
 */
typedef struct record {
    struct timespec tv;
    uint64_t timestamp;
    double_t value;
    int is_set;
} Record;

extern size_t ts_record_timestamp(const uint8_t *buf);

extern size_t ts_record_write(const Record *r, uint8_t *buf);

extern size_t ts_record_read(Record *r, const uint8_t *buf);

extern size_t ts_record_batch_write(const Record *r[], uint8_t *buf,
                                    size_t count);

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
    uint64_t start_ts;
    uint64_t end_ts;
    size_t max_index;
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
    char db_data_path[DATA_PATH_SIZE];
    Timeseries_Chunk head;
    Timeseries_Chunk prev;
    Partition partitions[TS_MAX_PARTITIONS];
    size_t partition_nr;
    Duplication_Policy policy;
} Timeseries;

extern int ts_init(Timeseries *ts);

extern void ts_close(Timeseries *ts);

extern int ts_insert(Timeseries *ts, uint64_t timestamp, double_t value);

extern int ts_find(const Timeseries *ts, uint64_t timestamp, Record *r);

extern int ts_range(const Timeseries *ts, uint64_t t0, uint64_t t1, Points *p);

extern void ts_print(const Timeseries *ts);

typedef struct timeseries_db {
    char data_path[DATA_PATH_SIZE];
} Timeseries_DB;

extern Timeseries_DB *tsdb_init(const char *data_path);

extern void tsdb_close(Timeseries_DB *tsdb);

extern Timeseries *ts_create(const Timeseries_DB *tsdb, const char *name,
                             int64_t retention, Duplication_Policy policy);

extern Timeseries *ts_get(const Timeseries_DB *tsdb, const char *name);

#endif
