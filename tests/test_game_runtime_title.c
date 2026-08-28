#include "jpb/game_runtime.h"

#include "jpb/game.h"
#include "jpb/menu.h"
#include "jpb/whook.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static void clear_test_framebuffer(void *user_data)
{
    JPBSoftwareFramebuffer *framebuffer =
        (JPBSoftwareFramebuffer *)user_data;

    framebuffer->pixels[0] = UINT32_C(0xff000000);
}

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
    allText[158] = "LOADING";
    allText[306] = "FEDERATION BATTLESHIP";
    menuVars.menuModeSP = 0;
    menuVars.menuMode[0] = 0x66;
    jpb_WHookSetClearWindowHook(
        clear_test_framebuffer, &framebuffer);
    result = jpb_GameRuntimeTitleFrame(&runtime, &framebuffer);
    jpb_WHookSetClearWindowHook(NULL, NULL);
    if (result != JPB_GAME_RUNTIME_OK) {
        fprintf(
            stderr,
            "level-load handoff frame returned %d instead of success\n",
            result);
        return 1;
    }
    if (runtime.textDrawCount != 0 || runtime.screenDrawCount != 0) {
        fputs("uninstalled draw hooks unexpectedly captured output\n", stderr);
        return 1;
    }
    if (pixel != UINT32_C(0xff000000)) {
        fprintf(
            stderr,
            "level-load handoff did not clear framebuffer: %08x\n",
            pixel);
        return 1;
    }

    puts("game runtime title handoff tests passed");
    return 0;
}
