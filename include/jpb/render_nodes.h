#ifndef JPB_RENDER_NODES_H
#define JPB_RENDER_NODES_H

#include "jpb/bmd.h"
#include "jpb/fmath.h"
#include "jpb/scene.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Exact matched-PC PDB type name used by nodes.c. The named fields are the
 * members established by _RenderPackets and render_CreateRenderPacket;
 * the two reserved regions have no observed reader in the retail renderer.
 */
typedef struct primRendPacket {
    geomData *geometry;
    uint8_t reserved0[16];
    FMATRIX modelMatrix;
    FMATRIX lightMatrix;
    uint8_t reserved1[8];
    int32_t sceneObjectIndex;
    uint32_t reserved2;
} primRendPacket;

enum {
    JPB_RENDER_MATRIX_STACK_CAPACITY = 16,
    /* render_CreateRenderPacket accepts indices zero through 0x200. */
    JPB_RENDER_PACKET_CAPACITY = 0x201
};

/*
 * Exact matched-PC PDB type name and 0x408-byte layout. Named members are
 * established by the reviewed nodes.c procedures. The zero-initialized tail
 * fields have no identified semantic consumer beyond the retail otag stub.
 */
typedef struct SramModelStack {
    FMATRIX matrixStack[JPB_RENDER_MATRIX_STACK_CAPACITY];
    int32_t sceneObjectIndex;
    int32_t matrixStackLevel;
    sceneObject *scene;
    FMATRIX *matrix;
    modelObject *model;
    _animFrame *keyFrame;
    FVECTOR transformedTranslation;
    uint8_t reserved0[20];
    Mnode *node;
    FMATRIX nodeMatrix;
    MATRIX sceneMatrix;
    _svector scenePosition;
    uint8_t reserved1[40];
    VECTOR otagCameraLocation;
    uint8_t reserved2[24];
} SramModelStack;

extern int32_t mCurRendPacket;
extern primRendPacket gRendPacket[JPB_RENDER_PACKET_CAPACITY];
extern int32_t modelsclipped;
extern int32_t drawme;
extern int32_t screenborders[2][5];
extern SramModelStack *sr;

int _RenderNode(
    geomData *pGeomData,
    FMATRIX *matrix,
    FMATRIX *light,
    FVECTOR *aPointCache);
void _RenderPackets(primRendPacket *pCurrentPacket, int num);
void render_CreateRenderPacket(FMATRIX *m3LocalMatrix);
void render_RenderModel(void);
void render_RenderNode(Mnode *pNode);
void render_RenderScene(void);

#if UINTPTR_MAX == UINT64_MAX
#if defined(__cplusplus)
#define JPB_RENDER_NODES_STATIC_ASSERT static_assert
#else
#define JPB_RENDER_NODES_STATIC_ASSERT _Static_assert
#endif

JPB_RENDER_NODES_STATIC_ASSERT(
    sizeof(primRendPacket) == 136,
    "primRendPacket must match the 0x88-byte retail stride");
JPB_RENDER_NODES_STATIC_ASSERT(
    offsetof(primRendPacket, modelMatrix) == 24,
    "primRendPacket.modelMatrix layout changed");
JPB_RENDER_NODES_STATIC_ASSERT(
    offsetof(primRendPacket, lightMatrix) == 72,
    "primRendPacket.lightMatrix layout changed");
JPB_RENDER_NODES_STATIC_ASSERT(
    offsetof(primRendPacket, sceneObjectIndex) == 128,
    "primRendPacket.sceneObjectIndex layout changed");
JPB_RENDER_NODES_STATIC_ASSERT(
    sizeof(SramModelStack) == 0x408,
    "SramModelStack must match PDB type 0x73A8");
JPB_RENDER_NODES_STATIC_ASSERT(
    offsetof(SramModelStack, sceneObjectIndex) == 0x300,
    "SramModelStack.sceneObjectIndex layout changed");
JPB_RENDER_NODES_STATIC_ASSERT(
    offsetof(SramModelStack, scene) == 0x308,
    "SramModelStack.scene layout changed");
JPB_RENDER_NODES_STATIC_ASSERT(
    offsetof(SramModelStack, transformedTranslation) == 0x328,
    "SramModelStack.transformedTranslation layout changed");
JPB_RENDER_NODES_STATIC_ASSERT(
    offsetof(SramModelStack, node) == 0x348,
    "SramModelStack.node layout changed");
JPB_RENDER_NODES_STATIC_ASSERT(
    offsetof(SramModelStack, sceneMatrix) == 0x380,
    "SramModelStack.sceneMatrix layout changed");
JPB_RENDER_NODES_STATIC_ASSERT(
    offsetof(SramModelStack, scenePosition) == 0x3b0,
    "SramModelStack.scenePosition layout changed");
JPB_RENDER_NODES_STATIC_ASSERT(
    offsetof(SramModelStack, otagCameraLocation) == 0x3e0,
    "SramModelStack.otagCameraLocation layout changed");

#undef JPB_RENDER_NODES_STATIC_ASSERT
#endif

#ifdef __cplusplus
}
#endif

#endif
