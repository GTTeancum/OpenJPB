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
#include "jpb/texture.h"
#include "jpb/camera.h"
#include <hal/video.h>
#include <hal/debug.h>
#include <windows.h>
#include <stdarg.h>
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
#include "save.h"
#include <pbkit/pbkit.h>

static JPBGameRuntime runtime;
static uint32_t pixels[JPB_XBOX_CANVAS_WIDTH * JPB_XBOX_CANVAS_HEIGHT];
enum { JPB_XBOX_FRONTEND_WIDTH=640, JPB_XBOX_FRONTEND_HEIGHT=360 };
extern int jpb_XboxInspectImage(const char *, int *, int *);
extern int jpb_XboxLoadImage(const char *, int, int, uint32_t *, int);
extern int jpb_XboxControllerInit(void);
extern int jpb_XboxMemorySelfTest(void);
extern void jpb_XboxControllerUpdate(void);
extern void jpb_XboxControllerRead(int pad, JPBControllerState *state);
extern int jpb_XboxControllerConnected(int pad);
/* Player one's mapped pad bits (test sequences may replace them) and player
   two's, served to the shared input layer by read_controller. */
static uint32_t controller_bits;
static uint32_t player_two_bits;
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
/* Front-end frame cost: [0] title frame, [1] presentation, [2] whole frame. */
volatile unsigned jpb_XboxFrontendMs[3];
volatile unsigned jpb_XboxTitleFramePhase;
volatile unsigned jpb_XboxMenuMainPhase;
volatile unsigned jpb_XboxTextDrawPhase;
volatile unsigned jpb_XboxFreePagesAfterVisualLevel;
volatile unsigned jpb_XboxSmokeInputPhase;
volatile unsigned jpb_XboxUiSmokeStep;
volatile unsigned jpb_XboxUiModeHistoryCount;
volatile unsigned jpb_XboxUiModeHistory[32];
/* Front-end/gameplay ownership changes: [0] gameplay starts, [1] returns to
   the front end, [2] last started level, [3] last game mode. */
volatile unsigned jpb_XboxSessionStats[4];
/* Free 4 KiB pages at ownership checkpoints of the latest round trip:
   [0] gameplay request, [1] front end released, [2] runtime built,
   [3] constructor done, [4] movies done, [5] gameplay released,
   [6] front end rebuilt. */
volatile unsigned jpb_XboxSessionPages[8];
/* Menu owner state for emulator-monitor diagnosis: menu mode, score mode,
   mover count, mover states 0-3, game mode. */
volatile unsigned jpb_XboxMenuDebug[8];
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

/* Test markers read once at startup. */
static int gpu_initialized;
static int frontend_smoke;
static int smoke, tick_smoke, fixed_step_benchmark, analog_smoke;
static int transparency_probe;
static uint32_t *frontend_pixels;

static void fatal(const char *format, ...)
{
    char message[256];
    va_list args;
    va_start(args, format);
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);
    if (gpu_initialized) pb_show_debug_screen();
    debugPrint("%s\n", message);
    for (;;) Sleep(1000);
}

static int marker_present(const char *path)
{
    FILE *flag = fopen(path, "rb");
    if (!flag) return 0;
    fclose(flag);
    return 1;
}

/* nxdk's pdclib malloc is dlmalloc; its statistics separate live heap use
   from memory the heap holds for reuse. */
struct jpb_dlmallinfo { size_t arena, ordblks, smblks, hblks, hblkhd,
    usmblks, fsmblks, uordblks, fordblks, keepcost; };
extern struct jpb_dlmallinfo dlmallinfo(void);
/* KiB: [0] heap footprint, [1] in use, [2] free inside the heap,
   [3] directly mapped large blocks. */
volatile unsigned jpb_XboxHeapKiB[4];
/* KiB of decoded CPU textures: world, player, enemy, UI, default caches. */
volatile unsigned jpb_XboxTextureCacheKiB[5];
/* Poke jpb_XboxCacheDumpRequest to 1 to list cached textures as text. */
volatile unsigned jpb_XboxCacheDumpRequest;
char jpb_XboxCacheDump[16384];
static void sample_heap(void)
{
    struct jpb_dlmallinfo info=dlmallinfo();
    jpb_XboxHeapKiB[0]=(unsigned)((info.arena+info.hblkhd)>>10);
    jpb_XboxHeapKiB[1]=(unsigned)(info.uordblks>>10);
    jpb_XboxHeapKiB[2]=(unsigned)(info.fordblks>>10);
    jpb_XboxHeapKiB[3]=(unsigned)(info.hblkhd>>10);
}

