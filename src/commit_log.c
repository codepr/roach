#include "commit_log.h"
#include "binary.h"
#include "disk_io.h"
#include "timeseries.h"

int c_log_init(Commit_Log *cl, const char *path, uint64_t base) {
    cl->fp = open_file(path, "log", base);
    cl->base_timestamp = base;
    cl->current_timestamp = base;
    cl->size = 0;
    return 0;
}

int c_log_from_disk(Commit_Log *cl, const char *path, uint64_t base) {
    cl->fp = open_file(path, "log", base);
    cl->base_timestamp = base;

    uint64_t record_size = 0;

    Buffer buffer;
    if (buf_read_file(cl->fp, &buffer) < 0)
        return -1;
    size_t size = buffer.size;
    uint8_t *buf = buffer.buf;

    while (size > 0) {
        record_size = read_i64(buf);
        size -= record_size;
        buf += record_size;
    }

    uint64_t latest_ts = ts_record_timestamp(buf - record_size);

    cl->size = buffer.size;
    cl->current_timestamp = latest_ts;
    cl->current_timestamp = latest_ts;

    free(buffer.buf);

    return 0;
}

int c_log_append_data(Commit_Log *cl, const uint8_t *data, size_t len) {
    int bytes = write_at(cl->fp, data, cl->size, len);
    if (bytes < 0) {
        perror("write_at");
        return -1;
    }
    cl->size += bytes;
    cl->current_timestamp = ts_record_timestamp(data);
    return 0;
}

int c_log_append_batch(Commit_Log *cl, const uint8_t *batch, size_t len) {
    printf("Writing %lu\n", len);
    cl->current_timestamp = ts_record_timestamp(batch);
    int bytes = write_at(cl->fp, batch + sizeof(uint64_t) * 2, cl->size,
                         len - (sizeof(uint64_t) * 2));
    if (bytes < 0) {
        perror("write_at");
        return -1;
    }
    printf("Written %i\n", bytes);
    cl->size += len - (sizeof(uint64_t) * 2);
    return 0;
}

int c_log_read_at(const Commit_Log *cl, uint8_t **buf, size_t offset,
                  size_t len) {
    return read_at(cl->fp, *buf, offset, len);
}
