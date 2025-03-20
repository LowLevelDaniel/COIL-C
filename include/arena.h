/**
 * @file arena.h
 * @brief Memory arena for efficient allocations with bulk free
 */

#ifndef ARENA_H
#define ARENA_H

#include <stddef.h>
#include <stdbool.h>

/**
 * @brief Memory block in the arena
 */
typedef struct ArenaBlock {
  void* memory;
  size_t size;
  size_t used;
  struct ArenaBlock* next;
} ArenaBlock;

/**
 * @brief Memory arena for efficient allocations with bulk free
 */
typedef struct Arena {
  ArenaBlock* first;
  ArenaBlock* current;
  size_t initial_block_size;
  size_t total_allocated;
  size_t total_used;
} Arena;

/**
 * @brief Create a new memory arena with initial capacity
 * @param initial_capacity Initial capacity in bytes
 * @return Newly created arena
 */
Arena* arena_create(size_t initial_capacity);

/**
 * @brief Allocate memory from an arena
 * @param arena The arena to allocate from
 * @param size Size in bytes to allocate
 * @return Pointer to allocated memory
 */
void* arena_alloc(Arena* arena, size_t size);

/**
 * @brief Allocate zeroed memory from an arena
 * @param arena The arena to allocate from
 * @param size Size in bytes to allocate
 * @return Pointer to allocated and zeroed memory
 */
void* arena_calloc(Arena* arena, size_t size);

/**
 * @brief Allocate and copy a string into the arena
 * @param arena The arena to allocate from
 * @param str The string to copy
 * @return Pointer to the copied string
 */
char* arena_strdup(Arena* arena, const char* str);

/**
 * @brief Reset an arena, freeing all allocations at once
 * @param arena The arena to reset
 */
void arena_reset(Arena* arena);

/**
 * @brief Destroy an arena, freeing all memory
 * @param arena The arena to destroy
 */
void arena_destroy(Arena* arena);

/**
 * @brief Get statistics about arena usage
 * @param arena The arena to analyze
 * @param total_allocated Output parameter for total allocated bytes
 * @param total_used Output parameter for total used bytes
 */
void arena_stats(Arena* arena, size_t* total_allocated, size_t* total_used);

#endif /* ARENA_H */