static unsigned free_pages(void)
{
    MM_STATISTICS memory={0};
    memory.Length=sizeof(memory);
    MmQueryStatistics(&memory);
    return memory.AvailablePages;
}

static int load_visual_level(const char *fbx_path, int level_index, void *user)
{
    char path[512];
    const char *dot=strrchr(fbx_path,'.');
    if(!dot || (size_t)(dot-fbx_path)>sizeof(path)-5)return 0;
    snprintf(path,sizeof(path),"%.*s.xlv",(int)(dot-fbx_path),fbx_path);
    for(char *p=path;*p;++p)if(*p=='/')*p='\\';
    const JPBSoftwareLevelMesh *mesh=jpb_XboxLoadLevel(path);
    jpb_XboxVisualLevelLoaded=mesh!=NULL;
    jpb_XboxFreePagesAfterVisualLevel=free_pages();
    if(!mesh || mesh->levelIndex!=level_index)return 0;
    jpb_GameRuntimeSetLevelRenderMesh((JPBGameRuntime *)user,mesh);
    return 1;
}
static uint32_t read_controller(int32_t pad_index, void *unused)
{
    (void)unused;
    return pad_index == 0 ? controller_bits :
        pad_index == 1 ? player_two_bits : 0;
}

/* The menu canvas exists only while the front end owns the frame; gameplay
   needs the memory. Movies also decode into it. */
static JPBSoftwareFramebuffer frontend_framebuffer(void)
{
    JPBSoftwareFramebuffer fb={0};
    if(!frontend_pixels){
        frontend_pixels=(uint32_t *)calloc(
            (size_t)JPB_XBOX_FRONTEND_WIDTH*JPB_XBOX_FRONTEND_HEIGHT,
            sizeof(*frontend_pixels));
        if(!frontend_pixels)fatal("Front-end framebuffer allocation failed");
    }
    fb.pixels=frontend_pixels;
    fb.width=fb.stridePixels=JPB_XBOX_FRONTEND_WIDTH;
    fb.height=JPB_XBOX_FRONTEND_HEIGHT;
    return fb;
}

static void play_queued_movies(void)
{
    unsigned movie;
    int flags;
    while(jpb_XboxUiTakeMovie(&movie,&flags)){
        JPBSoftwareFramebuffer fb=frontend_framebuffer();
        (void)flags;
        if(!jpb_XboxMoviePlay(movie,&fb,gpu_initialized,frontend_smoke))
            debugPrint("Movie %u failed; continuing\n",movie);
    }
}

static void set_title_render_hooks(void)
{
    if(!gpu_initialized)return;
    /* Menu artwork and text go straight to the GPU in the recovered
       panel/text order; compositing the 640x360 canvas in software held the
       front end near 8 FPS. */
    jpb_GameRuntimeSetTitleScreenDrawRenderHook(&runtime,
        jpb_XboxGpuHudScreenDraws,NULL);
    jpb_GameRuntimeSetTitleTextDrawRenderHook(&runtime,
        jpb_XboxGpuHudTextDraws,NULL);
}

/* Map both controller ports for this frame. Player one's continuous stick
   axes are returned for the caller (test sequences may replace them);
   player two's are published directly, matching the PC XInput provider. */
static void read_players(int in_menu, float *axis_x, float *axis_y)
{
    JPBControllerState state;
    jpb_XboxControllerUpdate();
    jpb_XboxControllerRead(0,&state);
    controller_bits=jpb_InputMapControllerState(&state,
        OptionStruct.WalkLimit[0],OptionStruct.RunLimit[0],
        OptionStruct.ControllerConfig[0],in_menu,axis_x,axis_y);
    jpb_XboxControllerRead(1,&state);
    player_two_bits=jpb_InputMapControllerState(&state,
        OptionStruct.WalkLimit[1],OptionStruct.RunLimit[1],
        OptionStruct.ControllerConfig[1],in_menu,&g_p2X,&g_p2Y);
    player1InputType=1;
    player2InputType=1;
    p2Connected=jpb_XboxControllerConnected(1);
}

