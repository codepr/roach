#ifndef PERSISTENT_LOG_H
#define PERSISTENT_LOG_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

typedef struct persistent_log {
    FILE *fp;
    size_t size;
    uint64_t base_timestamp;
    uint64_t current_timestamp;
} Persistent_Log;

int p_log_init(Persistent_Log *pl, const char *path, uint64_t base);

int p_log_from_disk(Persistent_Log *pl, const char *path, uint64_t base);

int p_log_append_data(Persistent_Log *pl, const uint8_t *data, size_t len);

int p_log_read_at(const Persistent_Log *pl, uint8_t **buf, size_t offset,
                  size_t len);

#endif
