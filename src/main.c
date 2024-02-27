#include "timeseries.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

int main(void) {
    uint64_t timestamps[50];
    Timeseries ts = ts_new("tsdata", 0);
    int ok = ts_init(&ts);
    if (!ok) {
        printf("Init\n");
        struct timespec tv;
        for (int i = 0; i < 50; ++i) {
            clock_gettime(CLOCK_REALTIME, &tv);
            uint64_t timestamp = tv.tv_sec * 1e9 + tv.tv_nsec;
            timestamps[i] = timestamp;
            ts_set_record(&ts, timestamp, (double_t)i);
            usleep(1000);
        }
    }

    ts_print(&ts);

    printf("TS: %lu\n", timestamps[3]);
    Record r = ts_find_record(&ts, timestamps[3]);
    printf(" {.timestamp: %lu, .value: %.02f\n", r.timestamp, r.value);

    Points coll = ts_range(&ts, timestamps[3], timestamps[9]);
    for (int i = 0; i < 6; i++) {
        Record r = VEC_AT(coll, i);
        printf(" {.timestamp: %lu, .value: %.02f\n", r.timestamp, r.value);
    }

    ts_destroy(&ts);

    return 0;
}