/* Host transitions recovered by the PC port (pc_front_end_requests_*). The
   menu owner publishes the selection; the host performs the load. */
static int front_end_requests_gameplay(int *level_index, int *game_mode)
{
    unsigned stack=menuVars.menuModeSP&7u;
    unsigned mode=menuVars.menuMode[stack];
    int selected=(int)(uint8_t)LevelSelect;

    /* New Game returns from difficulty to state 4; the recovered character
       selector is state 0x0e (same bridge as the PC host). */
    if(mode==4 && stack!=0 &&
       (menuVars.menuMode[(stack-1u)&7u]==0x37 ||
        menuVars.menuMode[(stack-1u)&7u]==0x99)){
        menu_initPlayerSelect();
        menu_pushMenu(0x0e);
        return 0;
    }
    if(GameStruct.gameMode==6 && GameStruct.inMenuFlag==0){
        int next=0;
        if(SaveGameStruct.validFlag!=0 && SaveGameStruct.lastlevel>=1 &&
           SaveGameStruct.lastlevel<=14)next=SaveGameStruct.lastlevel;
        else if(GameStruct.CurrentLevel>=1 && GameStruct.CurrentLevel<=14)
            next=(int)(uint8_t)GameStruct.CurrentLevel;
        else if(LevelSelect>=1 && LevelSelect<=14)next=selected;
        if(!next)return 0;
        *level_index=next;
        *game_mode=6;
        return 1;
    }
    if(mode==0x66 && selected>=1 && selected<=14){
        *level_index=selected;
        *game_mode=4;
        return 1;
    }
    /* Training. */
    if(mode==0x0c && GameStruct.gameMode==2 && selected>=16 && selected<=22){
        GameStruct.NumPlayers=1;
        *level_index=selected;
        *game_mode=2;
        return 1;
    }
    /* Versus arena. */
    if(mode==0x0d && selected==25 && GameStruct.NumPlayers==2 &&
       GameStruct.gameMode==2 && GameStruct.versusModeFlag!=0){
        *level_index=selected;
        *game_mode=2;
        return 1;
    }
    return 0;
}

static int gameplay_enters_score_mode(void)
{
    if(GameStruct.gameMode!=4 && GameStruct.gameMode!=5)return 0;
    camera_SetCameras();
    menu_enterScoreMode(3);
    GameStruct.CurrentLevel=(uint8_t)LevelSelect;
    return 1;
}

static int gameplay_requests_title(void)
{
    unsigned stack=menuVars.menuModeSP&7u;
    return GameStruct.inMenuFlag!=0 && menuVars.menuMode[stack]==0 &&
        (GameStruct.gameMode==0 || GameStruct.gameMode==9);
}

