#include "jpb/tga.h"

#include <stdio.h>
#include <string.h>

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

static void write_u16(uint8_t *bytes, uint16_t value)
{
    bytes[0] = (uint8_t)value;
    bytes[1] = (uint8_t)(value >> 8);
}

static void make_header(
    uint8_t *bytes,
    uint8_t color_map_type,
    uint8_t image_type,
    uint16_t width,
    uint16_t height,
    uint8_t pixel_bits,
    uint8_t descriptor)
{
    memset(bytes, 0, 18);
    bytes[1] = color_map_type;
    bytes[2] = image_type;
    write_u16(bytes + 12, width);
    write_u16(bytes + 14, height);
    bytes[16] = pixel_bits;
    bytes[17] = descriptor;
}

static int test_true_color_orientation(void)
{
    uint8_t file[18 + 4 * 3];
    JPBTgaView view;
    uint32_t pixels[4] = {0};

    make_header(file, 0, 2, 2, 2, 24, 0);
    /* Bottom row, then top row; each source pixel is BGR. */
    file[18] = 0xff;
    file[19] = 0;
    file[20] = 0;
    file[21] = 0;
    file[22] = 0xff;
    file[23] = 0;
    file[24] = 0;
    file[25] = 0;
    file[26] = 0xff;
    file[27] = 0xff;
    file[28] = 0xff;
    file[29] = 0xff;
    CHECK(jpb_TgaInspect(
              file, sizeof(file), &view) == JPB_TGA_OK);
    CHECK(jpb_TgaDecodeA8R8G8B8(
              &view, pixels, 4, 2) == JPB_TGA_OK);
    CHECK(pixels[0] == UINT32_C(0xffff0000));
    CHECK(pixels[1] == UINT32_C(0xffffffff));
    CHECK(pixels[2] == UINT32_C(0xff0000ff));
    CHECK(pixels[3] == UINT32_C(0xff00ff00));
    return 0;
}

static int test_rle_and_16_bit(void)
{
    uint8_t rle[18 + 1 + 4];
    uint8_t sixteen[18 + 2];
    JPBTgaView view;
    uint32_t pixels[3] = {0};

    make_header(rle, 0, 10, 3, 1, 32, 0x20);
    rle[18] = UINT8_C(0x82);
    rle[19] = 0x33;
    rle[20] = 0x22;
    rle[21] = 0x11;
    rle[22] = 0x80;
    CHECK(jpb_TgaInspect(
              rle, sizeof(rle), &view) == JPB_TGA_OK);
    CHECK(jpb_TgaDecodeA8R8G8B8(
              &view, pixels, 3, 3) == JPB_TGA_OK);
    CHECK(pixels[0] == UINT32_C(0x80112233));
    CHECK(pixels[2] == UINT32_C(0x80112233));

    make_header(sixteen, 0, 2, 1, 1, 16, 0x21);
    write_u16(sixteen + 18, UINT16_C(0xfc00));
    CHECK(jpb_TgaInspect(
              sixteen, sizeof(sixteen), &view) == JPB_TGA_OK);
    CHECK(jpb_TgaDecodeA8R8G8B8(
              &view, pixels, 1, 1) == JPB_TGA_OK);
    CHECK(pixels[0] == UINT32_C(0xffff0000));
    return 0;
}

static int test_color_map_and_grayscale(void)
{
    uint8_t mapped[18 + 2 * 3 + 2];
    uint8_t gray[18 + 2];
    JPBTgaView view;
    uint32_t pixels[2] = {0};

    make_header(mapped, 1, 1, 2, 1, 8, 0x20);
    write_u16(mapped + 3, 4);
    write_u16(mapped + 5, 2);
    mapped[7] = 24;
    mapped[18] = 0;
    mapped[19] = 0;
    mapped[20] = 0xff;
    mapped[21] = 0xff;
    mapped[22] = 0;
    mapped[23] = 0;
    mapped[24] = 4;
    mapped[25] = 5;
    CHECK(jpb_TgaInspect(
              mapped, sizeof(mapped), &view) == JPB_TGA_OK);
    CHECK(jpb_TgaDecodeA8R8G8B8(
              &view, pixels, 2, 2) == JPB_TGA_OK);
    CHECK(pixels[0] == UINT32_C(0xffff0000));
    CHECK(pixels[1] == UINT32_C(0xff0000ff));

    make_header(gray, 0, 3, 2, 1, 8, 0x30);
    gray[18] = 0x10;
    gray[19] = 0x80;
    CHECK(jpb_TgaInspect(
              gray, sizeof(gray), &view) == JPB_TGA_OK);
    CHECK(jpb_TgaDecodeA8R8G8B8(
              &view, pixels, 2, 2) == JPB_TGA_OK);
    CHECK(pixels[0] == UINT32_C(0xff808080));
    CHECK(pixels[1] == UINT32_C(0xff101010));
    return 0;
}

int main(void)
{
    CHECK(test_true_color_orientation() == 0);
    CHECK(test_rle_and_16_bit() == 0);
    CHECK(test_color_map_and_grayscale() == 0);
    puts("TGA tests passed");
    return 0;
}
