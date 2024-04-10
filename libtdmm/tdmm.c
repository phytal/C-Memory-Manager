#include <sys/mman.h>
#include <errno.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "tdmm.h"

#define PAGE_SIZE (1024 * 4) // 4096 bytes
#define META_SIZE sizeof(struct MemoryBlock) // 24 bytes

// Structure to represent a memory block
struct MemoryBlock {
    int size;
    bool free;
    bool used;
    struct MemoryBlock* prev;
    struct MemoryBlock* next;
};

// Head of the linked list to store allocated memory blocks
static struct MemoryBlock* head = NULL;
static alloc_strat_e alloc_strat = 0;
static void* mem_start = NULL;
static void* stack_bottom = NULL;
static void* stack_top = NULL;
static size_t total_size = 0;

// Function to align pointer to multiples of 4
void* align_ptr(void* ptr) {
    size_t address = (size_t)ptr;
    address = (address + 3) & (~3);

    return (void*)address;
}

// Function to add memory to the linked list
// Precondition: The current block is the last block in the linked list. 
void add_mem(struct MemoryBlock* current, size_t size) {
    if (size + META_SIZE > PAGE_SIZE) { // If the requested size is larger than a page
        void* new_mem_start = mmap(NULL, size + META_SIZE + 3, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);

        new_mem_start = align_ptr(new_mem_start);

        // Create a new memory block structure
        struct MemoryBlock* new_block = (struct MemoryBlock*)new_mem_start;
        new_block->size = size;
        new_block->free = true;
        new_block->prev = current;
        new_block->next = NULL;

        // Insert the new block into the linked list
        current->next = new_block;
    } else { // If the requested size is smaller than a page
        void* new_mem_start = mmap(NULL, PAGE_SIZE + 3, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);

        new_mem_start = align_ptr(new_mem_start);

        // Create a new memory block structure
        struct MemoryBlock* new_block = (struct MemoryBlock*)new_mem_start;
        new_block->size = PAGE_SIZE - META_SIZE;
        new_block->free = true;
        new_block->prev = current;
        new_block->next = NULL;

        // Insert the new block into the linked list
        current->next = new_block;
    }
}

// Function to write a memory block with the given size
// Precondition: The size is a multiple of 4.
void write_block(struct MemoryBlock* current, size_t size) {
    // If the current block is larger than the requested size, split it
    if (alloc_strat != BUDDY && current->size >= size + META_SIZE + 4) {
        // Create a new block for the remaining free memory
        struct MemoryBlock* remaining = (struct MemoryBlock*)((char*)current + META_SIZE + size);
        
        remaining->size = current->size - size - META_SIZE;
        remaining->free = true;
        remaining->next = current->next;
        remaining->prev = current;

        // Update the size of the current block and mark it as allocated
        current->size = size;
        current->free = false;
        current->next = remaining;
        printf("A\n");
    } else {
        // If the current block is just the right size (+3), mark it as allocated
        current->free = false;
        printf("B\n");
    }
}

void t_init (alloc_strat_e strat, void* stack_bot) {
    alloc_strat = strat;
    stack_bottom = stack_bot;
    char *temp;
    stack_top = temp;
    total_size = 0;

    mem_start = mmap(NULL, (PAGE_SIZE/4) + 3, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);

    mem_start = align_ptr(mem_start);

    head = (struct MemoryBlock*)mem_start;
    head->size = PAGE_SIZE - META_SIZE;
    head->free = true;
    head->next = NULL;
    head->prev = NULL;
}

// Function to allocate memory using First Fit policy
void* first_fit(size_t size) {
    struct MemoryBlock* current = head;
    while (current) {
        if (current->free && current->size >= size) {
            write_block(current, size);

            return (void*)(current); // Return pointer to start of block
        }
        if (current->next)
            current = current->next;
        else break;
    }

    // No suitable block found, must resize
    add_mem(current, size);
    write_block(current->next, size);

    return (void*)(current->next);
}

// Function to allocate memory using Best Fit policy
void* best_fit(size_t size) {
    struct MemoryBlock* best_block = NULL;
    struct MemoryBlock* current = head;
    while (current) {
        if (current->free && current->size >= size && (best_block == NULL || current->size < best_block->size)) {
            best_block = current;
        }
        if (current->next)
            current = current->next;
        else break;
    }
    if (best_block) {
        write_block(best_block, size);

        return (void*)(best_block); // Return pointer to start of block
    }
    // No suitable block found
    add_mem(current, size);

    write_block(current->next, size);

    return (void*)(current->next);
}

