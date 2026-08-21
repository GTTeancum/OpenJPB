#ifndef JPB_PC_PRESENT_D3D11_H
#define JPB_PC_PRESENT_D3D11_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stddef.h>
#include <stdint.h>

typedef struct JPBSoftwareFramebuffer JPBSoftwareFramebuffer;
typedef struct JPBSoftwareLevelMesh JPBSoftwareLevelMesh;
typedef struct JPBSoftwareJpxScene JPBSoftwareJpxScene;
typedef struct JPBSoftwareTexture JPBSoftwareTexture;
typedef struct JPBSoftwareDepthBuffer JPBSoftwareDepthBuffer;
typedef struct JPBSoftwareRenderStats JPBSoftwareRenderStats;
typedef struct JPBSoftwareMaterialVertex JPBSoftwareMaterialVertex;
typedef struct JPBGameRuntimeScreenDraw JPBGameRuntimeScreenDraw;
typedef struct MATRIX MATRIX;
typedef int (*JPBPCPresentTextureResolver)(
    void *user_data,
    const char *texture_name,
    JPBSoftwareTexture *texture);

typedef struct JPBPCD3D11Presenter JPBPCD3D11Presenter;

JPBPCD3D11Presenter *jpb_PCD3D11PresenterCreate(HWND window);
void jpb_PCD3D11PresenterDestroy(JPBPCD3D11Presenter *presenter);
int jpb_PCD3D11PresenterPresent(
    JPBPCD3D11Presenter *presenter,
    const JPBSoftwareFramebuffer *framebuffer);
const char *jpb_PCD3D11PresenterDescription(
    const JPBPCD3D11Presenter *presenter);
long jpb_PCD3D11PresenterLastError(
    const JPBPCD3D11Presenter *presenter);
int jpb_PCD3D11PresenterRenderLevel(
    JPBPCD3D11Presenter *presenter,
    const JPBSoftwareLevelMesh *mesh,
    const JPBSoftwareJpxScene *world_scene,
    MATRIX *view_matrix,
    JPBSoftwareFramebuffer *framebuffer,
    uint32_t clear_color,
    JPBPCPresentTextureResolver resolve_texture,
    void *texture_user_data,
    JPBSoftwareDepthBuffer *depth_buffer,
    JPBSoftwareRenderStats *stats);
int jpb_PCD3D11PresenterRenderLevelPass(
    JPBPCD3D11Presenter *presenter,
    const JPBSoftwareLevelMesh *mesh,
    JPBLevelFbxMeshPass pass,
    const JPBSoftwareJpxScene *world_scene,
    MATRIX *view_matrix,
    JPBSoftwareFramebuffer *framebuffer,
    uint32_t clear_color,
    JPBPCPresentTextureResolver resolve_texture,
    void *texture_user_data,
    JPBSoftwareDepthBuffer *depth_buffer,
    JPBSoftwareRenderStats *stats);
void jpb_PCD3D11PresenterWorldTiming(
    const JPBPCD3D11Presenter *presenter,
    unsigned *frames,
    double *prepare_seconds,
    double *submit_seconds,
    double *readback_seconds);
void jpb_PCD3D11PresenterModelTiming(
    const JPBPCD3D11Presenter *presenter,
    unsigned *frames,
    double *upload_seconds,
    double *submit_seconds,
    double *color_readback_seconds,
    double *depth_readback_seconds);
void jpb_PCD3D11PresenterTitleTiming(
    const JPBPCD3D11Presenter *presenter,
    unsigned *frames,
    double *prepare_seconds,
    double *submit_seconds,
    double *readback_seconds);
int jpb_PCD3D11PresenterRenderTitleScreenDraws(
    JPBPCD3D11Presenter *presenter,
    const JPBGameRuntimeScreenDraw *draws,
    size_t draw_count,
    JPBSoftwareFramebuffer *framebuffer);
int jpb_PCD3D11PresenterBeginModels(
    JPBPCD3D11Presenter *presenter,
    JPBSoftwareFramebuffer *framebuffer,
    JPBSoftwareDepthBuffer *depth_buffer);
int jpb_PCD3D11PresenterModelTriangle(
    void *presenter,
    const JPBSoftwareMaterialVertex *first,
    const JPBSoftwareMaterialVertex *second,
    const JPBSoftwareMaterialVertex *third,
    const JPBSoftwareTexture *texture);
int jpb_PCD3D11PresenterBeginScreenPolys(
    JPBPCD3D11Presenter *presenter,
    JPBSoftwareFramebuffer *framebuffer,
    JPBSoftwareDepthBuffer *depth_buffer);
int jpb_PCD3D11PresenterScreenPolyTriangle(
    void *presenter,
    const JPBSoftwareMaterialVertex *first,
    const JPBSoftwareMaterialVertex *second,
    const JPBSoftwareMaterialVertex *third,
    const JPBSoftwareTexture *texture);
int jpb_PCD3D11PresenterEndScreenPolys(
    JPBPCD3D11Presenter *presenter,
    JPBSoftwareFramebuffer *framebuffer,
    JPBSoftwareDepthBuffer *depth_buffer);
int jpb_PCD3D11PresenterEndModels(
    JPBPCD3D11Presenter *presenter,
    JPBSoftwareFramebuffer *framebuffer,
    JPBSoftwareDepthBuffer *depth_buffer);
int jpb_PCD3D11PresenterGameplayComposite(
    JPBPCD3D11Presenter *presenter,
    int stage,
    const JPBSoftwareFramebuffer *framebuffer,
    JPBSoftwareRenderStats *stats);
int jpb_PCD3D11PresenterReadbackGameplay(
    JPBPCD3D11Presenter *presenter,
    JPBSoftwareFramebuffer *framebuffer);
int jpb_PCD3D11PresenterFinalWorldCoverage(
    JPBPCD3D11Presenter *presenter,
    size_t *covered_pixels);
int jpb_PCD3D11PresenterPrewarmLevel(
    JPBPCD3D11Presenter *presenter,
    const JPBSoftwareLevelMesh *mesh,
    JPBPCPresentTextureResolver resolve_texture,
    void *texture_user_data);

#endif
