#ifndef JPB_MATERIAL_H
#define JPB_MATERIAL_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Exact names and semantic fields from matched-PC PDB type 0x1207.
 * The explicit enum width remains the compiler's ordinary int width on both
 * the PC reconstruction and the later nxdk renderer.
 */
typedef enum TEXTURE_SAMPLE_TYPE {
    TEXTURESAMPLER_LINEARCLAMP = 0,
    /* PDB spelling retained, including its missing "E". */
    TEXTURSAMPLER_POINTCLAMP = 1,
    TEXTURESAMPLER_POINTCLAMP = TEXTURSAMPLER_POINTCLAMP
} TEXTURE_SAMPLE_TYPE;

/*
 * Descriptive names for every value assigned to _Material.flags by the
 * matched executable. No PDB enum survives for this field. NoScaleEndPoly
 * treats zero as the ordinary backface-rejected mode, one as two-sided, and
 * two as two-sided with the submitted depth replaced by exact 0.0001f.
 */
typedef enum JPBMaterialMode {
    JPB_MATERIAL_MODE_BACKFACE_REJECT = 0,
    JPB_MATERIAL_MODE_TWO_SIDED = 1,
    JPB_MATERIAL_MODE_SCREEN_TILE = 2
} JPBMaterialMode;

/*
 * Matched-PC PDB type 0x1212. This is a live renderer record rather than a
 * serialized asset record, so texture deliberately remains a native pointer.
 * The x64 layout assertions below document the reference executable layout;
 * a 32-bit nxdk build keeps the same fields and semantics with its native
 * pointer width.
 */
typedef struct _Material {
    void *texture;
    uint32_t type;
    int16_t iw;
    int16_t ih;
    int16_t ix;
    int16_t iy;
    char filename[256];
    int32_t m_isTransparent;
    uint32_t flags;
    TEXTURE_SAMPLE_TYPE samplerType;
    int32_t colorOverride;
} _Material;

#if UINTPTR_MAX == UINT64_MAX
#if defined(__cplusplus)
#define JPB_MATERIAL_STATIC_ASSERT static_assert
#else
#define JPB_MATERIAL_STATIC_ASSERT _Static_assert
#endif

JPB_MATERIAL_STATIC_ASSERT(
    sizeof(TEXTURE_SAMPLE_TYPE) == 4,
    "TEXTURE_SAMPLE_TYPE must match PDB type 0x1207");
JPB_MATERIAL_STATIC_ASSERT(
    sizeof(_Material) == 296,
    "_Material must match matched-PC PDB type 0x1212");
JPB_MATERIAL_STATIC_ASSERT(
    offsetof(_Material, filename) == 20,
    "_Material.filename layout changed");
JPB_MATERIAL_STATIC_ASSERT(
    offsetof(_Material, flags) == 280,
    "_Material.flags layout changed");
JPB_MATERIAL_STATIC_ASSERT(
    offsetof(_Material, samplerType) == 284,
    "_Material.samplerType layout changed");
JPB_MATERIAL_STATIC_ASSERT(
    offsetof(_Material, colorOverride) == 288,
    "_Material.colorOverride layout changed");

#undef JPB_MATERIAL_STATIC_ASSERT
#endif

#ifdef __cplusplus
}
#endif

#endif
