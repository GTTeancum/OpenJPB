#include "jpb/game_runtime.h"
#include "jpb/resources.h"
#include "jpb/input.h"
#include "jpb/loader.h"
#include "jpb/filesys.h"
#include "jpb/sprite.h"
#include "jpb/game.h"
#include <hal/video.h>
#include <hal/debug.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "gpu.h"
#include <pbkit/pbkit.h>

static JPBGameRuntime runtime;
static uint32_t pixels[JPB_XBOX_CANVAS_WIDTH * JPB_XBOX_CANVAS_HEIGHT];
extern int jpb_XboxInspectImage(const char *, int *, int *);
extern int jpb_XboxLoadImage(const char *, int, int, uint32_t *, int);
extern int jpb_XboxControllerInit(void);
extern int jpb_XboxMemorySelfTest(void);
extern int jpb_XboxUiInit(void);
extern int jpb_XboxAudioInit(void);
extern void jpb_XboxAudioPump(void);
extern void jpb_XboxControllerPoll(JPBControllerState *state);
static uint32_t controller_bits;
/* Read-only marker for native emulator-monitor smoke synchronization. */
volatile unsigned jpb_XboxSmokeFrame;
volatile unsigned jpb_XboxStartupPhase;
volatile unsigned jpb_XboxVideoModeResult;
volatile int jpb_XboxGpuInitResult;
volatile int jpb_XboxLastLoadResult;
volatile unsigned jpb_XboxLastLoadFailureCode;
volatile unsigned jpb_XboxFreePagesBeforeConstructor;
volatile unsigned jpb_XboxFreePagesAfterConstructor;
volatile int jpb_XboxConstructorFileNotFound;
volatile unsigned jpb_XboxVisualLevelLoaded;
volatile unsigned jpb_XboxFreePagesAfterVisualLevel;
volatile unsigned jpb_XboxSmokeInputPhase;
volatile unsigned jpb_XboxFrameMs;
volatile unsigned jpb_XboxRuntimeMs;
volatile unsigned jpb_XboxSceneMs;
volatile unsigned jpb_XboxMenuMs;
volatile unsigned jpb_XboxGameStageMs;
volatile unsigned jpb_XboxWorldMs;
volatile unsigned jpb_XboxModelsMs;
volatile unsigned jpb_XboxEffectsMs;
volatile unsigned jpb_XboxHudMs;
volatile unsigned jpb_XboxScreenPolyMs,jpb_XboxHudReplayMs;
volatile unsigned jpb_XboxCompositeUploadMs,jpb_XboxCompositeFinishMs;
volatile unsigned jpb_XboxPresentMs;
/* Last main-loop stage for diagnosing a stalled, still-running guest. */
volatile unsigned jpb_XboxFramePhase;
volatile unsigned jpb_XboxRuntimeStage;
volatile unsigned jpb_XboxRunStageStep;
volatile unsigned jpb_XboxRunStageGameState;
volatile unsigned jpb_XboxResetStep;
volatile unsigned jpb_XboxFreePages;
volatile unsigned jpb_XboxEnemyClasses;
/* Post-frame atomic diagnostic records for the previous 512 frames. */
volatile unsigned jpb_XboxPerfRing[512][39];
extern volatile unsigned jpb_XboxTextureCount;
extern volatile unsigned jpb_XboxModelBatchFlushes;
extern volatile unsigned jpb_XboxModelBatchUs;
extern volatile unsigned jpb_XboxModelTriangleUs;
extern volatile unsigned jpb_XboxGpuStateChanges;
extern volatile unsigned jpb_XboxGpuTriangleCalls;
extern volatile uint32_t jpb_XboxBmdGeometryCacheHits;
extern volatile uint32_t jpb_XboxBmdGeometryCacheMisses;
extern volatile uint32_t jpb_XboxBmdGeometryCalls;
extern volatile uint32_t jpb_XboxBmdLastGeneration;
extern volatile unsigned jpb_XboxSpriteUpdateMs;
extern volatile unsigned jpb_XboxSpriteDrawMs;
extern volatile unsigned jpb_XboxSlowSpriteMs;
extern volatile uintptr_t jpb_XboxSlowSpriteFunction;
extern volatile int jpb_XboxSlowSpriteNum;
/* Emulator-monitor evidence for the process-local controller smoke. */
volatile int32_t jpb_XboxPlayerTelemetry[6];
static int load_visual_level(const char *fbx_path, int level_index, void *user)
{
    char path[512];
    const char *dot=strrchr(fbx_path,'.');
    if(!dot || (size_t)(dot-fbx_path)>sizeof(path)-5)return 0;
    snprintf(path,sizeof(path),"%.*s.xlv",(int)(dot-fbx_path),fbx_path);
    for(char *p=path;*p;++p)if(*p=='/')*p='\\';
    const JPBSoftwareLevelMesh *mesh=jpb_XboxLoadLevel(path);
    jpb_XboxVisualLevelLoaded=mesh!=NULL;
    {
        MM_STATISTICS memory={0};
        memory.Length=sizeof(memory);
        MmQueryStatistics(&memory);
        jpb_XboxFreePagesAfterVisualLevel=memory.AvailablePages;
    }
    if(!mesh || mesh->levelIndex!=level_index)return 0;
    jpb_GameRuntimeSetLevelRenderMesh((JPBGameRuntime *)user,mesh);
    return 1;
}
static uint32_t read_controller(int32_t pad_index, void *unused)
{
    (void)unused;
    return pad_index == 0 ? controller_bits : 0;
}

