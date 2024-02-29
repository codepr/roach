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
        vec_new(ts_chunk->points[i]);
    if (wal_init(&ts_chunk->wal, path, ts_chunk->base_offset, main) < 0)
        return -1;
    return 0;
}

static void ts_chunk_destroy(Timeseries_Chunk *ts_chunk) {
    for (int i = 0; i < TS_CHUNK_SIZE; ++i)
        vec_destroy(ts_chunk->points[i]);
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
    vec_push(ts_chunk->points[index], point);

    return 0;
}

Timeseries ts_new(const char *name, uint64_t retention) {
    Timeseries ts;
    ts.retention = retention;
    snprintf(ts.name, TS_NAME_MAX_LENGTH, "%s", name);
    return ts;
}

static int ts_chunk_from_disk(Timeseries_Chunk *tc, const char *pathbuf,
                              uint64_t base_timestamp, int main) {
    int err = wal_from_disk(&tc->wal, pathbuf, base_timestamp, main);
    if (err < 0)
        return -1;

    uint8_t buf[tc->wal.size + 1];
    int n = read_file(tc->wal.fd, buf);
    if (n < 0)
        return -1;

    ts_chunk_init(tc, pathbuf, base_timestamp, main);

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
    return 0;
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
                err = ts_chunk_from_disk(&ts->head, pathbuf, base_timestamp, 1);
            } else {
                err = ts_chunk_from_disk(&ts->prev, pathbuf, base_timestamp, 0);
            }
            ok = err == 0;
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
    // Let it crash for now if the timestamp is out of bounds in the ooo
    if (sec < ts->head.base_offset) {
        // If the chunk is empty, it also means the base offset is 0, we set
        // it here with the first record inserted
        if (ts->prev.base_offset == 0) {
            char pathbuf[PATH_BUF_MAXSIZE];
            snprintf(pathbuf, sizeof(pathbuf), "%s/%s", BASE_PATH, ts->name);
            ts_chunk_init(&ts->prev, pathbuf, sec, 0);
        }

        // Persist to disk for disaster recovery
        wal_append_record(&ts->prev.wal, timestamp, value);

        return ts_chunk_set_record(&ts->prev, sec, nsec, value);
    }

    // TODO
    // We want to persist here if the timestamp is out of bounds with the
    // current chunk and create a new in-memory segment, let's error for now

    if (ts->head.base_offset == 0) {
        char pathbuf[PATH_BUF_MAXSIZE];
        snprintf(pathbuf, sizeof(pathbuf), "%s/%s", BASE_PATH, ts->name);
        ts_chunk_init(&ts->head, pathbuf, sec, 1);
    }

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

int ts_find_record(const Timeseries *ts, uint64_t timestamp, Record *r) {
    uint64_t sec = timestamp / (uint64_t)1e9;
    size_t index = 0, idx = 0;
    Record target = {.timestamp = timestamp};
    // First check the current chunk
    if (ts->head.base_offset <= sec) {
        if ((index = sec - ts->head.base_offset) > TS_CHUNK_SIZE)
            return -1;

        if (vec_size(ts->head.points[index]) < 128)
            vec_search_cmp(ts->head.points[index], &target, record_cmp, &idx);
        else
            vec_bsearch_cmp(ts->head.points[index], &target, record_cmp, &idx);
        Record res = vec_at(ts->head.points[index], idx);
        *r = res;
        return 0;
    }
    // Then check the OOO chunk
    if (ts->prev.base_offset <= sec) {
        if ((index = sec - ts->prev.base_offset) > TS_CHUNK_SIZE)
            return -1;
        if (vec_size(ts->prev.points[index]) < 128)
            vec_search_cmp(ts->prev.points[index], &target, record_cmp, &idx);
        else
            vec_bsearch_cmp(ts->prev.points[index], &target, record_cmp, &idx);
        Record res = vec_at(ts->prev.points[index], idx);
        *r = res;
        return 0;
    }

    // TODO look for the record on disk

    return -1;
}

static void ts_chunk_range(const Timeseries_Chunk *tc, uint64_t t0, uint64_t t1,
                           Points *p) {
    uint64_t sec0 = t0 / (uint64_t)1e9;
    uint64_t sec1 = t1 / (uint64_t)1e9;
    size_t low, high;
    // Find the low
    low = sec0 - tc->base_offset;
    Record target = {.timestamp = t0};
    size_t idx_low = 0;
    if (vec_size(tc->points[low]) < 128)
        vec_search_cmp(tc->points[low], &target, record_cmp, &idx_low);
    else
        vec_bsearch_cmp(tc->points[low], &target, record_cmp, &idx_low);
    // Find the high
    // TODO let it crash on edge cases for now
    size_t idx_high = 0;
    high = sec1 - tc->base_offset;
    target.timestamp = t1;
    if (vec_size(tc->points[high]) < 128)
        vec_search_cmp(tc->points[high], &target, record_cmp, &idx_high);
    else
        vec_bsearch_cmp(tc->points[high], &target, record_cmp, &idx_high);
    // Collect the records
    for (size_t i = low; i < high + 1; ++i) {
        size_t end = i == high ? idx_high : vec_size(tc->points[i]);
        for (size_t j = idx_low; j < end; ++j) {
            Record r = vec_at(tc->points[i], j);
            vec_push(*p, r);
        }
        idx_low = 0;
    }
}

int ts_range(const Timeseries *ts, uint64_t t0, uint64_t t1, Points *p) {
    uint64_t sec0 = t0 / (uint64_t)1e9;
    // First check the current chunk
    if (ts->head.base_offset <= sec0) {
        if (sec0 - ts->head.base_offset > TS_CHUNK_SIZE)
            return -1;
        ts_chunk_range(&ts->head, t0, t1, p);
    } else {
        if (sec0 - ts->prev.base_offset > TS_CHUNK_SIZE)
            return -1;
        ts_chunk_range(&ts->prev, t0, t1, p);
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
            printf(" %lu {.sec: %lu, .nsec: %lu, .value: %.02f}\n", r.timestamp,
                   r.tv.tv_sec, r.tv.tv_nsec, r.value);
        }
    }
}
