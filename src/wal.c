#include "wal.h"
#include "binary.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define MAX_PATH_SIZE 512

static int make_dir(const char *path) {
    struct stat st = {0};

    if (stat(path, &st) == -1) {
        return mkdir(path, 0700);
    }
    return 0;
}

static int open_file(const char *path, const char *ext, uint64_t base, int m) {
    if (make_dir(path) < 0)
        return -1;
    const char t[2] = {'o', 'm'};
    char path_buf[MAX_PATH_SIZE];
    snprintf(path_buf, sizeof(path_buf), "%s/%c-%.20lu.%s", path, t[m], base,
             ext);
    int fd = open(path_buf, O_CREAT | O_RDWR, 0644);
    if (fd < 0) {
        fprintf(stderr, "Cannot open %s: %s", path_buf, strerror(errno));
        return -1;
    }
    return fd;
}

static long get_file_size(const char *filename) {
    FILE *fp = fopen(filename, "r");

    if (fp == NULL)
        return -1;

    if (fseek(fp, 0, SEEK_END) < 0) {
        fclose(fp);
        return -1;
    }

    long size = ftell(fp);
    // release the resources when not required
    fclose(fp);
    return size;
}

int wal_init(Wal *w, const char *path, uint64_t base_timestamp, int main) {
    int fd = open_file(path, "log", base_timestamp, main);
    if (fd < 0)
        goto errdefer;

    w->fd = fd;
    w->size = 0;

    return 0;

errdefer:
    fprintf(stderr, "WAL init %s: %s", path, strerror(errno));
    return -1;
}

int wal_from_disk(Wal *w, const char *path, uint64_t base_timestamp, int main) {
    int fd = open_file(path, "log", base_timestamp, main);
    if (fd < 0)
        goto errdefer;

    w->fd = fd;

    char path_buf[MAX_PATH_SIZE];
    snprintf(path_buf, sizeof(path_buf), "%s/%c-%.20lu.log", path,
             main == 1 ? 'm' : 'o', base_timestamp);
    w->size = get_file_size(path_buf);

    return 0;

errdefer:
    fprintf(stderr, "WAL from disk %s: %s", path, strerror(errno));
    return -1;
}

int wal_append_record(Wal *wal, uint64_t ts, double_t value) {
    size_t len = sizeof(uint64_t) + sizeof(double_t);
    uint8_t buf[len];

    write_i64(buf, ts);
    write_f64(buf + sizeof(uint64_t), value);

    // TODO Fix to handle multiple points in the same timestamp
    if (pwrite(wal->fd, buf, len, wal->size) < 0)
        return -1;
    wal->size += len;
    return 0;
}
