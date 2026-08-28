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

typedef struct primRendPacket {
    geomData *pGeomData;
    int32_t ZBufferOffset;
    int32_t ambientLightR;
    int32_t ambientLightG;
    int32_t ambientLightB;
    FMATRIX m3LocalMatrix;
    FMATRIX m3LightMatrix;
    uint32_t flags;
    int32_t ID;
    int32_t modelID;
} primRendPacket;

enum {
    JPB_RENDER_MATRIX_STACK_CAPACITY = 16,
    JPB_RENDER_PACKET_CAPACITY = 0x200,
    /* Retail accepts index 0x200 despite the PDB array ending at 0x1ff. */
    JPB_RENDER_PACKET_WRITE_LIMIT = 0x200
};

typedef struct SramModelStack {
    FMATRIX mm3MatrixArray[JPB_RENDER_MATRIX_STACK_CAPACITY];
    int32_t index;
    int32_t mMatrixLevel;
    sceneObject *pCurrentSceneModel;
    FMATRIX *m3LocalModelMatrix;
    modelObject *pRoot;
    _animFrame *pAnimFrame;
    FVECTOR mv3TempPivot;
    VECTOR scale;
    Mnode *pNode;
    FMATRIX mm3TempPSXMatrix;
    MATRIX m3SceneMatrix;
    _svector v3ViewPosition;
    _svector v3temp;
    VECTOR v3temp1;
    VECTOR v3temp2;
    VECTOR mCameraLocation;
    VECTOR mAmbient;
    int32_t node_flag;
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
    offsetof(primRendPacket, ZBufferOffset) == 8,
    "primRendPacket.ZBufferOffset layout changed");
JPB_RENDER_NODES_STATIC_ASSERT(
    offsetof(primRendPacket, m3LocalMatrix) == 24,
    "primRendPacket.m3LocalMatrix layout changed");
JPB_RENDER_NODES_STATIC_ASSERT(
    offsetof(primRendPacket, m3LightMatrix) == 72,
    "primRendPacket.m3LightMatrix layout changed");
JPB_RENDER_NODES_STATIC_ASSERT(
    offsetof(primRendPacket, flags) == 120,
    "primRendPacket.flags layout changed");
JPB_RENDER_NODES_STATIC_ASSERT(
    offsetof(primRendPacket, modelID) == 128,
    "primRendPacket.modelID layout changed");
JPB_RENDER_NODES_STATIC_ASSERT(
    sizeof(SramModelStack) == 0x408,
    "SramModelStack must match PDB type 0x73A8");
JPB_RENDER_NODES_STATIC_ASSERT(
    offsetof(SramModelStack, index) == 0x300,
    "SramModelStack.index layout changed");
JPB_RENDER_NODES_STATIC_ASSERT(
    offsetof(SramModelStack, pCurrentSceneModel) == 0x308,
    "SramModelStack.pCurrentSceneModel layout changed");
JPB_RENDER_NODES_STATIC_ASSERT(
    offsetof(SramModelStack, mv3TempPivot) == 0x328,
    "SramModelStack.mv3TempPivot layout changed");
JPB_RENDER_NODES_STATIC_ASSERT(
    offsetof(SramModelStack, scale) == 0x334,
    "SramModelStack.scale layout changed");
JPB_RENDER_NODES_STATIC_ASSERT(
    offsetof(SramModelStack, pNode) == 0x348,
    "SramModelStack.pNode layout changed");
JPB_RENDER_NODES_STATIC_ASSERT(
    offsetof(SramModelStack, m3SceneMatrix) == 0x380,
    "SramModelStack.m3SceneMatrix layout changed");
JPB_RENDER_NODES_STATIC_ASSERT(
    offsetof(SramModelStack, v3ViewPosition) == 0x3b0,
    "SramModelStack.v3ViewPosition layout changed");
JPB_RENDER_NODES_STATIC_ASSERT(
    offsetof(SramModelStack, mCameraLocation) == 0x3e0,
    "SramModelStack.mCameraLocation layout changed");
JPB_RENDER_NODES_STATIC_ASSERT(
    offsetof(SramModelStack, node_flag) == 0x400,
    "SramModelStack.node_flag layout changed");

#undef JPB_RENDER_NODES_STATIC_ASSERT
#endif

#ifdef __cplusplus
}
#endif

#endif
