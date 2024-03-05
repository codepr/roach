#ifndef VEC_H
#define VEC_H

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define VEC_BASE_CAPACITY 4

#define VEC(T)                                                                 \
    struct {                                                                   \
        size_t size;                                                           \
        size_t capacity;                                                       \
        T *data;                                                               \
    }

#define vec_init(vec, cap)                                                     \
    do {                                                                       \
        assert((cap) > 0);                                                     \
        (vec).size = 0;                                                        \
        (vec).capacity = (cap);                                                \
        (vec).data = calloc((cap), sizeof((vec).data[0]));                     \
    } while (0)

#define vec_new(vec) vec_init((vec), VEC_BASE_CAPACITY)

#define vec_destroy(vec) free((vec).data)

#define vec_size(vec) (vec).size

#define vec_capacity(vec) (vec).capacity

#define vec_push(vec, item)                                                    \
    do {                                                                       \
        if (vec_size((vec)) + 1 == vec_capacity((vec))) {                      \
            (vec).capacity *= 2;                                               \
            (vec).data =                                                       \
                realloc((vec).data, (vec).capacity * sizeof((vec).data[0]));   \
        }                                                                      \
        (vec).data[(vec).size++] = (item);                                     \
    } while (0)

#define vec_pop(vec, index)                                                    \
    do {                                                                       \
        assert((index) > 0 && (index) < vec_size((vec)));                      \
        memmove((vec).data + (index), (vec).data + (index) + 1,                \
                (vec).size - (index));                                         \
        (vec).size--;                                                          \
    } while (0)

#define vec_pop_cmp(vec, ptr)                                                  \
    do {                                                                       \
        for (size_t i = 0; i < vec_size((vec)); ++i) {                         \
            if ((ptr) == vec_at((vec), i)) {                                   \
                vec_pop((vec), i);                                             \
                break;                                                         \
            }                                                                  \
        }                                                                      \
    } while (0);

#define vec_at(vec, index) (vec).data[(index)]

#define vec_first(vec) vec_at((vec), 0)

#define vec_last(vec) vec_at((vec), vec_size((vec)) - 1)

#define vec_resize(vec, start)                                                 \
    do {                                                                       \
        memmove((vec).data, (vec).data + (start), (vec).size - (start));       \
        (vec).size -= (start);                                                 \
    } while (0)

#define vec_search_cmp(vec, target, cmp, res)                                  \
    do {                                                                       \
        *(res) = -1;                                                           \
        if ((cmp)(&(vec).data[0], (target)) < 1) {                             \
            for (size_t i = 0; i < vec_size((vec)); ++i) {                     \
                if ((cmp)(&(vec).data[i], (target)) == 0) {                    \
                    *(res) = i;                                                \
                }                                                              \
            }                                                                  \
        }                                                                      \
    } while (0)

#define vec_bsearch(vec, target, res)                                          \
    do {                                                                       \
        if ((vec).data[0] >= (target)) {                                       \
            *(res) = -1;                                                       \
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

#define vec_bsearch_cmp(vec, target, cmp, res)                                 \
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
