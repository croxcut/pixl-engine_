// core/os/memory.h
#ifndef MEMORY_H
#define MEMORY_H

#include "misc/types.h"
#include "misc/pre_compile.h"

inline static void* os_alloc(size_t size) {
#ifdef _WIN32
    return VirtualAlloc(nullptr, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#else     
    void* ptr = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANNONYMOUS, -1, 0);
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

#define PXL_ENABLE_DEBUG    0x001
#define PXL_ENABLE_NUMA     0x001
#define PXL_MAX_TRHEADS     0x0040

constexpr size_t PAGE_SIZE =        4096;
constexpr u64_t CANARY =            0xDEADC0DECAFEBABE;

constexpr size_t KB =               1024;
constexpr size_t MB =               KB * KB;

constexpr size_t CHUNK_SIZE =       MB;
constexpr size_t ALIGNMENT =        alignof(std::max_align_t);

constexpr size_t BLOCK_COUNT =      8;
constexpr size_t BLOCK_SIZES[BLOCK_COUNT] = {
    32, 64, 128, 256, 512, 1024, 2048, 4096
};



#endif
