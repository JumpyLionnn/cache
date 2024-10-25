#include "allocator.h"
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <assert.h>
#include <stdbool.h>

Allocator allocator_create(size_t max_size) {
    void* heap_start = mmap(NULL, max_size, PROT_NONE, MAP_PRIVATE | MAP_NORESERVE | MAP_ANONYMOUS, -1, 0);
    return (Allocator) {
        .first_free =  NULL,
        .last_free = NULL,
        .first_allocated = NULL,
        .last_allocated = NULL,
        .heap_start = heap_start,
        .heap_size = 0,
        .heap_max_size = max_size,
        .page_size = sysconf(_SC_PAGE_SIZE)

    };
}


NodeHeader* first_fit(Allocator* allocator, size_t size, NodeHeader** out_previous) {
    assert(allocator != NULL);
    NodeHeader* previous = NULL;
    NodeHeader* node = allocator->first_free;
    while (node != NULL) {
        if (node->size >= size) {
            break;
        }
        previous = node; 
        node = node->next;
    }
    *out_previous = previous;
    return node;
}

void make_node_allocated(Allocator* allocator, NodeHeader* header) {
    assert(header != NULL);
    assert(allocator != NULL);
    header->free = 0;
    
    if (header < allocator->first_allocated) {
        header->next = allocator->first_allocated;
        allocator->first_allocated = header;
    }
    else if (allocator->first_allocated == NULL) {
        allocator->first_allocated = header;
        allocator->last_allocated = header;
    }
    else {
        NodeFooter* previous_footer = (NodeFooter*)((char*)header - sizeof(NodeFooter));
        NodeHeader* previous_header = (NodeHeader*)((char*)previous_footer - sizeof(NodeHeader) - previous_footer->size);
        assert(previous_header->free == 0);
        header->next = previous_header->next;
        previous_header->next = header;
    }
}

size_t ceil_to_power_of_two_multiplies(size_t value, size_t power_of_two) {
    size_t page_size_mask = power_of_two - 1;
    if ((value & page_size_mask) != 0) { 
        value = (value | page_size_mask) + 1;
    }
    return value;
}

