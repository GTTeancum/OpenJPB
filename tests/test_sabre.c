#include "jpb/linkstubs.h"
#include "jpb/memory.h"
#include "jpb/prim.h"
#include "jpb/sabre.h"
#include "jpb/sprite.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#include <windows.h>
#endif

#define CHECK(condition)                                                     \
    do {                                                                     \
        if (!(condition)) {                                                  \
            fprintf(                                                         \
                stderr,                                                      \
                "CHECK failed at %s:%d: %s\n",                               \
                __FILE__,                                                    \
                __LINE__,                                                    \
                #condition);                                                 \
            return 1;                                                        \
        }                                                                    \
    } while (0)

static int test_spline_functions(void)
{
    int32_t catmull[4] = {100, 200, 400, 800};
    int hermite[4] = {100, 200, 300, 400};

    CHECK(sabre_gSpline(0, catmull) == 225);
    CHECK(sabre_gSpline(0x1c7, catmull) == 249);
    CHECK(sabre_gSpline(0x800, catmull) == 347);
    CHECK(sabre_gSpline(0x1000, catmull) == 525);

    CHECK(spline_gHermiteInterpolation(0, hermite) == 0);
    CHECK(spline_gHermiteInterpolation(0x400, hermite) == 60);
    CHECK(spline_gHermiteInterpolation(0x800, hermite) == -119);
    CHECK(spline_gHermiteInterpolation(0x1000, hermite) == 87);
    return 0;
}

static int test_catmull_data(void)
{
    static const int16_t expected[8] = {
        249, 275, 303, 331, 363, 396, 435, 477
    };
    Sabre sabre;
    int edge;
    int sub;

    memset(&sabre, 0, sizeof(sabre));
    sabre.aEdges[0].point[0].vx = 100;
    sabre.aEdges[1].point[0].vx = 200;
    sabre.aEdges[2].point[0].vx = 400;
    sabre.aEdges[3].point[0].vx = 800;
    for (edge = 0; edge < 4; ++edge) {
        sabre.aEdges[edge].point[0].vy =
            (int16_t)(sabre.aEdges[edge].point[0].vx + 10);
        sabre.aEdges[edge].point[0].vz =
            (int16_t)(sabre.aEdges[edge].point[0].vx + 20);
    }

    sabre_CatMullData(&sabre, 0, 0);
    CHECK(sabre.aEdges[1].point[0].vx == 225);
    CHECK(sabre.aEdges[2].point[0].vx == 525);
    for (sub = 0; sub < 8; ++sub) {
        CHECK(sabre.aEdges[1].aSubs[sub].point[0].vx == expected[sub]);
        CHECK(sabre.aEdges[1].aSubs[sub].point[0].vy == expected[sub] + 10);
        CHECK(sabre.aEdges[1].aSubs[sub].point[0].vz == expected[sub] + 20);
    }
    return 0;
}

static int test_conform_subedges(void)
{
    subSabreEdge subs[JPB_SABRE_SUBEDGE_COUNT];
    int sub;

    memset(subs, 0, sizeof(subs));
    for (sub = 0; sub < JPB_SABRE_SUBEDGE_COUNT; ++sub) {
        subs[sub].point[1].vx = (int16_t)(sub * 10);
        subs[sub].point[1].vy = (int16_t)(sub * 20);
        subs[sub].point[1].vz = (int16_t)(sub * 30);
    }

    sabre_ConformSubEdgeData(subs, 1);
    CHECK(subs[0].point[1].vx == 10);
    CHECK(subs[0].point[1].vy == 0);
    CHECK(subs[1].point[1].vx == 20);
    CHECK(subs[6].point[1].vx == 70);
    return 0;
}

static int test_hermite_trail(void)
{
    static const int16_t expected[8] = {
        14, 31, 50, 72, 98, 127, 160, 198
    };
    Sabre sabre;
    int sample;

    memset(&sabre, 0, sizeof(sabre));
    sabre.tail = 3;
    sabre.aEdges[0].point[0].vx = 1000;
    sabre.aEdges[1].point[0].vx = 2000;
    sabre.aEdges[1].tanvec[0].vx = 120;
    sabre.aEdges[2].tanvec[0].vx = -80;

    sabre_gSabreHermiteInterpolation(&sabre);
    CHECK(sabre.aEdges[0].flag == 1);
    for (sample = 0; sample < 8; ++sample) {
        CHECK(sabre.aEdges[0].aSubs[sample].point[0].vx == expected[sample]);
    }
    return 0;
}

static int test_create_and_add(void)
{
    Sabre *sabre;
    VECTOR p0 = {10, 20, 30, 0};
    VECTOR p1 = {100, 110, 120, 0};
    VECTOR t0 = {5, 6, 7, 0};
    VECTOR t1 = {8, 9, 10, 0};
    VECTOR duplicate_p0 = {50, 60, 70, 0};
    VECTOR duplicate_p1 = {102, 110, 120, 0};
    VECTOR next_p1 = {200, 210, 220, 0};

    memory_InitMemorySystem();
    memset(maSabreData, 0, sizeof(maSabreData));
    sabre_gInitSabrePool();
    sabre_gCreateSabre(7, 0x1234, 0x5678);
    CHECK(mSabreIndex == 1);
    sabre = &maSabreData[0];
    CHECK(sabre->decay == 7);
    CHECK(sabre->pPrims[0] != NULL);
    CHECK(sabre->pPrims[1] != NULL);
    CHECK(sabre->pPrims[0][0].r0 == 0x80);
    CHECK(sabre->pPrims[0][191].b3 == 0x80);

    CHECK(sabre_AddEdge(0, &p0, &p1, &t0, &t1) == 1);
    CHECK(sabre->tail == 1);
    CHECK(sabre->decay == 16);
    CHECK(sabre->aEdges[0].brightness == 255);
    CHECK(sabre->aEdges[0].point[0].vy == 20);
    CHECK(sabre->aEdges[0].tanvec[1].vz == 10);
    CHECK(sabre->pPrims[0][0].r0 == 0x60);
    CHECK(sabre->pPrims[1][0].b3 == 0x60);

    CHECK(sabre_AddEdge(
              0, &duplicate_p0, &duplicate_p1, &t0, &t1) == 1);
    CHECK(sabre->tail == 1);
    CHECK(sabre->aEdges[0].point[0].vx == 50);
    CHECK(sabre->aEdges[0].point[1].vx == 102);

    CHECK(sabre_AddEdge(0, &duplicate_p0, &next_p1, &t0, &t1) == 2);
    CHECK(sabre->tail == 2);
    sabre->tail = 24;
    CHECK(sabre_AddEdge(0, &p0, &p1, &t0, &t1) == 0);
    CHECK(sabre->decay == 24);
    return 0;
}

static int test_primitive_decay(void)
{
    POLY_G4 primitives[2][3];
    Sabre sabre;
    SramFloorStack *scratch =
        (SramFloorStack *)(void *)getScratchAddr(0);

    memset(&sabre, 0, sizeof(sabre));
    memset(primitives, 0, sizeof(primitives));
    memset(scratch, 0, sizeof(*scratch));
    sabre.pPrims[0] = primitives[0];
    sabre.pPrims[1] = primitives[1];
    sabre.tail = 3;
    sabre.decay = 16;
    sabre.aEdges[0].brightness = 30;
    sabre.aEdges[1].brightness = 100;
    mDrawingSurfaceId = 1;

    prim_gRendSabre(&sabre);
    CHECK(scratch->pCurrentSabrePrim == primitives[1]);
    CHECK(scratch->countX == 2);
    CHECK(sabre.aEdges[0].brightness == 0);
    CHECK(sabre.aEdges[1].brightness == 84);
    CHECK(sabre.head == 1);

    sabre.aEdges[1].brightness = 20;
    sabre.tail = 9;
    prim_gRendSabre(&sabre);
    CHECK(sabre.aEdges[1].brightness == 0);
    CHECK(sabre.head == 0);
    CHECK(sabre.tail == 0);
    return 0;
}

#if defined(_WIN32)
typedef struct RetailCode {
    void *address;
    size_t size;
} RetailCode;

static uint32_t sabre_test_random_state = UINT32_C(0x4a504253);

static uint32_t sabre_test_random(void)
{
    sabre_test_random_state =
        sabre_test_random_state * UINT32_C(1664525) + UINT32_C(1013904223);
    return sabre_test_random_state;
}

static RetailCode load_retail_code(
    const unsigned char *image,
    size_t image_size,
    uint32_t rva,
    size_t function_size)
{
    const IMAGE_DOS_HEADER *dos;
    const IMAGE_NT_HEADERS64 *nt;
    const IMAGE_SECTION_HEADER *section;
    RetailCode code = {0};
    unsigned index;

    if (image_size < sizeof(*dos)) {
        return code;
    }
    dos = (const IMAGE_DOS_HEADER *)(const void *)image;
    if (dos->e_magic != IMAGE_DOS_SIGNATURE || dos->e_lfanew < 0 ||
        (size_t)dos->e_lfanew + sizeof(*nt) > image_size) {
        return code;
    }
    nt = (const IMAGE_NT_HEADERS64 *)(const void *)(image + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE ||
        nt->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
        return code;
    }
    section = IMAGE_FIRST_SECTION(nt);
    for (index = 0; index < nt->FileHeader.NumberOfSections; ++index) {
        uint32_t start = section[index].VirtualAddress;
        uint32_t span = section[index].SizeOfRawData;
        size_t file_offset;

        if (rva < start || rva - start > span ||
            function_size > span - (rva - start)) {
            continue;
        }
        file_offset =
            (size_t)section[index].PointerToRawData + (size_t)(rva - start);
        if (file_offset > image_size || function_size > image_size - file_offset) {
            return code;
        }
        code.address = VirtualAlloc(
            NULL,
            function_size,
            MEM_COMMIT | MEM_RESERVE,
            PAGE_EXECUTE_READWRITE);
        if (code.address == NULL) {
            return code;
        }
        memcpy(code.address, image + file_offset, function_size);
        FlushInstructionCache(GetCurrentProcess(), code.address, function_size);
        code.size = function_size;
        return code;
    }
    return code;
}

static int load_file(
    const char *path, unsigned char **image_out, size_t *size_out)
{
    FILE *file = fopen(path, "rb");
    long length;
    unsigned char *image;

    if (file == NULL || fseek(file, 0, SEEK_END) != 0) {
        if (file != NULL) {
            fclose(file);
        }
        return 0;
    }
    length = ftell(file);
    if (length <= 0 || fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        return 0;
    }
    image = (unsigned char *)malloc((size_t)length);
    if (image == NULL || fread(image, 1, (size_t)length, file) != (size_t)length) {
        free(image);
        fclose(file);
        return 0;
    }
    fclose(file);
    *image_out = image;
    *size_out = (size_t)length;
    return 1;
}

static int test_retail_differential(const char *path)
{
    typedef int (*SplineFunction)(int, int32_t *);
    typedef void (*CatMullFunction)(Sabre *, int, int);
    typedef void (*ConformFunction)(subSabreEdge *, int);
    typedef void (*HermiteTrailFunction)(Sabre *);
    unsigned char *image = NULL;
    size_t image_size = 0;
    RetailCode spline_code;
    RetailCode hermite_function_code;
    RetailCode catmull_code;
    RetailCode conform_code;
    RetailCode hermite_trail_code;
    SplineFunction retail_spline;
    SplineFunction retail_hermite_function;
    CatMullFunction retail_catmull;
    ConformFunction retail_conform;
    HermiteTrailFunction retail_hermite_trail;
    int iteration;
    int result = 1;

    CHECK(load_file(path, &image, &image_size));
    spline_code = load_retail_code(image, image_size, 0xF0A00, 118);
    hermite_function_code =
        load_retail_code(image, image_size, 0xF0A80, 190);
    catmull_code = load_retail_code(image, image_size, 0xEF420, 2443);
    conform_code = load_retail_code(image, image_size, 0xEFDB0, 706);
    hermite_trail_code =
        load_retail_code(image, image_size, 0xF0370, 1669);
    CHECK(spline_code.address != NULL);
    CHECK(hermite_function_code.address != NULL);
    CHECK(catmull_code.address != NULL);
    CHECK(conform_code.address != NULL);
    CHECK(hermite_trail_code.address != NULL);
    retail_spline = (SplineFunction)spline_code.address;
    retail_hermite_function = (SplineFunction)hermite_function_code.address;
    retail_catmull = (CatMullFunction)catmull_code.address;
    retail_conform = (ConformFunction)conform_code.address;
    retail_hermite_trail =
        (HermiteTrailFunction)hermite_trail_code.address;

    for (iteration = 0; iteration < 4096; ++iteration) {
        int32_t values[4];
        int i;
        int value;

        for (value = 0; value < 4; ++value) {
            values[value] = (int32_t)sabre_test_random();
        }
        i = (int)sabre_test_random();
        CHECK(sabre_gSpline(i, values) == retail_spline(i, values));
        CHECK(
            spline_gHermiteInterpolation(i, values) ==
            retail_hermite_function(i, values));
    }

    for (iteration = 0; iteration < 256; ++iteration) {
        Sabre reconstructed;
        Sabre retail;
        subSabreEdge reconstructed_subs[JPB_SABRE_SUBEDGE_COUNT];
        subSabreEdge retail_subs[JPB_SABRE_SUBEDGE_COUNT];
        int edge;
        int point = (int)(sabre_test_random() & 1u);

        memset(&reconstructed, 0, sizeof(reconstructed));
        for (edge = 0; edge < 12; ++edge) {
            int component;

            reconstructed.aEdges[edge].flag =
                (int16_t)(sabre_test_random() % 3u);
            for (component = 0; component < 8; ++component) {
                ((int16_t *)(void *)&reconstructed.aEdges[edge])[component] =
                    (int16_t)sabre_test_random();
            }
        }
        reconstructed.tail =
            (int16_t)(3 + (sabre_test_random() % 9u));
        reconstructed.head = (int16_t)(
            sabre_test_random() % (uint32_t)(reconstructed.tail - 2));
        retail = reconstructed;

        sabre_CatMullData(&reconstructed, 2, point);
        retail_catmull(&retail, 2, point);
        if (memcmp(&reconstructed, &retail, sizeof(retail)) != 0) {
            size_t byte;

            for (byte = 0; byte < sizeof(retail); ++byte) {
                const unsigned char *actual =
                    (const unsigned char *)(const void *)&reconstructed;
                const unsigned char *expected =
                    (const unsigned char *)(const void *)&retail;

                if (actual[byte] != expected[byte]) {
                    fprintf(
                        stderr,
                        "CatMull differential iteration=%d point=%d "
                        "offset=%zu actual=%02x retail=%02x\n",
                        iteration,
                        point,
                        byte,
                        actual[byte],
                        expected[byte]);
                    break;
                }
            }
        }
        CHECK(memcmp(&reconstructed, &retail, sizeof(retail)) == 0);

        reconstructed = retail;
        sabre_gSabreHermiteInterpolation(&reconstructed);
        retail_hermite_trail(&retail);
        if (memcmp(&reconstructed, &retail, sizeof(retail)) != 0) {
            size_t byte;

            for (byte = 0; byte < sizeof(retail); ++byte) {
                const unsigned char *actual =
                    (const unsigned char *)(const void *)&reconstructed;
                const unsigned char *expected =
                    (const unsigned char *)(const void *)&retail;

                if (actual[byte] != expected[byte]) {
                    fprintf(
                        stderr,
                        "Hermite trail differential iteration=%d "
                        "offset=%zu actual=%02x retail=%02x "
                        "tail=%d head=%d flags=%d/%d,%d/%d,%d/%d\n",
                        iteration,
                        byte,
                        actual[byte],
                        expected[byte],
                        reconstructed.tail,
                        reconstructed.head,
                        reconstructed.aEdges[0].flag,
                        retail.aEdges[0].flag,
                        reconstructed.aEdges[1].flag,
                        retail.aEdges[1].flag,
                        reconstructed.aEdges[2].flag,
                        retail.aEdges[2].flag);
                    break;
                }
            }
        }
        CHECK(memcmp(&reconstructed, &retail, sizeof(retail)) == 0);

        for (edge = 0; edge < JPB_SABRE_SUBEDGE_COUNT; ++edge) {
            int component;

            for (component = 0; component < 6; ++component) {
                ((int16_t *)(void *)&reconstructed_subs[edge].point[0])
                    [component] = (int16_t)sabre_test_random();
            }
        }
        memcpy(retail_subs, reconstructed_subs, sizeof(retail_subs));
        sabre_ConformSubEdgeData(reconstructed_subs, point);
        retail_conform(retail_subs, point);
        CHECK(memcmp(reconstructed_subs, retail_subs, sizeof(retail_subs)) == 0);
    }
    result = 0;

    VirtualFree(spline_code.address, 0, MEM_RELEASE);
    VirtualFree(hermite_function_code.address, 0, MEM_RELEASE);
    VirtualFree(catmull_code.address, 0, MEM_RELEASE);
    VirtualFree(conform_code.address, 0, MEM_RELEASE);
    VirtualFree(hermite_trail_code.address, 0, MEM_RELEASE);
    free(image);
    return result;
}
#endif

int main(int argc, char **argv)
{
    CHECK(test_spline_functions() == 0);
    CHECK(test_catmull_data() == 0);
    CHECK(test_conform_subedges() == 0);
    CHECK(test_hermite_trail() == 0);
    CHECK(test_create_and_add() == 0);
    CHECK(test_primitive_decay() == 0);
#if defined(_WIN32)
    if (argc == 3 && strcmp(argv[1], "--retail-exe") == 0) {
        CHECK(test_retail_differential(argv[2]) == 0);
    }
#else
    (void)argc;
    (void)argv;
#endif
    puts("sabre tests passed");
    return 0;
}
