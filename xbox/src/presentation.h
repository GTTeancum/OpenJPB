#ifndef JPB_XBOX_PRESENTATION_H
#define JPB_XBOX_PRESENTATION_H

#include <hal/video.h>
#include <stdint.h>

typedef struct JPBXboxPresentation {
    int width, height, top;
} JPBXboxPresentation;

/* Keep the game's 16:9 composition on a standard television. The encoder
 * setting describes display shape; 720x480 storage pixels are anamorphic. */
static inline JPBXboxPresentation jpb_XboxPresentation(VIDEO_MODE mode)
{
    JPBXboxPresentation result = {mode.width, mode.height, 0};
    if (mode.height < 720 &&
        !(XVideoGetEncoderSettings() & VIDEO_WIDESCREEN)) {
        result.height = mode.height * 3 / 4;
        result.top = (mode.height - result.height) / 2;
    }
    return result;
}

static inline void jpb_XboxPresentSoftware(const uint32_t *pixels,
    int width, int height, int stride)
{
    VIDEO_MODE mode=XVideoGetMode();
    JPBXboxPresentation area=jpb_XboxPresentation(mode);
    uint32_t *screen=(uint32_t *)XVideoGetFB();
    for(int y=0;y<mode.height;++y)
        for(int x=0;x<mode.width;++x)
            screen[y*mode.width+x] = y<area.top || y>=area.top+area.height
                ? 0 : pixels[((y-area.top)*height/area.height)*stride+
                    x*width/mode.width];
    XVideoFlushFB();
}

#endif
