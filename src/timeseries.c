#include "timeseries.h"
#include "binary.h"
#include "disk_io.h"
#include "logging.h"
#include <dirent.h>
#include <stdio.h>
#include <string.h>

static const char *BASE_PATH = "logdata";
static const size_t LINEAR_THRESHOLD = 192;
static const size_t RECORD_BINARY_SIZE =
    (sizeof(uint64_t) * 2) + sizeof(double_t);
const size_t TS_FLUSH_SIZE = 512; // 512b
const size_t TS_BATCH_OFFSET = sizeof(uint64_t) * 3;
/* const size_t TS_FLUSH_SIZE = 4294967296; // 4Mb */

Timeseries_DB *tsdb_init(const char *data_path) {
    if (!data_path)
        return NULL;

    if (strlen(data_path) > DATA_PATH_SIZE)
        return NULL;

    if (make_dir(BASE_PATH) < 0)
        return NULL;

    Timeseries_DB *tsdb = malloc(sizeof(*tsdb));
    if (!tsdb)
        return NULL;

    strncpy(tsdb->data_path, data_path, strlen(data_path) + 1);

    // Create the DB path if it doesn't exist
    char pathbuf[MAX_PATH_SIZE];
    snprintf(pathbuf, sizeof(pathbuf), "%s/%s", BASE_PATH, tsdb->data_path);
    if (make_dir(pathbuf) < 0)
        return NULL;

    return tsdb;
}

void tsdb_close(Timeseries_DB *tsdb) { free(tsdb); }

Timeseries *ts_create(const Timeseries_DB *tsdb, const char *name,
                      int64_t retention, Duplication_Policy policy) {
    if (!tsdb || !name)
        return NULL;

    if (strlen(name) > TS_NAME_MAX_LENGTH)
        return NULL;

    Timeseries *ts = malloc(sizeof(*ts));
    if (!ts)
        return NULL;

    ts->retention = retention;
    ts->partition_nr = 0;
    ts->policy = policy;
    for (int i = 0; i < TS_MAX_PARTITIONS; ++i)
        memset(&ts->partitions[i], 0x00, sizeof(ts->partitions[i]));

    snprintf(ts->name, TS_NAME_MAX_LENGTH, "%s", name);
    snprintf(ts->db_data_path, DATA_PATH_SIZE, "%s", tsdb->data_path);

    // Create the timeseries path if it doesn't exist
    char pathbuf[MAX_PATH_SIZE];
    snprintf(pathbuf, sizeof(pathbuf), "%s/%s/%s", BASE_PATH, tsdb->data_path,
             ts->name);

    if (make_dir(pathbuf) < 0)
        perror("make dir");

    if (ts_init(ts) < 0) {
        ts_close(ts);
        free(ts);
        return NULL;
    }

    return ts;
}

Timeseries *ts_get(const Timeseries_DB *tsdb, const char *name) {
    if (!tsdb || !name)
        return NULL;

    if (strlen(name) > TS_NAME_MAX_LENGTH)
        return NULL;

    Timeseries *ts = malloc(sizeof(*ts));
    if (!ts)
        return NULL;

    for (int i = 0; i < TS_MAX_PARTITIONS; ++i)
        memset(&ts->partitions[i], 0x00, sizeof(ts->partitions[i]));

    snprintf(ts->db_data_path, DATA_PATH_SIZE, "%s", tsdb->data_path);
    snprintf(ts->name, TS_NAME_MAX_LENGTH, "%s", name);

    // TODO consider adding some metadata file which saves TS info such as the
    // duplication policy
    if (ts_init(ts) < 0) {
        ts_close(ts);
        return NULL;
    }

    return ts;
}

static void ts_chunk_zero(Timeseries_Chunk *tc) {
    tc->base_offset = 0;
    tc->start_ts = 0;
    tc->end_ts = 0;
    tc->max_index = 0;
    tc->wal.size = 0;
}

