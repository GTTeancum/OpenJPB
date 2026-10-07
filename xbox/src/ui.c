#include "jpb/menu.h"
#include "jpb/alltext.h"
#include "jpb/input.h"
#include "jpb/resources.h"
#include "jpb/whook.h"
#include "jpb/texture.h"
#include "ui.h"

volatile unsigned jpb_XboxMovieRequestCount;
volatile unsigned jpb_XboxLastMovieRequest;
volatile unsigned jpb_XboxMovieQueueDrops;
volatile unsigned jpb_XboxMovieTakeCount;
enum { MOVIE_QUEUE_CAPACITY=16 };
static unsigned movie_queue[MOVIE_QUEUE_CAPACITY];
static int movie_flags[MOVIE_QUEUE_CAPACITY];
static unsigned movie_queue_count;

static const char *controller_name(unsigned player,void *user)
{(void)player;(void)user;return "Xbox Controller";}

static unsigned controller_count(void *user)
{(void)user;return 1;}

static void trigger_movie(unsigned movie,int flags,void *user)
{
    (void)flags;(void)user;
    jpb_XboxLastMovieRequest=movie;
    ++jpb_XboxMovieRequestCount;
    if(movie_queue_count<MOVIE_QUEUE_CAPACITY){
        movie_queue[movie_queue_count]=movie;
        movie_flags[movie_queue_count]=flags;
        ++movie_queue_count;
    }else ++jpb_XboxMovieQueueDrops;
}

int jpb_XboxUiTakeMovie(unsigned *movie,int *flags)
{
    if(!movie_queue_count)return 0;
    ++jpb_XboxMovieTakeCount;
    if(movie)*movie=movie_queue[0];
    if(flags)*flags=movie_flags[0];
    --movie_queue_count;
    for(unsigned i=0;i<movie_queue_count;++i){
        movie_queue[i]=movie_queue[i+1];
        movie_flags[i]=movie_flags[i+1];
    }
    return 1;
}

int jpb_XboxUiMoviesPending(void)
{
    return movie_queue_count!=0;
}

int jpb_XboxUiInit(void)
{
    JPBMenuPlatformHooks hooks={0};
    hooks.controllerCount=controller_count;
    hooks.controllerName=controller_name;
    hooks.triggerMovie=trigger_movie;
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
