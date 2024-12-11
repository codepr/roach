#include "wal.h"
#include "binary.h"
#include "disk_io.h"
#include "logging.h"
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static const char t[2] = {'t', 'h'};

int wal_init(Wal *w, const char *path, uint64_t base_timestamp, int main)
{
    snprintf(w->path, sizeof(w->path), "%s/wal-%c-%.20" PRIu64, path, t[main],
             base_timestamp);
    w->fp = open_file(w->path, "log", "w+");
    if (!w->fp)
        goto errdefer;

    w->size = 0;

    return 0;

errdefer:
    log_error("WAL init %s: %s", path, strerror(errno));
    return -1;
}

int wal_delete(Wal *w)
{
    if (!w->fp)
        return -1;
    int err = fclose(w->fp);
    if (err < 0)
        return -1;
    w->size = 0;
    char tmp[WAL_PATH_SIZE + 5];
    snprintf(tmp, sizeof(tmp), "%s.log", w->path);

    return remove(tmp);
}

int wal_from_disk(Wal *w, const char *path, uint64_t base_timestamp, int main)
{
    char path_buf[MAX_PATH_SIZE];
    snprintf(path_buf, sizeof(path_buf), "%s/wal-%c-%.20" PRIu64, path, t[main],
             base_timestamp);
    w->fp = open_file(path_buf, "log", "r");
    if (!w->fp)
        goto errdefer;

    w->size = get_file_size(w->fp, 0);

    return 0;

errdefer:
    log_error("WAL from disk %s: %s", path, strerror(errno));
    return -1;
}

int wal_append_record(Wal *wal, uint64_t ts, double_t value)
{
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

size_t wal_size(const Wal *wal) { return wal->size; }
