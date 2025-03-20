/**
 * @file arena.c
 * @brief Implementation of memory arena for efficient allocations
 */

#include "../include/arena.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/**
 * Create a new arena block
 */
static ArenaBlock* arena_block_create(size_t size) {
  ArenaBlock* block = malloc(sizeof(ArenaBlock));
  if (!block) return NULL;
  
  block->memory = malloc(size);
  if (!block->memory) {
    free(block);
    return NULL;
  }
  
  block->size = size;
  block->used = 0;
  block->next = NULL;
  
  return block;
}

/**
 * Destroy an arena block
 */
static void arena_block_destroy(ArenaBlock* block) {
  if (block) {
    free(block->memory);
    free(block);
  }
}

Arena* arena_create(size_t initial_capacity) {
  Arena* arena = malloc(sizeof(Arena));
  if (!arena) return NULL;
  
  arena->first = arena_block_create(initial_capacity);
  if (!arena->first) {
    free(arena);
    return NULL;
  }
  
  arena->current = arena->first;
  arena->initial_block_size = initial_capacity;
  arena->total_allocated = initial_capacity;
  arena->total_used = 0;
  
  return arena;
}

void* arena_alloc(Arena* arena, size_t size) {
  // Align size to 8-byte boundary for better memory access
  size = (size + 7) & ~7;
  
  // Check if there's enough space in the current block
  if (arena->current->used + size > arena->current->size) {
    // Need a new block
    size_t new_block_size = size > arena->initial_block_size ? 
                           size : arena->initial_block_size;
    
    // Double the block size for future allocations
    new_block_size = new_block_size * 2;
    
    ArenaBlock* new_block = arena_block_create(new_block_size);
    if (!new_block) return NULL;
    
    arena->current->next = new_block;
    arena->current = new_block;
    arena->total_allocated += new_block_size;
  }
  
  // Allocate from the current block
  void* ptr = (char*)arena->current->memory + arena->current->used;
  arena->current->used += size;
  arena->total_used += size;
  
  return ptr;
}

void* arena_calloc(Arena* arena, size_t size) {
  void* ptr = arena_alloc(arena, size);
  if (ptr) {
    memset(ptr, 0, size);
  }
  return ptr;
}

char* arena_strdup(Arena* arena, const char* str) {
  if (!str) return NULL;
  
  size_t len = strlen(str) + 1;
  char* new_str = arena_alloc(arena, len);
  if (new_str) {
    memcpy(new_str, str, len);
  }
  return new_str;
}

void arena_reset(Arena* arena) {
  ArenaBlock* block = arena->first;
  while (block) {
    block->used = 0;
    block = block->next;
  }
  
  arena->current = arena->first;
  arena->total_used = 0;
}

void arena_destroy(Arena* arena) {
  if (!arena) return;
  
  ArenaBlock* block = arena->first;
  while (block) {
    ArenaBlock* next = block->next;
    arena_block_destroy(block);
    block = next;
  }
  
  free(arena);
}

void arena_stats(Arena* arena, size_t* total_allocated, size_t* total_used) {
  if (total_allocated) *total_allocated = arena->total_allocated;
  if (total_used) *total_used = arena->total_used;
}