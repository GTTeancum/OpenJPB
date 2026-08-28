#include "jpb/bucket.h"

#include "jpb/game.h"
#include "jpb/memory.h"
#include "jpb/world.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static int failures;

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "FAIL %s:%d: %s\n", \
            __FILE__, __LINE__, #condition); \
        ++failures; \
    } \
} while (0)

static void write_u32(uint8_t *bytes, uint32_t value)
{
    memcpy(bytes, &value, sizeof(value));
}

static void make_bucket(
    uint8_t *buffer, const char *name, const uint8_t *payload, size_t size)
{
    memset(buffer, 0, 768);
    write_u32(buffer, UINT32_C(0x4B55424A));
    write_u32(buffer + 4, 1);
    write_u32(buffer + 8, (uint32_t)size);
    write_u32(buffer + 12, 512);
    strcpy((char *)buffer + 16, name);
    memcpy(buffer + 512, payload, size);
}

static void test_registry(void)
{
    CHECK(strcmp(bucketCom[0], "?") == 0);
    CHECK(strcmp(bucketCom[12], "debugoff") == 0);
    CHECK(bucketModels[0] == 0);
    CHECK(bucketModels[9] == 0x35);
    CHECK(bucketModels[24] == 0x33);
    CHECK(strcmp(bucketList[0], "@bucket\\main.buk") == 0);
    CHECK(strcmp(bucketList[144], "@bucket\\obi_wan.buk") == 0);
    CHECK(strcmp(bucketList[446], "@bucket\\fed.buk") == 0);
    CHECK(strcmp(bucketList[4345], "@bucket\\front.buk") == 0);
    CHECK(strcmp(bucketList[JPB_BUCKET_LIST_COUNT - 1], "lastfile") == 0);
}

static void test_decompression(void)
{
    uint8_t compressed[] = {
        0x02, 'A', 'B', 'C',
        0x81, 'X',
        0x00, 'Z'
    };
    uint8_t output[8] = {0};

    bucketRLDecompress(compressed, output, sizeof(compressed));
    CHECK(memcmp(output, "ABCXXZ", 6) == 0);

    memset(output, 0, sizeof(output));
    bucketDecompressRLRandom(
        compressed, output, sizeof(compressed), 2, 3);
    CHECK(memcmp(output, "CXX", 3) == 0);

    bucketSubaddress = compressed;
    bucketSubCompressed = 1;
    bucketSubCompressedSize = sizeof(compressed);
    bucketSubUncompressedSize = 6;
    bucketUncomp1 = 0;
    bucketUncomp2 = 0;
    memset(output, 0, sizeof(output));
    bucketCopy(output, 0, 6);
    CHECK(memcmp(output, "ABCXXZ", 6) == 0);
    CHECK(bucketUncomp1 == 1);
    CHECK(bucketUncomp2 == 0);

    memset(output, 0, sizeof(output));
    bucketCopy(output, 1, 4);
    CHECK(memcmp(output, "BCXX", 4) == 0);
    CHECK(bucketUncomp2 == 1);

    bucketSubaddress = (uint8_t *)"012345";
    bucketSubCompressed = 0;
    memset(output, 0, sizeof(output));
    bucketCopy(output, 2, 3);
    CHECK(memcmp(output, "234", 3) == 0);
}

static void test_bucket_directory(void)
{
    uint8_t archive[768] = {0};
    uint8_t compressed[] = {
        'R', 'L', 'C', 'P',
        6, 0, 0, 0,
        0x44, 0x33, 0x22, 0x11,
        0x88, 0x77, 0x66, 0x55,
        0x02, 'A', 'B', 'C', 0x81, 'X'
    };
    unsigned size = 0;
    uint8_t *address = NULL;

    make_bucket(archive, "compressed.bin", compressed, sizeof(compressed));
    CHECK(bucketFindSubname(
        "compressed.bin", archive, &size, &address) == 1);
    CHECK(size == sizeof(compressed));
    CHECK(address == archive + 512 + 16);
    CHECK(bucketSubCompressed == 1);
    CHECK(bucketSubUncompressedSize == 6);
    CHECK(bucketUncompChecksum == UINT32_C(0x11223344));
    CHECK(bucketCompChecksum == UINT32_C(0x55667788));
    CHECK(bucketSubCompressedSize == sizeof(compressed) - 16);
    CHECK(strcmp(bucketSubName, "compressed.bin") == 0);

    CHECK(bucketFindSubname("missing.bin", archive, &size, &address) == 0);

    make_bucket((uint8_t *)mem_pool_3a, "cached.bin",
        (const uint8_t *)"DATA", 4);
    CHECK(bucketLoadBuk(0, "cached.bin") == 1);
    CHECK(bucketSubaddress == (uint8_t *)mem_pool_3a + 512);
    CHECK(bucketSubsize == 4);
    CHECK(bucketSubCompressed == 0);
}

static void test_lookup_and_utilities(void)
{
    unsigned bucket_index = UINT32_MAX;
    char mixed[] = "aBc_19";
    uint8_t checksum[] = {0, 1, 2, 0xFE, 0xFF};

    bucketWrapper = 749;
    CHECK(bucketFindFile("fed", &bucket_index) == 1);
    CHECK(bucket_index == 446);

    updateBucketWrapper((uint8_t *)"front");
    CHECK(bucketWrapper == 4345);

    bucketStringToCaps(mixed);
    CHECK(strcmp(mixed, "ABC_19") == 0);
    CHECK(ramBukChecksum(checksum, sizeof(checksum)) == 512);
    CHECK(checkBucketFP(UINT32_C(0x4B55424A)) == 1);
    CHECK(checkBucketFP(0) == 0);

    bucketLoadFP = (uint8_t *)(uintptr_t)1;
    bucketLoadOffset = 99;
    bucketLoaded = 1;
    strcpy(lastBukLoaded, "set");
    initBucket(1);
    CHECK(bucketLoaded == 1);
    initBucket(0);
    CHECK((uintptr_t)bucketFP == UINT32_MAX);
    CHECK(bucketLoadFP == NULL);
    CHECK(bucketLoadOffset == 0);
    CHECK(bucketLoaded == 0);
    CHECK(lastBukLoaded[0] == '\0');
}

int main(void)
{
    test_registry();
    test_decompression();
    test_bucket_directory();
    test_lookup_and_utilities();

    if (failures != 0) {
        fprintf(stderr, "%d bucket test(s) failed\n", failures);
        return 1;
    }
    puts("Bucket tests passed");
    return 0;
}
