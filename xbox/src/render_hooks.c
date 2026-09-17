/* Xbox platform realization of the shared capture hooks. Game logic is shared.
 * Payload contracts mirror original/wHook.cpp; D3D12/Steam are not linked. */
#include "jpb/whook.h"
#include "jpb/world.h"
#include "jpb/texture.h"
#include "jpb/resources.h"
#include "jpb/generic_hook.h"
#include "gpu.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <hal/debug.h>

#define HOOK(Name,Type) static Type Name; static void *Name##_user; \
void jpb_WHookSet##Name(Type h,void*u){Name=h;Name##_user=u;}
HOOK(DrawTextureHook,JPBDrawTextureHook)
HOOK(DrawTextureClippedHook,JPBDrawTextureClippedHook)
HOOK(DrawUITextUTF16Hook,JPBDrawUITextUTF16Hook)
HOOK(DrawUITextUTF163DHook,JPBDrawUITextUTF163DHook)
HOOK(DebugSphereHook,JPBDebugSphereHook)
HOOK(ClearWindowHook,JPBClearWindowHook)
HOOK(RenderLoadHook,JPBRenderLoadHook)
HOOK(GetWindowSizeHook,JPBGetWindowSizeHook)
HOOK(ScreenPolyHook,JPBScreenPolyHook)
HOOK(InitFBXLevelDataHook,JPBInitFBXLevelDataHook)

static JPBLevelTransformation transform;
static JPBScreenPolyVertex vertices[4];
static _Material *poly_material;
static uint32_t poly_flags;
static int vertex_count;
_Material *whitemat, *whitematAdd;
int refreshFontAtlasFlag;


void _DrawTexture(_Material*t,SCREENRECT d,const SCREENRECT*s,CVECTOR c,float z){if(DrawTextureHook)DrawTextureHook(DrawTextureHook_user,t,&d,s,c,z);}
void _DrawTextureClipped(_Material*t,SCREENRECT d,const SCREENRECT*s,CVECTOR c,float z,SCREENRECT clip){if(DrawTextureClippedHook)DrawTextureClippedHook(DrawTextureClippedHook_user,t,&d,s,c,z,&clip);}
void _DrawUITextUTF16(uint16_t*t,SCREENRECT d,int f,int p,CVECTOR c){if(DrawUITextUTF16Hook)DrawUITextUTF16Hook(DrawUITextUTF16Hook_user,t,&d,f,p,c,0,0);}
void _DrawUITextUTF16Depth(uint16_t*t,SCREENRECT d,int f,int p,CVECTOR c,float z){if(DrawUITextUTF16Hook)DrawUITextUTF16Hook(DrawUITextUTF16Hook_user,t,&d,f,p,c,1,z);}
void _DrawUITextUTF16_3D(uint16_t*t,float x,float y,float z,int f,int p,uint32_t c){if(DrawUITextUTF163DHook)DrawUITextUTF163DHook(DrawUITextUTF163DHook_user,t,x,y,z,f,p,c);}
void GetWindowSize(int*w,int*h){*w=JPB_XBOX_CANVAS_WIDTH;*h=JPB_XBOX_CANVAS_HEIGHT;if(GetWindowSizeHook)GetWindowSizeHook(w,h,GetWindowSizeHook_user);}
void ClearWindow(void){if(ClearWindowHook)ClearWindowHook(ClearWindowHook_user);}
void __RenderLoad(int endframe){if(RenderLoadHook)RenderLoadHook(RenderLoadHook_user);}
void _StartPoly(int n,_Material*m){poly_material=m;poly_flags=m?m->flags:0;vertex_count=n;}
void _SetVert(int i,float x,float y,float z,unsigned long c,float u,float v){if(i>=0&&i<4&&i<vertex_count){JPBScreenPolyVertex a={x,y,z,(uint32_t)c,u,v};vertices[i]=a;}}
static void end_poly(int no_scale){if(ScreenPolyHook&&poly_material&&vertex_count>0&&vertex_count<=4)ScreenPolyHook(ScreenPolyHook_user,poly_material,poly_flags,vertex_count,vertices,no_scale);}
void _EndPoly(void){end_poly(0);}
void _NoScaleEndPoly(void){end_poly(1);}
void _ApplyLevelTransformation(MATRIX*m,float x,float y,float z){int r,c;memset(&transform,0,sizeof(transform));for(r=0;r<3;r++)for(c=0;c<3;c++)transform.world[r][c]=m->m[c][r];for(c=0;c<3;c++)transform.world[3][c]=(float)m->t[c];transform.world[3][3]=1;transform.scale[0]=x;transform.scale[1]=y;transform.scale[2]=z;transform.scale[3]=1;}
const JPBLevelTransformation *jpb_WHookLevelTransformation(void){return &transform;}
void _InitFBXLevelData(ufbx_scene*s){if(InitFBXLevelDataHook)InitFBXLevelDataHook(InitFBXLevelDataHook_user,s);}