/* Run menu frames until the menu owner requests a level. */
static void run_front_end(int *level_index, int *game_mode)
{
    JPBSoftwareFramebuffer frontend_fb=frontend_framebuffer();
    unsigned frontend_frame=0;
    unsigned smoke_mode=UINT_MAX;
    unsigned smoke_mode_frames=0;
    int result;

    for(;;){
        float axes[2]={0,0};
        unsigned mode=menuVars.menuMode[menuVars.menuModeSP&7u];
        DWORD frontend_started;
        if(mode!=smoke_mode){
            smoke_mode=mode;
            smoke_mode_frames=0;
            if(jpb_XboxUiModeHistoryCount<32)
                jpb_XboxUiModeHistory[jpb_XboxUiModeHistoryCount++]=mode;
        }else ++smoke_mode_frames;
        jpb_XboxFrontendFrame=frontend_frame;
        jpb_XboxSmokeFrame=frontend_frame;
        jpb_XboxFrontendMode=mode;
        jpb_XboxMenuDebug[0]=mode;
        jpb_XboxMenuDebug[1]=menuVars.scoreMode;
        jpb_XboxMenuDebug[2]=menuVars.mmvCount;
        for(unsigned i=0;i<4;++i)jpb_XboxMenuDebug[3+i]=menuVars.mmv[i].state;
        jpb_XboxMenuDebug[7]=(unsigned)(uint8_t)GameStruct.gameMode;
        read_players(1,&axes[0],&axes[1]);
        /* Process-local UI smoke: open the main menu from the title prompt
           and confirm through to the first level. */
        if(frontend_smoke){
            static const unsigned steps[][2]={{1,30},{0,30},{0x90,30},
                {3,30},{0x37,30},{0x0e,60},{0x1a,60}};
            controller_bits=0;
            for(unsigned i=0;i<sizeof(steps)/sizeof(steps[0]);++i)
                if(mode==steps[i][0] && smoke_mode_frames==steps[i][1]){
                    controller_bits=JPB_PAD_COMBO_SOUTH;
                    jpb_XboxUiSmokeStep=i+1;
                }
        }
        deltaTime=1.0f/60.0f;
        if(gpu_initialized){
            jpb_XboxGpuBegin();
            jpb_XboxGpuSetCanvas(JPB_XBOX_FRONTEND_WIDTH,
                JPB_XBOX_FRONTEND_HEIGHT);
        }else{
            memset(frontend_pixels,0,
                (size_t)JPB_XBOX_FRONTEND_WIDTH*
                JPB_XBOX_FRONTEND_HEIGHT*sizeof(*frontend_pixels));
        }
        jpb_XboxFrontendPhase=1;
        frontend_started=GetTickCount();
        result=jpb_GameRuntimeTitleFrame(&runtime,&frontend_fb);
        jpb_XboxFrontendMs[0]=GetTickCount()-frontend_started;
        jpb_XboxFrontendPhase=2;
        if(result!=JPB_GAME_RUNTIME_OK&&!jpb_XboxUiMoviesPending())
            fatal("Front-end frame failed %d mode %u",result,mode);
        if(jpb_XboxUiMoviesPending()){
            /* The retail callback is blocking. Do not show the title frame
               that requested a movie before that movie. */
            jpb_XboxFrontendPhase=3;
            play_queued_movies();
            ++frontend_frame;
            continue;
        }
        jpb_XboxFrontendPhase=4;
        {
            DWORD present_started=GetTickCount();
            if(gpu_initialized){
                jpb_XboxGpuPresent();
            }else{
                jpb_XboxPresentSoftware(frontend_pixels,
                    JPB_XBOX_FRONTEND_WIDTH,JPB_XBOX_FRONTEND_HEIGHT,
                    JPB_XBOX_FRONTEND_WIDTH);
            }
            jpb_XboxFrontendMs[1]=GetTickCount()-present_started;
        }
        jpb_XboxAudioPump();
        jpb_XboxFrontendMs[2]=GetTickCount()-frontend_started;
        if((frontend_frame&31u)==0)jpb_XboxFrontendFreePages=free_pages();
        ++frontend_frame;
        if(front_end_requests_gameplay(level_index,game_mode))return;
        {
            /* Hold the front end near its authored 60 Hz step. */
            DWORD elapsed=GetTickCount()-frontend_started;
            if(elapsed<16)Sleep(16-elapsed);
        }
    }
}

/* Load the selected level, following the PC host hand-off
   (pc_start_selected_gameplay): runtime construction restores defaults, so
   the menu's options, players and difficulty are published again around the
   canonical constructor. */
