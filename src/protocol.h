#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <math.h>
#include <stdint.h>
#include <stdlib.h>

/*
 * Define a basic request, for the time being it's fine to treat
 * every request as a simple string paired with it's length.
 */
typedef struct {
    size_t length;
    char query[512];
} Request;

/*
 * Define a response of type string, ideally RC (return code) should have a
 * meaning going forward.
 */
typedef struct {
    size_t length;
    uint8_t rc;
    char message[512];
} String_Response;

/*
 * Define a response of type array, mainly used as SELECT response.
 */
typedef struct {
    size_t length;
    struct {
        uint64_t timestamp;
        double_t value;
    } *records;
} Array_Response;

typedef enum { STRING_RSP, ARRAY_RSP } Response_Type;

/*
 * Define a generic response which can either be a string response or an array
 * response for the time being
 */
typedef struct response {
    Response_Type type;
    union {
        String_Response string_response;
        Array_Response array_response;
    };
} Response;

// Encode a request into an array of bytes
ssize_t encode_request(const Request *r, uint8_t *dst);

// Decode a request from an array of bytes into a Request struct
ssize_t decode_request(const uint8_t *data, Request *dst);

// Encode a response into an array of bytes
ssize_t encode_response(const Response *r, uint8_t *dst);

// Decode a response from an array of bytes into a Response struct
ssize_t decode_response(const uint8_t *data, Response *dst);

#endif // PROTOCOL_H
