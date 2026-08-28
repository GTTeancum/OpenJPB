#include "jpb/list.h"
#include "jpb/tpanim.h"

#include <stdint.h>
#include <stdio.h>

#define CHECK(condition)                                                     \
    do {                                                                     \
        if (!(condition)) {                                                  \
            fprintf(stderr, "check failed at %s:%d: %s\n",                \
                    __FILE__, __LINE__, #condition);                         \
            return 1;                                                        \
        }                                                                    \
    } while (0)

int main(void)
{
    Node paletteNode;
    int i;

    tpanim_ResetTextureUVIndex();
    for (i = 0; i < 128; ++i) {
        tpanim_RegisterTextureUV(1000 + i, -2000 - i);
    }
    CHECK(UVCoreIndex[0] == 1000);
    CHECK(UVCoreIndex[1] == -2000);
    CHECK(UVCoreIndex[254] == 1127);
    CHECK(UVCoreIndex[255] == -2127);

    tpanim_RegisterTextureUV(7, 8);
    CHECK(UVCoreIndex[254] == 1127);
    CHECK(UVCoreIndex[255] == -2127);

    tpanim_ResetTextureUVIndex();
    CHECK(UVCoreIndex[0] == 1000);
    tpanim_RegisterTextureUV(7, 8);
    CHECK(UVCoreIndex[0] == 7);
    CHECK(UVCoreIndex[1] == 8);
    CHECK(UVCoreIndex[2] == 1001);

    paletteNode.next = (Node *)(uintptr_t)1;
    tpanim_gAnimatePalette((PalAnim *)&paletteNode);
    CHECK(paletteNode.next == NULL);
    tpanim_AnimateSCBTexture(NULL, NULL);

    puts("texture/palette animation tests passed");
    return 0;
}
