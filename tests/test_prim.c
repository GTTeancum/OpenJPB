#include "jpb/fmath.h"
#include "jpb/game.h"
#include "jpb/jonny.h"
#include "jpb/prim.h"
#include "jpb/scene.h"
#include "jpb/vectors.h"

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
                "CHECK failed at %s:%d: %s\n",                             \
                __FILE__,                                                    \
                __LINE__,                                                    \
                #condition);                                                 \
            return 1;                                                        \
        }                                                                    \
    } while (0)

static uint32_t fnv1a(const void *data, size_t size)
{
    const uint8_t *bytes = (const uint8_t *)data;
    uint32_t hash = UINT32_C(2166136261);
    size_t index;

    for (index = 0; index < size; ++index) {
        hash = (hash ^ bytes[index]) * UINT32_C(16777619);
    }
    return hash;
}

static int test_initialized_data_and_background(void)
{
    const CVECTOR *color;

    CHECK(fnv1a(digitsTim, sizeof(digitsTim)) == UINT32_C(0xc13f6f7c));
    memset(maDrawingSurface, 0x5a, sizeof(maDrawingSurface));
    prim_gSetBkColor(0x123, 0x145, 0x167);
    CHECK(maDrawingSurface[0].draw.r0 == 0x23);
    CHECK(maDrawingSurface[0].draw.g0 == 0x45);
    CHECK(maDrawingSurface[0].draw.b0 == 0x67);
    CHECK(maDrawingSurface[1].draw.r0 == 0x23);
    CHECK(maDrawingSurface[1].draw.g0 == 0x45);
    CHECK(maDrawingSurface[1].draw.b0 == 0x67);
    CHECK(maDrawingSurface[0].draw.isbg == 0x5a);
    CHECK(maDrawingSurface[0].draw.dr_env.tag == UINT32_C(0x5a5a5a5a));
    color = jpb_PrimGetBackgroundColor(1);
    CHECK(color != NULL);
    CHECK(color->r == 0x23 && color->g == 0x45 && color->b == 0x67);
    CHECK(jpb_PrimGetBackgroundColor(-1) == NULL);
    CHECK(jpb_PrimGetBackgroundColor(2) == NULL);
    return 0;
}

static int test_blur_quads(void)
{
    POLY_FT4 *p;

    memset(blurquad, 0x5a, sizeof(blurquad));
    AddBlur(1);
    p = blurquad[1];
    CHECK(p[0].tag == UINT32_C(0x5a5a5a5a));
    CHECK(p[0].r0 == 0x80 && p[0].g0 == 0x80 && p[0].b0 == 0xc0);
    CHECK(p[0].code == 0x5a);
    CHECK(p[0].x0 == 0 && p[0].x1 == 160);
    CHECK(p[0].x2 == 0 && p[0].x3 == 160);
    CHECK(p[0].y0 == 1 && p[0].y2 == 128);
    CHECK(p[0].u0 == 0 && p[0].u1 == 160);
    CHECK(p[0].v0 == 1 && p[0].v2 == 128);
    CHECK(p[0].tpage == 128);
    CHECK(p[0].clut == UINT16_C(0x5a5a));

    CHECK(p[1].x0 == 160 && p[1].x1 == 320);
    CHECK(p[1].u0 == 32 && p[1].u1 == 192);
    CHECK(p[1].tpage == 128);
    CHECK(p[2].y0 == 128 && p[2].y2 == 255);
    CHECK(p[2].v0 == 128 && p[2].v2 == 255);
    CHECK(p[2].tpage == 255);
    CHECK(p[3].x0 == 160 && p[3].u0 == 32 && p[3].u1 == 192);
    CHECK(p[3].tpage == 255);
    CHECK(blurquad[0][0].tag == UINT32_C(0x5a5a5a5a));
    return 0;
}

static int test_quick_surface_ownership(void)
{
    mDrawingSurfaceId = 1;
    mpCurrentDrawingSurface = NULL;
    maCurrentOT = NULL;
    QuickStart();
    CHECK(mpCurrentDrawingSurface == &maDrawingSurface[1]);
    CHECK(maCurrentOT == maDrawingSurface[1].aOT);
    QuickEnd();
    CHECK(mDrawingSurfaceId == 0);
    QuickEnd();
    CHECK(mDrawingSurfaceId == 1);
    return 0;
}

