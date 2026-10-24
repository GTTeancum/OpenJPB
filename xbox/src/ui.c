#include "jpb/menu.h"
#include "jpb/alltext.h"
#include "jpb/input.h"
#include "jpb/resources.h"
#include "jpb/whook.h"
#include "jpb/texture.h"
#include "ui.h"
#include "save.h"
#include <xboxkrnl/xboxkrnl.h>

volatile unsigned jpb_XboxMovieRequestCount;
volatile unsigned jpb_XboxLastMovieRequest;
volatile unsigned jpb_XboxMovieQueueDrops;
volatile unsigned jpb_XboxMovieTakeCount;
enum { MOVIE_QUEUE_CAPACITY=16 };
static unsigned movie_queue[MOVIE_QUEUE_CAPACITY];
static int movie_flags[MOVIE_QUEUE_CAPACITY];
/* VideoVolume at each request (the boot warning/logo request 0, the intro
   0.45); the PC host restores it when the queued movie launches. */
static float movie_volumes[MOVIE_QUEUE_CAPACITY];
static unsigned movie_queue_count;

extern unsigned jpb_XboxControllerCount(void);

static const char *controller_name(unsigned player,void *user)
{(void)player;(void)user;return "Xbox Controller";}

/* Player one is always offered; a second connected pad enables two-player
   modes. */
static unsigned controller_count(void *user)
{
    unsigned count=jpb_XboxControllerCount();
    (void)user;
    return count?count:1;
}

static void trigger_movie(unsigned movie,int flags,void *user)
{
    (void)flags;(void)user;
    jpb_XboxLastMovieRequest=movie;
    ++jpb_XboxMovieRequestCount;
    if(movie_queue_count<MOVIE_QUEUE_CAPACITY){
        movie_queue[movie_queue_count]=movie;
        movie_flags[movie_queue_count]=flags;
        movie_volumes[movie_queue_count]=VideoVolume;
        ++movie_queue_count;
    }else ++jpb_XboxMovieQueueDrops;
}

int jpb_XboxUiTakeMovie(unsigned *movie,int *flags)
{
    if(!movie_queue_count)return 0;
    ++jpb_XboxMovieTakeCount;
    if(movie)*movie=movie_queue[0];
    if(flags)*flags=movie_flags[0];
    VideoVolume=movie_volumes[0];
    --movie_queue_count;
    for(unsigned i=0;i<movie_queue_count;++i){
        movie_queue[i]=movie_queue[i+1];
        movie_flags[i]=movie_flags[i+1];
        movie_volumes[i]=movie_volumes[i+1];
    }
    return 1;
}

int jpb_XboxUiMoviesPending(void)
{
    return movie_queue_count!=0;
}

static void save_game_data(void *user)
{(void)user;jpb_XboxSaveWriteGame();}

static void save_settings_data(const optionstruct *options,void *user)
{(void)user;jpb_XboxSaveWriteOptions(options);}

static int activate_item(uint32_t destination,void *user)
{
    (void)user;
    /* As on PC: New Game without a campaign save skips the overwrite
       confirmation and takes the recovered no-save destination 9. */
    if(destination==0x90 && !jpb_XboxSaveHasGame()){
        menu_menuExit();
        (void)menu_handleMenuTriggers(9);
    }
    return 0;
}

static void request_exit(void *user)
{
    (void)user;
    /* Main-menu Quit returns to the dashboard. Saves are already flushed. */
    HalReturnToFirmware(HalQuickRebootRoutine);
}

int jpb_XboxUiInit(void)
{
    JPBMenuPlatformHooks hooks={0};
    hooks.controllerCount=controller_count;
    hooks.controllerName=controller_name;
    hooks.triggerMovie=trigger_movie;
    hooks.saveGameData=save_game_data;
    hooks.saveSettingsData=save_settings_data;
    hooks.activateItem=activate_item;
    hooks.requestExit=request_exit;
    jpb_MenuSetPlatformHooks(&hooks,NULL);
    lastUsedInputType=1;
    return 1;
}

int jpb_XboxUiLoadGameplay(void)
{
    /* Gameplay keeps only the controller bank and pause/options panels.
       The complete front-end bank is released before level construction. */
    for(unsigned i=0;i<9;++i) {
        const char *name=i<8?jpb_AllTextUtf8(0,controlTextList[i]):"controller";
        const char *path=resource_getPathWithExtension(
            name,JPB_RESOURCE_CONTROLLER_SECONDARY,"png");
        controlTextures[i]=_LoadTexture((char *)path,TT_FRONT,0);
        if(!controlTextures[i])return 0;
    }
    for(unsigned i=0;i<JPB_MENU_TEXTURE_ENTRY_COUNT;++i) {
        const JPBMenuTextureEntry *entry=&menuTextureList[i];
        if(entry->textureIndex<236 || entry->textureIndex>244)continue;
        menuTextures[entry->textureIndex]=_LoadTexture(
            (char *)resource_getPath(entry->filename,JPB_RESOURCE_FRONT),
            TT_FRONT,0);
        if(!menuTextures[entry->textureIndex])return 0;
    }
    return 1;
}
