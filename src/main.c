#include "commit_log.h"
#include "logging.h"
#include "partition.h"
#include "timeseries.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define POINTS_NR 90

int main(void) {
    uint64_t timestamps[POINTS_NR];

    Timeseries ts = ts_new("tsdata", 0);
    int ok = ts_init(&ts);
    if (!ok) {
        log_info("Init");
        struct timespec tv;
        for (int i = 0; i < POINTS_NR; ++i) {
            clock_gettime(CLOCK_REALTIME, &tv);
            uint64_t timestamp = tv.tv_sec * 1e9 + tv.tv_nsec;
            timestamps[i] = timestamp;
            ts_set_record(&ts, timestamp, (double_t)i);
            usleep(15000);
        }
    }
    (void)timestamps;

    ts_print(&ts);
    log_info("Print log");
    for (size_t i = 0; i <= ts.last_partition; ++i)
        c_log_print(&ts.partitions[i].clog);

    log_info("Find single record at %lu", timestamps[2]);
    Record r;
    (void)ts_find_record(&ts, timestamps[2], &r);
    log_info(" %lu {.sec: %lu, .nsec: %lu, .value: %.02f}", r.timestamp,
             r.tv.tv_sec, r.tv.tv_nsec, r.value);

    /* log_info("Find range records at %lu - %lu\n", timestamps[2], */
    /*          timestamps[88]); */
    /* Points coll; */
    /* vec_new(coll); */
    /* (void)ts_range(&ts, timestamps[2], timestamps[88], &coll); */
    /* for (size_t i = 0; i < vec_size(coll); i++) { */
    /*     Record r = vec_at(coll, i); */
    /*     log_info(" %lu {.sec: %lu, .nsec: %lu .value: %.02f", r.timestamp, */
    /*              r.tv.tv_sec, r.tv.tv_nsec, r.value); */
    /* } */

    /* vec_destroy(coll); */

    /* log_info("Attempting a read from disk"); */

    /* p_index_print(&p.index); */

    /* uint8_t buf[512]; */
    /* partition_find(&p, buf, timestamps[51]); */
    /* ts_record_read(&r, buf); */
    /* log_info("%lu %.20lf", r.timestamp, r.value); */

    /* partition_find(&p, buf, timestamps[0]); */
    /* ts_record_read(&r, buf); */
    /* log_info("%lu %.20lf", r.timestamp, r.value); */

    /* partition_find(&p, buf, timestamps[23]); */
    /* ts_record_read(&r, buf); */
    /* log_info("%lu %.20lf", r.timestamp, r.value); */

    /* partition_find(&p, buf, timestamps[89]); */
    /* ts_record_read(&r, buf); */
    /* log_info("%lu %.20lf", r.timestamp, r.value); */

    /* c_log_print(&p.clog); */

    log_info("Looking for record: %lu", timestamps[88]);
    ts_find_record(&ts, timestamps[88], &r);
    log_info(" %lu {.sec: %lu, .nsec: %lu, .value: %.02f}", r.timestamp,
             r.tv.tv_sec, r.tv.tv_nsec, r.value);

    ts_destroy(&ts);

    return 0;
}
