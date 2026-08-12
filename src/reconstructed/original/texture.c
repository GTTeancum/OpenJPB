/*
 * REVIEWED RECONSTRUCTION of W:\\SWJediPowerBattles\\work\\texture.c.
 *
 * The PDB names, TT_TEXTYPE values, 800-entry g_material pool, allocation
 * order, and field initialization were checked against RVAs 0x101FB0-
 * 0x102162. GPU texture construction remains behind one opaque platform hook.
 */

#include "jpb/texture.h"

#include <stdio.h>
#include <string.h>

/* Exact PDB global and local names. */
_Material g_material[JPB_TEXTURE_MATERIAL_CAPACITY];
static char texname[256];

static JPBPlatformTextureLoadHook texture_load_hook;
static JPBTextureUnloadHook texture_unload_hook;
static void *texture_platform_user_data;

void jpb_TextureSetPlatformHooks(
    JPBPlatformTextureLoadHook load_hook,
    JPBTextureUnloadHook unload_hook,
    void *user_data)
{
    texture_load_hook = load_hook;
    texture_unload_hook = unload_hook;
    texture_platform_user_data = user_data;
}

int jpb_TextureHasLoadHook(void)
{
    return texture_load_hook != NULL;
}

void *jpb_TextureLoadPlatformResource(
    const char *filename,
    unsigned option,
    int material_type,
    int16_t *width,
    int16_t *height)
{
    if (texture_load_hook == NULL) {
        return NULL;
    }
    return texture_load_hook(
        texture_platform_user_data,
        filename,
        option,
        material_type,
        width,
        height);
}

void jpb_TextureUnloadPlatformResource(void *texture)
{
    if (texture != NULL && texture_unload_hook != NULL) {
        texture_unload_hook(
            texture_platform_user_data, texture);
    }
}

/* Reference RVA 0x101FB0, 164 bytes. */
void texture_Flush(unsigned mask)
{
    int i;

    for (i = JPB_TEXTURE_MATERIAL_CAPACITY - 1; i >= 0; --i) {
        if ((g_material[i].type & mask) != 0) {
            _FreeTexture(&g_material[i]);
            g_material[i].type = TT_FREE;
        }
    }
}

/* Reference RVA 0x102060, 19 bytes. */
void texture_FreeMaterial(_Material *material)
{
    if (material != NULL && material->type != TT_FREE) {
        material->type = TT_FREE;
    }
}

/* Reference RVA 0x102080, 134 bytes. */
_Material *texture_GetMaterial(TT_TEXTYPE type)
{
    int i;

    if (type == TT_FREE) {
        return NULL;
    }
    for (i = 0; i < JPB_TEXTURE_MATERIAL_CAPACITY; ++i) {
        _Material *material = &g_material[i];

        if (material->type == TT_FREE) {
            material->texture = NULL;
            material->type = (uint32_t)type;
            material->iw = 0;
            material->ih = 0;
            material->samplerType = TEXTURESAMPLER_LINEARCLAMP;
            material->colorOverride = -1;
            return material;
        }
    }
    return NULL;
}

/* Reference RVA 0x102110, 82 bytes. */
char *texture_Name(char *filename)
{
    char *source;
    char *destination = texname;
    char *dot = NULL;
    size_t remaining = sizeof(texname) - 1;

    if (filename == NULL) {
        texname[0] = '\0';
        return texname;
    }
    source = filename;
    while (*source != '\0' && remaining != 0) {
        *destination = *source;
        if (*source == '.') {
            dot = destination;
        }
        ++source;
        ++destination;
        --remaining;
    }
    *destination = '\0';
    if (dot != NULL && (size_t)(dot - texname) + 5 <= sizeof(texname)) {
        memcpy(dot + 1, "tga", sizeof("tga"));
    }
    return texname;
}
