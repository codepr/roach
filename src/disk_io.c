#include "disk_io.h"
#include <errno.h>
#include <string.h>
#include <unistd.h>

static const size_t MAX_PATH_SIZE = 512;

FILE *open_file(const char *path, const char *ext, uint64_t base) {
    char path_buf[MAX_PATH_SIZE];
    snprintf(path_buf, sizeof(path_buf), "%s/%.20lu.%s", path, base, ext);
    FILE *fp = fopen(path_buf, "w+");
    if (!fp) {
        fprintf(stderr, "Cannot open %s: %s", path_buf, strerror(errno));
        return NULL;
    }
    return fp;
}

ssize_t read_file(FILE *fp, uint8_t *buf) {
    /* Get the buffer size */
    if (fseek(fp, 0, SEEK_END) < 0) {
        perror("fseek");
        return -1;
    }

    size_t size = ftell(fp);

    /* Set position of stream to the beginning */
    rewind(fp);

    /* Read the file into the buffer */
    fread(buf, 1, size, fp);

    /* NULL-terminate the buffer */
    buf[size] = '\0';
    return size;
}

ssize_t read_at(FILE *fp, uint8_t *buf, size_t count, size_t offset) {
    int fd = fileno(fp);
    return pread(fd, buf, count, offset);
}

ssize_t write_at(FILE *fp, const uint8_t *buf, size_t count, size_t offset) {
    int fd = fileno(fp);
    return pwrite(fd, buf, count, offset);
}
