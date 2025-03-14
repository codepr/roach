#include "disk_io.h"
#include "logging.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

const size_t MAX_PATH_SIZE = 1024;

int make_dir(const char *path)
{
    struct stat st = {0};

    if (stat(path, &st) == -1) {
        return mkdir(path, 0700);
    }
    return 0;
}

FILE *open_file(const char *path, const char *ext, const char *modes)
{
    char path_buf[MAX_PATH_SIZE];
    snprintf(path_buf, sizeof(path_buf), "%s.%s", path, ext);

    FILE *fp = fopen(path_buf, modes);
    if (!fp) {
        log_error("Cannot open %s (%s): %s", path_buf, modes, strerror(errno));
        return NULL;
    }
    return fp;
}

ssize_t get_file_size(FILE *fp, long offset)
{
    if (fseek(fp, 0, SEEK_END) < 0) {
        fclose(fp);
        return -1;
    }

    ssize_t size = ftell(fp);

    // reset to offset
    fseek(fp, offset, SEEK_SET);
    return size;
}

int buf_read_file(FILE *fp, Buffer *buffer)
{
    /* Get the buffer size */
    if (fseek(fp, 0, SEEK_END) < 0) {
        log_error("Error reading file: fseek %s", strerror(errno));
        return -1;
    }

    size_t size = ftell(fp);

    /* Set position of stream to the beginning */
    rewind(fp);

    /* Allocate the buffer (no need to initialize it with calloc) */
    buffer->buf = calloc(size + 1, sizeof(uint8_t));

    /* Read the file into the buffer */
    fread(buffer->buf, 1, size, fp);

    buffer->size      = size;

    /* NULL-terminate the buffer */
    buffer->buf[size] = '\0';
    return 0;
}

ssize_t read_file(FILE *fp, uint8_t *buf)
{
    /* Get the buffer size */
    if (fseek(fp, 0, SEEK_END) < 0) {
        perror("fseek");
        return -1;
    }

    ssize_t size = ftell(fp);

    /* Set position of stream to the beginning */
    rewind(fp);

    /* Read the file into the buffer */
    fread(buf, 1, size, fp);

    /* NULL-terminate the buffer */
    buf[size] = '\0';
    return size;
}

ssize_t read_at(FILE *fp, uint8_t *buf, size_t offset, size_t len)
{
    int fd = fileno(fp);
    return pread(fd, buf, len, offset);
}

ssize_t write_at(FILE *fp, const uint8_t *buf, size_t offset, size_t len)
{
    int fd = fileno(fp);
    return pwrite(fd, buf, len, offset);
}
