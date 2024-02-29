#include "persistent_log.h"
#include "binary.h"
#include "disk_io.h"
#include "timeseries.h"

int p_log_init(Persistent_Log *pl, const char *path, uint64_t base) {
    pl->fp = open_file(path, "log", base);
    pl->base_timestamp = base;
    pl->current_timestamp = base;
    pl->size = 0;
    return 0;
}

int p_log_from_disk(Persistent_Log *pl, const char *path, uint64_t base) {
    pl->fp = open_file(path, "log", base);
    pl->base_timestamp = base;

    uint64_t record_size = 0;

    Buffer buffer;
    if (buf_read_file(pl->fp, &buffer) < 0)
        return -1;
    size_t size = buffer.size;
    uint8_t *buf = buffer.buf;

    while (size > 0) {
        record_size = read_i64(buf);
        size -= record_size;
        buf += record_size;
    }

    uint64_t latest_ts = ts_record_timestamp(buf - record_size);

    pl->size = buffer.size;
    pl->current_timestamp = latest_ts;
    pl->current_timestamp = latest_ts;

    free(buffer.buf);

    return 0;
}

int p_log_append_data(Persistent_Log *pl, const uint8_t *data, size_t len) {
    int bytes = write_at(pl->fp, data, pl->size, len);
    if (bytes < 0) {
        perror("write_at");
        return -1;
    }
    pl->size += bytes;
    pl->current_timestamp = ts_record_timestamp(data);
    return 0;
}

int p_log_append_batch(Persistent_Log *pl, const uint8_t *batch, size_t len) {
    pl->current_timestamp = ts_record_timestamp(batch);
    int bytes = write_at(pl->fp, batch + sizeof(uint64_t) * 2, pl->size,
                         len - (sizeof(uint64_t) * 2));
    if (bytes < 0) {
        perror("write_at");
        return -1;
    }
    pl->size += len - (sizeof(uint64_t) * 2);
    return 0;
}

int p_log_read_at(const Persistent_Log *pl, uint8_t **buf, size_t offset,
                  size_t len) {
    return read_at(pl->fp, *buf, offset, len);
}