static void start_gameplay(int level, int game_mode)
{
    optionstruct selected_options=OptionStruct;
    int selected_model=GameStruct.ModelSelect[0];
    int selected_players=GameStruct.NumPlayers==2?2:1;
    int selected_player_two=GameStruct.ModelSelect[1];
    int selected_difficulty=GameStruct.difficulty;
    int selected_versus=GameStruct.versusModeFlag;
    uint8_t selected_player_index[2]={menuVars.pplayers[0],menuVars.pplayers[1]};
    char level_path[160],cad[128],bmd[128],cmb[128];
    char cad2[128],bmd2[128],cmb2[128];
    const char *model_name,*level_name;
    int result;

    if(selected_model<0 || selected_model>=JPB_MODEL_NAME_COUNT)selected_model=0;
    if(selected_player_two<0 || selected_player_two>=JPB_MODEL_NAME_COUNT)
        selected_player_two=0;
    if(level<1 || level>=JPB_LEVEL_COUNT)fatal("Invalid level request %d",level);
    model_name=sModelNames[selected_model];
    level_name=sLevelNames[level];
    snprintf(level_path,sizeof(level_path),
        "D:\\res\\level\\jpx\\%s\\%s.jpx",level_name,level_name);
    snprintf(cad,sizeof(cad),"D:\\res\\animation\\%s.cad",model_name);
    snprintf(bmd,sizeof(bmd),"D:\\res\\MODEL\\%s.bmd",model_name);
    snprintf(cmb,sizeof(cmb),"D:\\res\\combo\\%s.cmb",model_name);
    snprintf(cad2,sizeof(cad2),"D:\\res\\animation\\%s.cad",
        sModelNames[selected_player_two]);
    snprintf(bmd2,sizeof(bmd2),"D:\\res\\MODEL\\%s.bmd",
        sModelNames[selected_player_two]);
    snprintf(cmb2,sizeof(cmb2),"D:\\res\\combo\\%s.cmb",
        sModelNames[selected_player_two]);
    ++jpb_XboxSessionStats[0];
    jpb_XboxSessionStats[2]=(unsigned)level;
    jpb_XboxSessionStats[3]=(unsigned)game_mode;

    jpb_XboxSessionPages[0]=free_pages();
    LevelSelect=(char)level;
    menu_levelSelect();
    jpb_GameRuntimeShutdown(&runtime);
    menuTexLoaded=0;
    memset(menuTextures,0,sizeof(menuTextures));
    memset(controlTextures,0,sizeof(controlTextures));
    free(frontend_pixels);
    frontend_pixels=NULL;
    if(gpu_initialized)jpb_XboxGpuGarbageCollect();
    jpb_XboxSessionPages[1]=free_pages();

    jpb_XboxStartupPhase=5;
    result=jpb_GameRuntimeInitWithPlayerAssets(&runtime,level_path,cad,bmd,
        selected_model);
    jpb_XboxLastLoadResult=result;
    if(result!=0)fatal("Load failed %d: %s %s",result,
        jpb_GameRuntimeLastFailureStage(),jpb_GameRuntimeLastFailureDetail());
    jpb_XboxSessionPages[2]=free_pages();
    OptionStruct=selected_options;
    generateAllText(OptionStruct.Language);
    menuVars.pplayers[0]=selected_player_index[0];
    menuVars.pplayers[1]=selected_player_index[1];
    menu_setNumPlayers((unsigned)selected_players);
    menu_setPlayer(0,(unsigned)selected_model);
    if(selected_players==2)menu_setPlayer(1,(unsigned)selected_player_two);
    GameStruct.versusModeFlag=(int16_t)selected_versus;
    GameStruct.difficulty=(char)(selected_difficulty==0?0:1);
    GameStruct.CurrentLevel=(char)level;
    result=jpb_GameRuntimeAddPlayerComboData(&runtime,cmb);
    jpb_XboxLastLoadResult=result;
    if(result!=0)fatal("Combo load failed %d",result);
    if(selected_players==2){
        result=jpb_GameRuntimeActivateSecondPlayer(&runtime,cad2,bmd2,
            selected_player_two);
        if(result==0)result=jpb_GameRuntimeAddSecondPlayerComboData(&runtime,cmb2);
        if(result!=0)fatal("Player two load failed %d",result);
    }else if(level==12){
        /* The solo Kadu level carries a second rider (PC
           pc_activate_solo_kadu_rider). */
        menu_setPlayer(1,(unsigned)selected_model);
        result=jpb_GameRuntimeActivateSecondPlayer(&runtime,cad,bmd,
            selected_model);
        if(result==0)result=jpb_GameRuntimeAddSecondPlayerComboData(&runtime,cmb);
        menu_setNumPlayers(1);
        if(result!=0)fatal("Kadu rider load failed %d",result);
    }
    jpb_GameRuntimeUseUiTextureCache(&runtime);
    if(!jpb_XboxUiInit())fatal("Xbox UI platform initialization failed");
    if(!jpb_XboxUiLoadGameplay())fatal("Xbox gameplay UI load failed");
    if(game_mode!=2){
        result=jpb_GameRuntimeAddEnemyAssets(&runtime,
            "D:\\res\\animation\\battle_d.cad","D:\\res\\MODEL\\battle_d.bmd");
        jpb_XboxLastLoadResult=result;
        if(result!=0)fatal("Enemy load failed %d: %s %s",result,
            jpb_GameRuntimeLastFailureStage(),
            jpb_GameRuntimeLastFailureDetail());
    }
    jpb_XboxFreePagesBeforeConstructor=free_pages();
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
    jpb_XboxFreePagesAfterConstructor=jpb_XboxSessionPages[3]=free_pages();
    jpb_XboxStartupPhase=6;
    if(result!=0)fatal("Constructor failed %d: %s %s",result,
        jpb_GameRuntimeLastFailureStage(),jpb_GameRuntimeLastFailureDetail());
    OptionStruct=selected_options;
    generateAllText(OptionStruct.Language);
    menuVars.pplayers[0]=selected_player_index[0];
    menuVars.pplayers[1]=selected_player_index[1];
    menu_setNumPlayers((unsigned)selected_players);
    menu_setPlayer(0,(unsigned)selected_model);
    if(selected_players==2)menu_setPlayer(1,(unsigned)selected_player_two);
    GameStruct.versusModeFlag=(int16_t)selected_versus;
    GameStruct.difficulty=(char)(selected_difficulty==0?0:1);
    jpb_game_ApplyLevelDifficulty((unsigned)level);
    GameStruct.CurrentLevel=(char)level;
    /* game_gPlayTheGame changes load modes to active mode 6; keeping a
       request mode would enter the score screen on the first frame. */
    GameStruct.gameMode=6;

    play_queued_movies();
    free(frontend_pixels);
    frontend_pixels=NULL;
    if(gpu_initialized)jpb_XboxGpuGarbageCollect();
    jpb_XboxSessionPages[4]=free_pages();
    jpb_InputSetProvider(read_controller,NULL);
    player1InputType=1;
    if(gpu_initialized){
        const JPBSoftwareLevelMesh *mesh=runtime.levelRenderMesh;
        if(!mesh)fatal("Packed level load failed");
        jpb_GameRuntimeSetLevelRenderMesh(&runtime,mesh);
        jpb_GameRuntimeSetLevelRenderHook(&runtime,jpb_XboxGpuLevel,NULL);
        jpb_GameRuntimeSetModelRenderHooks(&runtime,jpb_XboxGpuModelBegin,
            jpb_XboxGpuTriangle,NULL,NULL);
        jpb_GameRuntimeSetScreenPolyRenderHooks(&runtime,
            jpb_XboxGpuEffectsBegin,jpb_XboxGpuTriangle,NULL,NULL);
        jpb_GameRuntimeSetGameplayCompositeHook(&runtime,
            jpb_XboxGpuComposite,NULL);
        jpb_GameRuntimeSetGameplayHudScreenDrawHook(&runtime,
            jpb_XboxGpuHudScreenDraws,NULL);
        jpb_GameRuntimeSetGameplayHudTextDrawHook(&runtime,
            jpb_XboxGpuHudTextDraws,NULL);
    }
    jpb_XboxStartupPhase=8;
}

