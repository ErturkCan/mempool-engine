#ifndef MEMPOOL_SLAB_H
#define MEMPOOL_SLAB_H

#include <stddef.h>
#include <stdint.h>

typedef struct slab_allocator slab_allocator_t;

/**
 * Create a slab allocator with pre-allocated blocks.
 *
 * @param block_size Size of each block (will be aligned to cache line)
 * @param num_blocks Number of blocks to pre-allocate
 * @return Pointer to slab allocator, or NULL on error
 */
slab_allocator_t *slab_create(size_t block_size, size_t num_blocks);

/**
 * Allocate a single block from the slab.
 * Returns NULL if all blocks are exhausted or allocator is invalid.
 *
 * @param alloc Pointer to slab allocator
 * @return Pointer to allocated block (cache-line aligned), or NULL
 */
void *slab_alloc(slab_allocator_t *alloc);

/**
 * Free a block back to the slab.
 * Returns 0 on success, -1 on error (invalid pointer or already freed).
 *
 * @param alloc Pointer to slab allocator
 * @param ptr   Pointer to block to free
 * @return 0 on success, -1 on error
 */
int slab_free(slab_allocator_t *alloc, void *ptr);

/**
 * Destroy the slab allocator and free all resources.
 *
 * @param alloc Pointer to slab allocator
 */
void slab_destroy(slab_allocator_t *alloc);

/**
 * Get statistics from the slab allocator.
 *
 * @param alloc        Pointer to slab allocator
 * @param used_blocks  Out parameter for number of allocated blocks
 * @param free_blocks  Out parameter for number of free blocks
 * @return 0 on success, -1 on error
 */
int slab_stats(slab_allocator_t *alloc, size_t *used_blocks, size_t *free_blocks);

#endif /* MEMPOOL_SLAB_H */
