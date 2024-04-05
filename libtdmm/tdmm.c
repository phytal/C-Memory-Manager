#include <sys/mman.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdint.h>
#include "tdmm.h"

#define PAGE_SIZE (1024 * 4) // 4 KB
#define META_SIZE sizeof(struct MemoryBlock)

// Structure to represent a memory block
struct MemoryBlock { // 24->32(buddy) bytes
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

// Function to align pointer to multiples of 4
void* align_ptr(void* ptr) {
    size_t address = (size_t)ptr;
    size_t remainder = address % 4;
    if (remainder != 0) {
        address += (4 - remainder);
    }
    return (void*)address;
}

// Function to add memory to the linked list
// Precondition: The current block is the last block in the linked list
void add_mem(struct MemoryBlock* current, size_t size) {
    // Ensure size is a multiple of 4 for alignment
    // size = (size + 3) & (~3);

    void* new_mem_start = mmap(NULL, size + META_SIZE + 3, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if (new_mem_start == MAP_FAILED) {
        fprintf(stderr, "Memory allocation failed");
        exit(EXIT_FAILURE);
    }
    new_mem_start = align_ptr(new_mem_start);

    // Create a new memory block structure
    struct MemoryBlock* new_block = (struct MemoryBlock*)new_mem_start;
    new_block->size = size;
    new_block->free = false;
    new_block->next = NULL;
    new_block->prev = current;

    // Insert the new block into the linked list
    if (current == NULL) { // should not happen but just in case
        head = new_block;
    } else {
        current->next = new_block;
        new_block->prev = current;
    }
}

// Function to write a memory block with the given size
void write_block(struct MemoryBlock* current, size_t size) {
    // If the current block is larger than the requested size, split it
    if (current->size >= size + META_SIZE + 4) {
        // Create a new block for the remaining free memory
        struct MemoryBlock* remaining = (struct MemoryBlock*)((char*)current + META_SIZE + size);
        
        // char* diff = (char*)align_ptr(remaining) - (char*)remaining;
        // remaining = align_ptr(remaining);
        remaining->size = current->size - size - META_SIZE;
        remaining->free = true;
        remaining->next = current->next;
        remaining->prev = current;

        // Update the size of the current block and mark it as allocated
        current->size = size; // + diff
        current->free = false;
        current->next = remaining;
    } else {
        // If the current block is just the right size (+3), mark it as allocated
        current->free = false;
    }
}

void t_init (alloc_strat_e strat, void* stack_bot) {
    alloc_strat = strat;
    stack_bottom = stack_bot;

    mem_start = mmap(NULL, PAGE_SIZE*10, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if (mem_start == MAP_FAILED) {
        fprintf(stderr, "Memory allocation failed");
        exit(EXIT_FAILURE);
    }
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
    while (current && (current->next || current->prev == NULL)) {
        if (current->free && current->size >= size) {
            write_block(current, size);

            return (void*)(current); // Return pointer to start of block
        }
        current = current->next;
    }

    // No suitable block found, must resize
    add_mem(current, size);

    return (void*)(current->next);
}

// Function to allocate memory using Best Fit policy
void* best_fit(size_t size) {
    struct MemoryBlock* best_block = NULL;
    struct MemoryBlock* current = head;
    while (current && (current->next || current->prev == NULL)) {
        if (current->free && current->size >= size && (best_block == NULL || current->size < best_block->size)) {
            best_block = current;
        }
        current = current->next;
    }
    if (best_block) {
        write_block(best_block, size);

        return (void*)(best_block); // Return pointer to start of block
    }
    // No suitable block found
    add_mem(current, size);

    return (void*)(current->next);
}

// Function to allocate memory using Worst Fit policy
void* worst_fit(size_t size) {
    struct MemoryBlock* worst_block = NULL;
    struct MemoryBlock* current = head;
    while (current && (current->next || current->prev == NULL)) {
        if (current->free && current->size >= size && (worst_block == NULL || current->size > worst_block->size)) {
            worst_block = current;
        }
        current = current->next;
    }
    if (worst_block) {
        write_block(worst_block, size);

        return (void*)(worst_block); // Return pointer to start of block
    }
    
    // No suitable block found
    add_mem(current, size);

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
    while (current && (current->next || current->prev == NULL)) {
        if (current->free && current->size >= block_size && (best_block == NULL || current->size < best_block->size)) {
            best_block = current;
        }
        current = current->next;
    }
    if (best_block) {
        // Split larger blocks until reaching the required level
        int best_block_size = best_block->size;
        int i = 0;
        while (best_block_size > 0) {
            best_block_size >>= 1;
            best_block_size -= META_SIZE;
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
        add_mem(current, block_size);

        return (void*)(current->next);
    }
}

void *t_malloc (size_t size) {
    // Ensure size is a multiple of 4 for alignment
    // size += META_SIZE;
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

    // Align pointer to ensure it's a multiple of 4
    ptr = align_ptr(ptr);

    // Create memory block structure
    struct MemoryBlock* block = (struct MemoryBlock*)ptr;
    printf("Block size: %lu\n", block->size);
    printf("Block free: %d\n", block->free);
    printf("Block next: %p\n", block->next);
    printf("Block prev: %p\n", block->prev);

    // Return pointer to user-visible memory region
    return (void*)(block + 1);
}

void t_free (void *ptr) {
      if (ptr == NULL) {
        return; // Nothing to free
    }

    // Find the corresponding memory block
    struct MemoryBlock* block = (struct MemoryBlock*)((char*)ptr - META_SIZE);

    // Mark the block as free
    block->free = true;

    // Merge adjacent free blocks here
    struct MemoryBlock* current = block;
    bool merged_left = false;
    bool merged_right = false;
    while (!merged_left && !merged_right) {
        if (current->free && current->prev != NULL && current->prev->free) {
            if (alloc_strat == BUDDY && current->size != current->prev->size) {
                break;
            }
            current->prev->size += current->size + META_SIZE;
            current->prev->next = current->next;
            current = current->prev;
            merged_left = false;
        } else {
            merged_left = true;
        }
        if (current->free && current->next != NULL && current->next->free) {
            if (alloc_strat == BUDDY && current->size != current->next->size) {
                break;
            }
            current->size += current->next->size + META_SIZE;
            current->next = current->next->next;
            current = current->next;
            merged_right = false;
        } else {
            merged_right = true;
        }
    }

    // Update the linked list here
    // struct MemoryBlock* prev = NULL;
    // current = head;
    // while (current) {
    //     if (current->free && current->next && current->next->free) {
    //         // Remove current from the linked list
    //         if (prev == NULL) {
    //             head = current->next;
    //         } else {
    //             prev->next = current->next;
    //         }

    //         // Merge current with the next block
    //         current->size += current->next->size + META_SIZE;
    //         current->next = current->next->next;
    //     } else {
    //         prev = current;
    //         current = current->next;
    //     }
    // }

    printf("Block size: %lu\n", current->size);
    printf("Block free: %d\n", current->free);
    printf("Block next: %p\n", current->next);
    printf("Block prev: %p\n", current->prev);
}

// Function to check if a given pointer is within the allocated memory regions
void check_valid_pointer(void* ptr) {
    struct MemoryBlock* current = head;
    while (current) {
        void* block_start = (void*)(current + 1);
        void* block_end = (void*)((char*)block_start + current->size);
        if (ptr >= block_start && ptr < block_end) {
            current->used = true; // Set the usage bit to 1
            break;
        }
        current = current->next;
    }
}

// basically if theres a pointer in the stack that points to anywhere in a valid memory block, set the usage bit to 1 (used)
// if the pointer is not in the stack but still in the heap, set the usage bit to 0 (unused) to be garbage collected

// Function to scan the stack for pointers to allocated memory regions
void t_gcollect (void) {
    char *temp;
    char *sp = temp + (size_t)(char*)stack_bottom;
    for (char* start = sp; start > (char*)stack_bottom-sizeof(char*); start++) { 
        check_valid_pointer((long*)((void*)start)); // Check if the pointer is valid
    }

    // Mark all unused memory blocks for garbage collection
    for (struct MemoryBlock* current = head; current; current = current->next) {
        if (!current->used) {
            t_free((void*)(current + 1));
        }
        current->used = false;
    }
}