/* The PC host keeps the level resident under the results and title menus.
   64 MB cannot hold both, so release the level once the menu owner has taken
   over (score mode is already entered) and rebuild the front-end runtime;
   menu state lives in the shared globals and is untouched. */
static void return_to_front_end(void)
{
    int result;
    ++jpb_XboxSessionStats[1];
    jpb_GameRuntimeShutdown(&runtime);
    jpb_XboxUnloadLevel();
    menuTexLoaded=0;
    memset(menuTextures,0,sizeof(menuTextures));
    memset(controlTextures,0,sizeof(controlTextures));
    if(gpu_initialized)jpb_XboxGpuGarbageCollect();
    jpb_XboxSessionPages[5]=free_pages();
    result=jpb_GameRuntimeInitFrontend(&runtime);
    if(result!=JPB_GAME_RUNTIME_OK)
        fatal("Front-end initialization failed %d: %s %s",result,
            jpb_GameRuntimeLastFailureStage(),
            jpb_GameRuntimeLastFailureDetail());
    _ClearTextureCache();
    menu_winLoadTextures();
    if(!menuTexLoaded)fatal("Front-end texture restore failed");
    if(!jpb_XboxUiInit())fatal("Xbox UI platform initialization failed");
    set_title_render_hooks();
    controller_bits=0;
    jpb_XboxSessionPages[6]=free_pages();
}

