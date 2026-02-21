#ifndef MEMPOOL_POOL_INTERNAL_H
#define MEMPOOL_POOL_INTERNAL_H

#include "slab.h"
#include <stddef.h>
#include <stdatomic.h>

/* Thread-local data structure for each thread */
typedef struct {
	void **local_cache;      /* Thread-local free list cache */
	size_t cache_size;       /* Max capacity of cache */
	_Atomic(size_t) cache_count; /* Current items in cache */
} thread_local_cache_t;

/* Main pool structure */
typedef struct mempool {
	slab_allocator_t *global_slab; /* Shared global slab */
	size_t block_size;             /* Size of each block */
	size_t blocks_per_thread;      /* Max blocks per thread cache */
	_Atomic(int) initialized;      /* Initialization flag */
} mempool_t;

#endif /* MEMPOOL_POOL_INTERNAL_H */