// Function to allocate memory using Worst Fit policy
void* worst_fit(size_t size) {
    struct MemoryBlock* worst_block = NULL;
    struct MemoryBlock* current = head;
    while (current) {
        if (current->free && current->size >= size && (worst_block == NULL || current->size > worst_block->size)) {
            worst_block = current;
        }
        if (current->next)
            current = current->next;
        else break;
    }
    if (worst_block) {
        write_block(worst_block, size);

        return (void*)(worst_block); // Return pointer to start of block
    }
    
    // No suitable block found
    add_mem(current, size);
    write_block(current->next, size);

    return (void*)(current->next);
}

void* buddy_alloc(size_t size) {
    // Find the smallest power of 2 that can accommodate the requested size
    size_t block_size = 1;
    int level = 0;
    while (block_size < size) {
        block_size <<= 1;
        level++;
    }

    // Find the best block to allocate from
    struct MemoryBlock* best_block = NULL;
    struct MemoryBlock* current = head;
    while (current) {
        if (current->free && current->size-META_SIZE >= block_size && (best_block == NULL || current->size-META_SIZE < best_block->size)) {
            best_block = current;
        }
        if (current->next)
            current = current->next;
        else break;
    }
    if (best_block) {
        // Split larger blocks until reaching the required level
        int best_block_size = best_block->size;
        int i = 0;
        while (best_block_size > 0) {
            best_block_size -= META_SIZE;
            best_block_size >>= 1;
            i++;
        }
        while (i > level) {
            // Split the block into two buddies
            size_t new_size = (best_block->size-META_SIZE) / 2;
            struct MemoryBlock* buddy1 = (struct MemoryBlock*)((char*)best_block);
            struct MemoryBlock* buddy2 = (struct MemoryBlock*)((char*)best_block + new_size);
            buddy1->size = new_size;
            buddy2->size = new_size;
            buddy1->free = true;
            buddy2->free = true;
            buddy2->next = best_block->next;
            buddy2->prev = buddy1;
            buddy1->next = buddy2;

            i--; // Move to the next lower level
            best_block = buddy1;
        }
        write_block(best_block, block_size);

        return (void*)(best_block); // Return pointer to start of block
    } else {
        // No suitable block found
        add_mem(current, block_size - META_SIZE);
        best_block = current->next;

        // Split larger blocks until reaching the required level
        int best_block_size = best_block->size;
        int i = 0;
        while (best_block_size > 0) {
            best_block_size -= META_SIZE;
            best_block_size >>= 1;
            i++;
        }
        while (i > level) {
            // Split the block into two buddies
            size_t new_size = (best_block->size-META_SIZE) / 2;
            struct MemoryBlock* buddy1 = (struct MemoryBlock*)((char*)best_block);
            struct MemoryBlock* buddy2 = (struct MemoryBlock*)((char*)best_block + new_size);
            buddy1->size = new_size;
            buddy2->size = new_size;
            buddy1->free = true;
            buddy2->free = true;
            buddy2->next = best_block->next;
            buddy2->prev = buddy1;
            buddy1->next = buddy2;

            i--; // Move to the next lower level
            best_block = buddy1;
        }
        write_block(best_block, block_size);

        return (void*)(current->next);
    }
}

void *t_malloc (size_t size) {
    // Ensure size is a multiple of 4 for alignment
    size = (size + 3) & (~3);

    void* ptr = NULL;

    switch (alloc_strat) {
        case FIRST_FIT:
            ptr = first_fit(size);
            break;
        case BEST_FIT:
            ptr = best_fit(size);
            break;
        case WORST_FIT:
            ptr = worst_fit(size);
            break;
        case BUDDY:
            ptr = buddy_alloc(size);
            break;
        default:
            fprintf(stderr, "Invalid memory allocation strat.\n");
            return NULL;
    }

    // Create memory block structure
    struct MemoryBlock* block = (struct MemoryBlock*)ptr;

    printf("Block allocated.\n");
    printf("Block size: %lu\n", block->size);
    printf("Block free: %d\n", block->free);
    printf("Block next: %p\n", block->next);
    printf("Block prev: %p\n", block->prev);

    total_size++;

    // Return pointer to user-visible memory region
    return (void*)(block + 1);
}

