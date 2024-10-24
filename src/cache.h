#ifndef CACHE_H
#define CACHE_H
#include <stddef.h>

typedef struct {
    size_t total_memory;
    size_t page_size;
    size_t available_memory;
} MemoryInfo;

MemoryInfo get_memory_info();

#endif // CACHE_H
