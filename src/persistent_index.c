#include "persistent_index.h"
#include "binary.h"
#include "disk_io.h"

// relative timestamp -> main segment offset position in the file
static const size_t ENTRY_SIZE = sizeof(uint64_t) * 2;
static const size_t RANGE_SIZE = 1 << 9;

int p_index_init(Persistent_Index *pi, const char *path, uint64_t base,
                 uint32_t interval) {
    pi->fp = open_file(path, "index", base);
    if (!pi->fp)
        return -1;
    pi->size = 0;
    pi->base_timestamp = base;
    pi->interval = interval;
    return 0;
}

int p_index_close(Persistent_Index *pi) { return fclose(pi->fp); }

int p_index_from_disk(Persistent_Index *pi, const char *path, uint64_t base,
                      uint32_t interval) {
    pi->fp = open_file(path, "index", base);
    if (!pi->fp)
        return -1;
    pi->size = get_file_size(pi->fp, 0);
    pi->base_timestamp = base;
    pi->interval = interval;
    return 0;
}

int p_index_append_offset(Persistent_Index *pi, uint64_t ts, uint64_t offset) {
    uint64_t relative_ts = ts - pi->base_timestamp;

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
        *r = (Range){{0}, {0}};
        return 0;
    }

    uint64_t relative_ts = ts - pi->base_timestamp;
    uint64_t start = (relative_ts / pi->interval) * ENTRY_SIZE;
    start = start == 0 ? start : start - ENTRY_SIZE;
    uint64_t end = pi->size >= (start + (ENTRY_SIZE * 2))
                       ? start + (ENTRY_SIZE * 2)
                       : pi->size;

    // 512b should be enough for any range
    uint8_t buf[RANGE_SIZE];

    if (read_at(pi->fp, buf, start, end - start) < 0) {
        perror("read_at");
        *r = (Range){{0}, {0}};
        return -1;
    }

    uint64_t record_relative_offset, record_position;
    int entry_size = 0;
    uint64_t positions[2][2] = {0};

    int p_steps = (end - start) / ENTRY_SIZE;

    for (int i = 0; i < p_steps; i++) {
        entry_size = ENTRY_SIZE * i;
        record_relative_offset = read_i64(buf + entry_size);
        record_position = read_i64(buf + entry_size + (ENTRY_SIZE / 2));
        positions[i][0] = record_relative_offset;
        positions[i][1] = record_position;
    }

    if (ts < positions[0][0]) {
        *r = (Range){{0}, {positions[0][0], positions[0][1]}};
        return 0;
    }

    if (end - start > 1) {
        *r = (Range){{positions[0][0], positions[0][1]},
                     {positions[1][0], positions[1][1]}};
        return 0;
    }

    *r = (Range){{positions[0][0], positions[0][1]},
                 {positions[0][0], positions[0][1]}};
    return 0;
}
