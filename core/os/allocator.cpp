//  core/os/allocator.cpp
#include "memory.h"

#ifndef PXL_MIN_SPLIT
#define PXL_MIN_SPLIT 32
#endif

#ifndef PXL_ENABLE_STATS
#define PXL_ENABLE_STATS 1
#endif

constexpr size_t CHUNK_SIZE = 1024 * 1024; 
constexpr size_t ALIGNMENT = alignof(std::max_align_t);

struct Block{
    size_t  size;
    bool    free;
    Block*  next;
    Block*  chunk_end;
};

struct Footer{
    size_t  size;
};

// Global Heap Lock
static  std::atomic_flag global_heap_lock = ATOMIC_FLAG_INIT;

// Free lists
static Block* global_bins[BLOCK_COUNT] = {};
static Block* global_large_blocks = nullptr;

#if PXL_ENABLE_STATS
static size_t global_bytes_allocated = 0;
static size_t global_peak_bytes      = 0;
static size_t global_alloc_count     = 0;
#endif

// OS Memory Helper
inline static void* os_alloc(size_t size) {
#ifdef _WIN32
    return VirtualAlloc(nullptr, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#else   
    void* ptr = mmap(nullptr, size, PROT_READ | PROT_WRITE,
                    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    return (ptr == MAP_FAILED) ? nullptr : ptr;
#endif
}

inline static void os_free(void* ptr, size_t size) {
#ifdef _WIN32
    VirtualFree(ptr, 0, MEM_RELEASE);
#else   
    munmap(ptr, size);
#endif
}

// Lock Helpers
static inline void lock_heap() {
    while (global_heap_lock.test_and_set(std::memory_order_acquire)) {}
}   

static inline void unlock_heap() {
    global_heap_lock.clear(std::memory_order_release);
}

// ALign Helpers
static inline size_t align_up(size_t v, size_t a) {
    return (v + a - 1) & ~(a - 1);
}

// Block Utilities
static inline Footer* block_footer(Block* block) {
    return (Footer*)((char*)(block + 1) + block->size);
}

static inline Block* next_physical(Block* block) {
    Block* next = (Block*)((char*)(block + 1) + block->size + sizeof(Footer));
    return (next < block->chunk_end) ? next : nullptr;
} 

static inline Block* prev_physical(Block* block) {
    if((void*)block <= (void*)((char*)block->chunk_end - CHUNK_SIZE)) return nullptr;
    Footer* prev_footer = (Footer*)((char*)block - sizeof(Footer));
    return (Block*)((char*)block - prev_footer->size - sizeof(Block) - sizeof(Footer));
}

static inline int bin_index(size_t size) {
    for(size_t i = 0; i < BLOCK_COUNT; i++) 
        if(size <= BLOCK_SIZES[i]) return (int)i;
    return -1;
}

// Free list helpers
static void insert_large_block(Block* block) {
    Block** curr = &global_large_blocks;
    while(*curr && (*curr)->size < block->size) 
        curr = &(*curr)->next;
    
    block->next = *curr;
    *curr = block;
}

static void remove_from_free_lists(Block* block) {
    int bin = bin_index(block->size);
    Block** list = (bin >= 0) ? &global_bins[bin] : &global_large_blocks;
    while(*list) {
        if(*list == block) {
            *list = block->next;
            return;
        }
        list = &(*list)->next;
    }
}

// Block ALlocation
static Block* alloc_block(size_t size) {
    size_t total = align_up(sizeof(Block) + size + sizeof(Footer), ALIGNMENT);
    size_t request = align_up(total, CHUNK_SIZE);
    void* memory = os_alloc(request);
    if(!memory) return nullptr;

    Block* block = (Block*)memory;
    block->size = request - sizeof(Block) - sizeof(Footer);
    block->free = false;
    block->next = nullptr;
    block->chunk_end = (Block*)((char*)memory + request);

    block_footer(block)->size = block->size;
    return block;
} 

// Split & Coallesce
static void split_block(Block* block, size_t size) {
    size_t remaining = block->size - size;
    if (remaining < sizeof(Block) + sizeof(Footer) + PXL_MIN_SPLIT) 
        return;

    Block* split = (Block*)((char*)(block+1) + size + sizeof(Footer));
    split->size = remaining - sizeof(Block) - sizeof(Footer);
    split->free = true;
    split->next = nullptr;
    split->chunk_end = block->chunk_end;

    block->size = size;

    block_footer(block)->size = block->size;
    block_footer(split)->size = split->size;

    int bin = bin_index(split->size);
    if (bin >= 0) {
        split->next = global_bins[bin];
        global_bins[bin] = split;
    } else {
        insert_large_block(split);
    }
}

static Block* coalesce(Block* block) {
    Block* next = next_physical(block);
    if (next && next->free) {
        remove_from_free_lists(next);
        block->next += sizeof(Block) + sizeof(Footer) + next->size;
    }

    Block* prev = prev_physical(block);
    if (prev && prev->free) {
        remove_from_free_lists(block);
        prev->size += sizeof(Block) + sizeof(Footer) + block->size;
        block = prev;
    }

    block_footer(block)->size = block->size;
    return block;
}

// ===== Allocation API =====
void* __pxl_malloc(size_t size) {
    if (size == 0) return nullptr;
    size = align_up(size, ALIGNMENT);

    lock_heap();
    int bin = bin_index(size);
    if (bin >= 0 && global_bins[bin]) {
        Block* block = global_bins[bin];
        global_bins[bin] = block->next;
        block->free = false;

#if PXL_ENABLE_STATS
        global_bytes_allocated += block->size;
        global_peak_bytes = std::max(global_peak_bytes, global_bytes_allocated);
        global_alloc_count++;
#endif
        unlock_heap();
        return block + 1;
    }

    Block* block = alloc_block(size);
    if (!block) { unlock_heap(); return nullptr; }

    split_block(block, size);

#if PXL_ENABLE_STATS
    global_bytes_allocated += block->size;
    global_peak_bytes = std::max(global_peak_bytes, global_bytes_allocated);
    global_alloc_count++;
#endif

    unlock_heap();
    return block + 1;
}

void* __pxl_realloc(void* ptr, size_t new_size) {
    if (!ptr) return __pxl_malloc(new_size);
    if (new_size == 0) { __pxl_free(ptr); return nullptr; }

    new_size = align_up(new_size, ALIGNMENT);
    Block* block = ((Block*)ptr) - 1;

    if (block->size >= new_size) return ptr;

    lock_heap();
    Block* next = next_physical(block);
    if (next && next->free && 
        block->size + sizeof(Block) + sizeof(Footer) + next->size >= new_size) {
        remove_from_free_lists(next);
        block->size += sizeof(Block) + sizeof(Footer) + next->size;
        split_block(block, new_size);
        unlock_heap();
        return ptr;
    }
    unlock_heap();

    void* new_ptr = __pxl_malloc(new_size);
    if (new_ptr) { memcpy(new_ptr, ptr, block->size); __pxl_free(ptr); }
    return new_ptr;
}

void* __pxl_calloc(size_t num, size_t size) {
    if (num && size > SIZE_MAX / num) return nullptr;
    size_t total = num * size;
    void* ptr = __pxl_malloc(total);
    if (ptr) memset(ptr, 0, total);
    return ptr;
}

void __pxl_free(void* ptr) {
    if (!ptr) return;
    Block* block = ((Block*)ptr) - 1;
    assert(!block->free && "double free");

#if PXL_ENABLE_STATS
    global_bytes_allocated -= block->size;
#endif

    lock_heap();
    block->free = true;
    block = coalesce(block);

    int bin = bin_index(block->size);
    if (bin >= 0) { block->next = global_bins[bin]; global_bins[bin] = block; }
    else insert_large_block(block);

    unlock_heap();
}

#if PXL_ENABLE_STATS
size_t pxl_allocated_bytes() { return global_bytes_allocated; }
size_t pxl_peak_bytes()      { return global_peak_bytes; }
size_t pxl_alloc_count()     { return global_alloc_count; }
#endif