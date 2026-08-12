#ifndef JPB_TEXTURE_H
#define JPB_TEXTURE_H

#include "jpb/material.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Exact matched-PC PDB enum type 0x114D. */
typedef enum TT_TEXTYPE {
    TT_FREE = 0,
    TT_SPRITE = 1,
    TT_PLAYERCHAR = 2,
    TT_LEVEL = 4,
    TT_NONPLAYERCHAR = 8,
    TT_FRONT = 16,
    TT_MISC = 32,
    TT_DEBUG = 64,
    TT_FRONT_TITLE = 128,
    TT_FRONT_PLAYER = 256,
    TT_FRONT_LEVEL = 512,
    TT_FRONT_SAVE = 1024,
    TT_FRONT_LOAD = 2048,
    TT_INGAME = 4096,
    TT_ANY = -1
} TT_TEXTYPE;

enum { JPB_TEXTURE_MATERIAL_CAPACITY = 800 };

extern _Material g_material[JPB_TEXTURE_MATERIAL_CAPACITY];

/*
 * Platform realization seam for el_chavo::LoadTexture/Texture::Restore.
 * The original owner continues to allocate, cache, and describe _Material;
 * a PC or later platform renderer supplies only the opaque pixel resource.
 */
typedef void *(*JPBPlatformTextureLoadHook)(
    void *user_data,
    const char *filename,
    unsigned option,
    int material_type,
    int16_t *width,
    int16_t *height);
typedef void (*JPBTextureUnloadHook)(
    void *user_data, void *texture);

void jpb_TextureSetPlatformHooks(
    JPBPlatformTextureLoadHook load_hook,
    JPBTextureUnloadHook unload_hook,
    void *user_data);
int jpb_TextureHasLoadHook(void);
void *jpb_TextureLoadPlatformResource(
    const char *filename,
    unsigned option,
    int material_type,
    int16_t *width,
    int16_t *height);
void jpb_TextureUnloadPlatformResource(void *texture);

void texture_Flush(unsigned mask);
void texture_FreeMaterial(_Material *material);
_Material *texture_GetMaterial(TT_TEXTYPE type);
char *texture_Name(char *filename);

_Material *_LoadTexture(
    char *filename, TT_TEXTYPE texturetype, unsigned long option);
_Material *_TryLoadTexture(
    const char *baseFileName,
    TT_TEXTYPE texturetype,
    unsigned long option);
void _FreeTexture(_Material *texture);

/* Reviewed basename policy used by SpriteAtlasTextureDatabase.cpp. */
int jpb_TextureIsPartOfAtlas(const char *filename);

#ifdef __cplusplus
}
#endif

#endif
