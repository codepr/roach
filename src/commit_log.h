#ifndef COMMIT_LOG_H
#define COMMIT_LOG_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

typedef struct commit_log {
    FILE *fp;
    size_t size;
    uint64_t base_timestamp;
    uint64_t base_ns;
    uint64_t current_timestamp;
} Commit_Log;

int c_log_init(Commit_Log *cl, const char *path, uint64_t base);

int c_log_from_disk(Commit_Log *cl, const char *path, uint64_t base);

void c_log_set_base_ns(Commit_Log *cl, uint64_t ns);

int c_log_append_data(Commit_Log *cl, const uint8_t *data, size_t len);

int c_log_append_batch(Commit_Log *cl, const uint8_t *batch, size_t len);

int c_log_read_at(const Commit_Log *cl, uint8_t **buf, size_t offset,
                  size_t len);

void c_log_print(const Commit_Log *cl);

#endif
