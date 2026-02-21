.PHONY: all clean test bench sanitize bench_alloc bench_contention bench_vs_malloc help

# Compiler and flags
CC := gcc
CFLAGS := -Wall -Wextra -Wpedantic -std=c11 -O2 -D_GNU_SOURCE
CFLAGS_DEBUG := -Wall -Wextra -Wpedantic -std=c11 -g -O0 -D_GNU_SOURCE
LDFLAGS := -lpthread -lm

# ASAN flags
ASAN_FLAGS := $(CFLAGS_DEBUG) -fsanitize=address -fsanitize=undefined
ASAN_LDFLAGS := $(LDFLAGS) -fsanitize=address -fsanitize=undefined

# TSAN flags
TSAN_FLAGS := $(CFLAGS_DEBUG) -fsanitize=thread
TSAN_LDFLAGS := $(LDFLAGS) -fsanitize=thread

# Directories
SRC_DIR := src
INCLUDE_DIR := include
TEST_DIR := tests
BENCH_DIR := bench
BUILD_DIR := build

# Source files
SLAB_SRC := $(SRC_DIR)/slab.c
ARENA_SRC := $(SRC_DIR)/arena.c
POOL_SRC := $(SRC_DIR)/pool.c
ALIGN_SRC := $(SRC_DIR)/align.c

COMMON_SRCS := $(SLAB_SRC) $(ARENA_SRC) $(POOL_SRC) $(ALIGN_SRC)
COMMON_OBJS := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(COMMON_SRCS))

# Test executables
TEST_SLAB := $(BUILD_DIR)/test_slab
TEST_ARENA := $(BUILD_DIR)/test_arena

# Bench executables
BENCH_ALLOC := $(BUILD_DIR)/bench_alloc
BENCH_CONTENTION := $(BUILD_DIR)/bench_contention
BENCH_VS_MALLOC := $(BUILD_DIR)/bench_vs_malloc

# Default target
all: $(BUILD_DIR) test

# Create build directory
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

# Compile object files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(INCLUDE_DIR) -c $< -o $@

# Test targets
test: $(TEST_SLAB) $(TEST_ARENA)
	@echo "Running unit tests..."
	@$(TEST_SLAB)
	@$(TEST_ARENA)
	@echo "All tests passed!"

$(TEST_SLAB): $(TEST_DIR)/test_slab.c $(COMMON_OBJS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(INCLUDE_DIR) $^ -o $@ $(LDFLAGS)

$(TEST_ARENA): $(TEST_DIR)/test_arena.c $(COMMON_OBJS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(INCLUDE_DIR) $^ -o $@ $(LDFLAGS)

# Benchmark targets
bench: bench_alloc bench_contention bench_vs_malloc

bench_alloc: $(BENCH_ALLOC)
	@echo "Running single-threaded allocation benchmark..."
	@$(BENCH_ALLOC)

bench_contention: $(BENCH_CONTENTION)
	@echo "Running multi-threaded contention benchmark..."
	@$(BENCH_CONTENTION) 4

bench_vs_malloc: $(BENCH_VS_MALLOC)
	@echo "Running malloc comparison benchmark..."
	@$(BENCH_VS_MALLOC)

$(BENCH_ALLOC): $(BENCH_DIR)/bench_alloc.c $(COMMON_OBJS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(INCLUDE_DIR) $^ -o $@ $(LDFLAGS)

$(BENCH_CONTENTION): $(BENCH_DIR)/bench_contention.c $(COMMON_OBJS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(INCLUDE_DIR) $^ -o $@ $(LDFLAGS)

$(BENCH_VS_MALLOC): $(BENCH_DIR)/bench_vs_malloc.c $(COMMON_OBJS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(INCLUDE_DIR) $^ -o $@ $(LDFLAGS)

# Sanitizer targets
sanitize: sanitize_asan sanitize_tsan

sanitize_asan: $(BUILD_DIR) $(TEST_DIR)/test_slab.c $(TEST_DIR)/test_arena.c $(COMMON_SRCS)
	@echo "Building with Address Sanitizer..."
	$(CC) $(ASAN_FLAGS) -I$(INCLUDE_DIR) $(TEST_DIR)/test_slab.c $(COMMON_SRCS) -o $(BUILD_DIR)/test_slab_asan $(ASAN_LDFLAGS)
	$(CC) $(ASAN_FLAGS) -I$(INCLUDE_DIR) $(TEST_DIR)/test_arena.c $(COMMON_SRCS) -o $(BUILD_DIR)/test_arena_asan $(ASAN_LDFLAGS)
	@echo "Running tests with ASAN..."
	@$(BUILD_DIR)/test_slab_asan
	@$(BUILD_DIR)/test_arena_asan
	@echo "ASAN tests passed!"

sanitize_tsan: $(BUILD_DIR) $(TEST_DIR)/test_slab.c $(BENCH_DIR)/bench_contention.c $(COMMON_SRCS)
	@echo "Building with Thread Sanitizer..."
	$(CC) $(TSAN_FLAGS) -I$(INCLUDE_DIR) $(TEST_DIR)/test_slab.c $(COMMON_SRCS) -o $(BUILD_DIR)/test_slab_tsan $(TSAN_LDFLAGS)
	$(CC) $(TSAN_FLAGS) -I$(INCLUDE_DIR) $(BENCH_DIR)/bench_contention.c $(COMMON_SRCS) -o $(BUILD_DIR)/bench_contention_tsan $(TSAN_LDFLAGS)
	@echo "Running TSAN tests..."
	@$(BUILD_DIR)/test_slab_tsan
	@echo "TSAN benchmark (2 threads)..."
	@$(BUILD_DIR)/bench_contention_tsan 2
	@echo "TSAN tests passed!"

# Clean target
clean:
	@echo "Cleaning build artifacts..."
	@rm -rf $(BUILD_DIR)
	@echo "Clean complete."

# Help target
help:
	@echo "Memory Pool Allocator - Build System"
	@echo "===================================="
	@echo ""
	@echo "Targets:"
	@echo "  all              - Build and run unit tests (default)"
	@echo "  test             - Run unit tests only"
	@echo "  bench            - Run all benchmarks"
	@echo "  bench_alloc      - Single-threaded allocation latency"
	@echo "  bench_contention - Multi-threaded contention test"
	@echo "  bench_vs_malloc  - Head-to-head malloc comparison"
	@echo "  sanitize         - Run with ASAN and TSAN"
	@echo "  sanitize_asan    - Run with Address Sanitizer only"
	@echo "  sanitize_tsan    - Run with Thread Sanitizer only"
	@echo "  clean            - Remove build artifacts"
	@echo "  help             - Show this help message"
