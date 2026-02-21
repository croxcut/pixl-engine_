#include "memory.h"

#ifndef PXL_MIN_SPLIT
#define PXL_MIN_SPLIT 32
#endif

#ifndef PXL_ENABLE_STATS
#define PXL_ENABLE_STATS 1
#endif

struct Block {
    size_t  size;
    bool    free;
    Block*  next;
    Block*  chuck_end;
};

struct Footer{
    size_t  size;
};

#if PXL_ENABLE_STATS
static size_t global_bytes_allocated =      0;
static size_t global_peak_bytes =           0;
static size_t global_alloc_count =          0;
#endif

static std::atomic_flag global_heap_lock = ATOMIC_FLAG_INIT;

static Block* global_bins[BLOCK_COUNT] = {};
static Block* global_large_blocks = nullptr;

static inline void lock_heap() {
    while(global_heap_lock.test_and_set(std::memory_order_acquire)) {}
}

static inline void unlock_heap() {
    global_heap_lock.clear(std::memory_order_release);
}

static inline size_t align_up(size_t v, size_t a) {
    return (v + a - 1) & ~(a - 1);
}

static inline Footer* block_footer(Block* block) {
    return (Footer*)((char*)(block + 1) + block->size);
}

static inline Block* next_physical(Block* block) {
    Block* next = (Block*)((char*)(block + 1) + block->size + sizeof(Footer));
    return (next < block->chuck_end) ? next : nullptr;
}

static inline Block* prev_physical(Block* block) {
    if((void*)block <= (void*)((char*)block->chuck_end - CHUNK_SIZE)) 
        return nullptr;

    Footer* prev_footer = (Footer*)((char*)block - sizeof(Footer));
    return (Block*)((char*)block - prev_footer->size - sizeof(Block) - sizeof(Footer));
}

static inline bool is_valid_block(Block* block) {
    return block && block->size > 0;
}

static int bin_index(size_t size) {
    for(size_t i = 0; i < BLOCK_COUNT; i++)
        if(size <= BLOCK_SIZES[i]) return (int)i;
    return -1;
}

static void insert_large_block(Block* block) {
    Block** curr = &global_large_blocks;
    while(*curr && (*curr)->size < block->size)
        curr = &(*curr)->next;
    
    block->next = *curr;
    *curr = block;
}

static void remove_from_free_list(Block* block) {
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

// static Block* find_large_block(size_t size) {
//     Block** prev = &global_large_blocks;
//     Block* curr = global_large_blocks;

//     while(curr) {
//         if(curr->size >= size) {
//             *prev = curr->next;
//             curr->next = nullptr;
//             return curr;
//         }
//         prev = &curr->next;
//         curr = curr->next;
//     }

//     return nullptr;
// }