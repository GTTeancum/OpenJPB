/*
 * Dependency-free decoder for the TGA variants present in the installed game
 * assets. jpb_ names identify this as a portable extraction boundary rather
 * than an original game symbol.
 */

#include "jpb/tga.h"

#include <limits.h>
#include <string.h>

enum {
    TGA_HEADER_BYTES = 18,
    TGA_TYPE_COLOR_MAPPED = 1,
    TGA_TYPE_TRUE_COLOR = 2,
    TGA_TYPE_GRAYSCALE = 3,
    TGA_TYPE_RLE_TRUE_COLOR = 10
};

static uint16_t tga_read_u16(const uint8_t *bytes)
{
    return (uint16_t)(
        (uint16_t)bytes[0] |
        ((uint16_t)bytes[1] << 8));
}

static size_t tga_bytes_per_pixel(uint8_t bits)
{
    return ((size_t)bits + 7u) / 8u;
}

static int tga_range_fits(
    size_t offset, size_t bytes, size_t file_size)
{
    return offset <= file_size && bytes <= file_size - offset;
}

static int tga_supported(const JPBTgaView *view)
{
    if (view->imageType == TGA_TYPE_COLOR_MAPPED) {
        return view->colorMapLength != 0 &&
               view->pixelBits == 8 &&
               (view->colorMapEntryBits == 16 ||
                view->colorMapEntryBits == 24 ||
                view->colorMapEntryBits == 32);
    }
    if (view->imageType == TGA_TYPE_GRAYSCALE) {
        return view->colorMapLength == 0 &&
               view->pixelBits == 8;
    }
    if (view->imageType == TGA_TYPE_TRUE_COLOR ||
        view->imageType == TGA_TYPE_RLE_TRUE_COLOR) {
        return view->colorMapLength == 0 &&
               (view->pixelBits == 16 ||
                view->pixelBits == 24 ||
                view->pixelBits == 32);
    }
    return 0;
}

static int tga_validate_rle(
    const JPBTgaView *view, size_t pixel_count)
{
    const uint8_t *cursor = view->imageData;
    const uint8_t *end = view->fileData + view->fileSize;
    size_t pixel_bytes = tga_bytes_per_pixel(view->pixelBits);
    size_t decoded = 0;

    while (decoded < pixel_count) {
        size_t count;
        size_t encoded_bytes;
        uint8_t packet;

        if (cursor >= end) {
            return 0;
        }
        packet = *cursor++;
        count = (size_t)(packet & UINT8_C(0x7f)) + 1u;
        if (count > pixel_count - decoded) {
            return 0;
        }
        encoded_bytes =
            (packet & UINT8_C(0x80)) != 0
                ? pixel_bytes
                : count * pixel_bytes;
        if ((size_t)(end - cursor) < encoded_bytes) {
            return 0;
        }
        cursor += encoded_bytes;
        decoded += count;
    }
    return 1;
}

JPBTgaResult jpb_TgaInspect(
    const void *file_data,
    size_t file_size,
    JPBTgaView *view)
{
    const uint8_t *bytes = (const uint8_t *)file_data;
    size_t color_map_bytes;
    size_t image_offset;
    size_t pixel_count;
    size_t pixel_bytes;

    if (file_data == NULL || view == NULL) {
        return JPB_TGA_INVALID_ARGUMENT;
    }
    memset(view, 0, sizeof(*view));
    if (file_size < TGA_HEADER_BYTES) {
        return JPB_TGA_TRUNCATED;
    }
    view->fileData = bytes;
    view->fileSize = file_size;
    view->imageType = bytes[2];
    view->colorMapFirst = tga_read_u16(bytes + 3);
    view->colorMapLength = tga_read_u16(bytes + 5);
    view->colorMapEntryBits = bytes[7];
    view->width = tga_read_u16(bytes + 12);
    view->height = tga_read_u16(bytes + 14);
    view->pixelBits = bytes[16];
    view->descriptor = bytes[17];
    if (view->width == 0 || view->height == 0 ||
        !tga_supported(view)) {
        memset(view, 0, sizeof(*view));
        return JPB_TGA_UNSUPPORTED;
    }
    color_map_bytes =
        (size_t)view->colorMapLength *
        tga_bytes_per_pixel(view->colorMapEntryBits);
    image_offset =
        TGA_HEADER_BYTES + (size_t)bytes[0];
    if (!tga_range_fits(
            image_offset, color_map_bytes, file_size)) {
        memset(view, 0, sizeof(*view));
        return JPB_TGA_TRUNCATED;
    }
    view->colorMap = bytes + image_offset;
    image_offset += color_map_bytes;
    view->imageData = bytes + image_offset;
    pixel_count = (size_t)view->width * (size_t)view->height;
    pixel_bytes = tga_bytes_per_pixel(view->pixelBits);
    if (pixel_bytes == 0 ||
        pixel_count > SIZE_MAX / pixel_bytes) {
        memset(view, 0, sizeof(*view));
        return JPB_TGA_INVALID_LAYOUT;
    }
    if (view->imageType == TGA_TYPE_RLE_TRUE_COLOR) {
        if (!tga_validate_rle(view, pixel_count)) {
            memset(view, 0, sizeof(*view));
            return JPB_TGA_TRUNCATED;
        }
    } else if (!tga_range_fits(
                   image_offset,
                   pixel_count * pixel_bytes,
                   file_size)) {
        memset(view, 0, sizeof(*view));
        return JPB_TGA_TRUNCATED;
    }
    return JPB_TGA_OK;
}

