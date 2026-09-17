/* Static nxdk SDL_ttf realization of the shared text dispatch table.
   nxdk ships SDL_ttf 2.0.14, so size changes reopen the same font face. */
#include <SDL.h>
#include <SDL_ttf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern FILE *jpb_XboxFopen(const char *,const char *);
typedef struct XboxFace { TTF_Font *font; int size; unsigned used; } XboxFace;
typedef struct XboxFont {
    TTF_Font *font; char path[512]; int size;
    unsigned clock; XboxFace faces[6];
} XboxFont;
static TTF_Font *open_face(const char *path,int size)
{
    FILE *file=jpb_XboxFopen(path,"rb");
    if(!file)return NULL;
    SDL_RWops *rw=SDL_RWFromFP(file,SDL_TRUE);
    if(!rw){fclose(file);return NULL;}
    return TTF_OpenFontRW(rw,1,size);
}
static void *open_font(const char *path,int size)
{
    if(strlen(path)>=512)return NULL;
    XboxFont *owner=calloc(1,sizeof(*owner));
    if(!owner)return NULL;
    owner->font=open_face(path,size);
    if(!owner->font){free(owner);return NULL;}
    strcpy(owner->path,path);owner->size=size;
    owner->faces[0]=(XboxFace){owner->font,size,++owner->clock};
    return owner;
}
static int set_size(XboxFont *owner,int size)
{
    if(owner->size==size)return 0;
    unsigned slot=0;
    for(unsigned i=0;i<6;++i) {
        if(owner->faces[i].font && owner->faces[i].size==size) {
            owner->font=owner->faces[i].font;owner->size=size;
            owner->faces[i].used=++owner->clock;
            return 0;
        }
        if(owner->faces[i].used<owner->faces[slot].used)slot=i;
    }
    TTF_Font *next=open_face(owner->path,size);
    if(!next)return -1;
    if(owner->faces[slot].font)TTF_CloseFont(owner->faces[slot].font);
    owner->faces[slot]=(XboxFace){next,size,++owner->clock};
    owner->font=next;owner->size=size;
    return 0;
}
static int metrics(XboxFont *owner,Uint16 ch,int *a,int *b,int *c,int *d,int *e)
{return TTF_GlyphMetrics(owner->font,ch,a,b,c,d,e);}
static SDL_Surface *render(XboxFont *owner,Uint16 ch,SDL_Color color)
{return TTF_RenderGlyph_Blended(owner->font,ch,color);}
static int ascent(XboxFont *owner){return TTF_FontAscent(owner->font);}
static void close_font(XboxFont *owner)
{for(unsigned i=0;i<6;++i)if(owner->faces[i].font)TTF_CloseFont(owner->faces[i].font);free(owner);}
void *jpb_StaticTextGetSymbol(const char *name)
{
#define SYMBOL(n,fn) if(!strcmp(name,n))return (void *)(fn)
    SYMBOL("TTF_Init",TTF_Init);
    SYMBOL("TTF_OpenFont",open_font);
    SYMBOL("TTF_SetFontSize",set_size);
    SYMBOL("TTF_GlyphMetrics",metrics);
    SYMBOL("TTF_RenderGlyph_Blended",render);
    SYMBOL("TTF_FontAscent",ascent);
    SYMBOL("TTF_CloseFont",close_font);
    SYMBOL("SDL_LockSurface",SDL_LockSurface);
    SYMBOL("SDL_UnlockSurface",SDL_UnlockSurface);
    SYMBOL("SDL_FreeSurface",SDL_FreeSurface);
    SYMBOL("SDL_GetError",SDL_GetError);
    return NULL;
}