/* Run gameplay frames until the game hands ownership back to the menus. */
static void run_gameplay(int marsh_smoke)
{
    JPBSoftwareFramebuffer fb={pixels,JPB_XBOX_CANVAS_WIDTH,
        JPB_XBOX_CANVAS_HEIGHT,JPB_XBOX_CANVAS_WIDTH};
    JPBSoftwareRenderStats stats;
    unsigned frame=0;
    unsigned previous_frame_started=0;
    unsigned smoke_start_tick=0;
    unsigned smoke_previous_phase=30;
    DWORD started=GetTickCount();
    int gpu=gpu_initialized;
    int result;

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
        if(frame%30==0) {
            jpb_XboxFreePages=free_pages();
            sample_heap();
            {
                JPBGameRuntimeTextureCache *caches[5]={runtime.worldTextureCache,
                    runtime.textureCache,runtime.enemyTextureCache,
                    runtime.uiTextureCache,runtime.defaultTextureCache};
                for(unsigned i=0;i<5;++i)jpb_XboxTextureCacheKiB[i]=
                    (unsigned)(jpb_GameRuntimeTextureCacheBytes(caches[i])>>10);
                if(jpb_XboxCacheDumpRequest){
                    size_t used=0;
                    for(unsigned i=0;i<5;++i){
                        size_t w,h;
                        const char *name;
                        for(size_t e=0;(name=jpb_GameRuntimeTextureCacheEntry(
                                caches[i],e,&w,&h))!=NULL;++e){
                            int n=snprintf(jpb_XboxCacheDump+used,
                                sizeof(jpb_XboxCacheDump)-used,"%u %ux%u %s\n",
                                i,(unsigned)w,(unsigned)h,name);
                            if(n<0||(size_t)n>=sizeof(jpb_XboxCacheDump)-used)break;
                            used+=(size_t)n;
                        }
                    }
                    jpb_XboxCacheDumpRequest=0;
                }
            }
            jpb_XboxEnemyClasses=(unsigned)runtime.enemyLoadedClassCount;
        }
        jpb_XboxAudioPump();
        jpb_XboxFramePhase=2;
        float axes[2];
        read_players(GameStruct.inMenuFlag!=0,&axes[0],&axes[1]);
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
        if (result != 0)
            fatal("Frame failed %d; RAM free pages %u", result, free_pages());
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
        /* In-game requests (level intro cards, cutscenes) queue movies. */
        if(jpb_XboxUiMoviesPending()){
            play_queued_movies();
            free(frontend_pixels);
            frontend_pixels=NULL;
            previous_frame_started=0;
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
        if(gameplay_enters_score_mode() || gameplay_requests_title())return;
    }
}

