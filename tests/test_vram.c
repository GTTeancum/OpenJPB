#include "jpb/vram.h"

#include <stdio.h>

#define CHECK(condition)                                                     \
    do {                                                                     \
        if (!(condition)) {                                                  \
            fprintf(stderr, "check failed at %s:%d: %s\n",              \
                    __FILE__, __LINE__, #condition);                         \
            return 1;                                                        \
        }                                                                    \
    } while (0)

int main(void)
{
    JPBVramRect rectangle;
    JPBVramPoint point;
    int index;

    for (index = 0; index < 16; ++index) {
        const JPBVramRect *page = vram_GetPageCoord(index, &rectangle);
        CHECK(page == &maTexturePageCoords[index]);
        CHECK(rectangle.x == 512 + (index % 8) * 64);
        CHECK(rectangle.y == index / 8);
        CHECK(rectangle.w == 64 && rectangle.h == 1);

        const JPBVramPoint *sub = vram_GetSubPageCoord(index, &point);
        CHECK(sub == &maTextureSubPageCoords[index]);
        CHECK(point.x == (index % 4) * 64);
        CHECK(point.y == (index / 4) * 64);
    }

    CHECK(vram_GetPageCoord(5, NULL) == &maTexturePageCoords[5]);
    CHECK(vram_GetSubPageCoord(-20, NULL) == &maTextureSubPageCoords[0]);
    CHECK(vram_GetSubPageCoord(99, NULL) == &maTextureSubPageCoords[15]);
    CHECK(gGlobalPaletteScale == 0x80000);

    puts("VRAM coordinate tests passed");
    return 0;
}
