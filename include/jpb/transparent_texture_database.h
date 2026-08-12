#ifndef JPB_TRANSPARENT_TEXTURE_DATABASE_H
#define JPB_TRANSPARENT_TEXTURE_DATABASE_H

#ifdef __cplusplus
#include <string>

/* Exact PDB procedures from TransparentTextureDatabase.cpp. */
bool isGlassTexture(const std::string &textureName);
bool isTextureTransparent(
    const std::string &textureID, int level);

extern "C" {
#endif

/* Dependency-light adapters over the exact classifier data. */
int jpb_IsGlassTexture(const char *texture_name);
int jpb_IsTextureTransparent(
    const char *texture_id, int level);

/*
 * The legacy JPX mirror stores many material IDs as collision-resolved 8.3
 * names. These helpers match that representation to the exact FBX-era name
 * sets without changing the exact procedures above.
 */
int jpb_IsGlassTextureForJpxMirror(const char *material_name);
int jpb_IsTextureGlassForJpxMirror(
    const char *material_name, int level);
int jpb_IsTextureTransparentForJpxMirror(
    const char *material_name, int level);

#ifdef __cplusplus
}
#endif

#endif
