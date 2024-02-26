#ifndef VEC_H
#define VEC_H

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define VEC_BASE_SIZE 4

#define VEC(T)                                                                 \
    struct {                                                                   \
        size_t size;                                                           \
        size_t capacity;                                                       \
        T *data;                                                               \
    }

#define VEC_INIT(vec, cap)                                                     \
    do {                                                                       \
        assert((cap) > 0);                                                     \
        (vec).size = 0;                                                        \
        (vec).capacity = (cap);                                                \
        (vec).data = calloc((cap), sizeof((vec).data[0]));                     \
    } while (0)

#define VEC_NEW(vec) VEC_INIT((vec), VEC_BASE_SIZE)

#define VEC_DESTROY(vec) free((vec).data)

#define VEC_SIZE(vec) (vec).size

#define VEC_CAPACITY(vec) (vec).capacity

#define VEC_APPEND(vec, item)                                                  \
    do {                                                                       \
        if (VEC_SIZE((vec)) + 1 == VEC_CAPACITY((vec))) {                      \
            (vec).capacity *= 2;                                               \
            (vec).data =                                                       \
                realloc((vec).data, (vec).capacity * sizeof((vec).data[0]));   \
        }                                                                      \
        (vec).data[(vec).size++] = (item);                                     \
    } while (0)

#define VEC_REMOVE(vec, index)                                                 \
    do {                                                                       \
        assert((index) > 0 && (index) < VEC_SIZE((vec)));                      \
        memmove((vec).data + (index), (vec).data + (index) + 1,                \
                (vec).size - (index));                                         \
        (vec).size--;                                                          \
    } while (0)

#define VEC_REMOVE_PTR(vec, ptr)                                               \
    do {                                                                       \
        for (size_t i = 0; i < VEC_SIZE((vec)); ++i) {                         \
            if ((ptr) == VEC_AT((vec), i)) {                                   \
                VEC_REMOVE((vec), i);                                          \
                break;                                                         \
            }                                                                  \
        }                                                                      \
    } while (0);

#define VEC_AT(vec, index) (vec).data[(index)]

#define VEC_FIRST(vec) VEC_AT((vec), 0)

#define VEC_LAST(vec) VEC_AT((vec), VEC_SIZE((vec)) - 1)

#define VEC_RESIZE(vec, start)                                                 \
    do {                                                                       \
        memmove((vec).data, (vec).data + (start), (vec).size - (start));       \
        (vec).size -= (start);                                                 \
    } while (0)

#define VEC_BINSEARCH(vec, target, res)                                        \
    do {                                                                       \
        if ((vec).data[0] >= (target)) {                                       \
            *(res) = 0;                                                        \
        } else if ((vec).data[(vec).size - 1] < (target)) {                    \
            *(res) = (vec).size - 1;                                           \
        } else {                                                               \
            size_t left = 0, middle = 0, right = (vec).size - 1;               \
            int found = 0;                                                     \
            while (left <= right) {                                            \
                middle = floor((left + right) / 2);                            \
                if ((vec).data[middle] < (target)) {                           \
                    left = middle + 1;                                         \
                } else if ((vec).data[middle] > (target)) {                    \
                    right = middle - 1;                                        \
                } else {                                                       \
                    *(res) = middle;                                           \
                    found = 1;                                                 \
                    break;                                                     \
                }                                                              \
            }                                                                  \
            if (found == 0) {                                                  \
                *(res) = left;                                                 \
            }                                                                  \
        }                                                                      \
    } while (0)

#define VEC_BINSEARCH_PTR(vec, target, cmp, res)                               \
    do {                                                                       \
        if ((cmp)(&(vec).data[0], (target)) >= 0) {                            \
            *(res) = 0;                                                        \
        } else if ((cmp)(&(vec).data[(vec).size - 1], (target)) <= 0) {        \
            *(res) = (vec).size - 1;                                           \
        } else {                                                               \
            size_t left = 0, middle = 0, right = (vec).size - 1;               \
            int found = 0;                                                     \
            while (left <= right) {                                            \
                middle = floor((left + right) / 2);                            \
                if ((cmp)(&(vec).data[middle], (target)) < 0) {                \
                    left = middle + 1;                                         \
                } else if ((cmp)(&(vec).data[middle], (target)) > 0) {         \
                    right = middle - 1;                                        \
                } else {                                                       \
                    *(res) = middle;                                           \
                    found = 1;                                                 \
                    break;                                                     \
                }                                                              \
            }                                                                  \
            if (found == 0) {                                                  \
                *(res) = left;                                                 \
            }                                                                  \
        }                                                                      \
    } while (0)

#endif
