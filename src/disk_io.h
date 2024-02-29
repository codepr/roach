#ifndef DISK_IO_H
#define DISK_IO_H

#include <stdint.h>
#include <stdio.h>

FILE *open_file(const char *path, const char *ext, uint64_t base);

ssize_t read_file(FILE *fp, uint8_t *buf);

ssize_t read_at(FILE *fp, uint8_t *buf, size_t offset, size_t len);

ssize_t write_at(FILE *fp, const uint8_t *buf, size_t offset, size_t len);

#endif
