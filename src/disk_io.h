#ifndef DISK_IO_H
#define DISK_IO_H

#include <stdint.h>
#include <stdio.h>

extern const size_t MAX_PATH_SIZE;

typedef struct buffer {
    uint8_t *buf;
    size_t size;
} Buffer;

int make_dir(const char *path);

FILE *open_file(const char *path, const char *ext, const char *modes);

ssize_t get_file_size(FILE *fp, long offset);

int buf_read_file(FILE *fp, Buffer *b);

ssize_t read_file(FILE *fp, uint8_t *buf);

ssize_t read_at(FILE *fp, uint8_t *buf, size_t offset, size_t len);

ssize_t write_at(FILE *fp, const uint8_t *buf, size_t offset, size_t len);

#endif