static int ts_chunk_init(Timeseries_Chunk *tc, const char *path,
                         uint64_t base_ts, int main) {
    tc->base_offset = base_ts;
    tc->start_ts = 0;
    tc->end_ts = 0;
    tc->max_index = 0;

    for (int i = 0; i < TS_CHUNK_SIZE; ++i)
        vec_new(tc->points[i]);

    if (wal_init(&tc->wal, path, tc->base_offset, main) < 0)
        return -1;

    return 0;
}

static void ts_chunk_destroy(Timeseries_Chunk *tc) {
    if (tc->base_offset != 0) {
        for (int i = 0; i < TS_CHUNK_SIZE; ++i) {
            vec_destroy(tc->points[i]);
            tc->points[i].size = 0;
        }
    }
    tc->base_offset = 0;
    tc->start_ts = 0;
    tc->end_ts = 0;
    tc->max_index = 0;
}

static int ts_chunk_record_fit(const Timeseries_Chunk *tc, uint64_t sec) {
    // Relative offset inside the 2 arrays
    size_t index = sec - tc->base_offset;

    // Index outside of the head chunk range
    // 1. Flush the tail chunk to persistence
    // 2. Create a new head chunk set on the next 15 min
    // 3. Make the current head chunk the new tail chunk
    if (index > TS_CHUNK_SIZE - 1)
        return -1;

    return 0;
}

/*
 * Compare two Record structures, taking into account the timestamp of each.
 *
 * It returns an integer following the rules:
 *
 * . -1 r1 is lesser than r2
 * .  0 r1 is equal to r2
 * .  1 r1 is greater than r2
 */