int main(void)
{
    int result;
    int video_width=720,video_height=480;
    int skip_frontend;
    int title_smoke;
    int level=1, game_mode=4;
    jpb_XboxStartupPhase=1;
    if(marker_present("D:\\xbox-720p.txt")){video_width=1280;video_height=720;}
    skip_frontend=marker_present("D:\\xbox-skip-frontend.txt");
    frontend_smoke=marker_present("D:\\xbox-ui-smoke.txt");
    title_smoke=marker_present("D:\\xbox-title-smoke.txt");
    smoke=marker_present("D:\\xbox-smoke.txt");
    if(marker_present("D:\\xbox-tick-smoke.txt"))tick_smoke=smoke=1;
    /* Test-only deterministic simulation for frame-aligned perf A/B. The
       staged game keeps the wall-time clock unless this marker is present. */
    if(marker_present("D:\\xbox-fixed-step-benchmark.txt"))
        fixed_step_benchmark=tick_smoke=smoke=1;
    analog_smoke=marker_present("D:\\xbox-analog-smoke.txt");
    transparency_probe=marker_present("D:\\xbox-transparency-probe.txt");
    jpb_XboxStartupPhase=2;
    if (!XVideoSetMode(video_width, video_height, 32, REFRESH_DEFAULT)) {
        jpb_XboxVideoModeResult=0;
        debugPrint("Xbox video mode unavailable: %dx%d; using 480p\n",
            video_width,video_height);
        video_width=720;
        video_height=480;
        if (!XVideoSetMode(video_width, video_height, 32, REFRESH_DEFAULT))
            fatal("Xbox 480p video mode unavailable");
    } else {
        jpb_XboxVideoModeResult=1;
    }
    jpb_XboxStartupPhase=3;
    if (!jpb_XboxMemorySelfTest())fatal("Xbox memory boundary self-test FAILED");
    jpb_XboxStartupPhase=4;
    /* An isolated disc marker selects the level for the direct-gameplay
       smoke (xbox-skip-frontend.txt). */
    {
        FILE *selection=fopen("D:\\xbox-start-level.txt","rb");
        if(selection){
            char name[32]={0};
            if(fgets(name,sizeof(name),selection)){
                name[strcspn(name,"\r\n")]=0;
                for(int i=1;i<JPB_LEVEL_COUNT;++i)
                    if(sLevelNames[i] && strcmp(name,sLevelNames[i])==0)level=i;
            }
            fclose(selection);
        }
    }
    jpb_ResourceSetBasePath("D:\\");
    jpb_GameRuntimeSetImageHooks(jpb_XboxInspectImage, jpb_XboxLoadImage);
    jpb_GameRuntimeSetAuthoredImageSizeHook(jpb_XboxAuthoredImageSize);
    /* The GPU renderer is the shipping path; the software renderer is only
       an explicit diagnostic. */
    if(!marker_present("D:\\xbox-software-renderer.txt")){
        result=jpb_XboxGpuInit();
        jpb_XboxGpuInitResult=result;
        if(result!=0)fatal("GPU init failed %d",result);
        gpu_initialized=1;
    }
    if(!jpb_XboxControllerInit())fatal("Xbox controller initialization failed");
    jpb_InputSetProvider(read_controller,NULL);
    player1InputType=1;
    if(!jpb_XboxAudioInit())fatal("Xbox audio initialization failed");
    game_setDefaultOptions();
    generateAllText(OptionStruct.Language);
    /* Options load before the front end publishes text or music, as on PC;
       campaign state loads after menu_mainInitMenu resets it. */
    if(jpb_XboxSaveInit())jpb_XboxSaveLoadOptions();
    /* The Xbox build is a port of an owned PC copy whose license was
       accepted there; it never shows the license screen. */
    OptionStruct.EULAaccepted=1;
    /* The shared menu owner lays out a 16:9 canvas; presentation maps it to
       either a widescreen output or letterboxed standard television. */
    result=jpb_GameRuntimeInitFrontend(&runtime);
    if(result!=JPB_GAME_RUNTIME_OK || !jpb_XboxUiInit())
        fatal("Front-end initialization failed %d: %s %s",result,
            jpb_GameRuntimeLastFailureStage(),
            jpb_GameRuntimeLastFailureDetail());
    menuTexLoaded=0;
    menu_mainInitMenu(0);
    jpb_XboxSaveLoadGame();
    if(title_smoke){introPlayed=1;menuVars.titleArt=1;}
    set_title_render_hooks();
    if(!menuTexLoaded || !menuTextures[4] || !menuTextures[242])
        fatal("Front-end texture publication failed");
    if(skip_frontend){
        GameStruct.NumPlayers=1;
        GameStruct.ModelSelect[0]=0;
    }
    for(;;){
        if(!skip_frontend)run_front_end(&level,&game_mode);
        skip_frontend=0;
        start_gameplay(level,game_mode);
        run_gameplay(strcmp(sLevelNames[level],"marsh")==0);
        return_to_front_end();
    }
}
