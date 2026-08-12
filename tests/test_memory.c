#include "jpb/memory.h"

#include <stdio.h>
#include <string.h>

#define CHECK(condition)                                                     \
    do {                                                                     \
        if (!(condition)) {                                                  \
            fprintf(stderr, "check failed at %s:%d: %s\n",                  \
                    __FILE__, __LINE__, #condition);                         \
            return 1;                                                        \
        }                                                                    \
    } while (0)

static void reset_descriptors(void)
{
    memset(maMemoryBanks, 0, sizeof(maMemoryBanks));
    mDefaultMemBank = 0;
}

static int test_custom_pool_and_flush(void)
{
    char storage[1024] = {0};
    void *allocation;

    reset_descriptors();
    CHECK(memory_InitMemoryPool(storage, 1, 0) == 0);
    CHECK(maMemoryBanks[0].pMemPool == storage);
    CHECK(maMemoryBanks[0].memSize == 1024);
    CHECK(maMemoryBanks[0].memFree == 1024);
    CHECK(maMemoryBanks[0].memUsed == 0);

    allocation = memory_gCallocMemoryFunc(3, 3, 0, 1, "test");
    CHECK(allocation == storage);
    CHECK(maMemoryBanks[0].memUsed == 16);
    CHECK(maMemoryBanks[0].memFree == 1008);
    ((char *)allocation)[0] = 0x5a;

    CHECK(memory_FlushMemoryPool(0) == 1024);
    CHECK(maMemoryBanks[0].memUsed == 0);
    CHECK(maMemoryBanks[0].memFree == 1024);
    allocation = memory_gCallocMemoryFunc(1, 1, 0, 1, "test");
    CHECK(allocation == storage);
    CHECK(((char *)allocation)[0] == 0x5a);
    return 0;
}

static int test_default_system_and_any_pool(void)
{
    static const uint32_t sizes[MEMORY_POOL_COUNT] = {
        0x20000, 0xAFC00, 0x180000, 0x500000
    };
    uint32_t initial_free = 0;
    void *first;
    void *second;
    void *any;
    int index;

    reset_descriptors();
    CHECK(memory_InitMemorySystem() == 0);
    for (index = 0; index < MEMORY_POOL_COUNT; ++index) {
        CHECK(maMemoryBanks[index].pMemPool != NULL);
        CHECK(maMemoryBanks[index].memSize == sizes[index]);
        CHECK(maMemoryBanks[index].memFree == sizes[index]);
        CHECK(maMemoryBanks[index].memUsed == 0);
        initial_free += sizes[index];
    }
    CHECK(memory_gMemUseage() == initial_free);

    first = memory_gCalloc(1, 1);
    second = memory_gCalloc(2, 5);
    CHECK(first == maMemoryBanks[0].pMemPool);
    CHECK(second == maMemoryBanks[0].pMemPool + 8);
    CHECK(maMemoryBanks[0].memUsed == 24);

    memory_gSetDefaultMemoryType(1);
    CHECK(mDefaultMemBank == 1);
    CHECK(memory_gCalloc(1, 8) == maMemoryBanks[1].pMemPool);

    any = memory_gCallocAnyMemory(1, 9);
    CHECK(any == maMemoryBanks[2].pMemPool);
    CHECK(maMemoryBanks[2].memUsed == 16);
    return 0;
}

static int test_drain_and_free_region_scan(void)
{
    unsigned drained = 0;
    char *start;

    memory_FlushMemoryPool(3);
    CHECK(memory_gTestMemoryPool(3) == 0);
    maMemoryBanks[3].pMemPool[17] = 1;
    CHECK(memory_gTestMemoryPool(3) == 1);
    maMemoryBanks[3].pMemPool[17] = 0;

    start = memory_gCallocMemoryFunc(1, 8, 3, 1, "test");
    CHECK(start == maMemoryBanks[3].pMemPool);
    start = memory_gDrainMemoryPool(&drained, 3);
    CHECK(start == maMemoryBanks[3].pMemPool + 8);
    CHECK(drained == 0x500000 - 8);
    CHECK(maMemoryBanks[3].memFree == 0);
    CHECK(maMemoryBanks[3].memUsed == 0x500000);
    return 0;
}

int main(void)
{
    int result = 0;

    result |= test_custom_pool_and_flush();
    result |= test_default_system_and_any_pool();
    result |= test_drain_and_free_region_scan();
    return result;
}