static int test_score_primitives(void)
{
    uint8_t storage[1024];
    VECTOR position = {0, 0, 10, 0};
    POLY_FT4 *p = (POLY_FT4 *)(void *)storage;
    uint32_t ot = UINT32_C(0xab123456);
    uint32_t expected_last_pointer;

    memset(storage, 0xcc, sizeof(storage));
    vec_IdentMatrix(&CameraMatrix);
    OptionStruct.ScreenHeight = 480;
    scorex = 10;
    scorey = 20;
    scoreclut = UINT16_C(0x3344);
    scoretpageplus = UINT16_C(0x5566);
    scoretpageminus = UINT16_C(0x7788);
    primptr = storage;
    primlimit = storage + sizeof(storage);
    maCurrentOT = &ot;

    plotscorenumber(&position, 42, UINT32_C(0x112233), 0x1000);
    CHECK(primptr == storage + 6 * sizeof(POLY_FT4));
    CHECK(p[0].tag == UINT32_C(0x09123456));
    CHECK(p[0].r0 == 0x33 && p[0].g0 == 0x22 && p[0].b0 == 0x11);
    CHECK(p[0].code == 0x2e);
    CHECK(p[0].x0 == 320 && p[0].y0 == 240);
    CHECK(p[0].x1 == 326 && p[0].y2 == 245);
    CHECK(p[0].u0 == 10 && p[0].u1 == 16);
    CHECK(p[0].v0 == 20 && p[0].v2 == 25);
    CHECK(p[0].clut == UINT16_C(0x3344));
    CHECK(p[0].tpage == UINT16_C(0x5566));
    CHECK(p[0].pad1 == 0 && p[0].pad2 == 0);

    CHECK(p[1].r0 == 0xff && p[1].g0 == 0xff && p[1].b0 == 0xff);
    CHECK(p[1].tpage == UINT16_C(0x7788));
    CHECK(p[1].tag ==
          (UINT32_C(0x09000000) |
           ((uint32_t)(uintptr_t)&p[0] & UINT32_C(0xffffff))));
    CHECK(p[2].x0 == 328 && p[2].u0 == 46);
    CHECK(p[4].x0 == 336 && p[4].u0 == 34);
    expected_last_pointer =
        (uint32_t)(uintptr_t)&p[5] & UINT32_C(0xffffff);
    CHECK(ot == (UINT32_C(0xab000000) | expected_last_pointer));
    return 0;
}

static int test_score_gates_and_noops(void)
{
    uint8_t storage[400];
    VECTOR position = {0, 0, 10, 0};
    uint32_t ot = UINT32_C(0x11111111);
    DR_MODE modes[2];
    uint32_t words[2] = {UINT32_C(0x12345678), UINT32_C(0xabcdef01)};

    memset(storage, 0x5a, sizeof(storage));
    memset(modes, 0x6b, sizeof(modes));
    memcpy(tw_mode, modes, sizeof(modes));
    vec_IdentMatrix(&CameraMatrix);
    OptionStruct.ScreenHeight = 480;
    primptr = storage;
    primlimit = storage + 319;
    maCurrentOT = &ot;
    plotscorenumber(&position, 42, UINT32_C(0xffffff), 0x1000);
    CHECK(primptr == storage);
    CHECK(ot == UINT32_C(0x11111111));
    CHECK(storage[0] == 0x5a);

    primlimit = storage + sizeof(storage);
    plotscorenumber(&position, 0, UINT32_C(0xffffff), 0x1000);
    CHECK(primptr == storage);
    prim_SetTextureWindow((JPB_RECT *)(void *)words);
    prim_SetTranslucency(words, words + 1, 7);
    prim_GpuCallback();
    CHECK(memcmp(tw_mode, modes, sizeof(modes)) == 0);
    CHECK(words[0] == UINT32_C(0x12345678));
    CHECK(words[1] == UINT32_C(0xabcdef01));
    return 0;
}

#if defined(_WIN32)
typedef struct RetailImage {
    uint8_t *base;
    size_t size;
} RetailImage;

