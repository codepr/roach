#include "binary.h"
#include "commit_log.h"
#include "disk_io.h"
#include "logging.h"
#include "partition.h"
#include "protocol.h"
#include "timeseries.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define POINTS_NR 90

/* static int read_timestamps(FILE *fp, uint64_t timestamps[POINTS_NR]) { */
/*     uint8_t buf[1024]; */
/*     ssize_t n = 0; */
/*     if ((n = read_file(fp, buf)) < 0) */
/*         return -1; */

/*     uint8_t *ptr = &buf[0]; */
/*     size_t i = 0; */
/*     while (n > 0) { */
/*         timestamps[i++] = read_i64(ptr); */
/*         ptr += sizeof(uint64_t); */
/*         n -= sizeof(uint64_t); */
/*     } */

/*     return 0; */
/* } */

/* static int dump_timestamps(FILE *fp, const uint64_t timestamps[POINTS_NR]) {
 */
/*     uint8_t buf[1024]; */
/*     uint8_t *ptr = &buf[0]; */
/*     for (size_t i = 0; i < POINTS_NR; ++i) { */
/*         write_i64(ptr, timestamps[i]); */
/*         ptr += sizeof(uint64_t); */
/*     } */
/*     if (write_at(fp, buf, 0, POINTS_NR * sizeof(uint64_t)) < 0) */
/*         return -1; */
/*     return 0; */
/* } */

int main(void) {
    /* uint64_t timestamps[POINTS_NR]; */
    /* Timeseries ts = ts_new("tsdata", 0); */
    /* int ok = ts_init(&ts); */
    /* if (!ok) { */
    /*     FILE *fp = open_file("logdata/tsdata/timestamps", "ts", "w"); */
    /*     struct timespec tv; */
    /*     for (int i = 0; i < POINTS_NR; ++i) { */
    /*         clock_gettime(CLOCK_REALTIME, &tv); */
    /*         uint64_t timestamp = tv.tv_sec * 1e9 + tv.tv_nsec; */
    /*         timestamps[i] = timestamp; */
    /*         ts_set_record(&ts, timestamp, (double_t)i); */
    /*         usleep(15000); */
    /*     } */
    /*     dump_timestamps(fp, timestamps); */
    /*     fclose(fp); */
    /* } else { */
    /*     FILE *fp = open_file("logdata/tsdata/timestamps", "ts", "r"); */
    /*     read_timestamps(fp, timestamps); */
    /*     fclose(fp); */
    /* } */

    /* ts_print(&ts); */
    /* log_info("Print log"); */
    /* for (size_t i = 0; i <= ts.last_partition; ++i) */
    /*     c_log_print(&ts.partitions[i].clog); */

    /* log_info("Find single record at %lu", timestamps[2]); */
    /* Record r; */
    /* (void)ts_find_record(&ts, timestamps[2], &r); */
    /* log_info(" %lu {.sec: %lu, .nsec: %lu, .value: %.02f}", r.timestamp, */
    /*          r.tv.tv_sec, r.tv.tv_nsec, r.value); */

    /* log_info("Find range records at %lu - %lu", timestamps[74],
     * timestamps[88]); */
    /* Points coll; */
    /* vec_new(coll); */
    /* (void)ts_range(&ts, timestamps[74], timestamps[88], &coll); */
    /* for (size_t i = 0; i < vec_size(coll); i++) { */
    /*     Record r = vec_at(coll, i); */
    /*     log_info(" %lu {.sec: %lu, .nsec: %lu .value: %.02f }", r.timestamp,
     */
    /*              r.tv.tv_sec, r.tv.tv_nsec, r.value); */
    /* } */

    /* vec_destroy(coll); */
    /* vec_new(coll); */

    /* /\* log_info("Attempting a read from disk"); *\/ */

    /* /\* p_index_print(&p.index); *\/ */

    /* log_info("Find single record at %lu", timestamps[51]); */
    /* ts_find_record(&ts, timestamps[51], &r); */
    /* log_info("%lu %.20lf", r.timestamp, r.value); */

    /* log_info("Find single record at %lu", timestamps[0]); */
    /* ts_find_record(&ts, timestamps[0], &r); */
    /* log_info("%lu %.20lf", r.timestamp, r.value); */

    /* log_info("Find single record at %lu", timestamps[23]); */
    /* ts_find_record(&ts, timestamps[23], &r); */
    /* log_info("%lu %.20lf", r.timestamp, r.value); */

    /* log_info("Find single record at %lu", timestamps[89]); */
    /* ts_find_record(&ts, timestamps[89], &r); */
    /* log_info("%lu %.20lf", r.timestamp, r.value); */

    /* /\* c_log_print(&p.clog); *\/ */

    /* /\* p_index_print(&ts.partitions[0].index); *\/ */

    /* log_info("Looking for record: %lu", timestamps[88]); */
    /* ts_find_record(&ts, timestamps[88], &r); */
    /* log_info(" %lu {.sec: %lu, .nsec: %lu, .value: %.02f}", r.timestamp, */
    /*          r.tv.tv_sec, r.tv.tv_nsec, r.value); */

    /* log_info("[1] Looking for a range: %lu - %lu", timestamps[1], */
    /*          timestamps[41]); */
    /* ts_range(&ts, timestamps[1], timestamps[41], &coll); */
    /* for (size_t i = 0; i < vec_size(coll); i++) { */
    /*     Record r = vec_at(coll, i); */
    /*     log_info(" %lu {.sec: %lu, .nsec: %lu .value: %.02f", r.timestamp, */
    /*              r.tv.tv_sec, r.tv.tv_nsec, r.value); */
    /* } */

    /* vec_destroy(coll); */
    /* vec_new(coll); */

    /* log_info("[2] Looking for a range: %lu - %lu", timestamps[1], */
    /*          timestamps[89]); */
    /* ts_range(&ts, timestamps[1], timestamps[89], &coll); */
    /* for (size_t i = 0; i < vec_size(coll); i++) { */
    /*     Record r = vec_at(coll, i); */
    /*     log_info(" %lu {.sec: %lu, .nsec: %lu .value: %.02f", r.timestamp, */
    /*              r.tv.tv_sec, r.tv.tv_nsec, r.value); */
    /* } */

    /* vec_destroy(coll); */
    /* ts_destroy(&ts); */

    Timeseries ts = {0};
    Record r = {0};

    while (1) {
        // Read command from client
        char input[256];
        fgets(input, sizeof(input), stdin);
        input[strcspn(input, "\r\n")] = 0;

        Command cmd = parse_command(input);

        switch (cmd.type) {
        case CREATE:
            ts = ts_new(cmd.metric, 0);
            printf("+Ok\n");
            break;
        case INSERT:
            ts_set_record(&ts, cmd.timestamp, cmd.value);
            printf("+Ok (%lu)\n", cmd.timestamp);
            break;
        case SELECT:
            ts_find_record(&ts, cmd.timestamp, &r);
            printf("%lu %0.2lf\n", r.timestamp, r.value);
            break;
        default:
            log_error("Unknown command");
        }
    }
    return 0;
}
