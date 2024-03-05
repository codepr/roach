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

    return 0;
}

int partition_from_disk(Partition *p, const char *path, uint64_t base) {
    int err = c_log_from_disk(&p->clog, path, base);
    if (err < 0)
        return -1;

    err = p_index_from_disk(&p->index, path, base);
    if (err < 0)
        return -1;

    return 0;
}

int partition_dump_ts_chunk(Partition *p, const Timeseries_Chunk *tc) {

    VEC(Record *) dump;
    vec_new(dump);

    size_t count = 0;
    uint8_t *buf = malloc(TS_DUMP_SIZE * 2);

    uint8_t *ptr = buf;
    size_t len = 0;
    int err = 0;

    for (size_t i = 0; i < TS_CHUNK_SIZE; ++i) {
        if (tc->points[i].size == 0)
            continue;
        for (size_t j = 0; j < vec_size(tc->points[i]); ++j) {
            if (count > 0 && count % BATCH_SIZE == 0) {
                log_info("Writing batch sized %lu", count);
                len = ts_record_batch_write(
                    (const Record **)(dump.data + (count - BATCH_SIZE)), buf,
                    BATCH_SIZE);
                err = c_log_append_batch(&p->clog, buf, len);
                if (err < 0)
                    fprintf(stderr, "batch write failed: %s\n",
                            strerror(errno));
                err = p_index_append_offset(&p->index, ts_record_timestamp(buf),
                                            len - sizeof(uint64_t) * 3);
                if (err < 0)
                    fprintf(stderr, "index write failed: %s\n",
                            strerror(errno));
                buf += len;
                // Hard choices
                c_log_set_base_ns(&p->clog, tc->points[0].data[0].tv.tv_nsec);
            }
            vec_push(dump, &tc->points[i].data[j]);
            count++;
        }
    }

    // Finish up any remaining record
    size_t rem = 0;
    if (count != 0)
        rem = count % BATCH_SIZE;
    if (rem != 0) {
        size_t last_len = ts_record_batch_write(
            (const Record **)(dump.data + (count - rem)), buf, rem);
        err = c_log_append_batch(&p->clog, buf, last_len);
        if (err < 0)
            fprintf(stderr, "batch write failed: %s\n", strerror(errno));
        err = p_index_append_offset(&p->index, ts_record_timestamp(buf),
                                    len + last_len - sizeof(uint64_t) * 3);
        if (err < 0)
            fprintf(stderr, "index write failed: %s\n", strerror(errno));
    }

    free(ptr);
    vec_destroy(dump);

    return 0;
}

static uint64_t end_offset(const Partition *p, const Range *r) {
    uint64_t end = 0;
    if (r->end == -1)
        end = p->clog.size;
    else if (r->end == r->start)
        end = sizeof(uint64_t) * 3;
    else
        end = r->end - r->start;

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

    log_info("Rstart: %lu Rend: %lu", range.start, end);
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

    /* uint64_t end0 = end_offset(p, &r0); */
    uint64_t end1 = end_offset(p, &r1);

    // TODO dynamic alloc this buffer
    uint8_t buf[BLOCK_SIZE];
    uint8_t *ptr = &buf[0];

    ssize_t n = c_log_read_at(&p->clog, &ptr, r0.start, end1);
    if (n < 0)
        return -1;

    size_t record_len = 0;
    uint64_t ts = 0;
    while (n > 0) {
        record_len = read_i64(ptr);
        ts = read_i64(ptr + sizeof(uint64_t));
        if (ts == t0)
            break;
        ptr += record_len;
        n -= record_len;
    }

    memcpy(dst, ptr, n);

    return n;
}
