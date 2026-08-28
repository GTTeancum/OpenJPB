/*
 * COMPLETE REVIEWED RECONSTRUCTION of
 * W:\SWJediPowerBattles\Work\memory.c.
 *
 * Provenance:
 *   direct     - API/global names, MemoryPool layout, bank sizes, addresses,
 *                signatures, source line arguments, and code extents.
 *   assembly   - all ten bodies checked instruction-by-instruction at RVAs
 *                0xBEA80 through 0xBEEFA, including signed rounding, address
 *                return residues, the exhaustion diagnostic, and _Exit(1).
 *
 * Despite their names, the calloc routines do not clear memory. Pools are
 * eight-byte-aligned bump allocators and accept an allocation only when its
 * aligned size is strictly less than memFree.
 */

#include "jpb/memory.h"

#include "jpb/alloc.h"
#include "jpb/whook.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

uint32_t mDefaultMemBank;
MemoryPool maMemoryBanks[MEMORY_POOL_COUNT];

char mem_pool_0[0x20000];
char mem_pool_1[0xAFC00];
char mem_pool_2[0x180000];
char mem_pool_3a[0x500000];

static uint32_t memory_align_eight(uint32_t value)
{
    if ((value & 7u) != 0) {
        value = (value & ~7u) + 8u;
    }
    return value;
}

static int32_t memory_align_eight_signed(int32_t value)
{
    if (value % 8 != 0) {
        value = (value / 8 + 1) * 8;
    }
    return value;
}

static void memory_fatal(void)
{
    _Exit(1);
}

/* Reference RVA 0xBEA80, 65 bytes. */
int memory_FlushMemoryPool(int type)
{
    MemoryPool *pool = &maMemoryBanks[type];

    if (pool->pMemPool == NULL) {
        memory_fatal();
    }
    pool->memFree = pool->memSize;
    pool->memUsed = 0;

    /* EAX contains memSize on the successful reference path. */
    return (int)pool->memSize;
}

/* Reference RVA 0xBEAD0, 75 bytes. */
int memory_InitMemoryPool(char *memory, uint32_t size, int type)
{
    MemoryPool *pool = &maMemoryBanks[type];
    uint32_t bytes;

    if (pool->pMemPool != NULL) {
        memory_fatal();
    }
    bytes = size << 10;
    pool->memSize = bytes;
    pool->memFree = bytes;
    pool->pMemPool = memory;
    pool->memUsed = 0;

    return (int)(uintptr_t)maMemoryBanks;
}

/* Reference RVA 0xBEB20, 217 bytes. */
int memory_InitMemorySystem(void)
{
    memory_InitMemoryPool(mem_pool_0, 0x80, 0);
    memory_InitMemoryPool(mem_pool_1, 0x2BF, 1);
    memory_InitMemoryPool(mem_pool_2, 0x600, 2);
    memory_InitMemoryPool(mem_pool_3a, 0x1400, 3);
    meminit();
    clearzerobss();
    return (int)(uintptr_t)mem_heapend;
}

/* Reference RVA 0xBEC00, 39 bytes. */
void *memory_gCalloc(unsigned n, unsigned size)
{
    return memory_gCallocMemoryFunc(
        n,
        size,
        (int)mDefaultMemBank,
        0x116,
        "W:\\SWJediPowerBattles\\Work\\memory.c");
}

/* Reference RVA 0xBEC30, 107 bytes. */
void *memory_gCallocAnyMemory(unsigned n, unsigned size)
{
    int32_t needed = memory_align_eight_signed((int32_t)(n * size));

    if ((uint32_t)needed < maMemoryBanks[2].memFree) {
        void *result = memory_gCallocMemoryFunc(
            n,
            size,
            2,
            0xBF,
            "W:\\SWJediPowerBattles\\Work\\memory.c");
        if (result != NULL) {
            return result;
        }
    }
    memory_fatal();
    return NULL;
}

/* Reference RVA 0xBECA0, 298 bytes. */
void *memory_gCallocMemoryFunc(
    unsigned n,
    unsigned size,
    int type,
    int line,
    char *file)
{
    uint32_t needed = (uint32_t)(n * size);
    MemoryPool *pool;
    uint32_t old_free;
    char *result;

    (void)line;
    (void)file;

    if (type == MEMORY_POOL_ANY) {
        return memory_gCallocAnyMemory(n, size);
    }

    needed = memory_align_eight(needed);
    pool = &maMemoryBanks[type];
    old_free = pool->memFree;
    if (needed >= old_free) {
        char stemp[255];

        memset(stemp, 0, sizeof(stemp));
        sprintf(
            stemp,
            "memory_gCallocMemory: pool %d too small for %d bytes "
            "(only %d bytes left)",
            type,
            (int)needed,
            (int)old_free);
        memory_fatal();
    }

    pool->memUsed += needed;
    pool->memFree = old_free - needed;
    result = pool->pMemPool + (pool->memSize - old_free);
    return result;
}

/* Reference RVA 0xBEDD0, 98 bytes. */
void *memory_gDrainMemoryPool(unsigned *rsize, int type)
{
    MemoryPool *pool = &maMemoryBanks[type];
    uint32_t old_free = pool->memFree;
    int32_t size = memory_align_eight_signed((int32_t)old_free);
    char *result;

    if (rsize != NULL) {
        *rsize = (uint32_t)size;
    }
    pool->memUsed += (uint32_t)size;
    pool->memFree = old_free - (uint32_t)size;
    result = pool->pMemPool + (pool->memSize - old_free);
    return result;
}

/* Reference RVA 0xBEE40, 75 bytes. */
uint32_t memory_gMemUseage(void)
{
    return maMemoryBanks[0].memFree
        + maMemoryBanks[1].memFree
        + maMemoryBanks[2].memFree
        + maMemoryBanks[3].memFree;
}

/* Reference RVA 0xBEE90, 7 bytes. */
void memory_gSetDefaultMemoryType(int type)
{
    mDefaultMemBank = (uint32_t)type;
}

/* Reference RVA 0xBEEA0, 91 bytes. */
int memory_gTestMemoryPool(int type)
{
    MemoryPool *pool = &maMemoryBanks[type];
    int garbage = 0;
    int start = (int)(pool->memSize - pool->memFree);
    int x = (int)pool->memSize;

    while (start < x) {
        if (pool->pMemPool[start] != 0) {
            ++garbage;
        }
        ++start;
    }
    return garbage;
}
