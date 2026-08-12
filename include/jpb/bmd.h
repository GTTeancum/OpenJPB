#ifndef JPB_BMD_H
#define JPB_BMD_H

#include "jpb/model.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    JPB_BMD_REFERENCE_CAPACITY = 0x40000,
    JPB_BMD_STORED_CHILD_CAPACITY = 7,
    /*
     * worm.bmd has eight children: model_MakeNode walks one element beyond
     * aChildren[7] and consumes the adjacent pVertNormals word as child 8.
     */
    JPB_BMD_CHILD_CAPACITY = 8,
    JPB_BMD_NODE_CAPACITY = JPB_MODEL_NODE_CAPACITY
};

/*
 * Exact matched-PC PDB type 0x11F9. On disk, all references are 32-bit
 * payload offsets or record indices and the record is pointer-free. Exact
 * model_MakeNode mutates the five geometry-stream fields into addPtr indices.
 */
typedef struct geomData {
    char name[32];
    uint32_t id;
    _svector trans;
    int32_t numFaces;
    int32_t numVerts;
    int32_t numShareVerts;
    int32_t pVertex;
    int32_t pNormal;
    int32_t pUV;
    int32_t pColor;
    union {
        char Texture[32];
        uint64_t TextureID;
    } t;
    int32_t pIndex;
    int32_t numChildren;
    int32_t aChildren[JPB_BMD_STORED_CHILD_CAPACITY];
    int32_t pVertNormals;
} geomData;

typedef enum JPBBmdResult {
    JPB_BMD_OK = 0,
    JPB_BMD_INVALID_ARGUMENT = -1,
    JPB_BMD_TRUNCATED = -2,
    JPB_BMD_INVALID_SIZE = -3,
    JPB_BMD_INVALID_LAYOUT = -4,
    JPB_BMD_IO_ERROR = -5,
    JPB_BMD_STORAGE_TOO_SMALL = -6,
    JPB_BMD_NODE_STORAGE_TOO_SMALL = -7
} JPBBmdResult;

typedef struct JPBBmdView {
    uint8_t *file_data;
    size_t file_size;
    uint8_t *payload;
    uint32_t payload_size;
    geomData *root;
    size_t node_count;
    /* model_MakeNode converted the five stream offsets through addPtr. */
    int geometry_streams_relocated;
    /* model_MakeNode replaced t.Texture with the retail _Material pointer. */
    int material_handles_relocated;
} JPBBmdView;

/*
 * Descriptive, data-backed views of the pointer-free arrays referenced by
 * geomData. These names are portable additions; the field encoding is
 * checked against _RenderNode and all installed BMD archives.
 */
typedef struct JPBBmdFaceRecord {
    int16_t vertex[4];
} JPBBmdFaceRecord;

typedef struct JPBBmdPackedFaceRecord {
    uint8_t vertex[4];
} JPBBmdPackedFaceRecord;

typedef enum JPBBmdFaceEncoding {
    JPB_BMD_FACE_SIGNED_16 = 0,
    JPB_BMD_FACE_PACKED_8 = 1
} JPBBmdFaceEncoding;

/* Exact matched-PC PDB types 0x705C and 0x709C. */
typedef struct pairUV {
    float u;
    float v;
} pairUV;

typedef struct faceUV {
    pairUV uv[4];
} faceUV;

typedef struct JPBBmdGeometryView {
    const geomData *geometry;
    const uint32_t *packed_vertices;
    size_t shared_vertex_count;
    /* _RenderNode expands three packed vertices per geomData.numVerts unit. */
    size_t local_vertex_count;
    size_t total_vertex_count;
    JPBBmdFaceEncoding face_encoding;
    const JPBBmdFaceRecord *faces;
    const JPBBmdPackedFaceRecord *packed_faces;
    size_t face_count;
    const faceUV *face_uvs;
    const CVECTOR *colors;
    const uint32_t *packed_normals;
    size_t corner_count;
} JPBBmdGeometryView;

JPBBmdResult jpb_BmdInspect(
    void *buffer, size_t buffer_size, JPBBmdView *view);
JPBBmdResult jpb_BmdLoadFile(
    const char *path,
    void *storage,
    size_t storage_capacity,
    JPBBmdView *view);
JPBBmdResult jpb_BmdBuildModel(
    const JPBBmdView *view,
    modelObject *model,
    Mnode *nodes,
    size_t node_capacity,
    int collision_player);
JPBBmdResult jpb_BmdGetGeometry(
    const JPBBmdView *view,
    const geomData *geometry,
    JPBBmdGeometryView *geometry_view);
void jpb_BmdDecodePackedVertex(
    uint32_t packed_vertex, FVECTOR *vertex);
size_t jpb_BmdFaceCornerCount(
    const JPBBmdGeometryView *geometry, size_t face);
int jpb_BmdFaceVertexIndex(
    const JPBBmdGeometryView *geometry,
    size_t face,
    size_t corner,
    size_t *vertex_index);
const pairUV *jpb_BmdFaceUv(
    const JPBBmdGeometryView *geometry,
    size_t face,
    size_t corner);
const CVECTOR *jpb_BmdFaceColor(
    const JPBBmdGeometryView *geometry,
    size_t face,
    size_t corner);
struct _Material *jpb_BmdGetMaterial(
    const JPBBmdView *view, const geomData *geometry);

#if defined(__cplusplus)
#define JPB_BMD_STATIC_ASSERT static_assert
#else
#define JPB_BMD_STATIC_ASSERT _Static_assert
#endif

JPB_BMD_STATIC_ASSERT(
    sizeof(geomData) == 144,
    "geomData must match PDB type 0x11F9");
JPB_BMD_STATIC_ASSERT(
    offsetof(geomData, id) == 32,
    "geomData.id layout changed");
JPB_BMD_STATIC_ASSERT(
    offsetof(geomData, t) == 72,
    "geomData.t layout changed");
JPB_BMD_STATIC_ASSERT(
    offsetof(geomData, aChildren) == 112,
    "geomData.aChildren layout changed");
JPB_BMD_STATIC_ASSERT(
    sizeof(JPBBmdFaceRecord) == 8,
    "BMD face records must remain eight bytes");
JPB_BMD_STATIC_ASSERT(
    sizeof(JPBBmdPackedFaceRecord) == 4,
    "packed BMD face records must remain four bytes");
JPB_BMD_STATIC_ASSERT(
    sizeof(pairUV) == 8,
    "pairUV must match PDB type 0x705C");
JPB_BMD_STATIC_ASSERT(
    sizeof(faceUV) == 32,
    "faceUV must match PDB type 0x709C");

#undef JPB_BMD_STATIC_ASSERT

#ifdef __cplusplus
}
#endif

#endif
