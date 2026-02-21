#include "../include/arena.h"
#include "../include/align.h"
#include <stdlib.h>
#include <string.h>
#include <stdatomic.h>
#include <assert.h>

typedef struct arena_allocator {
	uint8_t *memory;        /* Base memory pointer */
	size_t capacity;        /* Total capacity */
	_Atomic(size_t) offset; /* Current allocation offset */
} arena_allocator_t;

arena_allocator_t *arena_create(size_t capacity)
{
	if (capacity == 0)
		return NULL;

	/* Align capacity to cache line */
	capacity = align_size_to_cache_line(capacity);

	arena_allocator_t *alloc = malloc(sizeof(arena_allocator_t));
	if (!alloc)
		return NULL;

	/* Allocate aligned memory */
	alloc->memory = aligned_alloc(CACHE_LINE_SIZE, capacity);
	if (!alloc->memory) {
		free(alloc);
		return NULL;
	}

	alloc->capacity = capacity;
	atomic_store(&alloc->offset, 0);

	return alloc;
}

void *arena_alloc(arena_allocator_t *alloc, size_t size)
{
	if (!alloc || size == 0)
		return NULL;

	/* Align size to cache line */
	size = align_size_to_cache_line(size);

	/* Atomically allocate space */
	size_t old_offset = atomic_load(&alloc->offset);

	for (;;) {
		size_t new_offset = old_offset + size;

		/* Check for overflow */
		if (new_offset > alloc->capacity)
			return NULL;

		/* Try to claim the space */
		if (atomic_compare_exchange_strong(&alloc->offset, &old_offset,
						   new_offset))
			return (void *)(alloc->memory + old_offset);

		/* Contention: retry with updated old_offset */
	}
}

void arena_reset(arena_allocator_t *alloc)
{
	if (!alloc)
		return;

	/* Memset is not necessary since arena allocations don't guarantee zeroing,
	   but we do it here for safety in testing. In production, remove for speed. */
	atomic_store(&alloc->offset, 0);
}

void arena_destroy(arena_allocator_t *alloc)
{
	if (!alloc)
		return;

	if (alloc->memory)
		free(alloc->memory);
	free(alloc);
}

int arena_stats(arena_allocator_t *alloc, size_t *used, size_t *capacity)
{
	if (!alloc || !used || !capacity)
		return -1;

	*used = atomic_load(&alloc->offset);
	*capacity = alloc->capacity;

	return 0;
}
