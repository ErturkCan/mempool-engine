#include "../include/slab.h"
#include "../include/align.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdatomic.h>

/* Sentinel values for allocation tracking */
#define FREE_MARKER 0xDEADBEEFDEADBEEFUL
#define ALLOCATED_MARKER 0xA110CA7EDEAD0001UL

/* Metadata for each block (stored separately from data) */
typedef struct {
	_Atomic(unsigned long) magic;
	_Atomic(int) free;
	size_t block_index;  /* To verify pointer ownership */
} block_metadata_t;

typedef struct slab_allocator {
	void *memory;              /* Base of allocated memory (cache-line aligned data blocks) */
	block_metadata_t *metadata;/* Metadata array (parallel to memory) */
	size_t block_size;         /* Size of each block (cache-line aligned) */
	size_t num_blocks;         /* Total number of blocks */
	_Atomic(size_t) free_count;/* Number of free blocks */
	_Atomic(int) *free_list;   /* Free list: indices of free blocks */
	_Atomic(size_t) free_idx;  /* Current index in free list */
} slab_allocator_t;

slab_allocator_t *slab_create(size_t block_size, size_t num_blocks)
{
	if (block_size == 0 || num_blocks == 0)
		return NULL;

	/* Align block size to cache line */
	size_t aligned_block_size = align_size_to_cache_line(block_size);

	/* Allocate slab allocator structure */
	slab_allocator_t *alloc = malloc(sizeof(slab_allocator_t));
	if (!alloc)
		return NULL;

	/* Allocate metadata array */
	alloc->metadata = malloc(num_blocks * sizeof(block_metadata_t));
	if (!alloc->metadata) {
		free(alloc);
		return NULL;
	}

	/* Allocate free list */
	alloc->free_list = malloc(num_blocks * sizeof(_Atomic(int)));
	if (!alloc->free_list) {
		free(alloc->metadata);
		free(alloc);
		return NULL;
	}

	/* Allocate memory for all blocks (cache-line aligned) */
	size_t total_size = aligned_block_size * num_blocks;
	alloc->memory = aligned_alloc(CACHE_LINE_SIZE, total_size);
	if (!alloc->memory) {
		free(alloc->free_list);
		free(alloc->metadata);
		free(alloc);
		return NULL;
	}

	alloc->block_size = aligned_block_size;
	alloc->num_blocks = num_blocks;
	atomic_store(&alloc->free_count, num_blocks);
	atomic_store(&alloc->free_idx, num_blocks);

	/* Initialize metadata for all blocks */
	for (size_t i = 0; i < num_blocks; i++) {
		atomic_store(&alloc->free_list[i], i);
		atomic_store(&alloc->metadata[i].magic, FREE_MARKER);
		atomic_store(&alloc->metadata[i].free, 1);
		alloc->metadata[i].block_index = i;
	}

	return alloc;
}

void *slab_alloc(slab_allocator_t *alloc)
{
	if (!alloc)
		return NULL;

	/* Try to pop a free block from the free list */
	size_t old_idx = atomic_load(&alloc->free_idx);
	if (old_idx == 0)
		return NULL; /* No free blocks */

	size_t new_idx = old_idx - 1;
	if (!atomic_compare_exchange_strong(&alloc->free_idx, &old_idx, new_idx))
		return slab_alloc(alloc); /* Retry on contention */

	/* Get the block index from the free list */
	int block_idx = atomic_load(&alloc->free_list[new_idx]);
	if (block_idx < 0 || (size_t)block_idx >= alloc->num_blocks)
		return NULL;

	/* Get the block pointer and update metadata */
	void *ptr = (uint8_t *)alloc->memory + block_idx * alloc->block_size;
	block_metadata_t *meta = &alloc->metadata[block_idx];

	atomic_store(&meta->magic, ALLOCATED_MARKER);
	atomic_store(&meta->free, 0);
	atomic_fetch_sub(&alloc->free_count, 1);

	return ptr;
}

int slab_free(slab_allocator_t *alloc, void *ptr)
{
	if (!alloc || !ptr)
		return -1;

	/* Find which block this is based on memory address */
	if (ptr < alloc->memory)
		return -1;

	uintptr_t offset = (uintptr_t)ptr - (uintptr_t)alloc->memory;
	if (offset % alloc->block_size != 0)
		return -1; /* Not properly aligned to block boundary */

	size_t block_idx = offset / alloc->block_size;
	if (block_idx >= alloc->num_blocks)
		return -1; /* Out of bounds */

	block_metadata_t *meta = &alloc->metadata[block_idx];

	/* Verify magic number */
	unsigned long magic = atomic_load(&meta->magic);
	if (magic != ALLOCATED_MARKER)
		return -1; /* Invalid allocation */

	/* Check if already freed */
	if (atomic_load(&meta->free))
		return -1; /* Double free */

	/* Mark as free and restore free marker */
	atomic_store(&meta->free, 1);
	atomic_store(&meta->magic, FREE_MARKER);

	/* Add back to free list */
	size_t old_idx = atomic_load(&alloc->free_idx);
	size_t new_idx = old_idx + 1;

	if (new_idx > alloc->num_blocks)
		return -1; /* Free list overflow */

	atomic_store(&alloc->free_list[old_idx], block_idx);

	if (!atomic_compare_exchange_strong(&alloc->free_idx, &old_idx, new_idx))
		return slab_free(alloc, ptr); /* Retry on contention */

	atomic_fetch_add(&alloc->free_count, 1);
	return 0;
}

void slab_destroy(slab_allocator_t *alloc)
{
	if (!alloc)
		return;

	if (alloc->memory)
		free(alloc->memory);
	if (alloc->metadata)
		free(alloc->metadata);
	if (alloc->free_list)
		free(alloc->free_list);
	free(alloc);
}

int slab_stats(slab_allocator_t *alloc, size_t *used_blocks, size_t *free_blocks)
{
	if (!alloc || !used_blocks || !free_blocks)
		return -1;

	*free_blocks = atomic_load(&alloc->free_count);
	*used_blocks = alloc->num_blocks - *free_blocks;

	return 0;
}
