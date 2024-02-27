#include "timeseries.h"
#include "binary.h"
#include <assert.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>

static const char *BASE_PATH = "logdata";
static const size_t PATH_BUF_MAXSIZE = 1 << 10;

static ssize_t read_file(int fd, uint8_t *buf) {
    FILE *fp = fdopen(fd, "r");
    if (!fp) {
        perror("fdopen");
        return -1;
    }

    /* Get the buffer size */
    if (fseek(fp, 0, SEEK_END) < 0) {
        perror("fseek");
        return -1;
    }

    size_t size = ftell(fp);

    /* Set position of stream to the beginning */
    rewind(fp);

    /* Read the file into the buffer */
    fread(buf, 1, size, fp);

    /* NULL-terminate the buffer */
    buf[size] = '\0';
    return size;
}

static int ts_chunk_init(Timeseries_Chunk *ts_chunk, const char *path,
                         uint64_t base_ts, int main) {
    ts_chunk->base_offset = base_ts;
    for (int i = 0; i < TS_CHUNK_SIZE; ++i)
        VEC_NEW(ts_chunk->points[i]);
    if (wal_init(&ts_chunk->wal, path, ts_chunk->base_offset, main) < 0)
        return -1;
    return 0;
}

static void ts_chunk_destroy(Timeseries_Chunk *ts_chunk) {
    for (int i = 0; i < TS_CHUNK_SIZE; ++i)
        VEC_DESTROY(ts_chunk->points[i]);
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
int ts_chunk_set_record(Timeseries_Chunk *ts_chunk, uint64_t sec, uint64_t nsec,
                        double_t value) {
    // Relative offset inside the 2 arrays
    size_t index = sec - ts_chunk->base_offset;
    assert(index < TS_CHUNK_SIZE);

    // Append to the last record in this timestamp bucket
    Record point = {
        .value = value,
        .timestamp = sec * 1e9 + nsec,
        .tv = (struct timespec){.tv_sec = sec, .tv_nsec = nsec},
        .is_set = 1,
    };
    // May want to insertion sort here
    VEC_APPEND(ts_chunk->points[index], point);

    return 0;
}

Timeseries ts_new(const char *name, uint64_t retention) {
    Timeseries ts;
    ts.retention = retention;
    snprintf(ts.name, TS_NAME_MAX_LENGTH, "%s", name);
    return ts;
}

int ts_init(Timeseries *ts) {
    char pathbuf[PATH_BUF_MAXSIZE];
    snprintf(pathbuf, sizeof(pathbuf), "%s/%s", BASE_PATH, ts->name);

    struct dirent **namelist;
    int err = 0, ok = 0;
    int n = scandir(pathbuf, &namelist, NULL, alphasort);
    if (n == -1)
        goto exit;

    for (int i = 0; i < n; ++i) {
        const char *dot = strrchr(namelist[i]->d_name, '.');
        if (strncmp(dot, ".log", 4) == 0) {
            uint64_t base_timestamp = atoll(namelist[i]->d_name + 2);
            if (namelist[i]->d_name[0] == 'm') {
                err = wal_from_disk(&ts->current_chunk.wal, pathbuf,
                                    base_timestamp, 1);
                uint8_t buf[ts->current_chunk.wal.size + 1];
                int n = read_file(ts->current_chunk.wal.fd, buf);
                if (n < 0)
                    goto exit;

                ts_chunk_init(&ts->current_chunk, pathbuf, base_timestamp, 1);

                uint8_t *ptr = buf;
                uint64_t timestamp;
                double_t value;
                while (n > 0) {
                    timestamp = read_i64(ptr);
                    value = read_f64(ptr + sizeof(uint64_t));

                    uint64_t sec = timestamp / (uint64_t)1e9;
                    uint64_t nsec = timestamp % (uint64_t)1e9;

                    ts_chunk_set_record(&ts->current_chunk, sec, nsec, value);

                    ptr += sizeof(uint64_t) + sizeof(double_t);
                    n -= (sizeof(uint64_t) + sizeof(double_t));
                }
                ok = 1;
            } else {
                err = wal_from_disk(&ts->ooo_chunk.wal, pathbuf, base_timestamp,
                                    0);
                uint8_t buf[ts->ooo_chunk.wal.size];
                int n = read_file(ts->ooo_chunk.wal.fd, buf);
                if (n < 0)
                    goto exit;

                ts_chunk_init(&ts->ooo_chunk, pathbuf, base_timestamp, 0);

                uint8_t *ptr = buf;
                uint64_t timestamp;
                double_t value;
                while (n > 0) {
                    timestamp = read_i64(ptr);
                    value = read_f64(ptr + sizeof(uint64_t));

                    uint64_t sec = timestamp / (uint64_t)1e9;
                    uint64_t nsec = timestamp % (uint64_t)1e9;

                    ts_chunk_set_record(&ts->ooo_chunk, sec, nsec, value);

                    ptr += sizeof(uint64_t) + sizeof(double_t);
                    n -= (sizeof(uint64_t) + sizeof(double_t));
                }
                if (err < 0)
                    goto exit;
                ok = 1;
            }
        }

        free(namelist[i]);
    }

    free(namelist);

    return ok;

exit:

    return err;
}

void ts_destroy(Timeseries *ts) {
    ts_chunk_destroy(&ts->current_chunk);
    ts_chunk_destroy(&ts->ooo_chunk);
}

int ts_set_record(Timeseries *ts, uint64_t timestamp, double_t value) {
    uint64_t sec = timestamp / (uint64_t)1e9;
    uint64_t nsec = timestamp % (uint64_t)1e9;
    // Let it crash for now if the timestamp is out of bounds in the ooo
    if (sec < ts->current_chunk.base_offset) {
        // If the chunk is empty, it also means the base offset is 0, we set
        // it here with the first record inserted
        if (ts->ooo_chunk.base_offset == 0) {
            char pathbuf[PATH_BUF_MAXSIZE];
            snprintf(pathbuf, sizeof(pathbuf), "%s/%s", BASE_PATH, ts->name);
            ts_chunk_init(&ts->ooo_chunk, pathbuf, sec, 0);
        }

        // Persist to disk for disaster recovery
        wal_append_record(&ts->ooo_chunk.wal, timestamp, value);

        return ts_chunk_set_record(&ts->ooo_chunk, sec, nsec, value);
    }

    if (ts->current_chunk.base_offset == 0) {
        char pathbuf[PATH_BUF_MAXSIZE];
        snprintf(pathbuf, sizeof(pathbuf), "%s/%s", BASE_PATH, ts->name);
        ts_chunk_init(&ts->current_chunk, pathbuf, sec, 1);
    }

    // Persist to disk for disaster recovery
    wal_append_record(&ts->current_chunk.wal, timestamp, value);
    return ts_chunk_set_record(&ts->current_chunk, sec, nsec, value);
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

Record ts_find_record(const Timeseries *ts, uint64_t timestamp) {
    uint64_t sec = timestamp / (uint64_t)1e9;
    size_t index = 0;
    Record target = {.timestamp = timestamp};
    // First check the current chunk
    if (ts->current_chunk.base_offset <= sec &&
        (index = sec - ts->current_chunk.base_offset) < TS_CHUNK_SIZE) {
        size_t idx = 0;
        VEC_BSEARCH_PTR(ts->current_chunk.points[index], &target, record_cmp,
                        &idx);
        Record r = VEC_AT(ts->current_chunk.points[index], idx);
        return r;
    }
    // Then check the OOO chunk
    if (ts->ooo_chunk.base_offset <= sec &&
        (index = sec - ts->ooo_chunk.base_offset) < TS_CHUNK_SIZE) {
        size_t idx = 0;
        VEC_BSEARCH_PTR(ts->ooo_chunk.points[index], &target, record_cmp, &idx);
        Record r = VEC_AT(ts->ooo_chunk.points[index], idx);
        return r;
    }

    // TODO look for the record on disk

    return (Record){.is_set = 0};
}

Points ts_range(const Timeseries *ts, uint64_t t0, uint64_t t1) {
    uint64_t sec0 = t0 / (uint64_t)1e9;
    /* uint64_t nsec0 = t0 % (uint64_t)1e9; */
    uint64_t sec1 = t1 / (uint64_t)1e9;
    /* uint64_t nsec1 = t1 % (uint64_t)1e9; */
    size_t low, high;
    Points coll;
    VEC_NEW(coll);
    // First check the current chunk
    if (ts->current_chunk.base_offset <= sec0 &&
        (low = sec0 - ts->current_chunk.base_offset) < TS_CHUNK_SIZE) {
        // Find the low
        Record target = {.timestamp = t0};
        size_t idx_low = 0;
        VEC_BSEARCH_PTR(ts->current_chunk.points[low], &target, record_cmp,
                        &idx_low);
        // Find the high
        // TODO let it crash on edge cases for now
        size_t idx_high = 0;
        high = sec1 - ts->current_chunk.base_offset;
        target.timestamp = t1;
        VEC_BSEARCH_PTR(ts->current_chunk.points[high], &target, record_cmp,
                        &idx_high);
        // Collect the records
        for (size_t i = low; i < high; ++i) {
            for (size_t j = idx_low; j < VEC_SIZE(ts->current_chunk.points[i]);
                 ++j) {
                VEC_APPEND(coll, VEC_AT(ts->current_chunk.points[i], j));
            }
            idx_low = 0;
        }
    }

    return coll;
}

void ts_print(const Timeseries *ts) {
    for (int i = 0; i < TS_CHUNK_SIZE; ++i) {
        Points p = ts->current_chunk.points[i];
        for (size_t j = 0; j < VEC_SIZE(p); ++j) {
            Record r = VEC_AT(p, j);
            if (!r.is_set)
                continue;
            printf(" %lu {.sec: %lu, .nsec: %lu, .value: %.02f}\n", r.timestamp,
                   r.tv.tv_sec, r.tv.tv_nsec, r.value);
        }
    }
}