NodeHeader* expand_memory_area_by(Allocator* allocator, size_t size) {
    bool is_last_node = false;
    NodeHeader* last_free = allocator->last_free;
    size_t additional_size = size;
    char* heap_end = (char*)allocator->heap_start + allocator->heap_size;
    if (last_free != NULL) {
        char* last_free_end = (char*)last_free + sizeof(NodeHeader) + sizeof(NodeFooter) + last_free->size;

        is_last_node = last_free_end == heap_end;
        if (is_last_node) {
            additional_size -= last_free->size;
        }
    }


    // NOTE: assuming the page size if a power of 2
    size_t total_size = ceil_to_power_of_two_multiplies(additional_size, allocator->page_size);

    void* new_block = mmap(heap_end, total_size, PROT_WRITE | PROT_READ, MAP_FIXED | MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (new_block == (void*)-1) {
        return NULL;
    }
    allocator->heap_size += total_size;
    NodeFooter* footer = (NodeFooter*)((char*)new_block + total_size - sizeof(NodeFooter));
    if (is_last_node) {
        footer->size = total_size + last_free->size;
        last_free->size = footer->size;
        return last_free;
    }
    else {
        NodeHeader* header = new_block;
        header->size = total_size - sizeof(NodeFooter) - sizeof(NodeHeader);
        footer->size = header->size;
        header->free = 1;
        header->next = NULL;
        if (last_free == NULL) {
            allocator->first_free = header;
            allocator->last_free = header;
        }
        else {
            last_free->next = header;
        }
        return header;
    }
}
NodeFooter* get_footer(NodeHeader* header) {
    return (NodeFooter*)((char*)header + header->size + sizeof(NodeHeader));
}

NodeHeader* get_previous_header(Allocator* allocator, NodeHeader* node) {
    if (node == allocator->heap_start) {
        return NULL;
    }
    else {
        NodeFooter* footer = (NodeFooter*)((char*)node - sizeof(NodeFooter));
        return (NodeHeader*)((char*)footer - footer->size - sizeof(NodeHeader));
    }
}

NodeHeader* advance_node(NodeHeader* node) {
    return (NodeHeader*)((char*)node + sizeof(NodeHeader) + sizeof(NodeFooter) + node->size);
}

NodeHeader* get_next_header(Allocator* allocator, NodeHeader* node) {
    char* memory_end = (char*)allocator->heap_start + allocator->heap_size;
    NodeHeader* next = advance_node(node);
    if ((char*)next == memory_end) {
        return NULL;
    }
    else {
        return next;
    }
}

void* alloc(Allocator* allocator, size_t size) {
    if (size == 0) {
        return NULL;
    }
    
    size = ceil_to_power_of_two_multiplies(size, ALLOCATION_ALIGNMENT);

    NodeHeader* previous_free = NULL;
    NodeHeader* block = first_fit(allocator, size, &previous_free);
    size_t total_size = size + sizeof(NodeHeader) + sizeof(NodeFooter);
    if (block == NULL) {
        block = expand_memory_area_by(allocator, total_size);
        if (block == NULL) {
            printf("out of memory\n");
            return NULL;
        }
    }
    if (block->size >= total_size + MIN_ALLOCATION_SIZE) {
        // can split successfully
        size_t remaining_size = block->size - total_size; 

        NodeFooter* new_footer = (NodeFooter*)((char*)block + sizeof(NodeHeader) + size);
        NodeHeader* new_header = (NodeHeader*)((char*)new_footer + sizeof(NodeFooter));


        new_footer->size = size;
        new_header->size = remaining_size;
        new_header->free = 1;
        new_header->next = block->next;


        NodeFooter* footer = get_footer(block);
        footer->size = remaining_size;


        block->size = size;
        make_node_allocated(allocator, block);
        if (previous_free == NULL) {
            allocator->first_free = new_header;
        }
        else {
            previous_free->next = new_header;
        }

        return (char*)block + sizeof(NodeHeader);
    }
    else {
        // can't spit, need to use the whole block, even if there is a bit of padding

        // removing it form the free blocks linked list
        previous_free->next = block->next;

        make_node_allocated(allocator, block);
        return (char*)block + sizeof(NodeHeader);
    }
}


NodeHeader* find_previous_allocated_node(Allocator* allocator, NodeHeader* node) {
    NodeHeader* previous = get_previous_header(allocator, node);
    if (previous == NULL) {
        return NULL;
    }
    if (previous->free) {
        previous = get_previous_header(allocator, previous);
        assert(previous == NULL || previous->free == 0);
    }
    return previous;
}


NodeHeader* find_previous_free_node(Allocator* allocator, NodeHeader* node) {
    NodeHeader* previous = get_previous_header(allocator, node);
    while (previous != NULL && previous->free == 0) {
        previous = get_previous_header(allocator, previous);
    }
    return previous;
}

void mem_free(Allocator* allocator, void* ptr) {
    assert(allocator != NULL);
    if (ptr == NULL) {
        return;
    }
    NodeHeader* header = (NodeHeader*)((char*)ptr - sizeof(NodeHeader));

    NodeHeader* previous_allocated = find_previous_allocated_node(allocator, header);
    if (previous_allocated == NULL) {
        allocator->first_allocated = header->next;
    }
    else {
        previous_allocated->next = header->next;
    }
    if (header->next == NULL) {
        allocator->last_allocated = previous_allocated;
    }

    header->free = 1;
    NodeHeader* last_free = find_previous_free_node(allocator, header);
    NodeHeader* next_free = NULL;
    if (last_free == NULL) {
        next_free = allocator->first_free;
        header->next = allocator->first_free;
        allocator->first_free = header;
    }
    else {
        next_free = last_free->next;
        if (last_free == get_previous_header(allocator, header)) {
            // unify the 2
            size_t new_size = header->size + last_free->size + sizeof(NodeHeader) + sizeof(NodeFooter);
            last_free->size = new_size;
            NodeFooter* new_footer = get_footer(header);
            new_footer->size = new_size;
            header = last_free;
        }
        else {
            last_free->next = header;
        }
    }

    if (next_free == NULL) {
        header->next = NULL;
        allocator->last_free = header;
    }
    else {
        if (next_free == get_next_header(allocator, header)) {
            NodeFooter* new_footer = get_footer(next_free);
            header->next = next_free->next;
            size_t new_size = header->size + next_free->size + sizeof(NodeHeader) + sizeof(NodeFooter);
            header->size = new_size;
            new_footer->size = new_size;
        }
        else {
            header->next = next_free;
        }
    }
}



void dump_nodes(Allocator* allocator) {
    char* heap_end = (char*)allocator->heap_start + allocator->heap_size;
    char* current = (char*)allocator->heap_start;
    printf("first allocated: %p, first free: %p\n", (void*)allocator->first_allocated, (void*)allocator->first_free);
    while(current < heap_end) {
        NodeHeader* header = (NodeHeader*)current;
        const char* label = "";
        if (header->free) {
            label = "free";
        }
        else {
            label = "allocated";
        }
        printf("%p: %s node: %zu, next: %p\n", (void*)header, label, header->size, (void*)header->next);
        current = current + sizeof(NodeHeader) + sizeof(NodeFooter) + header->size;
    }
    printf("last allocated: %p, last free: %p\n", (void*)allocator->last_allocated, (void*)allocator->last_free);
}
