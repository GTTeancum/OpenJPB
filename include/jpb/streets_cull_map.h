#ifndef JPB_STREETS_CULL_MAP_H
#define JPB_STREETS_CULL_MAP_H

#include "jpb/jpx.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Dependency-free correspondence between the shipped Streets JPX mirror and
 * the named live-FBX WallNN_Solid/WallNN_Broken meshes.
 */
int jpb_IsMatchedStreetsJpx(const JPBJpxView *view);
uint32_t jpb_StreetsJpxTriangleCullMask(
    const JPBJpxPatchSite *site, uint16_t triangle_index);
int jpb_StreetsJpxCullMaskVisible(uint32_t mask);

#ifdef __cplusplus
}
#endif

#endif

