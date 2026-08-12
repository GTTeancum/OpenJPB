#include "jpb/game_runtime.h"

#include "jpb/game.h"
#include "jpb/menu.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    JPBGameRuntime runtime;
    JPBSoftwareFramebuffer framebuffer;
    uint32_t pixel = UINT32_C(0x00abcdef);
    int result;

    memset(&runtime, 0, sizeof(runtime));
    memset(&framebuffer, 0, sizeof(framebuffer));
    memset(&menuVars, 0, sizeof(menuVars));

    runtime.textHookReady = 1;
    framebuffer.pixels = &pixel;
    framebuffer.width = 1;
    framebuffer.height = 1;
    framebuffer.stridePixels = 1;

    LevelSelect = 1;
    menuVars.menuModeSP = 0;
    menuVars.menuMode[0] = 0x66;
    result = jpb_GameRuntimeTitleFrame(&runtime, &framebuffer);
    if (result != JPB_GAME_RUNTIME_OK) {
        fprintf(
            stderr,
            "level-load handoff frame returned %d instead of success\n",
            result);
        return 1;
    }
    if (runtime.textDrawCount != 0 || runtime.screenDrawCount != 0) {
        fputs("level-load handoff unexpectedly acquired a draw owner\n", stderr);
        return 1;
    }
    if (pixel != UINT32_C(0x00abcdef)) {
        fprintf(
            stderr,
            "level-load handoff frame mutated framebuffer: %08x\n",
            pixel);
        return 1;
    }

    puts("game runtime title handoff tests passed");
    return 0;
}
