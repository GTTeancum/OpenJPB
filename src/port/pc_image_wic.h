#ifndef JPB_PC_IMAGE_WIC_H
#define JPB_PC_IMAGE_WIC_H

#include <stdint.h>

int jpb_PCInspectImageWIC(
    const char *path,
    int *width,
    int *height);
int jpb_PCLoadImageWIC(
    const char *path,
    int width,
    int height,
    uint32_t *pixels,
    int stride_pixels);

#endif