static uint32_t tga_decode_color(
    const uint8_t *source, uint8_t bits, uint8_t alpha_bits)
{
    uint32_t red;
    uint32_t green;
    uint32_t blue;
    uint32_t alpha = UINT32_C(0xff);

    if (bits == 16) {
        uint16_t packed = tga_read_u16(source);

        blue = (uint32_t)(packed & UINT16_C(0x1f));
        green = (uint32_t)((packed >> 5) & UINT16_C(0x1f));
        red = (uint32_t)((packed >> 10) & UINT16_C(0x1f));
        blue = (blue << 3) | (blue >> 2);
        green = (green << 3) | (green >> 2);
        red = (red << 3) | (red >> 2);
        if (alpha_bits != 0) {
            alpha = (packed & UINT16_C(0x8000)) != 0
                        ? UINT32_C(0xff)
                        : 0;
        }
    } else {
        blue = source[0];
        green = source[1];
        red = source[2];
        if (bits == 32) {
            alpha = source[3];
        }
    }
    return (alpha << 24) |
           (red << 16) |
           (green << 8) |
           blue;
}

static int tga_read_pixel(
    const JPBTgaView *view,
    const uint8_t *source,
    uint32_t *pixel)
{
    if (view->imageType == TGA_TYPE_COLOR_MAPPED) {
        uint32_t index = source[0];
        size_t entry_bytes =
            tga_bytes_per_pixel(view->colorMapEntryBits);

        if (index < view->colorMapFirst ||
            index - view->colorMapFirst >=
                view->colorMapLength) {
            return 0;
        }
        *pixel = tga_decode_color(
            view->colorMap +
                (size_t)(index - view->colorMapFirst) *
                    entry_bytes,
            view->colorMapEntryBits,
            view->descriptor & UINT8_C(0x0f));
        return 1;
    }
    if (view->imageType == TGA_TYPE_GRAYSCALE) {
        uint32_t gray = source[0];

        *pixel = UINT32_C(0xff000000) |
                 (gray << 16) |
                 (gray << 8) |
                 gray;
        return 1;
    }
    *pixel = tga_decode_color(
        source,
        view->pixelBits,
        view->descriptor & UINT8_C(0x0f));
    return 1;
}

static void tga_store_pixel(
    const JPBTgaView *view,
    uint32_t *pixels,
    size_t stride_pixels,
    size_t file_index,
    uint32_t pixel)
{
    size_t file_x = file_index % view->width;
    size_t file_y = file_index / view->width;
    size_t output_x =
        (view->descriptor & UINT8_C(0x10)) != 0
            ? (size_t)view->width - 1u - file_x
            : file_x;
    size_t output_y =
        (view->descriptor & UINT8_C(0x20)) != 0
            ? file_y
            : (size_t)view->height - 1u - file_y;

    pixels[output_y * stride_pixels + output_x] = pixel;
}

JPBTgaResult jpb_TgaDecodeA8R8G8B8(
    const JPBTgaView *view,
    uint32_t *pixels,
    size_t pixel_capacity,
    size_t stride_pixels)
{
    const uint8_t *cursor;
    size_t pixel_count;
    size_t pixel_bytes;
    size_t index = 0;

    if (view == NULL || view->fileData == NULL ||
        pixels == NULL) {
        return JPB_TGA_INVALID_ARGUMENT;
    }
    if (stride_pixels < view->width ||
        (size_t)view->height >
            SIZE_MAX / stride_pixels ||
        pixel_capacity <
            stride_pixels * (size_t)view->height) {
        return JPB_TGA_OUTPUT_TOO_SMALL;
    }
    pixel_count = (size_t)view->width * (size_t)view->height;
    pixel_bytes = tga_bytes_per_pixel(view->pixelBits);
    cursor = view->imageData;
    if (view->imageType == TGA_TYPE_RLE_TRUE_COLOR) {
        while (index < pixel_count) {
            uint8_t packet = *cursor++;
            size_t count =
                (size_t)(packet & UINT8_C(0x7f)) + 1u;
            size_t packet_index;

            if ((packet & UINT8_C(0x80)) != 0) {
                uint32_t pixel;

                if (!tga_read_pixel(view, cursor, &pixel)) {
                    return JPB_TGA_INVALID_LAYOUT;
                }
                cursor += pixel_bytes;
                for (packet_index = 0;
                     packet_index < count;
                     ++packet_index) {
                    tga_store_pixel(
                        view,
                        pixels,
                        stride_pixels,
                        index++,
                        pixel);
                }
            } else {
                for (packet_index = 0;
                     packet_index < count;
                     ++packet_index) {
                    uint32_t pixel;

                    if (!tga_read_pixel(
                            view, cursor, &pixel)) {
                        return JPB_TGA_INVALID_LAYOUT;
                    }
                    cursor += pixel_bytes;
                    tga_store_pixel(
                        view,
                        pixels,
                        stride_pixels,
                        index++,
                        pixel);
                }
            }
        }
    } else {
        for (index = 0; index < pixel_count; ++index) {
            uint32_t pixel;

            if (!tga_read_pixel(view, cursor, &pixel)) {
                return JPB_TGA_INVALID_LAYOUT;
            }
            cursor += pixel_bytes;
            tga_store_pixel(
                view,
                pixels,
                stride_pixels,
                index,
                pixel);
        }
    }
    return JPB_TGA_OK;
}
