#ifndef MEMPOOL_ALIGN_H
#define MEMPOOL_ALIGN_H

#include <stddef.h>
#include <stdint.h>

/* Cache-line size detection */
#ifdef __x86_64__
  #define CACHE_LINE_SIZE 64
#elif defined(__aarch64__)
  #define CACHE_LINE_SIZE 64
#elif defined(__arm__)
  #define CACHE_LINE_SIZE 32
#else
  #define CACHE_LINE_SIZE 64
#endif

/* Alignment attribute */
#define CACHE_ALIGNED __attribute__((aligned(CACHE_LINE_SIZE)))

/**
 * Align pointer to the next cache-line boundary.
 * Returns the smallest address >= ptr that is cache-line aligned.
 */
static inline void *align_to_cache_line(void *ptr)
{
	uintptr_t p = (uintptr_t)ptr;
	uintptr_t aligned = (p + CACHE_LINE_SIZE - 1) & ~(CACHE_LINE_SIZE - 1);
	return (void *)aligned;
}

/**
 * Align size to the next cache-line boundary.
 * Returns the smallest size >= sz that is a multiple of CACHE_LINE_SIZE.
 */
static inline size_t align_size_to_cache_line(size_t sz)
{
	return (sz + CACHE_LINE_SIZE - 1) & ~(CACHE_LINE_SIZE - 1);
}

/**
 * Calculate padding needed to align ptr to cache-line.
 */
static inline size_t padding_for_alignment(void *ptr)
{
	uintptr_t p = (uintptr_t)ptr;
	size_t misalignment = p % CACHE_LINE_SIZE;
	return misalignment ? CACHE_LINE_SIZE - misalignment : 0;
}

/**
 * Check if a pointer is cache-line aligned.
 */
static inline int is_cache_aligned(void *ptr)
{
	return ((uintptr_t)ptr % CACHE_LINE_SIZE) == 0;
}

#endif /* MEMPOOL_ALIGN_H */
