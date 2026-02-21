#include "../include/align.h"
#include <assert.h>

/* This file provides implementations of inline functions from align.h
   Included for completeness and testing purposes. */

void _align_test_compile(void)
{
	/* Compile-time verification that alignment macros work */
	_Static_assert(CACHE_LINE_SIZE > 0, "Invalid cache line size");
	_Static_assert((CACHE_LINE_SIZE & (CACHE_LINE_SIZE - 1)) == 0,
		       "Cache line size must be power of 2");
}
