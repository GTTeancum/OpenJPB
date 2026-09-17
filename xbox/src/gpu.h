#ifndef OPENJPB_XBOX_GPU_H
#define OPENJPB_XBOX_GPU_H
#include "jpb/game_runtime.h"
/* 426:240 approximates 16:9 to within 0.2%; the Xbox 480p scanout is
   anamorphic 720x480, and 720p uses square pixels at 1280x720. */
#define JPB_XBOX_CANVAS_WIDTH 426
#define JPB_XBOX_CANVAS_HEIGHT 240
int jpb_XboxGpuInit(void);
const JPBSoftwareLevelMesh *jpb_XboxLoadLevel(const char *path);
void jpb_XboxLevelSetFrustum(const MATRIX *, float, float);
int jpb_XboxLevelChunkVisible(unsigned batch, unsigned first);
const uint16_t *jpb_XboxLevelIndices(unsigned batch);
int jpb_XboxLevelUsesPackedVertices(void);
void jpb_XboxLevelDecodeVertex(const JPBSoftwareLevelBatch *, unsigned,
    JPBSoftwareLevelVertex *);
int jpb_XboxLevelChunkNeedsDepthClip(unsigned batch, unsigned first);
void jpb_XboxGpuBegin(void);
void jpb_XboxGpuPresent(void);
void jpb_XboxGpuTransparencyProbe(void);
void jpb_XboxGpuStats(unsigned frame, unsigned elapsed, unsigned pad, int motion);
void jpb_XboxGpuProfile(const JPBGameRuntime *runtime);
int jpb_XboxGpuModelBegin(void *, JPBSoftwareFramebuffer *, JPBSoftwareDepthBuffer *);
int jpb_XboxGpuEffectsBegin(void *, JPBSoftwareFramebuffer *, JPBSoftwareDepthBuffer *);
int jpb_XboxGpuComposite(void *, enum JPBGameRuntimeGameplayCompositeStage,
    const JPBSoftwareFramebuffer *, JPBSoftwareRenderStats *);
int jpb_XboxGpuHudScreenDraws(void *, const JPBGameRuntimeScreenDraw *,
    size_t, JPBSoftwareFramebuffer *);
int jpb_XboxGpuHudTextDraws(void *, const JPBGameRuntimeTextDraw *,
    size_t, JPBSoftwareFramebuffer *);
int jpb_XboxGpuTriangle(void *, const JPBSoftwareMaterialVertex *,
    const JPBSoftwareMaterialVertex *, const JPBSoftwareMaterialVertex *,
    const JPBSoftwareTexture *);
int jpb_XboxGpuLevel(void *, const JPBSoftwareLevelMesh *, JPBLevelFbxMeshPass,
    const JPBSoftwareJpxScene *, MATRIX *, JPBSoftwareFramebuffer *, uint32_t,
    JPBSoftwareTextureResolver, void *, JPBSoftwareDepthBuffer *, JPBSoftwareRenderStats *);
#endif
