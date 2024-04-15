//
// Created by Abhinav Chadaga on 3/21/24.
//

#ifndef TDMM_H_
#define TDMM_H_

#include <stddef.h>

typedef enum
{
  FIRST_FIT,
  BEST_FIT,
  WORST_FIT,
  BUDDY
} alloc_strat_e;

/**
 * Initializes the memory allocator with the given strategy.
 * @param strat The strategy to use for memory allocation.
 * @param stack_bot The bottom of the stack.
 */
void t_init (alloc_strat_e strat, void* stack_bot);

/**
 * Allocates a block of memory of the given size.
 * @param size The size of the memory block to allocate.
 * @return A pointer to the allocated memory block
 * fails.
 */
void *t_malloc (size_t size);

/**
 * Frees the given memory block.
 * @param ptr The pointer to the memory block to free. This must be a
 * pointer returned by t_malloc.
 */
void t_free (void *ptr);

/**
 * Performs basic garbage collection by scanning the stack and heap managed
 * by t_malloc and t_free.
 */
void t_gcollect (void);

/**
 * Memory utilization as a percentage.
 * @return Memory utilization as a percentage
 */
double t_memutil (void);

/**
 * Returns the size of the overhead meta data.
 * @return The size of the overhead meta data.
 */
size_t get_overhead_meta_size();

#endif // TDMM_H_