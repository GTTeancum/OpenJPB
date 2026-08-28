#ifndef JPB_MEMORY_H
#define JPB_MEMORY_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    MEMORY_POOL_COUNT = 4,
    MEMORY_POOL_ANY = -1
};

/*
 * Direct PDB type 0x115C. Microsoft unsigned long is represented explicitly
 * as uint32_t so this stays correct on every evidence host and on Xbox.
 */
typedef struct MemoryPool {
    char *pMemPool;
    uint32_t memSize;
    uint32_t memFree;
    uint32_t memUsed;
    uint32_t memFlags;
} MemoryPool;

extern uint32_t mDefaultMemBank;
extern MemoryPool maMemoryBanks[MEMORY_POOL_COUNT];
extern char mem_pool_0[0x20000];
extern char mem_pool_1[0xAFC00];
extern char mem_pool_2[0x180000];
extern char mem_pool_3a[0x500000];

int memory_FlushMemoryPool(int type);
int memory_InitMemoryPool(char *memory, uint32_t size, int type);
int memory_InitMemorySystem(void);
void *memory_gCalloc(unsigned n, unsigned size);
void *memory_gCallocAnyMemory(unsigned n, unsigned size);
void *memory_gCallocMemoryFunc(
    unsigned n,
    unsigned size,
    int type,
    int line,
    char *file);
void *memory_gDrainMemoryPool(unsigned *rsize, int type);
uint32_t memory_gMemUseage(void);
void memory_gSetDefaultMemoryType(int type);
int memory_gTestMemoryPool(int type);

#ifdef __cplusplus
}
#endif

#if defined(__cplusplus)
#define JPB_MEMORY_STATIC_ASSERT(condition, message) static_assert(condition, message)
#else
#define JPB_MEMORY_STATIC_ASSERT(condition, message) _Static_assert(condition, message)
#endif

JPB_MEMORY_STATIC_ASSERT(
    offsetof(MemoryPool, pMemPool) == 0, "MemoryPool.pMemPool layout changed");
JPB_MEMORY_STATIC_ASSERT(
    offsetof(MemoryPool, memSize) == sizeof(void *),
    "MemoryPool.memSize layout changed");
JPB_MEMORY_STATIC_ASSERT(
    offsetof(MemoryPool, memFree) == sizeof(void *) + 4,
    "MemoryPool.memFree layout changed");
JPB_MEMORY_STATIC_ASSERT(
    offsetof(MemoryPool, memUsed) == sizeof(void *) + 8,
    "MemoryPool.memUsed layout changed");
JPB_MEMORY_STATIC_ASSERT(
    offsetof(MemoryPool, memFlags) == sizeof(void *) + 12,
    "MemoryPool.memFlags layout changed");

#if UINTPTR_MAX == UINT64_MAX
JPB_MEMORY_STATIC_ASSERT(sizeof(MemoryPool) == 24, "x64 pool must match PDB");
#elif UINTPTR_MAX == UINT32_MAX
JPB_MEMORY_STATIC_ASSERT(sizeof(MemoryPool) == 20, "Xbox pool must be 20 bytes");
#else
#error Unsupported pointer width for MemoryPool
#endif

#undef JPB_MEMORY_STATIC_ASSERT

#endif
