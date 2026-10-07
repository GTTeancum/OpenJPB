#include "jpb/game_runtime.h"
#include "jpb/resources.h"
#include "jpb/alltext.h"
#include "jpb/input.h"
#include "jpb/loader.h"
#include "jpb/level_world.h"
#include "jpb/filesys.h"
#include "jpb/sprite.h"
#include "jpb/game.h"
#include "jpb/menu.h"
#include <hal/video.h>
#include <hal/debug.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include "gpu.h"
#include "presentation.h"
#include "audio.h"
#include "movie.h"
#include "ui.h"
#include <pbkit/pbkit.h>

static JPBGameRuntime runtime;
static uint32_t pixels[JPB_XBOX_CANVAS_WIDTH * JPB_XBOX_CANVAS_HEIGHT];
enum { JPB_XBOX_FRONTEND_WIDTH=640, JPB_XBOX_FRONTEND_HEIGHT=360 };
extern int jpb_XboxInspectImage(const char *, int *, int *);
extern int jpb_XboxLoadImage(const char *, int, int, uint32_t *, int);
extern int jpb_XboxControllerInit(void);
extern int jpb_XboxMemorySelfTest(void);
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
volatile unsigned jpb_XboxFrontendFrame;
volatile unsigned jpb_XboxFrontendMode;
volatile unsigned jpb_XboxFrontendFreePages;
volatile unsigned jpb_XboxFrontendPhase;
volatile unsigned jpb_XboxTitleFramePhase;
volatile unsigned jpb_XboxMenuMainPhase;
volatile unsigned jpb_XboxTextDrawPhase;
volatile unsigned jpb_XboxFreePagesAfterVisualLevel;
volatile unsigned jpb_XboxSmokeInputPhase;
volatile unsigned jpb_XboxUiSmokeStep;
volatile unsigned jpb_XboxUiModeHistoryCount;
volatile unsigned jpb_XboxUiModeHistory[32];
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
    char level_path[160]="D:\\res\\level\\jpx\\fed\\fed.jpx";
    char player_cad_path[128]="D:\\res\\animation\\obi_wan.cad";
    char player_bmd_path[128]="D:\\res\\MODEL\\obi_wan.bmd";
    char player_combo_path[128]="D:\\res\\combo\\obi_wan.cmb";
    int selected_model=0;
    int marsh_smoke=0;
    DWORD started;
    int video_width=720,video_height=480;
    int skip_frontend=0;
    int frontend_smoke=0;
    int movie_smoke=0;
    int title_smoke=0;
    int gpu_requested=0;
    uint32_t *frontend_pixels=NULL;
    jpb_XboxStartupPhase=1;
    FILE *hd_marker=fopen("D:\\xbox-720p.txt","rb");
    if(hd_marker){fclose(hd_marker);video_width=1280;video_height=720;}
    {
        FILE *flag=fopen("D:\\xbox-skip-frontend.txt","rb");
        if(flag){skip_frontend=1;fclose(flag);}
        flag=fopen("D:\\xbox-ui-smoke.txt","rb");
        if(flag){frontend_smoke=1;fclose(flag);}
        flag=fopen("D:\\xbox-movie-smoke.txt","rb");
        if(flag){movie_smoke=1;fclose(flag);}
        flag=fopen("D:\\xbox-title-smoke.txt","rb");
        if(flag){title_smoke=1;fclose(flag);}
        flag=fopen("D:\\xbox-gpu.txt","rb");
        if(flag){gpu_requested=1;fclose(flag);}
    }
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
    /* An isolated disc marker selects another canonical level for the
       process-local XEMU smoke. It never drives host or emulator UI input. */
    {
        FILE *selection=fopen("D:\\xbox-start-level.txt","rb");
        if(selection){
            char name[32]={0};
            if(fgets(name,sizeof(name),selection) && strncmp(name,"marsh",5)==0 &&
               (name[5]=='\0' || name[5]=='\r' || name[5]=='\n'))
                strcpy(level_path,"D:\\res\\level\\jpx\\marsh\\marsh.jpx");
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
    /* The shared menu owner lays out a 16:9 canvas; presentation maps it
       to either a widescreen output or letterboxed standard television. */
    if(!skip_frontend){
        frontend_pixels=(uint32_t *)calloc(
            (size_t)JPB_XBOX_FRONTEND_WIDTH*JPB_XBOX_FRONTEND_HEIGHT,
            sizeof(*frontend_pixels));
        if(!frontend_pixels){
            debugPrint("Front-end framebuffer allocation failed\n");
            for(;;)Sleep(1000);
        }
        JPBSoftwareFramebuffer frontend_fb={frontend_pixels,
            JPB_XBOX_FRONTEND_WIDTH,JPB_XBOX_FRONTEND_HEIGHT,
            JPB_XBOX_FRONTEND_WIDTH};
        unsigned frontend_frame=0;
        unsigned smoke_mode=UINT_MAX;
        unsigned smoke_mode_frames=0;
        if(gpu_requested){
            if(!gpu_initialized){
                result=jpb_XboxGpuInit();
                jpb_XboxGpuInitResult=result;
                if(result!=0){
                    debugPrint("Front-end GPU init failed %d\n",result);
                    for(;;)Sleep(1000);
                }
                gpu_initialized=1;
            }
        }
        if(!jpb_XboxControllerInit()){
            debugPrint("Xbox controller initialization failed\n");
            for(;;)Sleep(1000);
        }
        jpb_InputSetProvider(read_controller,NULL);
        player1InputType=1;
        if(!jpb_XboxAudioInit()){
            debugPrint("Xbox audio initialization failed\n");
            for(;;)Sleep(1000);
        }
        game_setDefaultOptions();
        generateAllText(OptionStruct.Language);
        result=jpb_GameRuntimeInitFrontend(&runtime);
        if(result!=JPB_GAME_RUNTIME_OK || !jpb_XboxUiInit()){
            debugPrint("Front-end initialization failed %d: %s %s\n",result,
                jpb_GameRuntimeLastFailureStage(),
                jpb_GameRuntimeLastFailureDetail());
            for(;;)Sleep(1000);
        }
        if(movie_smoke||title_smoke)OptionStruct.EULAaccepted=1;
        menuTexLoaded=0;
        menu_mainInitMenu(0);
        if(title_smoke){introPlayed=1;menuVars.titleArt=1;}
        if(!menuTexLoaded || !menuTextures[4] || !menuTextures[242]){
            debugPrint("Front-end texture publication failed\n");
            for(;;)Sleep(1000);
        }
        for(;;){
            JPBControllerState controller_state;
            float axes[2]={0,0};
            unsigned x,y;
            uint32_t *screen=(uint32_t *)XVideoGetFB();
            unsigned mode=menuVars.menuMode[menuVars.menuModeSP&7u];
            if(mode!=smoke_mode){
                smoke_mode=mode;
                smoke_mode_frames=0;
                if(jpb_XboxUiModeHistoryCount<32)
                    jpb_XboxUiModeHistory[jpb_XboxUiModeHistoryCount++]=mode;
            }else ++smoke_mode_frames;
            jpb_XboxFrontendFrame=frontend_frame;
            jpb_XboxSmokeFrame=frontend_frame;
            jpb_XboxFrontendMode=mode;
            jpb_XboxControllerPoll(&controller_state);
            controller_bits=jpb_InputMapControllerState(&controller_state,
                OptionStruct.WalkLimit[0],OptionStruct.RunLimit[0],
                OptionStruct.ControllerConfig[0],1,&axes[0],&axes[1]);
            /* Process-local UI smoke: scroll the legal text, accept it, then
               open the main menu from the title prompt. */
            if(frontend_smoke){
                controller_bits=0;
                if(mode==0x9f && frontend_frame>=30){
                    controller_bits=(frontend_frame>=300 &&
                        (frontend_frame%60)==0)
                        ? JPB_PAD_COMBO_SOUTH : JPB_PAD_DOWN;
                }
                if(mode==1 && smoke_mode_frames==30){
                    controller_bits=JPB_PAD_COMBO_SOUTH;
                    jpb_XboxUiSmokeStep=1;
                }else if(mode==0 && smoke_mode_frames==30){
                    controller_bits=JPB_PAD_COMBO_SOUTH;
                    jpb_XboxUiSmokeStep=2;
                }else if(mode==0x90 && smoke_mode_frames==30){
                    controller_bits=JPB_PAD_COMBO_SOUTH;
                    jpb_XboxUiSmokeStep=3;
                }else if(mode==3 && smoke_mode_frames==30){
                    controller_bits=JPB_PAD_COMBO_SOUTH;
                    jpb_XboxUiSmokeStep=4;
                }else if(mode==0x37 && smoke_mode_frames==30){
                    controller_bits=JPB_PAD_COMBO_SOUTH;
                    jpb_XboxUiSmokeStep=5;
                }else if(mode==0x0e && smoke_mode_frames==60){
                    controller_bits=JPB_PAD_COMBO_SOUTH;
                    jpb_XboxUiSmokeStep=6;
                }else if(mode==0x1a && smoke_mode_frames==60){
                    controller_bits=JPB_PAD_COMBO_SOUTH;
                    jpb_XboxUiSmokeStep=7;
                }
            }
            deltaTime=1.0f/60.0f;
            memset(frontend_pixels,0,
                (size_t)JPB_XBOX_FRONTEND_WIDTH*
                JPB_XBOX_FRONTEND_HEIGHT*sizeof(*frontend_pixels));
            if(gpu_initialized)jpb_XboxGpuBegin();
            jpb_XboxFrontendPhase=1;
            result=jpb_GameRuntimeTitleFrame(&runtime,&frontend_fb);
            jpb_XboxFrontendPhase=2;
            if(result!=JPB_GAME_RUNTIME_OK&&!jpb_XboxUiMoviesPending()){
                if(gpu_initialized)pb_show_debug_screen();
                debugPrint("Front-end frame failed %d mode %u\n",result,mode);
                for(;;)Sleep(1000);
            }
            {
                unsigned movie;
                int flags;
                int played_movie=0;
                while(jpb_XboxUiTakeMovie(&movie,&flags)){
                    jpb_XboxFrontendPhase=3;
                    (void)flags;
                    played_movie=1;
                    if(!jpb_XboxMoviePlay(movie,&frontend_fb,
                            gpu_initialized,frontend_smoke))
                        debugPrint("Movie %u failed; continuing menu flow\n",movie);
                }
                /* The retail callback is blocking. Do not show the title
                   frame that requested a boot movie before that movie. */
                if(played_movie){++frontend_frame;continue;}
            }
            jpb_XboxFrontendPhase=4;
            if(gpu_initialized){
                if(!jpb_XboxGpuPresentFrontend(&frontend_fb)){
                    pb_show_debug_screen();
                    debugPrint("Front-end presentation failed\n");
                    for(;;)Sleep(1000);
                }
                jpb_XboxGpuPresent();
            }else{
                jpb_XboxPresentSoftware(frontend_pixels,
                    JPB_XBOX_FRONTEND_WIDTH,JPB_XBOX_FRONTEND_HEIGHT,
                    JPB_XBOX_FRONTEND_WIDTH);
            }
            jpb_XboxAudioPump();
            if((frontend_frame&31u)==0){
                MM_STATISTICS memory={0};
                memory.Length=sizeof(memory);
                MmQueryStatistics(&memory);
                jpb_XboxFrontendFreePages=memory.AvailablePages;
            }
            ++frontend_frame;
            if(GameStruct.inMenuFlag==0 ||
               (mode==0x66 && GameStruct.gameMode==3))break;
            Sleep(15);
        }
        if((int)(int8_t)LevelSelect>0 &&
           (int)(int8_t)LevelSelect<JPB_LEVEL_COUNT &&
           GameStruct.ModelSelect[0]>=0 &&
           GameStruct.ModelSelect[0]<JPB_MODEL_NAME_COUNT){
            const char *level_name=sLevelNames[(int)(int8_t)LevelSelect];
            const char *model_name=sModelNames[GameStruct.ModelSelect[0]];
            selected_model=GameStruct.ModelSelect[0];
            snprintf(level_path,sizeof(level_path),
                "D:\\res\\level\\jpx\\%s\\%s.jpx",level_name,level_name);
            snprintf(player_cad_path,sizeof(player_cad_path),
                "D:\\res\\animation\\%s.cad",model_name);
            snprintf(player_bmd_path,sizeof(player_bmd_path),
                "D:\\res\\MODEL\\%s.bmd",model_name);
            snprintf(player_combo_path,sizeof(player_combo_path),
                "D:\\res\\combo\\%s.cmb",model_name);
        }
        jpb_GameRuntimeShutdown(&runtime);
        menuTexLoaded=0;
        memset(menuTextures,0,sizeof(menuTextures));
        memset(controlTextures,0,sizeof(controlTextures));
        if(gpu_initialized)jpb_XboxGpuGarbageCollect();
    }else{
        if(!jpb_XboxControllerInit()){
            debugPrint("Xbox controller initialization failed\n");
            for(;;)Sleep(1000);
        }
        jpb_InputSetProvider(read_controller,NULL);
        player1InputType=1;
        if(!jpb_XboxAudioInit()){
            debugPrint("Xbox audio initialization failed\n");
            for(;;)Sleep(1000);
        }
    }
    result = jpb_GameRuntimeInitWithPlayerAssets(&runtime,
        level_path,
        player_cad_path,
        player_bmd_path, selected_model);
    jpb_XboxLastLoadResult=result;
    jpb_XboxStartupPhase=5;
    if (result != 0) {
        debugPrint("Load failed %d: %s %s\n", result,
            jpb_GameRuntimeLastFailureStage(), jpb_GameRuntimeLastFailureDetail());
        for (;;) Sleep(1000);
    }
    result = jpb_GameRuntimeAddPlayerComboData(&runtime, player_combo_path);
    jpb_XboxLastLoadResult=result;
    if (result != 0) {
        debugPrint("Combo load failed %d\n", result);
        for (;;) Sleep(1000);
    }
    debugPrint("Loading canonical enemy assets\n");
    jpb_GameRuntimeUseUiTextureCache(&runtime);
    if(!jpb_XboxUiInit()) {
        debugPrint("Xbox UI platform initialization failed\n");
        for(;;)Sleep(1000);
    }
    if(!jpb_XboxUiLoadGameplay()) {
        debugPrint("Xbox gameplay UI load failed\n");
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
    {
        if(!frontend_pixels){
            frontend_pixels=(uint32_t *)calloc(
                (size_t)JPB_XBOX_FRONTEND_WIDTH*JPB_XBOX_FRONTEND_HEIGHT,
                sizeof(*frontend_pixels));
            if(!frontend_pixels){
                debugPrint("Movie framebuffer allocation failed\n");
                for(;;)Sleep(1000);
            }
        }
        JPBSoftwareFramebuffer movie_fb={frontend_pixels,
            JPB_XBOX_FRONTEND_WIDTH,JPB_XBOX_FRONTEND_HEIGHT,
            JPB_XBOX_FRONTEND_WIDTH};
        unsigned movie;
        int flags;
        while(jpb_XboxUiTakeMovie(&movie,&flags)){
            (void)flags;
            if(!jpb_XboxMoviePlay(movie,&movie_fb,gpu_initialized,
                    frontend_smoke))
                debugPrint("Movie %u failed before gameplay; continuing\n",movie);
        }
    }
    free(frontend_pixels);
    frontend_pixels=NULL;
    if(gpu_initialized)jpb_XboxGpuGarbageCollect();
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
        if (gpu_requested) {
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
        jpb_XboxPresentSoftware(pixels,JPB_XBOX_CANVAS_WIDTH,
            JPB_XBOX_CANVAS_HEIGHT,JPB_XBOX_CANVAS_WIDTH);
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