static uint32_t prim_random_state = UINT32_C(0x5042494d);

static uint32_t prim_random(void)
{
    prim_random_state =
        prim_random_state * UINT32_C(1664525) + UINT32_C(1013904223);
    return prim_random_state;
}

static int load_file(
    const char *path, uint8_t **image_out, size_t *size_out)
{
    FILE *file = fopen(path, "rb");
    long length;
    uint8_t *image;

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
    image = (uint8_t *)malloc((size_t)length);
    if (image == NULL ||
        fread(image, 1, (size_t)length, file) != (size_t)length) {
        free(image);
        fclose(file);
        return 0;
    }
    fclose(file);
    *image_out = image;
    *size_out = (size_t)length;
    return 1;
}

static RetailImage map_retail_image(const char *path)
{
    uint8_t *file_image = NULL;
    size_t file_size = 0;
    const IMAGE_DOS_HEADER *dos;
    const IMAGE_NT_HEADERS64 *nt;
    const IMAGE_SECTION_HEADER *section;
    RetailImage mapped = {0};
    unsigned index;

    if (!load_file(path, &file_image, &file_size) ||
        file_size < sizeof(*dos)) {
        free(file_image);
        return mapped;
    }
    dos = (const IMAGE_DOS_HEADER *)(const void *)file_image;
    if (dos->e_magic != IMAGE_DOS_SIGNATURE || dos->e_lfanew < 0 ||
        (size_t)dos->e_lfanew + sizeof(*nt) > file_size) {
        free(file_image);
        return mapped;
    }
    nt = (const IMAGE_NT_HEADERS64 *)(const void *)(
        file_image + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE ||
        nt->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
        free(file_image);
        return mapped;
    }
    mapped.size = nt->OptionalHeader.SizeOfImage;
    mapped.base = (uint8_t *)VirtualAlloc(
        NULL,
        mapped.size,
        MEM_COMMIT | MEM_RESERVE,
        PAGE_EXECUTE_READWRITE);
    if (mapped.base == NULL) {
        mapped.size = 0;
        free(file_image);
        return mapped;
    }
    memset(mapped.base, 0, mapped.size);
    memcpy(
        mapped.base,
        file_image,
        nt->OptionalHeader.SizeOfHeaders < file_size
            ? nt->OptionalHeader.SizeOfHeaders
            : file_size);
    section = IMAGE_FIRST_SECTION(nt);
    for (index = 0; index < nt->FileHeader.NumberOfSections; ++index) {
        size_t source = section[index].PointerToRawData;
        size_t destination = section[index].VirtualAddress;
        size_t length = section[index].SizeOfRawData;

        if (source <= file_size && length <= file_size - source &&
            destination <= mapped.size && length <= mapped.size - destination) {
            memcpy(mapped.base + destination, file_image + source, length);
        }
    }
    FlushInstructionCache(GetCurrentProcess(), mapped.base, mapped.size);
    free(file_image);
    return mapped;
}

static void set_mapped_pointer(uint8_t *base, size_t rva, void *value)
{
    memcpy(base + rva, &value, sizeof(value));
}

static void *get_mapped_pointer(uint8_t *base, size_t rva)
{
    void *value;

    memcpy(&value, base + rva, sizeof(value));
    return value;
}

