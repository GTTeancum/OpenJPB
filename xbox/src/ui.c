#include "jpb/menu.h"
#include "jpb/alltext.h"
#include "jpb/input.h"
#include "jpb/resources.h"
#include "jpb/whook.h"
#include "jpb/texture.h"

static const char *controller_name(unsigned player,void *user)
{(void)player;(void)user;return "Xbox Controller";}

int jpb_XboxUiInit(void)
{
    JPBMenuPlatformHooks hooks={0};
    hooks.controllerName=controller_name;
    jpb_MenuSetPlatformHooks(&hooks,NULL);
    lastUsedInputType=1;
    /* Same bank and filename selection as menu_loadControllerBank; other
       platform controller families need not consume Xbox memory. */
    for(unsigned i=0;i<9;++i) {
        const char *name=i<8?jpb_AllTextUtf8(0,controlTextList[i]):"controller";
        const char *path=resource_getPathWithExtension(name,JPB_RESOURCE_CONTROLLER_SECONDARY,"png");
        controlTextures[i]=_LoadTexture((char *)path,TT_FRONT,0);
        if(!controlTextures[i])return 0;
    }
    for(unsigned i=0;i<JPB_MENU_TEXTURE_ENTRY_COUNT;++i) {
        const JPBMenuTextureEntry *entry=&menuTextureList[i];
        if(entry->textureIndex<236 || entry->textureIndex>244)continue;
        menuTextures[entry->textureIndex]=_LoadTexture((char *)resource_getPath(entry->filename,JPB_RESOURCE_FRONT),TT_FRONT,0);
        if(!menuTextures[entry->textureIndex])return 0;
    }
    return 1;
}
