#include <stdio.h>
#include <assert.h>

#include "cache.h"
#include "allocator.h"


struct Dummy {
    size_t a;
    int b;
    float c;
};

int main() {
    // MemoryInfo m = get_memory_info();
    // printf("total memory: %zu\n", m.total_memory);
    // printf("available memory: %zu\n", m.available_memory);
    // printf("page size: %zu\n", m.page_size);

    Allocator all = allocator_create(4 * MB);
    
    size_t count = 10;
    char** vector = alloc(&all, sizeof(char*) * count);
    assert(vector != NULL); 

    for (size_t i = 0; i < count; i++) {
        vector[i] = (char*)alloc(&all, i + 1);
    }

    dump_nodes(&all);
    printf("-----\n");
    
    for (size_t i = 1; i < count; i += 2) {
        mem_free(&all, vector[i]);
    }

    dump_nodes(&all);
    printf("-----\n");

        mem_free(&all, vector[2]);

    dump_nodes(&all);
    printf("-----\n");

    mem_free(&all, vector);
    return 0;
}
