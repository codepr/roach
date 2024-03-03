#include "wal.h"
#include "binary.h"
#include "disk_io.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static const char t[2] = {'t', 'h'};

int wal_init(Wal *w, const char *path, uint64_t base_timestamp, int main) {
    char path_buf[MAX_PATH_SIZE];
    snprintf(path_buf, sizeof(path_buf), "%s/wal-%c-%.20lu", path, t[main],
             base_timestamp);
    w->fp = open_file(path_buf, "log", "w+");
    if (!w->fp)
        goto errdefer;

    w->size = 0;

    return 0;

errdefer:
    fprintf(stderr, "WAL init %s: %s", path, strerror(errno));
    return -1;
}

int wal_from_disk(Wal *w, const char *path, uint64_t base_timestamp, int main) {
    char path_buf[MAX_PATH_SIZE];
    snprintf(path_buf, sizeof(path_buf), "%s/wal-%c-%.20lu", path, t[main],
             base_timestamp);
    w->fp = open_file(path_buf, "log", "r");
    if (!w->fp)
        goto errdefer;

    w->size = get_file_size(w->fp, 0);

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
    if (write_at(wal->fp, buf, wal->size, len) < 0)
        return -1;
    wal->size += len;
    return 0;
}