void t_free (void *ptr) {
    if (ptr == NULL) {
        return; // Nothing to free
    }

    // Find the corresponding memory block
    struct MemoryBlock* block = (struct MemoryBlock*)ptr - 1;

    // Mark the block as free
    block->free = true;

    // Merge adjacent free blocks here
    struct MemoryBlock* current = block;
    bool merged_left = true;
    bool merged_right = true;
    while (merged_left || merged_right) {
        if (current->free && current->prev != NULL && current->prev->free) {
            if (alloc_strat == BUDDY && current->size != current->prev->size) {
                merged_left = false;
            } else {
                if ((char*)current->prev+current->prev->size+META_SIZE != (char*)current) {
                    merged_left = false;
                } else {
                    current->prev->size += current->size + META_SIZE;
                    current->prev->next = current->next;
                    current = current->prev;
                    merged_left = true;
                }
            }
        } else {
            merged_left = false;
        }
        if (current->free && current->next != NULL && current->next->free) {
            if (alloc_strat == BUDDY && current->size != current->next->size) {
                merged_right = false;
            } else {
                if ((char*)current+current->size+META_SIZE != (char*)current->next) {
                    merged_right = false;
                } else {
                    current->size += current->next->size + META_SIZE;
                    current->next = current->next->next;
                    merged_right = true;
                }
            }
        } else {
            merged_right = false;
        }
    }

    printf("Block freed.\n");
    printf("Block size: %lu\n", current->size);
    printf("Block free: %d\n", current->free);
    printf("Block next: %p\n", current->next);
    printf("Block prev: %p\n", current->prev);
}

// Function to check if a given pointer is within the allocated memory regions
void check_valid_pointer(void* ptr) {
    struct MemoryBlock* current = head;
    bool founder = false;
    while (current) {
        void* block_start = (void*)(current + 1);
        void* block_end = (void*)((char*)block_start + current->size);
        if (ptr >= block_start && ptr <= block_end && current->size % 4 == 0) {
            current->used = true; // Set the usage bit to 1
            break;
        }
        if(current->next == NULL || founder) break;
        else current = current->next;
    }
}

// void gcollect_mark_region(void* start, void* end) {
//     struct MemoryBlock* current = head;
//     while (current) {
//         void* block_start = (void*)(current + 1);
//         void* block_end = (void*)((char*)block_start + current->size);
//         if (block_start >= start && block_end <= end) {
//             current->used = true; // Set the usage bit to 1
//         }
//         current = current->next;
//     }
// }

// basically if theres a pointer in the stack that points to anywhere in a valid memory block, set the usage bit to 1 (used)
// if the pointer is not in the stack but still in the heap, set the usage bit to 0 (unused) to be garbage collected

// Function to scan the stack and heap for pointers to allocated memory regions
void t_gcollect (void) {
    long x = 0;
    char * stack_top = (char*)&x;
    char * secondary = (char *) &x;
    
    // printf("Stack bottom: %p\n", stack_bottom);
    // printf("Temp: %p\n", &temp);
    // char *sp = stack_bottom + (size_t)(char*)temp;
    // printf("SP: %p\n", sp);

    // Scan the stack for pointers to allocated memory regions
    // for (; current < (char*)stack_bottom; current++) {
    //     check_valid_pointer(current);
    // }

    while (stack_top < (char*) stack_bottom + total_size) {
        check_valid_pointer(*(void**)stack_top);
        // void * currRef = *(void **)stack_top;

        // struct MemoryBlock* current = head;
        // bool founder = false;
        // while (current != NULL) {
        //     if (current->free) {
        //         current = current->next;
        //         continue;
        //     }
        //     if (currRef >= (void*)(current+1) && currRef < (void*)((char*)(current+1) + current->size) && current->size % 4 == 0) {
        //         current->used = true; // Set the usage bit to 1
        //         break;
        //     }
        //     if(current->next == NULL || founder) break;
        //     current = current->next;
        // }
        stack_top++;
    }

    // void* sp = temp + (size_t)(char*)stack_bottom;
    // for (char* start = sp; start < (char*)stack_bottom-sizeof(char*); start++) { 
    //     check_valid_pointer((long)((void*)start)); // Check if the pointer is valid
    // }

    printf("Stack scanned.\n");

    // Scan the heap for pointers to allocated memory regions
    // struct MemoryBlock* current = head;
    // while (current) {
    //     for (char* current_ptr = (char*)(current + 1); current_ptr < ((char*)(current + 1) + current->size); current_ptr++) {
    //         check_valid_pointer_heap((long)(void*)current_ptr);
    //     }
    //     current = current->next;
    // }

    // printf("Heap scanned.\n");

    // Mark all unused memory blocks for garbage collection
    for (struct MemoryBlock* current = head; current; current = current->next) {
        if (!current->used) {
            t_free((void*)(current + 1));
        }
        current->used = false;
    }
}

// double get_memory_usage_percentage(){
//     return ((double)memory_in_use / total_memory_allocated) * 100;
// }