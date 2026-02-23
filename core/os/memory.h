// core/os/memory.h
#ifndef MEMORY_H
#define MEMORY_H

#include "misc/types.h"
#include "misc/pre_compile.h"

constexpr size_t BLOCK_COUNT = 8;
constexpr size_t BLOCK_SIZES[BLOCK_COUNT] = {
    32, 64, 128, 256, 512, 1024, 2048, 4096
};

enum class MemoryTag : u8_t {
    UNKNOWN,
    TEMP,
    ECS,
    RENDERER,
    PHYSICS,
    AUDIO,
    UI,
    COUNT
};

void*   __pxl_malloc(size_t size);
void*   __pxl_realloc(void* ptr, size_t new_size);
void*   __pxl_calloc(size_t num, size_t size);
void    __pxl_free(void* ptr);

void*   __pixl_arena_alloc(size_t size, MemoryTag tag);
void    __pixl_arena_reset();

#define pmalloc(size)               __pxl_malloc(size)
#define prealloc(ptr, new_size)     __pxl_realloc(ptr, new_size)
#define pcalloc(num, size)          __pxl_calloc(num, size)
#define pfree(ptr)                  __pxl_free(ptr)

#define palloc(size, tag)           __pxl_arena_alloc(size, tag)
#define preset()                    __pxl_arena_reset()

#if PXL_ENABLE_STATS
    size_t pxl_allocated_bytes();
    size_t pxl_peak_bytes();
    size_t pxl_alloc_count();
#endif


#endif
