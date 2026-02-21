#ifndef MEMPOOL_ARENA_H
#define MEMPOOL_ARENA_H

#include <stddef.h>

typedef struct arena_allocator arena_allocator_t;

/**
 * Create an arena (bump) allocator.
 *
 * @param capacity Total capacity of the arena in bytes
 * @return Pointer to arena allocator, or NULL on error
 */
arena_allocator_t *arena_create(size_t capacity);

/**
 * Allocate memory from the arena.
 * Fast sequential allocation without free (use arena_reset instead).
 *
 * @param alloc Pointer to arena allocator
 * @param size  Number of bytes to allocate (will be aligned to cache line)
 * @return Pointer to allocated memory (cache-line aligned), or NULL on overflow
 */
void *arena_alloc(arena_allocator_t *alloc, size_t size);

/**
 * Reset the arena, freeing all allocations at once.
 * This is the only way to free memory in an arena allocator.
 *
 * @param alloc Pointer to arena allocator
 */
void arena_reset(arena_allocator_t *alloc);

/**
 * Destroy the arena allocator and free all resources.
 *
 * @param alloc Pointer to arena allocator
 */
void arena_destroy(arena_allocator_t *alloc);

/**
 * Get statistics from the arena allocator.
 *
 * @param alloc     Pointer to arena allocator
 * @param used      Out parameter for bytes used
 * @param capacity  Out parameter for total capacity
 * @return 0 on success, -1 on error
 */
int arena_stats(arena_allocator_t *alloc, size_t *used, size_t *capacity);

#endif /* MEMPOOL_ARENA_H */