static int compare_score_output(
    const uint8_t *retail_storage,
    const uint8_t *reconstructed_storage,
    size_t primitive_count,
    uint32_t initial_ot,
    uint32_t retail_ot,
    uint32_t reconstructed_ot)
{
    size_t index;

    for (index = 0; index < primitive_count; ++index) {
        const POLY_FT4 *retail =
            (const POLY_FT4 *)(const void *)retail_storage + index;
        const POLY_FT4 *reconstructed =
            (const POLY_FT4 *)(const void *)reconstructed_storage + index;
        uint32_t expected_retail_tag = index == 0
            ? (initial_ot & UINT32_C(0xffffff))
            : ((uint32_t)(uintptr_t)(retail - 1) & UINT32_C(0xffffff));
        uint32_t expected_reconstructed_tag = index == 0
            ? (initial_ot & UINT32_C(0xffffff))
            : ((uint32_t)(uintptr_t)(reconstructed - 1) &
               UINT32_C(0xffffff));

        CHECK(memcmp(
                  (const uint8_t *)retail + sizeof(retail->tag),
                  (const uint8_t *)reconstructed +
                      sizeof(reconstructed->tag),
                  sizeof(*retail) - sizeof(retail->tag)) == 0);
        CHECK(retail->tag ==
              (UINT32_C(0x09000000) ^ expected_retail_tag));
        CHECK(reconstructed->tag ==
              (UINT32_C(0x09000000) ^ expected_reconstructed_tag));
    }
    if (primitive_count != 0) {
        const POLY_FT4 *retail_last =
            (const POLY_FT4 *)(const void *)retail_storage +
            primitive_count - 1;
        const POLY_FT4 *reconstructed_last =
            (const POLY_FT4 *)(const void *)reconstructed_storage +
            primitive_count - 1;

        CHECK(retail_ot ==
              ((initial_ot & UINT32_C(0xff000000)) |
               ((uint32_t)(uintptr_t)retail_last & UINT32_C(0xffffff))));
        CHECK(reconstructed_ot ==
              ((initial_ot & UINT32_C(0xff000000)) |
               ((uint32_t)(uintptr_t)reconstructed_last &
                UINT32_C(0xffffff))));
    }
    return 0;
}

