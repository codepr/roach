#include "persistent_index.h"
#include "binary.h"
#include "disk_io.h"
#include "logging.h"

// relative timestamp -> main segment offset position in the file
static const size_t ENTRY_SIZE = sizeof(uint64_t) * 2;
static const size_t INDEX_SIZE = 1 << 12;

int p_index_init(Persistent_Index *pi, const char *path, uint64_t base) {
    char path_buf[MAX_PATH_SIZE];
    snprintf(path_buf, sizeof(path_buf), "%s/i-%.20lu", path, base);

    pi->fp = open_file(path_buf, "index", "w+");
    if (!pi->fp)
        return -1;

    pi->size = 0;
    pi->base_timestamp = base;

    return 0;
}

int p_index_close(Persistent_Index *pi) { return fclose(pi->fp); }

int p_index_from_disk(Persistent_Index *pi, const char *path, uint64_t base) {
    char path_buf[MAX_PATH_SIZE];
    snprintf(path_buf, sizeof(path_buf), "%s/i-%.20lu", path, base);

    pi->fp = open_file(path_buf, "index", "r");
    if (!pi->fp)
        return -1;

    pi->size = get_file_size(pi->fp, 0);
    pi->base_timestamp = base;

    return 0;
}

int p_index_append_offset(Persistent_Index *pi, uint64_t ts, uint64_t offset) {
    uint64_t relative_ts = ts - (pi->base_timestamp * 1e9);

    // Serialize the position into integer 64bits
    uint8_t buf[ENTRY_SIZE];
    write_i64(buf, relative_ts);
    write_i64(buf + sizeof(uint64_t), offset);

    if (write_at(pi->fp, buf, pi->size, ENTRY_SIZE) < 0) {
        perror("write_at");
        return -1;
    }

    pi->size += ENTRY_SIZE;

    return 0;
}

int p_index_find_offset(const Persistent_Index *pi, uint64_t ts, Range *r) {
    if (pi->size == 0) {
        *r = (Range){0, 0};
        return 0;
    }

    uint64_t base_ts = pi->base_timestamp * (uint64_t)1e9;

    // 1st simple approach, read the entire file and look for the offset
    // linearly
    uint8_t buf[INDEX_SIZE];
    ssize_t len = read_file(pi->fp, buf);
    if (len < 0)
        return -1;

    uint8_t *ptr = &buf[0];
    int64_t prev_offset = 0, offset = 0;
    uint64_t timestamp = 0, entry_ts = 0;
    while (len > 0) {
        // Decode from binary
        timestamp = read_i64(ptr);
        offset = read_i64(ptr + sizeof(uint64_t));
        entry_ts = timestamp + base_ts;
        // We went too forward
        if (entry_ts > ts)
            break;
        // Remember the just read offset
        prev_offset = offset;
        // Forward the pointer and subtract the total length
        ptr += ENTRY_SIZE;
        len -= ENTRY_SIZE;
        // Found exact match
        if (entry_ts == ts)
            break;
    }

    // -1 as end only in the case where we read the entire index and we didn't
    // find anything close to the request timestamp, which means it must be
    // at the end of the log
    r->start = prev_offset;
    r->end = len == 0 ? -1 : offset;

    return 0;
}

void p_index_print(const Persistent_Index *pi) {
    uint8_t buf[INDEX_SIZE];
    uint8_t *p = &buf[0];
    ssize_t read = 0;
    uint64_t ts = 0, value = 0;
    ssize_t len = read_file(pi->fp, buf);
    while (read < len) {
        ts = read_i64(p);
        value = read_i64(p + sizeof(uint64_t));
        read += ENTRY_SIZE;
        p += ENTRY_SIZE;
        log_info("%lu -> %lu", ts, value);
    }
}
