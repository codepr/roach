#include "binary.h"
#include <stdint.h>

void write_u8(uint8_t *buf, uint8_t val) { *buf++ = val; }

uint8_t read_u8(const uint8_t *const buf) { return ((uint8_t)*buf); }

/*
 * write_u16() -- store a 16-bit int into a char buffer (like htons())
 */
void write_u16(uint8_t *buf, uint16_t val) {
    *buf++ = val >> 8;
    *buf++ = val;
}

/*
 * read_u16() -- unpack a 16-bit unsigned from a char buffer (like ntohs())
 */
uint16_t read_u16(const uint8_t *const buf) {
    return ((uint16_t)buf[0] << 8) | buf[1];
}

/*
 * write_u32() -- store a 32-bit int into a char buffer (like htonl())
 */
void write_u32(uint8_t *buf, uint32_t val) {
    *buf++ = val >> 24;
    *buf++ = val >> 16;
    *buf++ = val >> 8;
    *buf++ = val;
}

/*
 * read_u32() -- unpack a 32-bit unsigned from a char buffer (like ntohl())
 */
uint32_t read_u32(const uint8_t *const buf) {
    return ((uint32_t)buf[0] << 24) | ((uint32_t)buf[1] << 16) |
           ((uint32_t)buf[2] << 8) | buf[3];
}

/*
 * write_i64() -- store a 64-bit int into a char buffer (like htonl())
 */
void write_i64(uint8_t *buf, uint64_t val) {
    *buf++ = val >> 56;
    *buf++ = val >> 48;
    *buf++ = val >> 40;
    *buf++ = val >> 32;
    *buf++ = val >> 24;
    *buf++ = val >> 16;
    *buf++ = val >> 8;
    *buf++ = val;
}

/*
 * read_i64() -- unpack a 64-bit unsigned from a char buffer (like ntohl())
 */
uint64_t read_i64(const uint8_t *const buf) {
    return ((uint64_t)buf[0] << 56) | ((uint64_t)buf[1] << 48) |
           ((uint64_t)buf[2] << 40) | ((uint64_t)buf[3] << 32) |
           ((uint64_t)buf[4] << 24) | ((uint64_t)buf[5] << 16) |
           ((uint64_t)buf[6] << 8) | buf[7];
}

/*
 * write_f64() -- store a 64-bit float into a char buffer, taken from beej.us
 * guide
 */
void write_f64(uint8_t *buf, double_t val) {
    unsigned bits = 64, expbits = 11;
    long double fnorm;
    int shift;
    long long sign, exp, significand;
    unsigned significandbits = bits - expbits - 1; // -1 for sign bit

    if (val == 0.0) {
        write_u32(buf, 0);
    } else {
        // check sign and begin normalization
        if (val < 0) {
            sign = 1;
            fnorm = -val;
        } else {
            sign = 0;
            fnorm = val;
        }

        // get the normalized form of f and track the exponent
        shift = 0;
        while (fnorm >= 2.0) {
            fnorm /= 2.0;
            shift++;
        }
        while (fnorm < 1.0) {
            fnorm *= 2.0;
            shift--;
        }
        fnorm = fnorm - 1.0;

        // calculate the binary form (non-float) of the significand data
        significand = fnorm * ((1LL << significandbits) + 0.5f);

        // get the biased exponent
        exp = shift + ((1 << (expbits - 1)) - 1); // shift + bias

        // return the final answer
        uint64_t d =
            (sign << (bits - 1)) | (exp << (bits - expbits - 1)) | significand;

        write_i64(buf, d);
    }
}

/*
 * read_f64() -- unpack a 64-bit float into a char buffer, taken from beej.us
 * guide
 */
double_t read_f64(const uint8_t *const buf) {
    uint64_t i = read_i64(buf);
    unsigned bits = 64, expbits = 11;
    long double result;
    long long shift;
    unsigned bias;
    unsigned significandbits = bits - expbits - 1; // -1 for sign bit

    if (i == 0)
        return 0.0;

    // pull the significand
    result = (i & ((1LL << significandbits) - 1)); // mask
    result /= (1LL << significandbits);            // convert back to float
    result += 1.0f;                                // add the one back on

    // deal with the exponent
    bias = (1 << (expbits - 1)) - 1;
    shift = ((i >> significandbits) & ((1LL << expbits) - 1)) - bias;
    while (shift > 0) {
        result *= 2.0;
        shift--;
    }
    while (shift < 0) {
        result /= 2.0;
        shift++;
    }

    // sign it
    result *= (i >> (bits - 1)) & 1 ? -1.0 : 1.0;

    return result;
}
