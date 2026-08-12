#ifndef JPB_TGA_H
#define JPB_TGA_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum JPBTgaResult {
    JPB_TGA_OK = 0,
    JPB_TGA_INVALID_ARGUMENT = -1,
    JPB_TGA_TRUNCATED = -2,
    JPB_TGA_UNSUPPORTED = -3,
    JPB_TGA_INVALID_LAYOUT = -4,
    JPB_TGA_OUTPUT_TOO_SMALL = -5
} JPBTgaResult;

/*
 * Descriptive pointer-free view of a TGA asset. The game installation uses
 * uncompressed color-mapped, true-color, grayscale, and RLE true-color files.
 */
typedef struct JPBTgaView {
    const uint8_t *fileData;
    size_t fileSize;
    const uint8_t *colorMap;
    const uint8_t *imageData;
    uint16_t colorMapFirst;
    uint16_t colorMapLength;
    uint16_t width;
    uint16_t height;
    uint8_t imageType;
    uint8_t colorMapEntryBits;
    uint8_t pixelBits;
    uint8_t descriptor;
} JPBTgaView;

JPBTgaResult jpb_TgaInspect(
    const void *file_data,
    size_t file_size,
    JPBTgaView *view);

/*
 * Decodes into caller-owned A8R8G8B8 pixels with a top-left image origin.
 * Keeping allocation and presentation outside this boundary makes it usable
 * by the PC game runtime and a later platform texture upload path.
 */
JPBTgaResult jpb_TgaDecodeA8R8G8B8(
    const JPBTgaView *view,
    uint32_t *pixels,
    size_t pixel_capacity,
    size_t stride_pixels);

#ifdef __cplusplus
}
#endif

#endif