static int record_cmp(const Record *r1, const Record *r2) {
    if (r1->timestamp == r2->timestamp)
        return 0;
    if (r1->timestamp > r2->timestamp)
        return 1;
    return -1;
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
 * value.
 *
 * Remarks
 *
 * - This function assumes the record will fit in the chunk, by previously
 *   checking it with `ts_chunk_record_fit(2)`
 *
 */
static int ts_chunk_set_record(Timeseries_Chunk *tc, uint64_t sec,
                               uint64_t nsec, double_t value) {
    // Relative offset inside the 2 arrays
    size_t index = sec - tc->base_offset;

    // Append to the last record in this timestamp bucket
    Record point = {
        .value = value,
        .timestamp = sec * 1e9 + nsec,
        .tv = (struct timespec){.tv_sec = sec, .tv_nsec = nsec},
        .is_set = 1,
    };

    // Check if the timestamp is ordered
    if (tc->end_ts != 0 && tc->end_ts > point.timestamp) {
        ssize_t i = 0;
        vec_bsearch_cmp(tc->points[index], &point, record_cmp, &i);
        if (i < 0)
            return -1;
        // Simple shift of existing elements, maybe worth adding a support
        // vector for out of order (in chunk range) records and merge them
        // when flushing, must profile
        // NB WAL doesn't need any change as it will act as an event
        // log, replayable to obtain the up-to-date state
        vec_insert_at(tc->points[index], (size_t)i, point);
    } else {
        vec_push(tc->points[index], point);
        tc->max_index = index > tc->max_index ? index : tc->max_index;
    }

    tc->start_ts = tc->start_ts == 0 ? point.timestamp : tc->start_ts;
    tc->end_ts = point.timestamp;

    return 0;
}

static int ts_chunk_from_disk(Timeseries_Chunk *tc, const char *pathbuf,
                              uint64_t base_timestamp, int main) {

    int err = wal_from_disk(&tc->wal, pathbuf, base_timestamp, main);
    if (err < 0)
        return -1;

    uint8_t *buf = malloc(tc->wal.size + 1);
    if (!buf)
        return -1;
    ssize_t n = read_file(tc->wal.fp, buf);
    if (n < 0)
        return -1;

    tc->base_offset = base_timestamp;
    for (int i = 0; i < TS_CHUNK_SIZE; ++i)
        vec_new(tc->points[i]);

    uint8_t *ptr = buf;
    uint64_t timestamp;
    double_t value;

    while (n > 0) {
        timestamp = read_i64(ptr);
        value = read_f64(ptr + sizeof(uint64_t));

        uint64_t sec = timestamp / (uint64_t)1e9;
        uint64_t nsec = timestamp % (uint64_t)1e9;

        ts_chunk_set_record(tc, sec, nsec, value);

        ptr += sizeof(uint64_t) + sizeof(double_t);
        n -= (sizeof(uint64_t) + sizeof(double_t));
    }

    free(buf);

    return 0;
}

int ts_init(Timeseries *ts) {
    char pathbuf[MAX_PATH_SIZE];
    snprintf(pathbuf, sizeof(pathbuf), "%s/%s/%s", BASE_PATH, ts->db_data_path,
             ts->name);

    ts_chunk_zero(&ts->head);
    ts_chunk_zero(&ts->head);

    struct dirent **namelist;
    int err = 0, ok = 0;
    int n = scandir(pathbuf, &namelist, NULL, alphasort);
    if (n == -1)
        goto exit;

    for (int i = 0; i < n; ++i) {
        const char *dot = strrchr(namelist[i]->d_name, '.');
        if (strncmp(namelist[i]->d_name, "wal-", 4) == 0 &&
            strncmp(dot, ".log", 4) == 0) {
            uint64_t base_timestamp = atoll(namelist[i]->d_name + 6);
            if (namelist[i]->d_name[4] == 'h') {
                err = ts_chunk_from_disk(&ts->head, pathbuf, base_timestamp, 1);
            } else if (namelist[i]->d_name[4] == 't') {
                err = ts_chunk_from_disk(&ts->prev, pathbuf, base_timestamp, 0);
            }
            ok = err == 0;
        } else if (namelist[i]->d_name[0] == 'c') {
            // There is a log partition
            uint64_t base_timestamp = atoll(namelist[i]->d_name + 3);
            err = partition_from_disk(&ts->partitions[ts->partition_nr++],
                                      pathbuf, base_timestamp);
        }

        free(namelist[i]);

        if (err < 0)
            goto exit;
    }

    free(namelist);

    return ok;

exit:
    free(namelist);
    return err;
}

void ts_close(Timeseries *ts) {
    ts_chunk_destroy(&ts->head);
    ts_chunk_destroy(&ts->prev);
    free(ts);
}

static void ts_deinit(Timeseries *ts) {
    ts_chunk_destroy(&ts->head);
    ts_chunk_destroy(&ts->prev);
    wal_delete(&ts->head.wal);
    wal_delete(&ts->prev.wal);
}

/*
 * Set a record in a timeseries.
 *
 * This function sets a record with the specified timestamp and value in the
 * given timeseries. The function handles the storage of records in memory and
 * on disk to ensure data integrity and efficient usage of resources.
 *
 * @param ts A pointer to the Timeseries structure representing the timeseries.
 * @param timestamp The timestamp of the record to be set, in nanoseconds.
 * @param value The value of the record to be set.
 * @return 0 on success, -1 on failure.
 */
int ts_insert(Timeseries *ts, uint64_t timestamp, double_t value) {
    // Extract seconds and nanoseconds from timestamp
    uint64_t sec = timestamp / (uint64_t)1e9;
    uint64_t nsec = timestamp % (uint64_t)1e9;

    char pathbuf[MAX_PATH_SIZE];
    snprintf(pathbuf, sizeof(pathbuf), "%s/%s/%s", BASE_PATH, ts->db_data_path,
             ts->name);

    // if the limit is reached we dump the chunks into disk and create 2 new
    // ones
    if (wal_size(&ts->head.wal) >= TS_FLUSH_SIZE) {
        uint64_t base = ts->prev.base_offset > 0 ? ts->prev.base_offset
                                                 : ts->head.base_offset;
        size_t partition_nr = ts->partition_nr == 0 ? 0 : ts->partition_nr - 1;

        if (ts->partitions[partition_nr].clog.base_timestamp < base) {
            if (partition_init(&ts->partitions[ts->partition_nr], pathbuf,
                               base) < 0) {
                return -1;
            }
            partition_nr = ts->partition_nr;
            ts->partition_nr++;
        }

        // Dump chunks into disk and create new ones
        if (partition_flush_chunk(&ts->partitions[partition_nr], &ts->prev) < 0)
            return -1;
        if (partition_flush_chunk(&ts->partitions[partition_nr], &ts->head) < 0)
            return -1;

        // Reset clean both head and prev in-memory chunks
        ts_deinit(ts);
    }
    // Let it crash for now if the timestamp is out of bounds in the ooo
    if (sec < ts->head.base_offset) {
        // If the chunk is empty, it also means the base offset is 0, we set
        // it here with the first record inserted
        if (ts->prev.base_offset == 0)
            ts_chunk_init(&ts->prev, pathbuf, sec, 0);

        // Persist to disk for disaster recovery
        wal_append_record(&ts->prev.wal, timestamp, value);

        // If we successfully insert the record, we can return
        if (ts_chunk_record_fit(&ts->prev, sec) == 0)
            return ts_chunk_set_record(&ts->prev, sec, nsec, value);
    }

    if (ts->head.base_offset == 0)
        ts_chunk_init(&ts->head, pathbuf, sec, 1);

    // Persist to disk for disaster recovery
    wal_append_record(&ts->head.wal, timestamp, value);
    // Check if the timestamp is in range of the current chunk, otherwise
    // create a new in-memory segment
    if (ts_chunk_record_fit(&ts->head, sec) < 0) {
        // Flush the prev chunk to persistence
        if (partition_flush_chunk(&ts->partitions[ts->partition_nr],
                                  &ts->prev) < 0)
            return -1;
        // Clean up the prev chunk and delete it's WAL
        ts_chunk_destroy(&ts->prev);
        wal_delete(&ts->prev.wal);
        // Set the current head as new prev
        ts->prev = ts->head;
        // Reset the current head as new head
        ts_chunk_destroy(&ts->head);
        wal_delete(&ts->head.wal);
    }
    // Insert it into the head chunk
    return ts_chunk_set_record(&ts->head, sec, nsec, value);
}

static int ts_search_index(const Timeseries_Chunk *tc, uint64_t sec,
                           const Record *target, Record *dst) {
    if (tc->base_offset > sec)
        return 1;

    size_t index = 0;
    ssize_t idx = 0;

    if ((index = sec - tc->base_offset) > TS_CHUNK_SIZE)
        return -1;

    if (vec_size(tc->points[index]) < LINEAR_THRESHOLD)
        vec_search_cmp(tc->points[index], target, record_cmp, &idx);
    else
        vec_bsearch_cmp(tc->points[index], target, record_cmp, &idx);

    if (idx < 0)
        return 1;

    *dst = vec_at(tc->points[index], idx);

    return 0;
}

/*
 * Finds a record in a timeseries data structure.
 *
 * This function searches for a record with the specified timestamp in the
 * given timeseries data structure. It first checks the in-memory chunks
 * (head and previous) and then looks for the record on disk if not found
 * in memory.
 *
 * @param ts A pointer to the Timeseries structure representing the timeseries.
 * @param timestamp The timestamp of the record to be found, specified in
 * nanoseconds since the Unix epoch.
 * @param r A pointer to a Record structure where the found record will be
 * stored.
 * @return 0 if the record is found and successfully stored in 'r', -1 if an
 * error occurs, or a negative value indicating the result of the search:
 *         - 1 if the record is found in memory.
 *         - 0 if the record is not found in memory but found on disk.
 *         - Negative value if an error occurs during the search.
 */
int ts_find(const Timeseries *ts, uint64_t timestamp, Record *r) {
    uint64_t sec = timestamp / (uint64_t)1e9;
    Record target = {.timestamp = timestamp};
    int err = 0;

    if (ts->head.base_offset > 0) {
        // First check the current chunk
        err = ts_search_index(&ts->head, sec, &target, r);
        if (err <= 0)
            return err;
    }
    // Then check the OOO chunk
    if (err != 1 && ts->prev.base_offset > 0) {
        err = ts_search_index(&ts->prev, sec, &target, r);
        if (err <= 0)
            return err;
    }

    // We have no persistence, can't stop here, no record found
    if (ts->partitions[0].start_ts == 0)
        return -1;

    // Look for the record on disk
    uint8_t buf[RECORD_BINARY_SIZE];
    ssize_t partition_i = 0;
    for (size_t n = 0; n <= ts->partition_nr; ++n) {
        if (ts->partitions[n].clog.base_timestamp > 0 &&
            ts->partitions[n].clog.base_timestamp <= sec) {
            uint64_t curr_ts =
                ts->partitions[n].clog.base_timestamp * (uint64_t)1e9 +
                ts->partitions[n].clog.base_ns;
            partition_i = curr_ts > timestamp ? n - 1 : n;
        }
        if (ts->partitions[n].clog.base_timestamp > sec)
            break;
    }

    // This shouldn't happen, but in case, we return timestamp not found
    if (partition_i < 0)
        return -1;

    // Fetch single record from the partition
    err = partition_find(&ts->partitions[partition_i], buf, timestamp);
    if (err < 0)
        return -1;

    // We got a match
    ts_record_read(r, buf);

    return 0;
}

static void ts_chunk_range(const Timeseries_Chunk *tc, uint64_t t0, uint64_t t1,
                           Points *p) {
    uint64_t sec0 = t0 / (uint64_t)1e9;
    uint64_t sec1 = t1 / (uint64_t)1e9;
    size_t low, high, idx_low = 0, idx_high = 0;
    // Find the low
    low = sec0 - tc->base_offset;
    Record target = {.timestamp = t0};

    if (vec_size(tc->points[low]) < LINEAR_THRESHOLD)
        vec_search_cmp(tc->points[low], &target, record_cmp, &idx_low);
    else
        vec_bsearch_cmp(tc->points[low], &target, record_cmp, &idx_low);

    // Find the high
    // TODO let it crash on edge cases for now (can this happen now?)
    high = sec1 - tc->base_offset;
    target.timestamp = t1;

    if (vec_size(tc->points[high]) < LINEAR_THRESHOLD)
        vec_search_cmp(tc->points[high], &target, record_cmp, &idx_high);
    else
        vec_bsearch_cmp(tc->points[high], &target, record_cmp, &idx_high);

    // Collect the records
    for (size_t i = low; i < high + 1; ++i) {

        size_t end = i == high ? idx_high : vec_size(tc->points[i]);

        for (size_t j = idx_low; j < end + 1; ++j) {
            Record r = vec_at(tc->points[i], j);
            if (r.is_set == 1)
                vec_push(*p, r);
        }
        idx_low = 0;
    }
}

// Helper function to fetch records from a partition within a given time range
static int fetch_records_from_partition(const Partition *partition,
                                        uint64_t start, uint64_t end,
                                        Points *points) {
    // TODO malloc
    uint8_t buf[4096];
    uint8_t *ptr = &buf[0];
    size_t record_len = 0;
    int n = partition_range(partition, ptr, start, end);
    if (n < 0)
        return -1;

    while (n > 0) {
        Record record;
        record_len = ts_record_read(&record, ptr);
        vec_push(*points, record);
        ptr += record_len;
        n -= record_len;
        start = record.timestamp;
    }
    return 0;
}

int ts_range(const Timeseries *ts, uint64_t start, uint64_t end, Points *p) {
    uint64_t sec0 = start / (uint64_t)1e9;
    // Check if the range falls in the current chunk
    if (ts->head.base_offset > 0 && ts->head.base_offset <= sec0 &&
        ts->head.start_ts <= start) {
        // The starting timestamp is in the future, return not found
        if (sec0 - ts->head.base_offset > TS_CHUNK_SIZE)
            return -1;
        ts_chunk_range(&ts->head, start, end, p);
    } else if (ts->prev.base_offset > 0 && ts->prev.base_offset <= sec0 &&
               ts->prev.start_ts <= end) {
        // TODO remove
        // The starting timestamp is in the future for the prev chunk, this
        // shouldn't happen
        if (sec0 - ts->prev.base_offset > TS_CHUNK_SIZE)
            return -1;
        ts_chunk_range(&ts->prev, start, end, p);
    } else {
        // Search in the persistence
        size_t partition_i = 0;
        const Partition *curr_p = NULL;

        // Find the starting partition
        while (partition_i < ts->partition_nr &&
               ts->partitions[partition_i].end_ts < start)
            partition_i++;

        // Fetch records from partitions within the time range
        while (partition_i < ts->partition_nr &&
               ts->partitions[partition_i].start_ts <= end) {
            curr_p = &ts->partitions[partition_i];

            // Fetch records from the current partition
            if (curr_p->end_ts >= end) {
                if (fetch_records_from_partition(curr_p, start, end, p) < 0)
                    return -1;
                return 0;
            } else {
                if (fetch_records_from_partition(curr_p, start, curr_p->end_ts,
                                                 p) < 0)
                    return -1;
                start = curr_p->end_ts;
                partition_i++;
            }
        }

        // Fetch records from the previous chunk if it exists
        if (ts->prev.base_offset != 0) {
            ts_chunk_range(&ts->prev, start, end, p);
            start = vec_last(ts->prev.points[ts->prev.max_index]).timestamp;
        }

        // Fetch records from the current chunk if it exists
        if (ts->head.base_offset != 0) {
            ts_chunk_range(&ts->head, ts->head.start_ts, end, p);
        }
    }

    return 0;
}

void ts_print(const Timeseries *ts) {
    for (int i = 0; i < TS_CHUNK_SIZE; ++i) {
        Points p = ts->head.points[i];
        for (size_t j = 0; j < vec_size(p); ++j) {
            Record r = vec_at(p, j);
            if (!r.is_set)
                continue;
            log_info("%lu {.sec: %lu, .nsec: %lu, .value: %.02f}", r.timestamp,
                     r.tv.tv_sec, r.tv.tv_nsec, r.value);
        }
    }
}

size_t ts_record_timestamp(const uint8_t *buf) {
    return read_i64(buf + sizeof(uint64_t));
}

size_t ts_record_write(const Record *r, uint8_t *buf) {
    // Record full size u64
    size_t record_size = RECORD_BINARY_SIZE;
    write_i64(buf, record_size);
    buf += sizeof(uint64_t);
    // Timestamp u64
    write_i64(buf, r->timestamp);
    buf += sizeof(uint64_t);
    // Value
    write_f64(buf, r->value);
    return record_size;
}

size_t ts_record_read(Record *r, const uint8_t *buf) {
    // Record size u64
    size_t record_size = read_i64(buf);
    buf += sizeof(uint64_t);
    // Timestamp u64
    r->timestamp = read_i64(buf);
    r->tv.tv_sec = r->timestamp / (uint64_t)1e9;
    r->tv.tv_nsec = r->timestamp % (uint64_t)1e9;
    buf += sizeof(uint64_t);
    // Value
    r->value = read_f64(buf);

    return record_size;
}

size_t ts_record_batch_write(const Record *r[], uint8_t *buf, size_t count) {
    uint64_t last_timestamp = r[count - 1]->timestamp;
    // For now we assume fixed size of records
    uint64_t batch_size = count * ((sizeof(uint64_t) * 2) + sizeof(double_t));
    write_i64(buf, batch_size);
    write_i64(buf + sizeof(uint64_t), last_timestamp);
    size_t offset = sizeof(uint64_t) * 2;
    for (size_t i = 0; i < count; ++i) {
        offset += ts_record_write(r[i], buf + offset);
    }
    return batch_size;
}