static int test_retail_differential(const char *path)
{
    typedef void (*VoidFunction)(void);
    typedef void (*IntFunction)(int);
    typedef void (*ScoreFunction)(VECTOR *, int, uint32_t, int);
    typedef void (*ColorFunction)(int, int, int);
    RetailImage image = map_retail_image(path);
    IntFunction retail_add_blur;
    VoidFunction retail_quick_end;
    VoidFunction retail_quick_start;
    ScoreFunction retail_plot_score;
    ColorFunction retail_background;
    uint8_t *retail_blur;
    uint8_t *retail_surfaces;
    int iteration;

    CHECK(image.base != NULL);
    retail_add_blur = (IntFunction)(void *)(image.base + 0xE8B80);
    retail_quick_end = (VoidFunction)(void *)(image.base + 0xE8CE0);
    retail_quick_start = (VoidFunction)(void *)(image.base + 0xE8D50);
    retail_plot_score = (ScoreFunction)(void *)(image.base + 0xE8E70);
    retail_background = (ColorFunction)(void *)(image.base + 0xE9360);
    retail_blur = image.base + 0x94F500;
    retail_surfaces = image.base + 0x94F660;

    CHECK(memcmp(image.base + 0x4CBAF0, digitsTim, sizeof(digitsTim)) == 0);
    for (iteration = 0; iteration < 256; ++iteration) {
        uint8_t before[sizeof(blurquad)];
        size_t byte;
        int draw_id = (int)(prim_random() & 1u);

        for (byte = 0; byte < sizeof(before); ++byte) {
            before[byte] = (uint8_t)prim_random();
        }
        memcpy(retail_blur, before, sizeof(before));
        memcpy(blurquad, before, sizeof(before));
        retail_add_blur(draw_id);
        AddBlur(draw_id);
        CHECK(memcmp(retail_blur, blurquad, sizeof(blurquad)) == 0);
    }

    for (iteration = 0; iteration < 64; ++iteration) {
        uint8_t before[sizeof(maDrawingSurface)];
        size_t byte;
        int red = (int)prim_random();
        int green = (int)prim_random();
        int blue = (int)prim_random();

        for (byte = 0; byte < sizeof(before); ++byte) {
            before[byte] = (uint8_t)prim_random();
        }
        memcpy(retail_surfaces, before, sizeof(before));
        memcpy(maDrawingSurface, before, sizeof(before));
        retail_background(red, green, blue);
        prim_gSetBkColor(red, green, blue);
        CHECK(memcmp(
                  retail_surfaces,
                  maDrawingSurface,
                  sizeof(maDrawingSurface)) == 0);
    }

    *(int *)(void *)(image.base + 0x53D270) = 1;
    mDrawingSurfaceId = 1;
    retail_quick_start();
    QuickStart();
    CHECK(get_mapped_pointer(image.base, 0x94F658) ==
          image.base + 0x94F660 + sizeof(primDrawingSurface));
    CHECK(get_mapped_pointer(image.base, 0x10D8EC0) ==
          image.base + 0x94F660 + sizeof(primDrawingSurface) +
              offsetof(primDrawingSurface, aOT));
    CHECK(mpCurrentDrawingSurface == &maDrawingSurface[1]);
    CHECK(maCurrentOT == maDrawingSurface[1].aOT);
    retail_quick_end();
    QuickEnd();
    CHECK(*(int *)(void *)(image.base + 0x53D270) == mDrawingSurfaceId);

    {
        uint8_t retail_storage[1024];
        uint8_t reconstructed_storage[1024];
        VECTOR position = {0, 0, 10, 0};
        uint32_t retail_ot = UINT32_C(0xab123456);
        uint32_t reconstructed_ot = UINT32_C(0xab123456);
        uint16_t value16;
        uint32_t height = 480;
        void *pointer;
        size_t retail_count;
        size_t reconstructed_count;

        memset(retail_storage, 0xcc, sizeof(retail_storage));
        memset(reconstructed_storage, 0xcc, sizeof(reconstructed_storage));
        vec_IdentMatrix(&CameraMatrix);
        memcpy(image.base + 0x515C08, &CameraMatrix, sizeof(CameraMatrix));
        memcpy(image.base + 0x10DA128, &height, sizeof(height));
        OptionStruct.ScreenHeight = height;
        value16 = 10;
        memcpy(image.base + 0x53D318, &value16, sizeof(value16));
        scorex = value16;
        value16 = 20;
        memcpy(image.base + 0x53D31C, &value16, sizeof(value16));
        scorey = value16;
        value16 = UINT16_C(0x3344);
        memcpy(image.base + 0x53D314, &value16, sizeof(value16));
        scoreclut = value16;
        value16 = UINT16_C(0x5566);
        memcpy(image.base + 0x53D30C, &value16, sizeof(value16));
        scoretpageplus = value16;
        value16 = UINT16_C(0x7788);
        memcpy(image.base + 0x53D310, &value16, sizeof(value16));
        scoretpageminus = value16;
        set_mapped_pointer(image.base, 0x53D280, retail_storage);
        pointer = retail_storage + sizeof(retail_storage);
        set_mapped_pointer(image.base, 0x53D290, pointer);
        set_mapped_pointer(image.base, 0x10D8EC0, &retail_ot);
        primptr = reconstructed_storage;
        primlimit = reconstructed_storage + sizeof(reconstructed_storage);
        maCurrentOT = &reconstructed_ot;

        retail_plot_score(
            &position, -907, UINT32_C(0x123456), 0x7ab);
        plotscorenumber(
            &position, -907, UINT32_C(0x123456), 0x7ab);
        retail_count =
            ((uint8_t *)get_mapped_pointer(image.base, 0x53D280) -
             retail_storage) /
            sizeof(POLY_FT4);
        reconstructed_count =
            (primptr - reconstructed_storage) / sizeof(POLY_FT4);
        CHECK(retail_count == reconstructed_count);
        CHECK(retail_count == 8);
        CHECK(compare_score_output(
                  retail_storage,
                  reconstructed_storage,
                  retail_count,
                  UINT32_C(0xab123456),
                  retail_ot,
                  reconstructed_ot) == 0);
        CHECK(memcmp(image.base + 0x538390, gaScratch, 4) == 0);
    }

    VirtualFree(image.base, 0, MEM_RELEASE);
    return 0;
}
#endif

int main(int argc, char **argv)
{
    CHECK(test_initialized_data_and_background() == 0);
    CHECK(test_blur_quads() == 0);
    CHECK(test_quick_surface_ownership() == 0);
    CHECK(test_score_primitives() == 0);
    CHECK(test_score_gates_and_noops() == 0);
#if defined(_WIN32)
    if (argc == 3 && strcmp(argv[1], "--retail-exe") == 0) {
        CHECK(test_retail_differential(argv[2]) == 0);
    }
#else
    (void)argc;
    (void)argv;
#endif
    puts("prim tests passed");
    return 0;
}
