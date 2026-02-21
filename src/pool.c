#include "../include/mempool.h"
#include "../include/slab.h"
#include "../include/pool_internal.h"
#include "../include/align.h"
#include <stdlib.h>
#include <string.h>
#include <stdatomic.h>
#include <assert.h>
#include <pthread.h>

/* Thread-local storage key */
static pthread_key_t tls_key;
static _Atomic(int) tls_initialized = 0;

/**
 * Initialize thread-local storage.
 */
static void init_tls(void)
{
	if (atomic_exchange(&tls_initialized, 1))
		return;
	pthread_key_create(&tls_key, free);
}

/**
 * Get or create thread-local cache.
 */
static thread_local_cache_t *get_thread_local_cache(size_t blocks_per_thread)
{
	thread_local_cache_t *cache = pthread_getspecific(tls_key);
	if (cache)
		return cache;

	/* Create new cache */
	cache = malloc(sizeof(thread_local_cache_t));
	if (!cache)
		return NULL;

	cache->local_cache = malloc(blocks_per_thread * sizeof(void *));
	if (!cache->local_cache) {
		free(cache);
		return NULL;
	}

	cache->cache_size = blocks_per_thread;
	atomic_store(&cache->cache_count, 0);

	pthread_setspecific(tls_key, cache);
	return cache;
}

mempool_t *pool_create(size_t block_size, size_t blocks_per_thread,
		       size_t total_blocks)
{
	if (block_size == 0 || blocks_per_thread == 0 || total_blocks == 0)
		return NULL;

	init_tls();

	mempool_t *pool = malloc(sizeof(mempool_t));
	if (!pool)
		return NULL;

	/* Create global slab allocator */
	pool->global_slab = slab_create(block_size, total_blocks);
	if (!pool->global_slab) {
		free(pool);
		return NULL;
	}

	pool->block_size = block_size;
	pool->blocks_per_thread = blocks_per_thread;
	atomic_store(&pool->initialized, 1);

	return pool;
}

void *pool_alloc(mempool_t *pool)
{
	if (!pool || !atomic_load(&pool->initialized))
		return NULL;

	/* Get or create thread-local cache */
	thread_local_cache_t *cache = get_thread_local_cache(pool->blocks_per_thread);
	if (!cache)
		return NULL;

	/* Try to allocate from thread-local cache */
	size_t cache_count = atomic_load(&cache->cache_count);
	if (cache_count > 0) {
		size_t new_count = cache_count - 1;
		if (atomic_compare_exchange_strong(&cache->cache_count, &cache_count,
						   new_count)) {
			return cache->local_cache[new_count];
		}
	}

	/* Cache miss: allocate from global slab */
	void *ptr = slab_alloc(pool->global_slab);
	if (!ptr)
		return NULL;

	return ptr;
}

int pool_free(mempool_t *pool, void *ptr)
{
	if (!pool || !ptr || !atomic_load(&pool->initialized))
		return -1;

	/* Get thread-local cache */
	thread_local_cache_t *cache = get_thread_local_cache(pool->blocks_per_thread);
	if (!cache)
		return slab_free(pool->global_slab, ptr);

	/* Try to add to thread-local cache */
	size_t cache_count = atomic_load(&cache->cache_count);
	if (cache_count < pool->blocks_per_thread) {
		if (atomic_compare_exchange_strong(&cache->cache_count, &cache_count,
						   cache_count + 1)) {
			cache->local_cache[cache_count] = ptr;
			return 0;
		}
	}

	/* Cache full: free to global slab */
	return slab_free(pool->global_slab, ptr);
}

void pool_destroy(mempool_t *pool)
{
	if (!pool)
		return;

	if (pool->global_slab)
		slab_destroy(pool->global_slab);

	free(pool);
}

int pool_stats(mempool_t *pool, size_t *allocated, size_t *free_count)
{
	if (!pool || !allocated || !free_count)
		return -1;

	return slab_stats(pool->global_slab, allocated, free_count);
}
