#include "timeseries.h"
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

int main(void) {
    Timeseries ts = ts_new("tsdata", 0);
    for (int i = 0; i < 20; ++i) {
        ts_set_record(&ts, time(NULL), (double_t)i);
        sleep(1);
    }
    return 0;
}
