#ifndef MEMPOOL_H
#define MEMPOOL_H

#include <stddef.h>

typedef struct mempool mempool_t;

/**
 * Create a memory pool with thread-local optimization.
 *
 * @param block_size       Size of each block (will be cache-line aligned)
 * @param blocks_per_thread Number of blocks each thread can cache locally
 * @param total_blocks     Total number of blocks in global pool
 * @return Pointer to memory pool, or NULL on error
 */
mempool_t *pool_create(size_t block_size, size_t blocks_per_thread, size_t total_blocks);

/**
 * Allocate a block from the pool.
 * Uses thread-local cache first, steals from global pool if needed.
 *
 * @param pool Pointer to memory pool
 * @return Pointer to allocated block (cache-line aligned), or NULL
 */
void *pool_alloc(mempool_t *pool);

/**
 * Free a block back to the pool.
 * Returns to thread-local cache if space available, otherwise to global pool.
 *
 * @param pool Pointer to memory pool
 * @param ptr  Pointer to block to free
 * @return 0 on success, -1 on error
 */
int pool_free(mempool_t *pool, void *ptr);

/**
 * Destroy the memory pool and free all resources.
 *
 * @param pool Pointer to memory pool
 */
void pool_destroy(mempool_t *pool);

/**
 * Get statistics from the memory pool.
 *
 * @param pool       Pointer to memory pool
 * @param allocated  Out parameter for number of allocated blocks
 * @param free_count Out parameter for number of free blocks
 * @return 0 on success, -1 on error
 */
int pool_stats(mempool_t *pool, size_t *allocated, size_t *free_count);

#endif /* MEMPOOL_H */
