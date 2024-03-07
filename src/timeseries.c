#include "timeseries.h"
#include "binary.h"
#include "disk_io.h"
#include "logging.h"
#include <assert.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>

static const char *BASE_PATH = "logdata";
static const size_t LINEAR_THRESHOLD = 192;
static const size_t RECORD_BINARY_SIZE =
    (sizeof(uint64_t) * 2) + sizeof(double_t);
const size_t TS_DUMP_SIZE = 512; // 512b
const size_t TS_BATCH_OFFSET = sizeof(uint64_t) * 3;
/* const size_t TS_DUMP_SIZE = 4294967296; // 4Mb */

static int ts_chunk_init(Timeseries_Chunk *tc, const char *path,
                         uint64_t base_ts, int main) {
    tc->base_offset = base_ts;
    tc->start_ts = 0;
    tc->max_index = 0;
    for (int i = 0; i < TS_CHUNK_SIZE; ++i)
        vec_new(tc->points[i]);
    if (wal_init(&tc->wal, path, tc->base_offset, main) < 0)
        return -1;
    return 0;
}

static void ts_chunk_destroy(Timeseries_Chunk *tc) {
    for (int i = 0; i < TS_CHUNK_SIZE; ++i) {
        vec_destroy(tc->points[i]);
        tc->points[i].size = 0;
    }
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
int ts_chunk_set_record(Timeseries_Chunk *tc, uint64_t sec, uint64_t nsec,
                        double_t value) {
    // Relative offset inside the 2 arrays
    size_t index = sec - tc->base_offset;
    assert(index < TS_CHUNK_SIZE);

    // Append to the last record in this timestamp bucket
    Record point = {
        .value = value,
        .timestamp = sec * 1e9 + nsec,
        .tv = (struct timespec){.tv_sec = sec, .tv_nsec = nsec},
        .is_set = 1,
    };
    // May want to insertion sort here
    vec_push(tc->points[index], point);
    tc->max_index = index > tc->max_index ? index : tc->max_index;

    tc->start_ts = tc->start_ts == 0 ? point.timestamp : tc->start_ts;

    return 0;
}

Timeseries ts_new(const char *name, uint64_t retention) {
    Timeseries ts;
    ts.retention = retention;
    ts.last_partition = 0;
    snprintf(ts.name, TS_NAME_MAX_LENGTH, "%s", name);
    return ts;
}

static int ts_chunk_from_disk(Timeseries_Chunk *tc, const char *pathbuf,
                              uint64_t base_timestamp, int main) {
    int err = wal_from_disk(&tc->wal, pathbuf, base_timestamp, main);
    if (err < 0)
        return -1;

    uint8_t *buf = malloc(tc->wal.size + 1);
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
    snprintf(pathbuf, sizeof(pathbuf), "%s/%s", BASE_PATH, ts->name);

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
            err = partition_from_disk(&ts->partitions[ts->last_partition++],
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

void ts_destroy(Timeseries *ts) {
    ts_chunk_destroy(&ts->head);
    ts_chunk_destroy(&ts->prev);
}

int ts_set_record(Timeseries *ts, uint64_t timestamp, double_t value) {
    uint64_t sec = timestamp / (uint64_t)1e9;
    uint64_t nsec = timestamp % (uint64_t)1e9;

    char pathbuf[MAX_PATH_SIZE];
    snprintf(pathbuf, sizeof(pathbuf), "%s/%s", BASE_PATH, ts->name);

    // if the limit is reached we dump the chunks into disk and create 2 new
    // ones
    if (wal_size(&ts->head.wal) >= TS_DUMP_SIZE) {
        uint64_t base = ts->prev.base_offset > 0 ? ts->prev.base_offset
                                                 : ts->head.base_offset;
        size_t last_partition =
            ts->last_partition == 0 ? 0 : ts->last_partition - 1;

        if (ts->partitions[last_partition].clog.base_timestamp < base) {
            if (partition_init(&ts->partitions[ts->last_partition], pathbuf,
                               base) < 0) {
                return -1;
            }
            last_partition = ts->last_partition;
            ts->last_partition++;
        }

        if (partition_dump_ts_chunk(&ts->partitions[last_partition],
                                    &ts->prev) < 0)
            return -1;
        if (partition_dump_ts_chunk(&ts->partitions[last_partition],
                                    &ts->head) < 0)
            return -1;

        // Delete WAL here
        wal_delete(&ts->head.wal);
        wal_delete(&ts->prev.wal);
        ts_destroy(ts);
        // Reset clean both head and prev in-memory chunks
        ts->head.base_offset = 0;
        ts->prev.base_offset = 0;
    }
    // Let it crash for now if the timestamp is out of bounds in the ooo
    if (sec < ts->head.base_offset) {
        // If the chunk is empty, it also means the base offset is 0, we set
        // it here with the first record inserted
        if (ts->prev.base_offset == 0)
            ts_chunk_init(&ts->prev, pathbuf, sec, 0);

        // Persist to disk for disaster recovery
        wal_append_record(&ts->prev.wal, timestamp, value);

        return ts_chunk_set_record(&ts->prev, sec, nsec, value);
    }

    // TODO
    // We want to persist here if the timestamp is out of bounds with the
    // current chunk and create a new in-memory segment, let's error for now

    if (ts->head.base_offset == 0)
        ts_chunk_init(&ts->head, pathbuf, sec, 1);

    // Persist to disk for disaster recovery
    wal_append_record(&ts->head.wal, timestamp, value);
    return ts_chunk_set_record(&ts->head, sec, nsec, value);
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

int ts_find_record(const Timeseries *ts, uint64_t timestamp, Record *r) {
    uint64_t sec = timestamp / (uint64_t)1e9;
    uint64_t nsec = timestamp % (uint64_t)1e9;
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

    // Finally look for the record on disk
    uint8_t buf[RECORD_BINARY_SIZE];
    size_t partition_i = 0;
    for (size_t n = 0; n <= ts->last_partition; ++n) {
        if (ts->partitions[n].clog.base_timestamp == sec) {
            partition_i = ts->partitions[n].clog.base_ns > nsec ? n - 1 : n;
            break;
        }
    }

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
    // TODO let it crash on edge cases for now
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
            vec_push(*p, r);
        }
        idx_low = 0;
    }
}

int ts_range(const Timeseries *ts, uint64_t t0, uint64_t t1, Points *p) {
    uint64_t sec0 = t0 / (uint64_t)1e9;
    // First check the current chunk
    if (ts->head.base_offset > 0 && ts->head.base_offset <= sec0 &&
        ts->head.start_ts <= t0) {
        if (sec0 - ts->head.base_offset > TS_CHUNK_SIZE)
            return -1;
        ts_chunk_range(&ts->head, t0, t1, p);
    } else if (ts->prev.base_offset > 0 && ts->prev.base_offset <= sec0 &&
               ts->prev.start_ts <= t1) {
        if (sec0 - ts->prev.base_offset > TS_CHUNK_SIZE)
            return -1;
        ts_chunk_range(&ts->prev, t0, t1, p);
    } else {
        // We start looking on the persistence
        // 1st case, the entire range is on persistence
        size_t partition_i = 0, record_len = 0;
        // TODO malloc
        uint8_t buf[4096];
        uint8_t *ptr = &buf[0];
        int complete = 0;
        while (partition_i < ts->last_partition) {
            const Partition *curr_p = &ts->partitions[partition_i];

            if (curr_p->start_ts <= t0) {
                int n = 0;
                if (curr_p->end_ts >= t1) {
                    if ((n = partition_range(curr_p, ptr, t0, t1)) < 0)
                        return -1;

                    while (n > 0) {
                        Record r;
                        record_len = ts_record_read(&r, ptr);
                        vec_push(*p, r);
                        ptr += record_len;
                        n -= record_len;
                        t0 = r.timestamp;
                    }

                    complete = 1;

                    break;
                } else {
                    if ((n = partition_range(curr_p, ptr, t0, curr_p->end_ts)) <
                        0)
                        return -1;

                    while (n > 0) {
                        Record r;
                        record_len = ts_record_read(&r, ptr);
                        vec_push(*p, r);
                        ptr += record_len;
                        n -= record_len;
                    }
                    t0 = curr_p->end_ts;
                }
            }
            partition_i++;
        }

        if (complete)
            return 0;

        if (ts->prev.base_offset != 0) {
            ts_chunk_range(&ts->prev, t0, t1, p);
            t0 = vec_last(ts->prev.points[ts->prev.max_index]).timestamp;
        }

        if (ts->head.base_offset != 0) {
            ts_chunk_range(&ts->head, ts->head.start_ts, t1, p);
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
