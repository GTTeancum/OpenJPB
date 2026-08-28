#include "jpb/compress.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#include <windows.h>
#endif

#define CHECK(condition)                                                     \
    do {                                                                     \
        if (!(condition)) {                                                  \
            fprintf(stderr, "check failed at %s:%d: %s\n",                \
                    __FILE__, __LINE__, #condition);                         \
            return 1;                                                        \
        }                                                                    \
    } while (0)

static int check_vector(
    const unsigned char *encoded,
    size_t encoded_size,
    short pdsz,
    int x,
    int y,
    int z,
    unsigned pad)
{
    unsigned char stream[32];
    _svector vectors[2];
    unsigned char *end;

    memset(stream, 0xcc, sizeof(stream));
    memcpy(stream, encoded, encoded_size);
    memset(vectors, 0, sizeof(vectors));
    end = comp_GetDeltaFrame(stream, vectors, 0, 0);
    if (pdsz == 0 && (encoded[0] & 0x80u) != 0) {
        CHECK(end == stream + encoded_size - 1);
    } else {
        CHECK(end == stream + encoded_size);
    }
    CHECK(vectors[0].vx == x);
    CHECK(vectors[0].vy == y);
    CHECK(vectors[0].vz == z);
    if (pdsz != 0) {
        CHECK((uint16_t)vectors[0].pad == pad);
    }
    return 0;
}

static int check_packing_modes(void)
{
    static const unsigned char zero[] = {0x00};
    static const unsigned char x4[] = {0x1f};
    static const unsigned char y4[] = {0x27};
    static const unsigned char z4[] = {0x38};
    static const unsigned char xyz4[] = {0x4e, 0x97};
    static const unsigned char xyz676[] = {0x5c, 0xb3, 0x46};
    static const unsigned char xyz9[] = {0x6f, 0x05, 0xfe, 0x02};
    static const unsigned char xyz12[] = {0x78, 0x01, 0x7f, 0xf8, 0x00};
    static const unsigned char padded[] = {0x80, 0x34, 0x12};
    unsigned char single_pad[] = {
        0, 0, 0, 0, 0, 0, 0, 0,
        0x80, 0x7a
    };
    _svector vector[2];
    unsigned char *end;

    CHECK(check_vector(zero, sizeof(zero), 1, 0, 0, 0, 0x5aa5) == 0);
    CHECK(check_vector(x4, sizeof(x4), 1, -1, 0, 0, 0x5aa5) == 0);
    CHECK(check_vector(y4, sizeof(y4), 1, 0, 7, 0, 0x5aa5) == 0);
    CHECK(check_vector(z4, sizeof(z4), 1, 0, 0, -8, 0x5aa5) == 0);
    CHECK(check_vector(xyz4, sizeof(xyz4), 1, -2, -7, 7, 0x5aa5) == 0);
    CHECK(check_vector(xyz676, sizeof(xyz676), 1, -27, -26, -58, 0x5aa5) == 0);
    CHECK(check_vector(xyz9, sizeof(xyz9), 1, -32, -129, -255, 0x5aa5) == 0);
    CHECK(check_vector(xyz12, sizeof(xyz12), 1, -2047, 2047, -2048, 0x5aa5) == 0);
    CHECK(check_vector(padded, sizeof(padded), 1, 0, 0, 0, 0x1234) == 0);

    end = comp_GetDeltaFrame(single_pad, vector, 1, 1);
    CHECK(end == single_pad + sizeof(single_pad));
    CHECK((uint16_t)vector[1].pad == 0x007a);
    return 0;
}

static int check_delta_frames(void)
{
    unsigned char stream[] = {
        0x34, 0x12, 0x78, 0x56, 0xbc, 0x9a, 0xf0, 0xde,
        0x41, 0x23,
        0x80, 0x44
    };
    _svector vectors[3];
    unsigned char *end;

    memset(vectors, 0, sizeof(vectors));
    end = comp_GetDeltaFrame(stream, vectors, 1, 2);
    CHECK(end == stream + sizeof(stream));
    CHECK((uint16_t)vectors[0].vx == 0x1234);
    CHECK((uint16_t)vectors[0].vy == 0x5678);
    CHECK((uint16_t)vectors[0].vz == 0x9abc);
    CHECK((uint16_t)vectors[0].pad == 0xdef0);
    CHECK(vectors[1].vx == 1 && vectors[1].vy == 2 && vectors[1].vz == 3);
    CHECK((uint16_t)vectors[1].pad == 0x5aa5);
    CHECK(vectors[2].vx == 0 && vectors[2].vy == 0 && vectors[2].vz == 0);
    CHECK((uint16_t)vectors[2].pad == 0x0044);
    return 0;
}

static int check_frame_addition(void)
{
    _svector last[2] = {
        {30000, -30000, 1000, 0x1234},
        {2000, -2000, 2047, 0x4321}
    };
    _svector next[2] = {
        {10000, -10000, -2000, (int16_t)0x5aa5},
        {100, -100, 2, (int16_t)0x5aa5}
    };

    comp_AddFrames(last, next, 1);
    CHECK((uint16_t)next[0].vx == (uint16_t)40000);
    CHECK((uint16_t)next[0].vy == (uint16_t)-40000);
    CHECK(next[0].vz == -1000);
    CHECK((uint16_t)next[0].pad == 0x1234);
    CHECK(next[1].vx == -1996);
    CHECK(next[1].vy == 1996);
    CHECK(next[1].vz == -2047);
    CHECK((uint16_t)next[1].pad == 0x4321);
    return 0;
}

static int check_frame_translation(void)
{
    unsigned char raw[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    _svector vector;
    int result = comp_GetFrameTrans(raw, &vector);

    CHECK((uint16_t)vector.vx == 0x0201);
    CHECK((uint16_t)vector.vy == 0x0403);
    CHECK((uint16_t)vector.vz == 0x0605);
    CHECK((uint16_t)vector.pad == 0x0807);
    CHECK(result == (int)(intptr_t)(raw + 8));
    return 0;
}

#if defined(_WIN32)
typedef struct RetailImage {
    unsigned char *base;
    size_t size;
} RetailImage;

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
    unsigned char *file_image = NULL;
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
    mapped.base = (unsigned char *)VirtualAlloc(
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
    free(file_image);
    return mapped;
}

static uint32_t random_state = UINT32_C(0x434f4d50);

static uint32_t next_random(void)
{
    random_state =
        random_state * UINT32_C(1664525) + UINT32_C(1013904223);
    return random_state;
}

static int check_retail_differential(const char *path)
{
    typedef void (*RetailAddFrames)(_svector *, _svector *, int);
    typedef unsigned char *(*RetailGetDeltaFrame)(
        unsigned char *, _svector *, short, int);
    typedef int (*RetailGetFrameTrans)(unsigned char *, _svector *);
    RetailImage image = map_retail_image(path);
    RetailAddFrames retail_add_frames;
    RetailGetDeltaFrame retail_get_delta_frame;
    RetailGetFrameTrans retail_get_frame_trans;
    unsigned control;
    int iteration;

    CHECK(image.base != NULL);
    CHECK(image.size > 0x4f2ff0u + sizeof(void *));
    retail_add_frames =
        (RetailAddFrames)(void *)(image.base + 0x27af0u);
    retail_get_delta_frame =
        (RetailGetDeltaFrame)(void *)(image.base + 0x27c00u);
    retail_get_frame_trans =
        (RetailGetFrameTrans)(void *)(image.base + 0x27cd0u);

    for (control = 0; control < 256; ++control) {
        unsigned char linked_stream[24];
        unsigned char retail_stream[24];
        _svector linked_vector[2];
        _svector retail_vector[2];
        unsigned char *linked_end;
        unsigned char *retail_end;
        size_t byte;

        for (byte = 0; byte < sizeof(linked_stream); ++byte) {
            linked_stream[byte] = (unsigned char)(control * 37u + byte * 53u);
        }
        linked_stream[0] = (unsigned char)control;
        memcpy(retail_stream, linked_stream, sizeof(linked_stream));
        memset(linked_vector, 0xa5, sizeof(linked_vector));
        memset(retail_vector, 0xa5, sizeof(retail_vector));

        linked_end = comp_GetDeltaFrame(
            linked_stream, linked_vector, 0, 0);
        retail_end = retail_get_delta_frame(
            retail_stream, retail_vector, 0, 0);
        CHECK(memcmp(linked_vector, retail_vector, sizeof(_svector)) == 0);
        CHECK(
            linked_end - linked_stream == retail_end - retail_stream);

        memset(linked_vector, 0xa5, sizeof(linked_vector));
        memset(retail_vector, 0xa5, sizeof(retail_vector));
        memmove(linked_stream + 8, linked_stream, 16);
        memmove(retail_stream + 8, retail_stream, 16);
        memset(linked_stream, 0x3c, 8);
        memset(retail_stream, 0x3c, 8);
        linked_end = comp_GetDeltaFrame(
            linked_stream, linked_vector, 1, 1);
        retail_end = retail_get_delta_frame(
            retail_stream, retail_vector, 1, 1);
        CHECK(memcmp(linked_vector, retail_vector, sizeof(linked_vector)) == 0);
        CHECK(
            linked_end - linked_stream == retail_end - retail_stream);
    }

    for (iteration = 0; iteration < 2048; ++iteration) {
        _svector linked_last[17];
        _svector retail_last[17];
        _svector linked_next[17];
        _svector retail_next[17];
        int count = (int)(next_random() % 17u) - 1;
        size_t byte;

        for (byte = 0; byte < sizeof(linked_last); ++byte) {
            ((unsigned char *)linked_last)[byte] = (unsigned char)next_random();
            ((unsigned char *)linked_next)[byte] = (unsigned char)next_random();
        }
        memcpy(retail_last, linked_last, sizeof(retail_last));
        memcpy(retail_next, linked_next, sizeof(retail_next));
        comp_AddFrames(linked_last, linked_next, count);
        retail_add_frames(retail_last, retail_next, count);
        CHECK(memcmp(linked_next, retail_next, sizeof(linked_next)) == 0);
    }

    for (iteration = 0; iteration < 256; ++iteration) {
        unsigned char raw[8];
        _svector linked;
        _svector retail;
        int linked_result;
        int retail_result;
        int byte;

        for (byte = 0; byte < 8; ++byte) {
            raw[byte] = (unsigned char)next_random();
        }
        linked_result = comp_GetFrameTrans(raw, &linked);
        retail_result = retail_get_frame_trans(raw, &retail);
        CHECK(memcmp(&linked, &retail, sizeof(linked)) == 0);
        CHECK(linked_result == retail_result);
    }

    VirtualFree(image.base, 0, MEM_RELEASE);
    return 0;
}
#endif

int main(int argc, char **argv)
{
    CHECK(check_packing_modes() == 0);
    CHECK(check_delta_frames() == 0);
    CHECK(check_frame_addition() == 0);
    CHECK(check_frame_translation() == 0);
#if defined(_WIN32)
    if (argc == 3 && strcmp(argv[1], "--retail-exe") == 0) {
        CHECK(check_retail_differential(argv[2]) == 0);
    } else {
        CHECK(argc == 1);
    }
#else
    (void)argc;
    (void)argv;
#endif
    puts("compressed animation tests passed");
    return 0;
}
