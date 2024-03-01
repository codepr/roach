#include "partition.h"
#include "commit_log.h"
#include "persistent_index.h"
#include "timeseries.h"
#include "vec.h"
#include <errno.h>
#include <string.h>

static const size_t INTERVAL = 1 << 6;

static int partition_init(Partition *p, const char *path, uint64_t base) {
    int err = c_log_init(&p->clog, path, base);
    if (err < 0)
        return -1;

    err = p_index_init(&p->index, path, base, INTERVAL);
    if (err < 0)
        return -1;

    return 0;
}

int partition_dump_timeseries_chunk(Partition *p, const Timeseries_Chunk *tc) {
    int err = partition_init(p, "logdata/tsdata", tc->base_offset);
    if (err < 0)
        return -1;
    VEC(Record *) dump;
    vec_new(dump);

    size_t count = 0;
    uint8_t *buf =
        malloc(sizeof(uint64_t) * 2 +
               (((sizeof(uint64_t) * 2) + sizeof(double_t)) * INTERVAL * 900));

    uint8_t *ptr = buf;

    for (size_t i = 0; i < 900; ++i) {
        if (tc->points[i].size == 0)
            continue;
        for (size_t j = 0; j < vec_size(tc->points[i]); ++j) {
            if (count > 0 && count % INTERVAL == 0) {
                printf("Write %lu\n", count);
                size_t len = ts_record_batch_write(
                    (const Record **)(dump.data +
                                      (count - INTERVAL) * sizeof(Record *)),
                    buf, INTERVAL);
                err = c_log_append_batch(&p->clog, buf, len);
                if (err < 0)
                    fprintf(stderr, "batch write failed: %s\n",
                            strerror(errno));
                buf += len;
                err = p_index_append_offset(&p->index, ts_record_timestamp(buf),
                                            len);
                if (err < 0)
                    fprintf(stderr, "index write failed: %s\n",
                            strerror(errno));
            }
            vec_push(dump, &tc->points[i].data[j]);
            count++;
        }
    }

    // Finish up any remaining record
    size_t rem = count != 0 && count % INTERVAL;
    printf("Interval %lu Rem %lu size %lu\n", count, rem, vec_size(dump));
    if (rem != 0) {
        size_t len = ts_record_batch_write(
            (const Record **)(&dump.data[(count - rem)]), buf, rem);
        err = c_log_append_batch(&p->clog, buf, len);
        if (err < 0)
            fprintf(stderr, "batch write failed: %s\n", strerror(errno));
        err = p_index_append_offset(&p->index, ts_record_timestamp(buf), len);
        if (err < 0)
            fprintf(stderr, "index write failed: %s\n", strerror(errno));
    }

    free(ptr);
    vec_destroy(dump);

    return 0;
}

int partition_find(const Partition *p, uint8_t *dst, uint64_t timestamp) {
    (void)p;
    (void)dst;
    (void)timestamp;
    return 0;
}
