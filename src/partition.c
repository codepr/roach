#include "partition.h"
#include "binary.h"
#include "commit_log.h"
#include "logging.h"
#include "persistent_index.h"
#include "timeseries.h"
#include "vec.h"
#include <errno.h>
#include <string.h>

static const size_t BATCH_SIZE = 1 << 6;
static const size_t BLOCK_SIZE = 1 << 12;

int partition_init(Partition *p, const char *path, uint64_t base) {
    int err = c_log_init(&p->clog, path, base);
    if (err < 0)
        return -1;

    err = p_index_init(&p->index, path, base);
    if (err < 0)
        return -1;

    p->start_ts = 0;
    p->end_ts = 0;

    return 0;
}

int partition_from_disk(Partition *p, const char *path, uint64_t base) {
    int err = c_log_from_disk(&p->clog, path, base);
    if (err < 0)
        return -1;

    err = p_index_from_disk(&p->index, path, base);
    if (err < 0)
        return -1;

    p->start_ts = p->clog.base_timestamp * (uint64_t)1e9 + p->clog.base_ns;
    p->end_ts = p->clog.current_timestamp;

    return 0;
}

static int commit_records_to_log(Partition *p, const uint8_t *buf, size_t len) {
    int err = c_log_append_batch(&p->clog, buf, len);
    if (err < 0)
        return -1;

    size_t commit_log_size = p->clog.size;
    err = p_index_append_offset(&p->index, ts_record_timestamp(buf),
                                commit_log_size - TS_BATCH_OFFSET);
    if (err < 0)
        return -1;

    return 0;
}

int partition_flush_chunk(Partition *p, const Timeseries_Chunk *tc) {

    VEC(const Record *) records;
    vec_new(records);

    size_t total_records = 0;
    size_t batch_size = 0;
    uint8_t *buf = malloc(TS_FLUSH_SIZE * 2);
    if (!buf) {
        vec_destroy(records);
        return -1;
    }

    uint8_t *bufptr = buf;
    int err = 0;

    for (size_t i = 0; i < TS_CHUNK_SIZE; ++i) {
        if (tc->points[i].size == 0)
            continue;

        for (size_t j = 0; j < vec_size(tc->points[i]); ++j) {
            vec_push(records, &tc->points[i].data[j]);
            total_records++;

            if (++batch_size == BATCH_SIZE) {
                // Poor man slice
                size_t len = ts_record_batch_write(
                    (records.data + (total_records - BATCH_SIZE)), bufptr,
                    BATCH_SIZE);
                err = commit_records_to_log(p, bufptr, len);
                if (err < 0)
                    fprintf(stderr, "batch write failed: %s\n",
                            strerror(errno));
                bufptr += len;
                batch_size = 0;
            }
        }
    }

    // Finish up any remaining record
    size_t remaining_records = total_records % BATCH_SIZE;
    if (remaining_records != 0) {
        size_t len = ts_record_batch_write(
            (records.data + (total_records - remaining_records)), bufptr,
            remaining_records);
        err = commit_records_to_log(p, bufptr, len);
        if (err < 0)
            fprintf(stderr,
                    "Error writing remaining records to commit log: %s\n",
                    strerror(errno));
    }

    // Set base nanoseconds for the commit log
    if (p->start_ts == 0) {
        uint64_t base_ns = tc->start_ts % (uint64_t)1e9;
        c_log_set_base_ns(&p->clog, base_ns);
    }

    // Update timestamps
    p->start_ts = p->start_ts != 0 ? p->start_ts : tc->base_offset;
    p->end_ts = vec_size(records) == 0 ? 0 : vec_last(records)->timestamp;

    free(buf);
    vec_destroy(records);

    return 0;
}

static uint64_t end_offset(const Partition *p, const Range *r) {
    uint64_t end = 0;
    if (r->end == -1)
        end = p->clog.size;
    else if (r->end == r->start)
        end = sizeof(uint64_t) * 3;
    else
        end = r->end;

    return end;
}

int partition_find(const Partition *p, uint8_t *dst, uint64_t timestamp) {
    Range range;
    int err = p_index_find_offset(&p->index, timestamp, &range);
    if (err < 0)
        return -1;

    // TODO dynamic alloc this buffer
    uint8_t buf[BLOCK_SIZE];
    uint8_t *ptr = &buf[0];

    uint64_t end = end_offset(p, &range);

    ssize_t n = c_log_read_at(&p->clog, &ptr, range.start, end);
    if (n < 0)
        return -1;

    size_t record_len = 0;
    uint64_t ts = 0;
    while (n > 0) {
        record_len = read_i64(ptr);
        ts = read_i64(ptr + sizeof(uint64_t));
        if (ts == timestamp)
            break;
        ptr += record_len;
        n -= record_len;
    }

    record_len = read_i64(ptr);
    memcpy(dst, ptr, record_len);

    return 0;
}

int partition_range(const Partition *p, uint8_t *dst, uint64_t t0,
                    uint64_t t1) {
    Range r0, r1;
    int err = p_index_find_offset(&p->index, t0, &r0);
    if (err < 0)
        return -1;

    err = p_index_find_offset(&p->index, t1, &r1);
    if (err < 0)
        return -1;

    uint64_t end1 = end_offset(p, &r1);

    // TODO dynamic alloc this buffer
    uint8_t buf[BLOCK_SIZE];
    uint8_t *ptr = &buf[0];

    ssize_t n = c_log_read_at(&p->clog, &ptr, r0.start, end1);
    if (n < 0)
        return -1;

    size_t record_len = 0, base_size = n;
    size_t start = n;
    uint64_t ts = 0;

    while (n > 0) {
        record_len = read_i64(ptr);
        ts = read_i64(ptr + sizeof(uint64_t));
        if (ts == t0)
            start = n;
        ptr += record_len;
        n -= record_len;
        if (ts == t1)
            break;
    }

    memcpy(dst, buf + (base_size - start), start - n);

    return start - n;
}
