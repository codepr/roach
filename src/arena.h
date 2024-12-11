#ifndef ARENA_H
#define ARENA_H

#include <stdint.h>
#include <stdlib.h>

typedef struct allocator {
    void *(*alloc)(size_t size, void *context);
    void (*free)(size_t size, void *ptr, void *context);
    void *context;
} Allocator;

#define alloc(T, n, a)   ((T *)((a).alloc(sizeof(T) * n, (a).context)))
#define release(s, p, a) ((a).free(s, p, (a).context))

typedef struct arena {
    void *base;
    size_t size;
    size_t offset;
    size_t committed;
} Arena;

#endif
