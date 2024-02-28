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
    printf("ok = %i\n", ok);
    if (!ok) {
        printf("Init\n");
        struct timespec tv;
        for (int i = 0; i < POINTS_NR; ++i) {
            clock_gettime(CLOCK_REALTIME, &tv);
            uint64_t timestamp = tv.tv_sec * 1e9 + tv.tv_nsec;
            timestamps[i] = timestamp;
            ts_set_record(&ts, timestamp, (double_t)i);
            usleep(10000);
        }
    }

    ts_print(&ts);

    printf("\nFind single record at %lu\n\n", timestamps[3]);
    Record r;
    (void)ts_find_record(&ts, timestamps[3], &r);
    printf(" %lu {.sec: %lu, .nsec: %lu, .value: %.02f\n", r.timestamp,
           r.tv.tv_sec, r.tv.tv_nsec, r.value);

    printf("\nFind range records at %lu - %lu\n\n", timestamps[2],
           timestamps[88]);
    Points coll;
    vec_new(coll);
    (void)ts_range(&ts, timestamps[2], timestamps[88], &coll);
    for (size_t i = 0; i < vec_size(coll); i++) {
        Record r = vec_at(coll, i);
        printf(" %lu {.sec: %lu, .nsec: %lu .value: %.02f\n", r.timestamp,
               r.tv.tv_sec, r.tv.tv_nsec, r.value);
    }

    vec_destroy(coll);

    ts_destroy(&ts);

    return 0;
}
