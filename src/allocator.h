#ifndef ALLOCATOR_H
#define ALLOCATOR_H
#include <stddef.h>


#define KB (1024)
#define MB (KB * 1024)
#define GB (MB * 1024)

#define MIN_ALLOCATION_SIZE sizeof(void*)

typedef struct NodeHeader NodeHeader;
struct NodeHeader {
    // the size does not inlcude the header and the footer
    size_t size;
    int free;
    NodeHeader* next;
};

typedef struct NodeFooter {
    size_t size;
} NodeFooter;


typedef struct Allocator {
    NodeHeader* first_free;
    NodeHeader* last_free;
    NodeHeader* first_allocated;
    NodeHeader* last_allocated;
    void* heap_start;
    size_t heap_size;
    size_t heap_max_size;
    size_t page_size;
} Allocator;


Allocator allocator_create(size_t max_size);

void* alloc(Allocator* allocator, size_t size);
void mem_free(Allocator* allocator, void* ptr);


void dump_nodes(Allocator* allocator);


#endif // ALLOCATOR_H
