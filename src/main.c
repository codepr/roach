#include "timeseries.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

int main(void) {
    Timeseries ts = ts_new("tsdata", 0);
    int ok = ts_init(&ts);
    if (!ok) {
        printf("Init\n");
        struct timespec tv;
        for (int i = 0; i < 20; ++i) {
            clock_gettime(CLOCK_REALTIME, &tv);
            uint64_t timestamp = tv.tv_sec * 1e9 + tv.tv_nsec;
            ts_set_record(&ts, timestamp, (double_t)i);
            usleep(100);
        }
    }

    ts_print(&ts);

    ts_destroy(&ts);

    return 0;
}
