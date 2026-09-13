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

typedef struct TitleOrderProbe {
    JPBGameRuntime *runtime;
    unsigned calls;
    unsigned textAttempts[2];
    float depths[2];
    size_t counts[2];
    int fail;
} TitleOrderProbe;

static int capture_title_batch(
    void *user_data, const JPBGameRuntimeScreenDraw *draws,
    size_t count, JPBSoftwareFramebuffer *framebuffer)
{
    TitleOrderProbe *probe = (TitleOrderProbe *)user_data;
    unsigned call = probe->calls++;
    (void)framebuffer;
    if (call < 2) {
        probe->textAttempts[call] = probe->runtime->textTrueTypeDrawCount +
            probe->runtime->textFailedDrawCount;
        probe->depths[call] = draws[0].layerDepth;
        probe->counts[call] = count;
    }
    return !probe->fail;
}

static int test_title_text_depth_order(void)
{
    static JPBGameRuntime runtime;
    uint32_t pixels[64 * 64] = {0};
    JPBSoftwareFramebuffer framebuffer = {pixels, 64, 64, 64};
    TitleOrderProbe probe = {0};
    unsigned i;
    memset(&runtime, 0, sizeof(runtime));
    probe.runtime = &runtime;
    runtime.screenDrawCount = 3;
    runtime.screenDraws[0].layerDepth = 0.4f;
    runtime.screenDraws[1].layerDepth = 0.9f;
    runtime.screenDraws[2].layerDepth = 0.0f;
    runtime.textDrawCount = 2;
    for (i = 0; i < 2; ++i) {
        runtime.textDraws[i].text[0] = 'A';
        runtime.textDraws[i].pointSize = 12;
        runtime.textDraws[i].color = UINT32_C(0xffffffff);
        runtime.textDraws[i].depthEnabled = 1;
    }
    runtime.textDraws[0].depth = 0.5f;
    runtime.textDraws[1].depth = 0.0f;
    jpb_GameRuntimeSetTitleScreenDrawRenderHook(&runtime, capture_title_batch, &probe);
    if (jpb_GameRuntimeRenderTitleDraws(&runtime, &framebuffer) != JPB_GAME_RUNTIME_OK ||
        probe.calls != 2 || probe.counts[0] != 1 || probe.counts[1] != 2 ||
        probe.depths[0] != 0.9f || probe.depths[1] != 0.4f ||
        probe.textAttempts[0] != 0 || probe.textAttempts[1] != 1 ||
        runtime.textTrueTypeDrawCount + runtime.textFailedDrawCount != 2) {
        fputs("title panels and text were not interleaved by depth\n", stderr);
        return 1;
    }
    probe.fail = 1;
    probe.calls = 0;
    if (jpb_GameRuntimeRenderTitleDraws(&runtime, &framebuffer) != JPB_GAME_RUNTIME_RENDER_FAILED ||
        probe.calls != 1) {
        fputs("title renderer failure was not propagated\n", stderr);
        return 1;
    }
    return 0;
}

int main(void)
{
    JPBGameRuntime runtime;
    JPBSoftwareFramebuffer framebuffer;
    uint32_t pixel = UINT32_C(0x00abcdef);
    int result;

    if (test_title_text_depth_order() != 0) return 1;

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
    if (pixel != UINT32_C(0x00abcdef)) {
        fprintf(
            stderr,
            "gameplay-owned title handoff modified framebuffer: %08x\n",
            pixel);
        return 1;
    }

    memset(&runtime, 0, sizeof(runtime));
    pixel = UINT32_C(0x00abcdef);
    runtime.textHookReady = 1;
    runtime.screenDrawCount = 1;
    runtime.screenDraws[0].destination.left = 0;
    runtime.screenDraws[0].destination.top = 0;
    runtime.screenDraws[0].destination.right = 1;
    runtime.screenDraws[0].destination.bottom = 1;
    runtime.screenDraws[0].color.r = 0x12;
    runtime.screenDraws[0].color.g = 0x34;
    runtime.screenDraws[0].color.b = 0x56;
    runtime.screenDraws[0].color.cd = 0xff;
    result = jpb_GameRuntimeRenderLoadScreen(&runtime, &framebuffer);
    if (result != JPB_GAME_RUNTIME_OK) {
        fprintf(
            stderr,
            "load-screen render returned %d instead of success\n",
            result);
        return 1;
    }
    if (pixel != UINT32_C(0x00123456)) {
        fprintf(
            stderr,
            "load-screen draw did not composite: %08x\n",
            pixel);
        return 1;
    }
    if (runtime.screenDrawCount != 0 || runtime.textDrawCount != 0 ||
        runtime.drawOrder != 0) {
        fputs("load-screen render did not drain captured draws\n", stderr);
        return 1;
    }

    puts("game runtime title handoff tests passed");
    return 0;
}
