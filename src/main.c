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
        for (int i = 0; i < 20; ++i) {
            ts_set_record(&ts, time(NULL), (double_t)i);
            sleep(1);
        }
    }

    for (int i = 0; i < 20; ++i) {
        printf("%lu - %.02f\n", ts.current_chunk.columns[i].timestamp,
               ts.current_chunk.columns[i].value);
    }
    return 0;
}
