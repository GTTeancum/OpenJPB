#ifndef JPB_JPX_H
#define JPB_JPX_H

#include "jpb/material.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    JPB_JPX_MAX_MATERIALS = 1024,
    JPB_JPX_DESCRIPTOR_SIZE = 16,
    JPB_JPX_VERTEX_SIZE = 16,
    JPB_JPX_REFERENCE_WORLD_CAPACITY = 0x500000
};

enum JPBJpxResult {
    JPB_JPX_OK = 0,
    JPB_JPX_TRUNCATED = -1,
    JPB_JPX_INVALID_HEADER = -2,
    JPB_JPX_INVALID_MATERIAL = -3,
    JPB_JPX_INVALID_STRIP = -4,
    JPB_JPX_IO_ERROR = -5,
    JPB_JPX_STORAGE_TOO_SMALL = -6,
    JPB_JPX_MATERIAL_FAILED = -7,
    JPB_JPX_VISITOR_FAILED = -8
};

/*
 * PDB types MATHEAD (0x1306) and _BINHEADER (0x137F). _BINHEADER's trailing
 * array has room for 1,024 material definitions, but files serialize only
 * NumMats entries before the name table.
 */
typedef struct JPBJpxMaterialDef {
    int16_t nameOffset;
    int8_t listType;
    int8_t pad;
} JPBJpxMaterialDef;

typedef struct JPBJpxBinHeader {
    int16_t numMaterials;
    int16_t worldOffset;
    int16_t firstStripOffset;
    JPBJpxMaterialDef materialDefs[JPB_JPX_MAX_MATERIALS];
} JPBJpxBinHeader;

typedef struct JPBJpxDescriptor {
    uint8_t bytes[JPB_JPX_DESCRIPTOR_SIZE];
} JPBJpxDescriptor;

typedef struct JPBJpxView {
    uint8_t *data;
    size_t size;
    uint16_t numMaterials;
    uint16_t worldOffset;
    uint16_t firstStripOffset;
    size_t stripCount;
} JPBJpxView;

typedef int (*JPBJpxMaterialResolver)(
    const char *level_name,
    const char *material_name,
    int8_t list_type,
    JPBJpxDescriptor *descriptor,
    void *user_data);

typedef void (*JPBJpxProgressHook)(int amount, void *user_data);

typedef struct JPBJpxPatchSite {
    const uint8_t *data;
    size_t offset;
    uint32_t stride;
    uint32_t materialIndex;
    const uint8_t *vertexData;
    uint16_t vertexCount;
    uint16_t followingMetadataSize;
    int hasStripHeadMarker;
} JPBJpxPatchSite;

typedef int (*JPBJpxPatchSiteVisitor)(
    const JPBJpxPatchSite *site, void *user_data);

typedef struct JPBJpxVertex {
    const uint8_t *data;
    float x;
    float z;
    float y;
    int16_t rawU;
    int16_t rawV;
    float u;
    float v;
    uint32_t attributes;
} JPBJpxVertex;

typedef struct JPBJpxSpatialNode {
    const uint8_t *data;
    size_t offset;
    uint16_t index;
    float x;
    float z;
    float y;
    float radius;
    uint16_t vertexCount;
} JPBJpxSpatialNode;

typedef int (*JPBJpxSpatialNodeVisitor)(
    const JPBJpxSpatialNode *node, void *user_data);

typedef struct JPBJpxLoadConfig {
    void *storage;
    size_t storageCapacity;
    JPBJpxDescriptor *descriptors;
    size_t descriptorCapacity;
    const char *levelName;
    JPBJpxMaterialResolver resolveMaterial;
    JPBJpxPatchSiteVisitor visitPatchSite;
    JPBJpxSpatialNodeVisitor visitSpatialNode;
    JPBJpxProgressHook progress;
    void *userData;
} JPBJpxLoadConfig;

extern int32_t *WorldmeshData;
extern size_t gJpxWorldmeshSize;
extern _Material *level_materials[512];

int jpx_Inspect(void *data, size_t size, JPBJpxView *view);
const char *jpx_GetMaterialName(
    const JPBJpxView *view, uint16_t material_index);
int8_t jpx_GetMaterialListType(
    const JPBJpxView *view, uint16_t material_index);
int jpx_ForEachPatchSite(
    const JPBJpxView *view,
    JPBJpxPatchSiteVisitor visitor,
    void *user_data);
int jpx_DecodeVertex(
    const JPBJpxPatchSite *site,
    uint16_t vertex_index,
    JPBJpxVertex *vertex);
int jpx_ForEachStaticSpatialNode(
    const JPBJpxView *view,
    JPBJpxSpatialNodeVisitor visitor,
    void *user_data);
int jpx_PatchMaterials(
    const JPBJpxView *view,
    const JPBJpxDescriptor *descriptors,
    size_t descriptor_count);
int jpx_LoadFile(
    const char *path,
    const JPBJpxLoadConfig *config,
    JPBJpxView *view);
int InitJPX(char *name);

#ifdef __cplusplus
}
#endif

#endif