_Material *_LoadTexture(char *filename,TT_TEXTYPE type,unsigned long option){
    int i,material_type=0;int16_t w=0,h=0;const char *base;_Material*m;char path[256];
    if(!filename)filename=(char*)resource_getPath("white.png",JPB_RESOURCE_DEFAULT);
    if(!filename)return NULL;
    for(i=0;i<JPB_TEXTURE_MATERIAL_CAPACITY;i++)if(g_material[i].type!=TT_FREE&&g_material[i].texture&&strcmp(g_material[i].filename,filename)==0)return &g_material[i];
    m=texture_GetMaterial(type);if(!m)return NULL;
    snprintf(m->filename,sizeof(m->filename),"%s",filename);
    snprintf(path,sizeof(path),"%s",filename);base=path;
    for(i=0;path[i];i++)if(path[i]=='\\'||path[i]=='/')base=path+i+1;
    if(base[0]=='a'&&base[1]=='_')material_type=2;else if(base[0]=='p'&&base[1]=='_')material_type=1;
    {char *dot=strrchr(path,'.');if(dot&&(!strcmp(dot,".pvr")||!strcmp(dot,".PVR")))strcpy(dot,".tga");}
    m->texture=jpb_TextureLoadPlatformResource(path,option&(0x02000000u|0xffu),material_type,&w,&h);
    if(!m->texture){debugPrint("Missing texture: %s\n",path);texture_FreeMaterial(m);return NULL;}
    m->iw=w;m->ih=h;m->samplerType=jpb_TextureIsPartOfAtlas(filename)?TEXTURSAMPLER_POINTCLAMP:TEXTURESAMPLER_LINEARCLAMP;
    SetTextureColorOverride((int)(int8_t)LevelSelect,m);return m;
}
void _FreeTexture(_Material*m){texture_FreeMaterial(m);}
void _ClearTextureCache(void){} /* g_material owns the cache, no second map. */
void _StoreDescriptorHeapOffsetsEnd(void){}
void _StoreDescriptorHeapOffsetsStart(void){}
void texture_GarbageCollect(void){}
void whook_RestoreTextures(void){}
void CleanupLevelData(void){}
void MarkFontAtlasForRefresh(void){refreshFontAtlasFlag=1;}
void RefreshFontAtlas(void){refreshFontAtlasFlag=0;}
void SetInMenu(int v){GameStruct.inMenuFlag=v;}
void debug_drawsphere(int x,int y,int z,int r,uint32_t c){if(DebugSphereHook)DebugSphereHook(DebugSphereHook_user,x,y,z,r,c);}
int seecull(FVECTOR4*p,FVECTOR4*q){return 0;} /* Retail implementation is inert. */
void __PCTrace(char *fmt,...){char s[1024];va_list a;va_start(a,fmt);vsnprintf(s,sizeof(s),fmt,a);va_end(a);debugPrint("%s",s);}
