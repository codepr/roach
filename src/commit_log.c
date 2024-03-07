#include "commit_log.h"
#include "logging.h"
#include "binary.h"
#include "disk_io.h"
#include "timeseries.h"

int c_log_init(Commit_Log *cl, const char *path, uint64_t base) {
    char path_buf[MAX_PATH_SIZE];
    snprintf(path_buf, sizeof(path_buf), "%s/c-%.20lu", path, base);
    cl->fp = open_file(path_buf, "log", "w+");
    cl->base_timestamp = base;
    cl->base_ns = 0;
    cl->current_timestamp = base;
    cl->size = 0;
    return 0;
}

void c_log_set_base_ns(Commit_Log *cl, uint64_t ns) { cl->base_ns = ns; }

int c_log_from_disk(Commit_Log *cl, const char *path, uint64_t base) {
    char path_buf[MAX_PATH_SIZE];
    snprintf(path_buf, sizeof(path_buf), "%s/c-%.20lu", path, base);
    
    cl->fp = open_file(path_buf, "log", "r");
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

    uint64_t first_ts = ts_record_timestamp(buffer.buf);
    uint64_t latest_ts = ts_record_timestamp(buf - record_size);

    cl->size = buffer.size;
    cl->current_timestamp = latest_ts;
    cl->base_ns = first_ts % (uint64_t)1e9;

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
    cl->current_timestamp = ts_record_timestamp(batch);
    size_t start_offset = sizeof(uint64_t) * 2;
    if (cl->base_ns == 0) {
        uint64_t first_timestamp = ts_record_timestamp(batch + start_offset);
        cl->base_ns = first_timestamp % (uint64_t)1e9;
    }

    int n = write_at(cl->fp, batch + start_offset, cl->size, len);
    if (n < 0) {
        perror("write_at");
        return -1;
    }

    cl->size += len;

    return 0;
}

int c_log_read_at(const Commit_Log *cl, uint8_t **buf, size_t offset,
                  size_t len) {
    return read_at(cl->fp, *buf, offset, len);
}

void c_log_print(const Commit_Log *cl) {
    if (cl->size == 0) return;
    uint8_t buf[4096];
    uint8_t *p = &buf[0];
    ssize_t read = 0;
    uint64_t ts = 0;
    double_t value = 0.0;
    ssize_t len = read_file(cl->fp, buf);
    while (read < len) {
        ts = read_i64(p + sizeof(uint64_t));
        value = read_f64(p + sizeof(uint64_t) * 2);
        read += sizeof(uint64_t) * 2 + sizeof(double_t);
        p += sizeof(uint64_t) * 2 + sizeof(double_t);
        log_info("%lu -> %.02f", ts, value);
    }
}