int main(void)
{
    JPBSoftwareFramebuffer fb = { pixels, JPB_XBOX_CANVAS_WIDTH,
        JPB_XBOX_CANVAS_HEIGHT, JPB_XBOX_CANVAS_WIDTH };
    JPBSoftwareRenderStats stats;
    int result;
    unsigned frame = 0;
    unsigned previous_frame_started = 0;
    int smoke = 0;
    int tick_smoke = 0;
    int fixed_step_benchmark = 0;
    unsigned smoke_start_tick = 0;
    unsigned smoke_previous_phase = 30;
    int analog_smoke=0;
    int gpu = 0;
    int gpu_initialized = 0;
    int transparency_probe=0;
    const char *level_path="D:\\res\\level\\jpx\\fed\\fed.jpx";
    int marsh_smoke=0;
    DWORD started;
    int video_width=720,video_height=480;
    jpb_XboxStartupPhase=1;
    FILE *hd_marker=fopen("D:\\xbox-720p.txt","rb");
    if(hd_marker){fclose(hd_marker);video_width=1280;video_height=720;}
    jpb_XboxStartupPhase=2;
    if (!XVideoSetMode(video_width, video_height, 32, REFRESH_DEFAULT)) {
        jpb_XboxVideoModeResult=0;
        debugPrint("Xbox video mode unavailable: %dx%d; using 480p\n",
            video_width,video_height);
        video_width=720;
        video_height=480;
        if (!XVideoSetMode(video_width, video_height, 32, REFRESH_DEFAULT)) {
            debugPrint("Xbox 480p video mode unavailable\n");
            for (;;) Sleep(1000);
        }
    } else {
        jpb_XboxVideoModeResult=1;
    }
    jpb_XboxStartupPhase=3;
    if (!jpb_XboxMemorySelfTest()) {
        debugPrint("Xbox memory boundary self-test FAILED\n");
        for(;;)Sleep(1000);
    }
    jpb_XboxStartupPhase=4;
    /* Reserve the larger 720p framebuffer/depth pair before level assets. */
    if (video_height == 720) {
        FILE *gpu_flag=fopen("D:\\xbox-gpu.txt","rb");
        if (gpu_flag) {
            fclose(gpu_flag);
            result=jpb_XboxGpuInit();
            jpb_XboxGpuInitResult=result;
            if (result != 0) {
                debugPrint("Early 720p GPU init failed %d\n",result);
                for (;;) Sleep(1000);
            }
            gpu_initialized=1;
        }
    }
    /* An isolated disc marker selects another canonical level for the
       process-local XEMU smoke. It never drives host or emulator UI input. */
    {
        FILE *selection=fopen("D:\\xbox-start-level.txt","rb");
        if(selection){
            char name[32]={0};
            if(fgets(name,sizeof(name),selection) && strncmp(name,"marsh",5)==0 &&
               (name[5]=='\0' || name[5]=='\r' || name[5]=='\n'))
                level_path="D:\\res\\level\\jpx\\marsh\\marsh.jpx";
            fclose(selection);
        }
    }
    marsh_smoke=strstr(level_path,"\\marsh\\")!=NULL;
    debugPrint("OpenJPB Xbox: loading %s\n",level_path);
    {
        const char *path = level_path;
        FILE *probe = fopen(path, "rb");
        debugPrint("Disc probe: file=%p errno=%d\n", probe, errno);
        if (probe) {
            long size;
            int seek_result = fseek(probe, 0, SEEK_END);
            size = ftell(probe);
            debugPrint("Disc probe: seek=%d size=%ld\n", seek_result, size);
            fclose(probe);
        }
    }
    jpb_ResourceSetBasePath("D:\\");
    jpb_GameRuntimeSetImageHooks(jpb_XboxInspectImage, jpb_XboxLoadImage);
    result = jpb_GameRuntimeInitWithPlayerAssets(&runtime,
        level_path,
        "D:\\res\\animation\\obi_wan.cad",
        "D:\\res\\MODEL\\obi_wan.bmd", 0);
    jpb_XboxLastLoadResult=result;
    jpb_XboxStartupPhase=5;
    if (result != 0) {
        debugPrint("Load failed %d: %s %s\n", result,
            jpb_GameRuntimeLastFailureStage(), jpb_GameRuntimeLastFailureDetail());
        for (;;) Sleep(1000);
    }
    result = jpb_GameRuntimeAddPlayerComboData(&runtime, "D:\\res\\combo\\obi_wan.cmb");
    jpb_XboxLastLoadResult=result;
    if (result != 0) {
        debugPrint("Combo load failed %d\n", result);
        for (;;) Sleep(1000);
    }
    debugPrint("Loading canonical enemy assets\n");
    if(!jpb_XboxAudioInit()) {
        debugPrint("Xbox audio initialization failed\n");
        for(;;)Sleep(1000);
    }
    jpb_GameRuntimeUseUiTextureCache(&runtime);
    if(!jpb_XboxUiInit()) {
        debugPrint("Xbox controller artwork load failed\n");
        for(;;)Sleep(1000);
    }
    result=jpb_GameRuntimeAddEnemyAssets(&runtime,
        "D:\\res\\animation\\battle_d.cad", "D:\\res\\MODEL\\battle_d.bmd");
    jpb_XboxLastLoadResult=result;
    if(result!=0) {
        debugPrint("Enemy load failed %d: %s %s\n",result,
            jpb_GameRuntimeLastFailureStage(),jpb_GameRuntimeLastFailureDetail());
        for(;;)Sleep(1000);
    }
    debugPrint("Running canonical level constructor\n");
    {
        MM_STATISTICS memory={0};
        memory.Length=sizeof(memory);
        MmQueryStatistics(&memory);
        jpb_XboxFreePagesBeforeConstructor=memory.AvailablePages;
    }
    jpb_LoaderSetVisualLevelProvider(load_visual_level,&runtime);
    result=jpb_GameRuntimeRunCanonicalConstructor(&runtime);
    jpb_XboxLastLoadResult=result;
    jpb_XboxConstructorFileNotFound=gFileNotFound;
    {
        const char *stage=jpb_GameRuntimeLastFailureStage();
        if (strstr(stage,"loader-chain"))jpb_XboxLastLoadFailureCode=1;
        else if (strstr(stage,"player-model-view"))jpb_XboxLastLoadFailureCode=2;
        else if (strstr(stage,"load-screen-present"))jpb_XboxLastLoadFailureCode=3;
        else if (strstr(stage,"invalid-runtime"))jpb_XboxLastLoadFailureCode=4;
    }
    {
        MM_STATISTICS memory={0};
        memory.Length=sizeof(memory);
        MmQueryStatistics(&memory);
        jpb_XboxFreePagesAfterConstructor=memory.AvailablePages;
    }
    jpb_XboxStartupPhase=6;
    if(result!=0) {
        debugPrint("Constructor failed %d: %s %s\n",result,
            jpb_GameRuntimeLastFailureStage(),jpb_GameRuntimeLastFailureDetail());
        for(;;)Sleep(1000);
    }
    jpb_XboxControllerInit();
    jpb_InputSetProvider(read_controller, NULL);
    player1InputType = 1;
    {
        FILE *flag = fopen("D:\\xbox-smoke.txt", "rb");
        if (flag) { smoke = 1; fclose(flag); }
        flag = fopen("D:\\xbox-tick-smoke.txt", "rb");
        if (flag) { tick_smoke = smoke = 1; fclose(flag); }
        /* Test-only deterministic simulation for frame-aligned perf A/B.
           The staged game keeps the wall-time clock unless this marker is
           explicitly present in the isolated test disc. */
        flag = fopen("D:\\xbox-fixed-step-benchmark.txt", "rb");
        if (flag) { fixed_step_benchmark = tick_smoke = smoke = 1; fclose(flag); }
        flag = fopen("D:\\xbox-analog-smoke.txt", "rb");
        if (flag) { analog_smoke=1; fclose(flag); }
    }
    started = GetTickCount();
    {
        FILE *flag=fopen("D:\\xbox-transparency-probe.txt","rb");
        if(flag) {transparency_probe=1;fclose(flag);}
    }
    {
        FILE *flag = fopen("D:\\xbox-gpu.txt", "rb");
        if (flag) {
            fclose(flag);
            result = gpu_initialized ? 0 : jpb_XboxGpuInit();
            jpb_XboxGpuInitResult=result;
            jpb_XboxStartupPhase=7;
            if (result != 0) {
                debugPrint("GPU init failed %d\n", result);
                for (;;) Sleep(1000);
            }
            gpu = 1;
            const JPBSoftwareLevelMesh *level = runtime.levelRenderMesh;
            if (!level) {
                pb_show_debug_screen();
                debugPrint("Packed level load failed\n");
                for (;;) Sleep(1000);
            }
            jpb_GameRuntimeSetLevelRenderMesh(&runtime, level);
            jpb_GameRuntimeSetLevelRenderHook(&runtime, jpb_XboxGpuLevel, NULL);
            jpb_GameRuntimeSetModelRenderHooks(&runtime, jpb_XboxGpuModelBegin, jpb_XboxGpuTriangle, NULL, NULL);
            jpb_GameRuntimeSetScreenPolyRenderHooks(&runtime, jpb_XboxGpuEffectsBegin, jpb_XboxGpuTriangle, NULL, NULL);
            jpb_GameRuntimeSetGameplayCompositeHook(&runtime, jpb_XboxGpuComposite, NULL);
            jpb_GameRuntimeSetGameplayHudScreenDrawHook(&runtime,
                jpb_XboxGpuHudScreenDraws, NULL);
            jpb_GameRuntimeSetGameplayHudTextDrawHook(&runtime,
                jpb_XboxGpuHudTextDraws, NULL);
        }
    }
    jpb_XboxStartupPhase=8;
    for (;;) {
        unsigned frame_started=GetTickCount();
        /* Gameplay advances at the authored 60 Hz step on PC.  On Xbox a
           rendered frame can take longer, so feed actual wall time into the
           simulation rather than letting a missed refresh slow the game. */
        float frame_seconds = previous_frame_started
            ? (float)(frame_started - previous_frame_started) / 1000.0f
            : 1.0f / 60.0f;
        if (fixed_step_benchmark) frame_seconds = 1.0f / 60.0f;
        previous_frame_started = frame_started;
        jpb_XboxSmokeFrame=frame;
        jpb_XboxFramePhase=1;
        if(smoke && frame%30==0) {
            MM_STATISTICS memory={0};
            memory.Length=sizeof(memory);
            MmQueryStatistics(&memory);
            jpb_XboxFreePages=memory.AvailablePages;
            jpb_XboxEnemyClasses=(unsigned)runtime.enemyLoadedClassCount;
        }
        jpb_XboxAudioPump();
        jpb_XboxFramePhase=2;
        unsigned x, y;
        JPBControllerState controller_state;
        float axes[2];
        jpb_XboxControllerPoll(&controller_state);
        controller_bits=jpb_InputMapControllerState(&controller_state,
            OptionStruct.WalkLimit[0],OptionStruct.RunLimit[0],
            OptionStruct.ControllerConfig[0],GameStruct.inMenuFlag!=0,
            &axes[0],&axes[1]);
        /* Opt-in process-local test input. It only reaches this game's pad
           provider and never generates operating-system or window input. */
        if (smoke) {
            unsigned sequence_frame = frame;
            unsigned previous_sequence_frame = frame > 0 ? frame - 1 : 0;
            if (tick_smoke && frame >= 31) {
                if (frame == 31) smoke_start_tick = (unsigned)totalframes;
                sequence_frame = 31 + (unsigned)totalframes - smoke_start_tick;
                previous_sequence_frame = smoke_previous_phase;
                smoke_previous_phase = sequence_frame;
            }
            jpb_XboxSmokeInputPhase = sequence_frame;
            controller_bits = 0;
            axes[0]=axes[1]=0.0f;
            if (marsh_smoke) {
                /* Dismiss the authored level card, then test forward travel
                   before combat without the FED opening's turn-around. */
                if (frame == 30) controller_bits = JPB_PAD_COMBO_SOUTH;
                if (frame >= 60 && frame < 360) controller_bits = JPB_PAD_UP;
                if (frame >= 380 && frame < 660 && frame % 20 == 0)
                    controller_bits = JPB_PAD_COMBO_NORTH;
            } else {
                if (frame == 30) controller_bits = JPB_PAD_COMBO_SOUTH;
                if (sequence_frame >= 60 && sequence_frame < 120) controller_bits = JPB_PAD_RIGHT;
                if (sequence_frame >= 120 && sequence_frame < 180) controller_bits = JPB_PAD_LEFT;
                if ((previous_sequence_frame < 180 && sequence_frame >= 180) ||
                    (previous_sequence_frame < 210 && sequence_frame >= 210))
                    controller_bits = JPB_PAD_JUMP;
                if (sequence_frame >= 240 && sequence_frame < 600 &&
                    (tick_smoke ? sequence_frame / 20 != previous_sequence_frame / 20
                                : sequence_frame % 20 == 0))
                    controller_bits = JPB_PAD_COMBO_NORTH;
                /* The opening scripted encounter holds P1 through the early
                   frames. Exercise locomotion only after combat releases P1. */
                if (sequence_frame >= 700 && sequence_frame < 760) controller_bits = JPB_PAD_RIGHT;
                if (sequence_frame >= 760 && sequence_frame < 820) controller_bits = JPB_PAD_LEFT;
                if ((previous_sequence_frame < 830 && sequence_frame >= 830) ||
                    (previous_sequence_frame < 860 && sequence_frame >= 860))
                    controller_bits = JPB_PAD_JUMP;
                if (sequence_frame >= 900 && sequence_frame < 1500) controller_bits = JPB_PAD_UP;
            }
            axes[0]=((controller_bits&JPB_PAD_LEFT)?1.0f:0.0f)-
                ((controller_bits&JPB_PAD_RIGHT)?1.0f:0.0f);
            axes[1]=((controller_bits&JPB_PAD_DOWN)?1.0f:0.0f)-
                ((controller_bits&JPB_PAD_UP)?1.0f:0.0f);
            if(analog_smoke && marsh_smoke && frame>=60 && frame<360){
                JPBControllerState synthetic={0};
                synthetic.name="Xbox Series X Controller";
                synthetic.axis[1]=frame<180?-16384:-32767;
                controller_bits=jpb_InputMapControllerState(&synthetic,
                    OptionStruct.WalkLimit[0],OptionStruct.RunLimit[0],
                    OptionStruct.ControllerConfig[0],0,&axes[0],&axes[1]);
            }
        }
        /* Match PC XInput: preserve continuous stick axes for facing/speed
           while the pad bits report thresholded directions and actions. */
        g_p1X=axes[0];
        g_p1Y=axes[1];
        uint32_t *screen = (uint32_t *)XVideoGetFB();
        if (gpu) {
            memset(pixels, 0, sizeof(pixels));
            jpb_XboxGpuBegin();
        }
        result = jpb_GameRuntimeFrame(&runtime, frame_seconds, &fb, &stats);
        jpb_XboxRuntimeMs=(unsigned)(runtime.profileLastFrameSeconds*1000.0f);
        jpb_XboxSceneMs=(unsigned)(runtime.profileLastSceneSeconds*1000.0f);
        jpb_XboxWorldMs=(unsigned)(runtime.profileLastWorldSeconds*1000.0f);
        jpb_XboxModelsMs=(unsigned)(runtime.profileLastModelsSeconds*1000.0f);
        jpb_XboxEffectsMs=(unsigned)(runtime.profileLastEffectsSeconds*1000.0f);
        jpb_XboxHudMs=(unsigned)(runtime.profileLastHudSeconds*1000.0f);
        jpb_XboxScreenPolyMs=(unsigned)(runtime.profileLastScreenPolySeconds*1000.0f);
        jpb_XboxHudReplayMs=(unsigned)(runtime.profileLastHudReplaySeconds*1000.0f);
        jpb_XboxCompositeUploadMs=(unsigned)(runtime.profileLastCompositeUploadSeconds*1000.0f);
        jpb_XboxCompositeFinishMs=(unsigned)(runtime.profileLastCompositeFinishSeconds*1000.0f);
        jpb_XboxFramePhase=3;
        if (smoke) {
            sceneObject *scene = (sceneObject *)runtime.player->playerRoot.pParent;
            if (scene) {
                jpb_XboxPlayerTelemetry[0] = scene->v3WorldPosition.vx;
                jpb_XboxPlayerTelemetry[1] = scene->v3WorldPosition.vy;
                jpb_XboxPlayerTelemetry[2] = scene->v3WorldPosition.vz;
            }
            jpb_XboxPlayerTelemetry[3] = runtime.player->currentMotion;
            jpb_XboxPlayerTelemetry[4] = (int32_t)controller_bits;
            jpb_XboxPlayerTelemetry[5] = (int32_t)runtime.enemyDamageProcessedCount;
        }
        if (result != 0) {
            if (gpu) pb_show_debug_screen();
            debugPrint("Frame failed %d\n", result);
            MM_STATISTICS memory = {0};
            memory.Length = sizeof(memory);
            MmQueryStatistics(&memory);
            debugPrint("RAM free pages %lu total pages %lu\n", memory.AvailablePages, memory.TotalPhysicalPages);
            for (;;) Sleep(1000);
        }
        if (gpu) {
            unsigned present_started=GetTickCount();
            jpb_XboxFramePhase=4;
            if(transparency_probe)jpb_XboxGpuTransparencyProbe();
            if (smoke) jpb_XboxGpuStats(frame, GetTickCount() - started,
                controller_bits, runtime.player->currentMotion);
            if (smoke) jpb_XboxGpuProfile(&runtime);
            jpb_XboxGpuPresent();
            jpb_XboxPresentMs=GetTickCount()-present_started;
            jpb_XboxFramePhase=5;
        }
        else {
        VIDEO_MODE output_mode=XVideoGetMode();
        for (y = 0; y < (unsigned)output_mode.height; ++y)
            for (x = 0; x < (unsigned)output_mode.width; ++x)
                screen[y * (unsigned)output_mode.width + x] =
                    pixels[(y * JPB_XBOX_CANVAS_HEIGHT / (unsigned)output_mode.height)
                        * JPB_XBOX_CANVAS_WIDTH +
                        x * JPB_XBOX_CANVAS_WIDTH / (unsigned)output_mode.width];
        XVideoFlushFB();
        }
        if(gpu){
            unsigned elapsed=GetTickCount()-frame_started;
            /* Keep a 60 Hz presentation ceiling; slower frames run without
               an artificial 30 Hz delay. */
            if(elapsed<15)Sleep(15-elapsed);
        }
        ++frame;
        jpb_XboxFrameMs=GetTickCount()-frame_started;
        {
            volatile unsigned *record=jpb_XboxPerfRing[frame&511u];
            record[0]=frame;
            record[1]=jpb_XboxFrameMs;
            record[2]=jpb_XboxRuntimeMs;
            record[3]=jpb_XboxSceneMs;
            record[4]=jpb_XboxWorldMs;
            record[5]=jpb_XboxModelsMs;
            record[6]=jpb_XboxEffectsMs;
            record[7]=jpb_XboxTextureCount;
            record[8]=jpb_XboxFreePages;
            record[9]=(unsigned)(runtime.profileLastSceneSetupSeconds*1000.0);
            record[10]=(unsigned)(runtime.profileLastSceneAnimationsSeconds*1000.0);
            record[11]=(unsigned)(runtime.profileLastSceneOverlaySeconds*1000.0);
            record[12]=(unsigned)(runtime.profileLastSceneSabreSeconds*1000.0);
            record[13]=(unsigned)(runtime.profileLastScenePlayerSeconds*1000.0);
            record[14]=(unsigned)(runtime.profileLastScenePowerupsSeconds*1000.0);
            record[15]=(unsigned)(runtime.profileLastSceneSpritesSeconds*1000.0);
            record[16]=(unsigned)(runtime.profileLastSceneEnemiesSeconds*1000.0);
            record[17]=(unsigned)(runtime.profileLastSceneBackdropSeconds*1000.0);
            record[18]=(unsigned)(runtime.profileLastScenePhysicsSeconds*1000.0);
            record[19]=(unsigned)(runtime.profileLastSceneLevelOwnerSeconds*1000.0);
            record[20]=jpb_XboxSpriteUpdateMs;
            record[21]=jpb_XboxSpriteDrawMs;
            record[22]=(unsigned)numSprite;
            record[23]=(unsigned)numSCB;
            record[24]=jpb_XboxSlowSpriteMs;
            record[25]=(unsigned)jpb_XboxSlowSpriteFunction;
            record[26]=(unsigned)jpb_XboxSlowSpriteNum;
            record[27]=(unsigned)totalframes;
            record[28]=(unsigned)runtime.enemyDamageProcessedCount;
            record[29]=jpb_XboxModelBatchFlushes;
            record[30]=jpb_XboxGpuStateChanges;
            record[31]=(unsigned)stats.modelTriangles;
            record[32]=jpb_XboxGpuTriangleCalls;
            record[33]=jpb_XboxBmdGeometryCacheHits;
            record[34]=jpb_XboxBmdGeometryCacheMisses;
            record[35]=jpb_XboxBmdGeometryCalls;
            record[36]=jpb_XboxBmdLastGeneration;
            record[37]=jpb_XboxModelBatchUs;
            record[38]=jpb_XboxModelTriangleUs;
        }
        jpb_XboxFramePhase=6;
        if (smoke && !gpu) {
            debugMoveCursor(0, 0);
            debugPrint("F %u T %lu ms input %04lx motion %d\n", frame,
                (unsigned long)(GetTickCount() - started),
                (unsigned long)controller_bits, runtime.player->currentMotion);
        }
    }
}
