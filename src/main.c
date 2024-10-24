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

    struct Dummy* vector = alloc(&all, sizeof(struct Dummy) * 16);
    assert(vector != NULL); 



    vector[0].a = 10;
    vector[0].b = -4;
    vector[0].c = 1.2f;


    vector[15].a = 15;
    vector[15].b = -15;
    vector[15].c = 1.15f;

    dump_nodes(&all);

    mem_free(&all, vector);
    printf("-----\n");
    dump_nodes(&all);
    return 0;
}
