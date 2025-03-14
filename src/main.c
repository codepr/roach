#include "commit_log.h"
#include "logging.h"
#include "parser.h"
#include "persistent_index.h"
#include "protocol.h"
#include "server.h"
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

int main(void)
{
    /* uint64_t timestamps[POINTS_NR]; */

    /* Timeseries_DB *db = tsdb_init("testdb"); */
    /* if (!db) { */
    /*     log_error("Panic: mkdir"); */
    /*     abort(); */
    /* } */

    /* Timeseries *ts = ts_create(db, "temperatures", 0, IGNORE); */
    /* /\* Timeseries *ts = ts_get(db, "temperatures"); *\/ */
    /* if (!ts) { */
    /*     log_error("Panic: mkdir"); */
    /*     abort(); */
    /* } */

    /* struct timespec tv; */
    /* for (int i = 0; i < POINTS_NR; ++i) { */
    /*     clock_gettime(CLOCK_REALTIME, &tv); */
    /*     uint64_t timestamp = tv.tv_sec * 1e9 + tv.tv_nsec; */
    /*     timestamps[i] = timestamp; */
    /*     ts_insert(ts, timestamp, (double_t)i); */
    /*     usleep(115000); */
    /* } */

    /* index_print(&ts->partitions[0].index); */

    /* ts_print(ts); */
    /* log_info("Print log"); */
    /* for (size_t i = 0; i <= ts->partition_nr; ++i) */
    /*     c_log_print(&ts->partitions[i].clog); */

    /* log_info("Find single record at %lu", timestamps[2]); */
    /* Record r; */
    /* (void)ts_find(ts, timestamps[2], &r); */
    /* log_info(" %lu {.sec: %lu, .nsec: %lu, .value: %.02f}", r.timestamp, */
    /*          r.tv.tv_sec, r.tv.tv_nsec, r.value); */

    /* log_info("Find range records at %lu - %lu", timestamps[74],
     * timestamps[88]); */
    /* Points coll; */
    /* vec_new(coll); */
    /* (void)ts_range(ts, timestamps[74], timestamps[88], &coll); */
    /* for (size_t i = 0; i < vec_size(coll); i++) { */
    /*     Record r = vec_at(coll, i); */
    /*     log_info(" %lu {.sec: %lu, .nsec: %lu .value: %.02f }", r.timestamp,
     */
    /*              r.tv.tv_sec, r.tv.tv_nsec, r.value); */
    /* } */

    /* vec_destroy(coll); */
    /* vec_new(coll); */

    /* /\* log_info("Attempting a read from disk"); *\/ */

    /* /\* index_print(&p.index); *\/ */

    /* log_info("Find single record at %lu", timestamps[51]); */
    /* ts_find(ts, timestamps[51], &r); */
    /* log_info("%lu %.20lf", r.timestamp, r.value); */

    /* log_info("Find single record at %lu", timestamps[0]); */
    /* ts_find(ts, timestamps[0], &r); */
    /* log_info("%lu %.20lf", r.timestamp, r.value); */

    /* log_info("Find single record at %lu", timestamps[23]); */
    /* ts_find(ts, timestamps[23], &r); */
    /* log_info("%lu %.20lf", r.timestamp, r.value); */

    /* log_info("Find single record at %lu", timestamps[89]); */
    /* ts_find(ts, timestamps[89], &r); */
    /* log_info("%lu %.20lf", r.timestamp, r.value); */

    /* /\* c_log_print(&p.clog); *\/ */

    /* /\* index_print(ts->partitions[0].index); *\/ */

    /* log_info("Looking for record: %lu", timestamps[88]); */
    /* ts_find(ts, timestamps[88], &r); */
    /* log_info(" %lu {.sec: %lu, .nsec: %lu, .value: %.02f}", r.timestamp, */
    /*          r.tv.tv_sec, r.tv.tv_nsec, r.value); */

    /* log_info("[1] Looking for a range: %lu - %lu", timestamps[1], */
    /*          timestamps[41]); */
    /* ts_range(ts, timestamps[1], timestamps[41], &coll); */
    /* for (size_t i = 0; i < vec_size(coll); i++) { */
    /*     Record r = vec_at(coll, i); */
    /*     log_info(" %lu {.sec: %lu, .nsec: %lu .value: %.02f}", r.timestamp,
     */
    /*              r.tv.tv_sec, r.tv.tv_nsec, r.value); */
    /* } */

    /* vec_destroy(coll); */
    /* vec_new(coll); */

    /* log_info("[2] Looking for a range: %lu - %lu", timestamps[1], */
    /*          timestamps[89]); */
    /* ts_range(ts, timestamps[1], timestamps[89], &coll); */
    /* for (size_t i = 0; i < vec_size(coll); i++) { */
    /*     Record r = vec_at(coll, i); */
    /*     log_info(" %lu {.sec: %lu, .nsec: %lu .value: %.02f}", r.timestamp,
     */
    /*              r.tv.tv_sec, r.tv.tv_nsec, r.value); */
    /* } */

    /* log_info("[3] Add an out of bounds timestamp"); */
    /* ts_insert(ts, timestamps[89] + 5e9, 181.1); */
    /* for (size_t i = 0; i <= ts->partition_nr; ++i) */
    /*     c_log_print(&ts->partitions[i].clog); */
    /* (void)ts_find(ts, timestamps[89] + 5e9, &r); */
    /* log_info(" %lu {.sec: %lu, .nsec: %lu .value: %.02f}", r.timestamp, */
    /*          r.tv.tv_sec, r.tv.tv_nsec, r.value); */

    /* log_info("[4] Add a prev range timestamp"); */
    /* ts_insert(ts, timestamps[89] + 5e7, 141.231); */
    /* for (size_t i = 0; i <= ts->partition_nr; ++i) */
    /*     c_log_print(&ts->partitions[i].clog); */
    /* (void)ts_find(ts, timestamps[89] + 5e7, &r); */
    /* log_info(" %lu {.sec: %lu, .nsec: %lu .value: %.02f}", r.timestamp, */
    /*          r.tv.tv_sec, r.tv.tv_nsec, r.value); */

    /* log_info("[5] Add an out of order in range timestamp"); */
    /* ts_insert(ts, timestamps[85] + 3e4, 111.231); */
    /* ts_print(ts); */
    /* (void)ts_find(ts, timestamps[85] + 3e4, &r); */
    /* log_info(" %lu {.sec: %lu, .nsec: %lu .value: %.02f}", r.timestamp, */
    /*          r.tv.tv_sec, r.tv.tv_nsec, r.value); */

    /* vec_destroy(coll); */
    /* ts_close(ts); */

    /* Timeseries_DB *db = tsdb_init("testdb"); */
    /* if (!db) { */
    /*     log_info("Panic: mkdir"); */
    /*     abort(); */
    /* } */

    /* /\* Timeseries *ts = ts_create(db, "temperatures", 0); *\/ */
    /* Timeseries *ts = ts_get(db, "temperatures"); */
    /* if (!ts) { */
    /*     log_info("Panic: mkdir"); */
    /*     abort(); */
    /* } */

    /* struct timespec tv; */
    /* clock_gettime(CLOCK_REALTIME, &tv); */
    /* uint64_t timestamp1 = tv.tv_sec * 1e9 + tv.tv_nsec; */
    /* ts_insert(ts, timestamp1, 25.5); */
    /* usleep(15000); */
    /* clock_gettime(CLOCK_REALTIME, &tv); */
    /* uint64_t timestamp2 = tv.tv_sec * 1e9 + tv.tv_nsec; */
    /* ts_insert(ts, timestamp2, 27.3); */

    /* Record r = {0}; */
    /* int result = ts_find(ts, timestamp1, &r); */
    /* if (result == 0) { */
    /*     printf("Record found: timestamp=%lu value=%.2f\n", r.timestamp, */
    /*            r.value); */
    /* } else { */
    /*     printf("Record not found.\n"); */
    /* } */
    /* result = ts_find(ts, timestamp2, &r); */
    /* if (result == 0) { */
    /*     printf("Record found: timestamp=%lu value=%.2f\n", r.timestamp, */
    /*            r.value); */
    /* } else { */
    /*     printf("Record not found.\n"); */
    /* } */

    /* ts_close(ts); */
    /* int run = 1; */
    /* Timeseries_DB *db = tsdb_init("testdb"); */
    /* if (!db) { */
    /*     log_info("Panic: mkdir"); */
    /*     abort(); */
    /* } */
    /* Timeseries *ts; */
    /* Record r = {0}; */

    /* while (run) { */
    /*     // Read command from client */
    /*     char input[256]; */
    /*     fgets(input, sizeof(input), stdin); */
    /*     input[strcspn(input, "\r\n")] = 0; */

    /*     Command cmd = parse_command(input); */

    /*     switch (cmd.type) { */
    /*     case CREATE: */
    /*         ts = ts_create(db, cmd.metric, 0); */
    /*         printf("+Ok\n"); */
    /*         break; */
    /*     case INSERT: */
    /*         ts = ts != NULL && strncmp(cmd.metric, ts->name,
     * strlen(cmd.metric)) */
    /*                  ? ts */
    /*                  : ts_get(db, cmd.metric); */
    /*         ts_insert(ts, cmd.timestamp, cmd.value); */
    /*         printf("+Ok (%lu)\n", cmd.timestamp); */
    /*         break; */
    /*     case SELECT: */
    /*         ts = ts != NULL && strncmp(cmd.metric, ts->name,
     * strlen(cmd.metric)) */
    /*                  ? ts */
    /*                  : ts_get(db, cmd.metric); */
    /*         printf("metric %s (%lu) vs ts name %s (%lu), %i\n",
     * cmd.metric,
     */
    /*                strlen(cmd.metric), ts->name, strlen(ts->name), */
    /*                strncmp(cmd.metric, ts->name, strlen(cmd.metric))); */
    /*         ts_find(ts, cmd.timestamp, &r); */
    /*         printf("%lu %0.2lf\n", r.timestamp, r.value); */
    /*         break; */
    /*     case QUIT: */
    /*         free(ts); */
    /*         run = 0; */
    /*         break; */
    /*     default: */
    /*         log_error("Unknown command"); */
    /*     } */
    /* } */

    /* tsdb_close(db); */

    /* Statement s = parse("CREATE timeseries"); */

    /* print_statement(&s); */
    /* printf("\n"); */

    /* s = parse("INSERT temperatures INTO db_test 12, 98.2, 15, 96.2, 18, 99.1
     * "); */

    /* print_statement(&s); */
    /* printf("\n"); */

    /* s = parse("SELECT temperatures FROM test_db RANGE 10 TO 45 WHERE value >
     * " */
    /*           "67.8 AGGREGATE AVG BY 3600"); */

    /* print_statement(&s); */

    uint8_t dst[64];
    /* Response rs = {.type = STRING_RSP, */
    /*                .string_response = {.length = 3, .rc = 0, .message =
     * "Ok!"}}; */
    /* encode_response(&rs, &dst[0]); */
    /* printf("%s", dst); */
    /* Response rsb; */
    /* decode_response(&dst[0], &rsb); */
    /* printf("(%i) %s (%lu)\n", rsb.type, rsb.string_response.message, */
    /*        rsb.string_response.length); */

    Request rq = {.length = 36,
                  .query  = "SELECT temp FROM test RANGE 10 TO 45"};
    encode_request(&rq, &dst[0]);
    printf("%s", dst);

    Request rqb;
    decode_request(&dst[0], &rqb);
    printf("%s (%lu)\n", rqb.query, rqb.length);
    /* Select_Response r = {.length = 2, */
    /*                      .db_name = (String_View){.p = "test-db", .length
     * = 8}, */
    /*                      .ts_name = (String_View){.p = "test-ts", .length
     * = 8}, */
    /*                      .records = {{.timestamp = 1982398, .value =
     * 0.7227},
     */
    /*                                  {.timestamp = 1982398, .value =
     * 0.7227}}}; */

    roachdb_server_run("127.0.0.1", 17678);

    return 0;
}
