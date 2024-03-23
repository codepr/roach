#include "protocol.h"
#include <stdio.h>
#include <string.h>

static ssize_t encode_string(uint8_t *dst, const char *src, size_t length) {
    size_t i = 0, j = 0;

    // Payload length just after the $ indicator
    size_t n = snprintf((char *)dst, 21, "%lu", length);
    i += n;

    // CRLF
    dst[i++] = '\r';
    dst[i++] = '\n';

    // The query content
    while (length-- > 0)
        dst[i++] = src[j++];

    // CRLF
    dst[i++] = '\r';
    dst[i++] = '\n';

    return i;
}

ssize_t encode_request(const Request *r, uint8_t *dst) {
    dst[0] = '$';
    return 1 + encode_string(dst + 1, r->query, r->length);
}

ssize_t decode_request(const uint8_t *data, Request *dst) {
    if (data[0] != '$')
        return -1;

    size_t i = 0;
    const uint8_t *ptr = &data[1];

    // Read length
    while (*ptr != '\r' && *(ptr + 1) != '\n') {
        dst->length *= 10;
        dst->length += *ptr - '0';
        ptr++;
    }

    // Jump over \r\n
    ptr += 2;

    // Read query string
    while (*ptr != '\r' && *(ptr + 1) != '\n')
        dst->query[i++] = *ptr++;

    return dst->length;
}

ssize_t encode_response(const Response *r, uint8_t *dst) {
    if (r->type == STRING_RSP) {
        // String response
        dst[0] = r->string_response.rc == 0 ? '$' : '!';
        return 1 + encode_string(dst + 1, r->string_response.message,
                                 r->string_response.length);
    }
    // Array response
    dst[0] = '#';
    ssize_t i = 1;
    size_t j = 0;

    // Array length
    size_t n = snprintf((char *)dst + i, 20, "%lu", r->array_response.length);
    i += n;

    // CRLF
    dst[i++] = '\r';
    dst[i++] = '\n';

    // Records
    while (j < r->array_response.length) {
        // Timestamp
        dst[i++] = ':';
        n = snprintf((char *)dst + i, 21, "%lu",
                     r->array_response.records[j].timestamp);
        i += n;
        dst[i++] = '\r';
        dst[i++] = '\n';
        // Value
        dst[i++] = ';';
        n = snprintf((char *)dst + i, 21, "%lf",
                     r->array_response.records[j].value);
        i += n;
        dst[i++] = '\r';
        dst[i++] = '\n';
        j++;
    }

    return i;
}

static ssize_t decode_string(const uint8_t *ptr, Response *dst) {
    size_t i = 0, n = 1;

    // For simplicty, assume the only error code is 1 for now, it's not used ATM
    dst->string_response.rc = *ptr == '!' ? 1 : 0;
    ptr++;

    // Read length
    while (*ptr != '\r' && *(ptr + 1) != '\n' && n++) {
        dst->string_response.length *= 10;
        dst->string_response.length += *ptr - '0';
        ptr++;
    }

    // Move forward after CRLF
    ptr += 2;
    n += 2;

    while (*ptr != '\r' && *(ptr + 1) != '\n')
        dst->string_response.message[i++] = *ptr++;

    return i + n;
}

ssize_t decode_response(const uint8_t *data, Response *dst) {
    uint8_t byte = *data;
    ssize_t length = 0;

    dst->type = byte == '#' ? ARRAY_RSP : STRING_RSP;

    switch (byte) {
    case '$':
    case '!':
        // Treat error and common strings the same for now
        length = decode_string(data, dst);
        break;
    case '#':
        data++;
        length++;
        // Read length
        dst->array_response.length = 0;
        while (*data != '\r' && *(data + 1) != '\n' && length++) {
            dst->array_response.length *= 10;
            dst->array_response.length += *data - '0';
            data++;
        }

        // Jump over \r\n
        data += 2;
        length += 2;

        // Read records
        size_t j = 0;
        size_t total_records = dst->array_response.length;
        uint8_t buf[32];
        size_t k = 0;
        // TODO arena malloc here
        dst->array_response.records =
            malloc(total_records * sizeof(*dst->array_response.records));
        while (total_records-- > 0) {
            // Timestamp
            if (*data++ != ':')
                goto cleanup;

            while (*data != '\r' && *(data + 1) != '\n' && length++)
                buf[k++] = *data++;

            dst->array_response.records[j].timestamp = atoll((const char *)buf);
            memset(buf, 0x00, sizeof(buf));
            k = 0;

            // Skip CRLF + ;
            data += 3;
            length += 3;

            // Value
            while (*data != '\r' && *(data + 1) != '\n' && length++)
                buf[k++] = *data++;

            buf[k] = '\0';

            dst->array_response.records[j].value = strtold((char *)buf, NULL);

            // Skip CRLF
            data += 2;
            length += 2;
            j++;
        }
        break;
    default:
        break;
    }

    return length;

cleanup:
    free(dst->array_response.records);
    return -1;
}

void free_response(Response *rs) {
    if (rs->type == ARRAY_RSP)
        free(rs->array_response.records);
}
