#ifndef BINARY_H
#define BINARY_H

#include <math.h>
#include <stdint.h>

void write_u8(uint8_t *, uint8_t);

uint8_t read_u8(const uint8_t *const);

void write_u16(uint8_t *, uint16_t);

uint16_t read_u16(const uint8_t *const);

void write_u32(uint8_t *, uint32_t);

uint32_t read_u32(const uint8_t *const);

void write_i64(uint8_t *, uint64_t);

uint64_t read_i64(const uint8_t *const);

void write_f64(uint8_t *, double_t);

double_t read_f64(const uint8_t *const);

#endif
