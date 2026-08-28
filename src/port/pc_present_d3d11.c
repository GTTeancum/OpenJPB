#define COBJMACROS
#include "jpb/game_runtime.h"
#include "jpb/level.h"
#include "jpb/projection.h"
#define NODE_INVALID MSXML_NODE_INVALID
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <dxgi.h>
#include <dxgi1_2.h>
#undef NODE_INVALID

#include "pc_present_d3d11.h"

#include <float.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct JPBPCD3D11Constants {
    float inverseSourceWidth;
    float inverseSourceHeight;
    float padding[2];
} JPBPCD3D11Constants;

typedef struct JPBPCD3D11WorldConstants {
    float row0[4];
    float row1[4];
    float row2[4];
    float projection[4];
    float transparentPass[4];
    float uvScrollSpeed[2];
    float padding[2];
} JPBPCD3D11WorldConstants;

typedef struct JPBPCD3D11WorldTexture {
    const uint32_t *sourcePixels;
    size_t width;
    size_t height;
    ID3D11Texture2D *texture;
    ID3D11ShaderResourceView *view;
} JPBPCD3D11WorldTexture;

typedef struct JPBPCD3D11ModelBatch {
    UINT firstVertex;
    UINT vertexCount;
    ID3D11ShaderResourceView *textureView;
    ID3D11SamplerState *sampler;
    int materialType;
} JPBPCD3D11ModelBatch;

enum { PC_WORLD_TEXTURE_CAPACITY = 512 };
enum { PC_GAMEPLAY_HUD_BUFFER_COUNT = 3 };

struct JPBPCD3D11Presenter {
    HWND window;
    ID3D11Device *device;
    ID3D11DeviceContext *context;
    IDXGISwapChain *swapChain;
    ID3D11RenderTargetView *renderTarget;
    ID3D11Texture2D *uploadTexture;
    ID3D11ShaderResourceView *uploadView;
    ID3D11VertexShader *vertexShader;
    ID3D11PixelShader *pixelShader;
    ID3D11SamplerState *sampler;
    ID3D11Buffer *constants;
    ID3D11VertexShader *worldVertexShader;
    ID3D11PixelShader *worldPixelShader;
    ID3D11InputLayout *worldInputLayout;
    ID3D11Buffer *worldConstants;
    ID3D11Buffer *worldVertexBuffer;
    ID3D11Texture2D *worldColor;
    ID3D11RenderTargetView *worldColorTarget;
    ID3D11ShaderResourceView *worldColorView;
    ID3D11Texture2D *worldColorReadback;
    ID3D11Texture2D *gameplayComposite;
    ID3D11RenderTargetView *gameplayCompositeTarget;
    ID3D11ShaderResourceView *gameplayCompositeView;
    ID3D11Texture2D *gameplayHud[PC_GAMEPLAY_HUD_BUFFER_COUNT][2];
    ID3D11ShaderResourceView *gameplayHudView[
        PC_GAMEPLAY_HUD_BUFFER_COUNT][2];
    int gameplayHudBuffer;
    ID3D11Texture2D *worldLinearDepth;
    ID3D11RenderTargetView *worldLinearDepthTarget;
    ID3D11ShaderResourceView *worldLinearDepthView;
    ID3D11Texture2D *worldLinearDepthSnapshot;
    ID3D11Texture2D *worldLinearDepthReadback;
    ID3D11Texture2D *worldDepth;
    ID3D11DepthStencilView *worldDepthTarget;
    ID3D11BlendState *worldOpaqueBlend;
    ID3D11BlendState *worldAlphaBlend;
    ID3D11BlendState *worldGlassBlend;
    ID3D11BlendState *worldAdditiveBlend;
    ID3D11DepthStencilState *worldDepthWrite;
    ID3D11DepthStencilState *worldDepthRead;
    ID3D11DepthStencilState *screenDepthRead;
    ID3D11RasterizerState *worldRasterizer;
    ID3D11RasterizerState *worldScissorRasterizer;
    ID3D11SamplerState *worldSampler;
    ID3D11ShaderResourceView *worldWhiteView;
    ID3D11VertexShader *modelVertexShader;
    ID3D11VertexShader *screenVertexShader;
    ID3D11PixelShader *modelPixelShader;
    ID3D11PixelShader *screenOpaquePixelShader;
    ID3D11PixelShader *screenTransparentPixelShader;
    ID3D11PixelShader *gameplayCompositePixelShader;
    ID3D11InputLayout *modelInputLayout;
    ID3D11InputLayout *screenInputLayout;
    ID3D11Buffer *modelConstants;
    ID3D11Buffer *modelVertexBuffer;
    ID3D11SamplerState *modelLinearSampler;
    ID3D11SamplerState *modelPointSampler;
    JPBSoftwareMaterialVertex *modelVertices;
    size_t modelVertexCount;
    size_t modelVertexCapacity;
    size_t modelGpuVertexCapacity;
    JPBPCD3D11ModelBatch *modelBatches;
    size_t modelBatchCount;
    size_t modelBatchCapacity;
    JPBPCD3D11WorldTexture worldTextures[PC_WORLD_TEXTURE_CAPACITY];
    size_t worldTextureCount;
    const JPBSoftwareLevelMesh *worldMesh;
    const JPBSoftwareLevelBatch *worldMeshBatches;
    size_t worldVertexCount;
    int worldWidth;
    int worldHeight;
    int gameplayCompositeReady;
    unsigned worldTimingFrames;
    double worldPrepareSeconds;
    double worldSubmitSeconds;
    double worldReadbackSeconds;
    unsigned modelTimingFrames;
    double modelUploadSeconds;
    double modelSubmitSeconds;
    double modelColorReadbackSeconds;
    double modelDepthReadbackSeconds;
    unsigned titleTimingFrames;
    double titlePrepareSeconds;
    double titleSubmitSeconds;
    double titleReadbackSeconds;
    int sourceWidth;
    int sourceHeight;
    int outputWidth;
    int outputHeight;
    HRESULT lastError;
    char description[160];
};

static const char pc_present_shader[] =
    "cbuffer PresentConstants : register(b0) {\n"
    "  float2 inverseSourceSize;\n"
    "  float2 padding;\n"
    "};\n"
    "Texture2D sourceTexture : register(t0);\n"
    "SamplerState sourceSampler : register(s0);\n"
    "struct VertexOutput { float4 position : SV_Position; float2 uv : TEXCOORD0; };\n"
    "VertexOutput VSMain(uint id : SV_VertexID) {\n"
    "  VertexOutput output;\n"
    "  float2 uv = float2((id << 1) & 2, id & 2);\n"
    "  output.uv = uv;\n"
    "  output.position = float4(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0, 0.0, 1.0);\n"
    "  return output;\n"
    "}\n"
    "float luma(float3 color) {\n"
    "  return dot(color, float3(0.2109375, 0.71484375, 0.07421875));\n"
    "}\n"
    "float4 PSMain(VertexOutput input) : SV_Target {\n"
    "  float3 rgbNW = sourceTexture.Sample(sourceSampler, input.uv + float2(-1.0, -1.0) * inverseSourceSize).rgb;\n"
    "  float3 rgbNE = sourceTexture.Sample(sourceSampler, input.uv + float2( 1.0, -1.0) * inverseSourceSize).rgb;\n"
    "  float3 rgbSW = sourceTexture.Sample(sourceSampler, input.uv + float2(-1.0,  1.0) * inverseSourceSize).rgb;\n"
    "  float3 rgbSE = sourceTexture.Sample(sourceSampler, input.uv + float2( 1.0,  1.0) * inverseSourceSize).rgb;\n"
    "  float3 rgbM = sourceTexture.Sample(sourceSampler, input.uv).rgb;\n"
    "  float lumaNW = luma(rgbNW), lumaNE = luma(rgbNE);\n"
    "  float lumaSW = luma(rgbSW), lumaSE = luma(rgbSE), lumaM = luma(rgbM);\n"
    "  float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));\n"
    "  float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));\n"
    "  if (lumaMax - lumaMin < max(1.0 / 16.0, lumaMax * (1.0 / 8.0))) return float4(rgbM, 1.0);\n"
    "  float2 direction;\n"
    "  direction.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));\n"
    "  direction.y =  ((lumaNW + lumaSW) - (lumaNE + lumaSE));\n"
    "  float directionReduce = max((lumaNW + lumaNE + lumaSW + lumaSE) * (1.0 / 32.0), 1.0 / 128.0);\n"
    "  float inverseMinimum = 1.0 / (min(abs(direction.x), abs(direction.y)) + directionReduce);\n"
    "  direction = clamp(direction * inverseMinimum, -8.0, 8.0) * inverseSourceSize;\n"
    "  float3 rgbA = 0.5 * (sourceTexture.Sample(sourceSampler, input.uv + direction * (1.0 / 3.0 - 0.5)).rgb + sourceTexture.Sample(sourceSampler, input.uv + direction * (2.0 / 3.0 - 0.5)).rgb);\n"
    "  float3 rgbB = rgbA * 0.5 + 0.25 * (sourceTexture.Sample(sourceSampler, input.uv + direction * -0.5).rgb + sourceTexture.Sample(sourceSampler, input.uv + direction * 0.5).rgb);\n"
    "  float lumaB = luma(rgbB);\n"
    "  return float4((lumaB < lumaMin || lumaB > lumaMax) ? rgbA : rgbB, 1.0);\n"
    "}\n";

static const char pc_gameplay_composite_shader[] =
    "Texture2D worldTexture : register(t0);\n"
    "Texture2D hudBlackTexture : register(t1);\n"
    "Texture2D hudWhiteTexture : register(t2);\n"
    "SamplerState sourceSampler : register(s0);\n"
    "struct VertexOutput { float4 position : SV_Position; float2 uv : TEXCOORD0; };\n"
    "float4 PSGameplayComposite(VertexOutput input) : SV_Target {\n"
    "  float3 world = worldTexture.Sample(sourceSampler, input.uv).rgb;\n"
    "  float3 blackBase = hudBlackTexture.Sample(sourceSampler, input.uv).rgb;\n"
    "  float3 whiteBase = hudWhiteTexture.Sample(sourceSampler, input.uv).rgb;\n"
    "  float3 result = blackBase + world * saturate(whiteBase - blackBase);\n"
    "  return float4(saturate(result), 1.0);\n"
    "}\n";

static const char pc_world_shader[] =
    "cbuffer WorldConstants : register(b0) {\n"
    "  float4 viewRow0; float4 viewRow1; float4 viewRow2;\n"
    "  float4 projection; float4 transparentPass; float2 uvScrollSpeed; float2 padding;\n"
    "};\n"
    "Texture2D materialTexture : register(t0);\n"
    "SamplerState materialSampler : register(s0);\n"
    "struct VSInput { float3 position : POSITION; float2 uv : TEXCOORD0; float4 color : COLOR0; float2 uvScroll : TEXCOORD1; };\n"
    "struct VSOutput { float4 position : SV_Position; float2 uv : TEXCOORD0; float4 color : COLOR0; float depth : TEXCOORD1; };\n"
    "VSOutput VSWorld(VSInput input) {\n"
    "  VSOutput output;\n"
    "  float3 camera = float3(dot(float4(input.position, 1.0), viewRow0), dot(float4(input.position, 1.0), viewRow1), dot(float4(input.position, 1.0), viewRow2));\n"
    "  output.position = float4(camera.x * projection.x, -camera.y * projection.y, camera.z * projection.z + projection.w, camera.z);\n"
    "  output.uv = input.uv + abs(input.uvScroll) - sign(input.uvScroll) * uvScrollSpeed; output.color = input.color / 255.0; output.depth = camera.z / 10240.0;\n"
    "  return output;\n"
    "}\n"
    "struct PSOutput { float4 color : SV_Target0; float depth : SV_Target1; };\n"
    "PSOutput PSWorld(VSOutput input) {\n"
    "  PSOutput output; float4 sample = materialTexture.Sample(materialSampler, input.uv);\n"
    "  if (all(sample == 0.0)) discard;\n"
    "  float4 vertex = float4(max(input.color.rgb, 0.1), input.color.a);\n"
    "  output.color = sample * vertex;\n"
    "  if (transparentPass.x > 0.5 && output.color.a < 0.1) discard;\n"
    "  output.depth = input.depth; return output;\n"
    "}\n";

static const char pc_model_shader[] =
    "cbuffer ModelConstants : register(b0) { float4 viewport; };\n"
    "Texture2D materialTexture : register(t0);\n"
    "SamplerState materialSampler : register(s0);\n"
    "struct VSInput { float3 screen : POSITION; float inverseDepth : TEXCOORD0; float2 uv : TEXCOORD1; float4 color : COLOR0; };\n"
    "struct VSImmediateInput { float3 screen : POSITION; float inverseDepth : TEXCOORD0; float clipDepth : TEXCOORD2; float2 uv : TEXCOORD1; float4 color : COLOR0; };\n"
    "struct VSOutput { float4 position : SV_Position; float2 uv : TEXCOORD0; float4 color : COLOR0; float depth : TEXCOORD1; };\n"
    "VSOutput VSModel(VSInput input) { VSOutput output; float cameraZ = input.screen.z * 10240.0;\n"
    " output.position = float4((input.screen.x * viewport.x - 1.0) * cameraZ, (1.0 - input.screen.y * viewport.y) * cameraZ, cameraZ * 1.00010001 - 1.00010001, cameraZ);\n"
    " output.uv = input.uv; output.color = input.color / 255.0; output.depth = input.screen.z; return output; }\n"
    "VSOutput VSImmediate(VSImmediateInput input) { VSOutput output;\n"
    " output.position = float4(input.screen.x * viewport.x - 1.0, 1.0 - input.screen.y * viewport.y, input.clipDepth, 1.0);\n"
    " output.uv = input.uv; output.color = input.color / 255.0; output.depth = input.screen.z; return output; }\n"
    "struct PSOutput { float4 color : SV_Target0; float depth : SV_Target1; };\n"
    "PSOutput PSModel(VSOutput input) { PSOutput output; float4 sample = materialTexture.Sample(materialSampler, input.uv);\n"
    " if (all(sample.rgb == 0.0)) discard; if (all(input.color > 0.0)) sample.rgb *= input.color.rgb;\n"
    " output.color = sample; output.depth = input.depth; return output; }\n"
    "PSOutput PSImmediateOpaque(VSOutput input) { PSOutput output; float4 sample = materialTexture.Sample(materialSampler, input.uv);\n"
    " if (all(sample == 0.0)) discard; output.color = sample * float4(max(input.color.rgb, 0.1), input.color.a); output.depth = input.depth; return output; }\n"
    "PSOutput PSImmediateTransparent(VSOutput input) { PSOutput output; float4 sample = materialTexture.Sample(materialSampler, input.uv);\n"
    " output.color = sample * input.color; output.depth = input.depth; return output; }\n"
    ;

static void pc_present_release_target(JPBPCD3D11Presenter *presenter)
{
    if (presenter->renderTarget != NULL) {
        ID3D11RenderTargetView_Release(presenter->renderTarget);
        presenter->renderTarget = NULL;
    }
}

static void pc_present_release_upload(JPBPCD3D11Presenter *presenter)
{
    if (presenter->uploadView != NULL) {
        ID3D11ShaderResourceView_Release(presenter->uploadView);
        presenter->uploadView = NULL;
    }
    if (presenter->uploadTexture != NULL) {
        ID3D11Texture2D_Release(presenter->uploadTexture);
        presenter->uploadTexture = NULL;
    }
    presenter->sourceWidth = 0;
    presenter->sourceHeight = 0;
}

static void pc_present_release_world_targets(
    JPBPCD3D11Presenter *presenter)
{
    int buffer_index;
    int layer_index;

    if (presenter->worldDepthTarget != NULL) {
        ID3D11DepthStencilView_Release(presenter->worldDepthTarget);
        presenter->worldDepthTarget = NULL;
    }
    if (presenter->worldDepth != NULL) {
        ID3D11Texture2D_Release(presenter->worldDepth);
        presenter->worldDepth = NULL;
    }
    if (presenter->worldLinearDepthReadback != NULL) {
        ID3D11Texture2D_Release(presenter->worldLinearDepthReadback);
        presenter->worldLinearDepthReadback = NULL;
    }
    if (presenter->worldLinearDepthSnapshot != NULL) {
        ID3D11Texture2D_Release(presenter->worldLinearDepthSnapshot);
        presenter->worldLinearDepthSnapshot = NULL;
    }
    if (presenter->worldLinearDepthTarget != NULL) {
        ID3D11RenderTargetView_Release(presenter->worldLinearDepthTarget);
        presenter->worldLinearDepthTarget = NULL;
    }
    if (presenter->worldLinearDepthView != NULL) {
        ID3D11ShaderResourceView_Release(
            presenter->worldLinearDepthView);
        presenter->worldLinearDepthView = NULL;
    }
    if (presenter->worldLinearDepth != NULL) {
        ID3D11Texture2D_Release(presenter->worldLinearDepth);
        presenter->worldLinearDepth = NULL;
    }
    if (presenter->worldColorReadback != NULL) {
        ID3D11Texture2D_Release(presenter->worldColorReadback);
        presenter->worldColorReadback = NULL;
    }
    for (buffer_index = 0;
         buffer_index < PC_GAMEPLAY_HUD_BUFFER_COUNT;
         ++buffer_index) {
        for (layer_index = 0; layer_index < 2; ++layer_index) {
        if (presenter->gameplayHudView[buffer_index][layer_index] != NULL) {
            ID3D11ShaderResourceView_Release(
                presenter->gameplayHudView[buffer_index][layer_index]);
            presenter->gameplayHudView[buffer_index][layer_index] = NULL;
        }
        if (presenter->gameplayHud[buffer_index][layer_index] != NULL) {
            ID3D11Texture2D_Release(
                presenter->gameplayHud[buffer_index][layer_index]);
            presenter->gameplayHud[buffer_index][layer_index] = NULL;
        }
        }
    }
    if (presenter->gameplayCompositeView != NULL) {
        ID3D11ShaderResourceView_Release(
            presenter->gameplayCompositeView);
        presenter->gameplayCompositeView = NULL;
    }
    if (presenter->gameplayCompositeTarget != NULL) {
        ID3D11RenderTargetView_Release(
            presenter->gameplayCompositeTarget);
        presenter->gameplayCompositeTarget = NULL;
    }
    if (presenter->gameplayComposite != NULL) {
        ID3D11Texture2D_Release(presenter->gameplayComposite);
        presenter->gameplayComposite = NULL;
    }
    if (presenter->worldColorView != NULL) {
        ID3D11ShaderResourceView_Release(presenter->worldColorView);
        presenter->worldColorView = NULL;
    }
    if (presenter->worldColorTarget != NULL) {
        ID3D11RenderTargetView_Release(presenter->worldColorTarget);
        presenter->worldColorTarget = NULL;
    }
    if (presenter->worldColor != NULL) {
        ID3D11Texture2D_Release(presenter->worldColor);
        presenter->worldColor = NULL;
    }
    presenter->worldWidth = 0;
    presenter->worldHeight = 0;
    presenter->gameplayCompositeReady = 0;
}

static void pc_present_release_world_mesh(
    JPBPCD3D11Presenter *presenter)
{
    size_t index;

    if (presenter->worldVertexBuffer != NULL) {
        ID3D11Buffer_Release(presenter->worldVertexBuffer);
        presenter->worldVertexBuffer = NULL;
    }
    for (index = 0; index < presenter->worldTextureCount; ++index) {
        if (presenter->worldTextures[index].view != NULL) {
            ID3D11ShaderResourceView_Release(
                presenter->worldTextures[index].view);
        }
        if (presenter->worldTextures[index].texture != NULL) {
            ID3D11Texture2D_Release(
                presenter->worldTextures[index].texture);
        }
    }
    memset(presenter->worldTextures, 0,
           sizeof(presenter->worldTextures));
    presenter->worldTextureCount = 0;
    presenter->worldMesh = NULL;
    presenter->worldMeshBatches = NULL;
    presenter->worldVertexCount = 0;
}

static HRESULT pc_present_create_target(JPBPCD3D11Presenter *presenter)
{
    ID3D11Texture2D *back_buffer = NULL;
    HRESULT result;

    result = IDXGISwapChain_GetBuffer(
        presenter->swapChain,
        0,
        &IID_ID3D11Texture2D,
        (void **)&back_buffer);
    if (SUCCEEDED(result)) {
        result = ID3D11Device_CreateRenderTargetView(
            presenter->device,
            (ID3D11Resource *)back_buffer,
            NULL,
            &presenter->renderTarget);
    }
    if (back_buffer != NULL) {
        ID3D11Texture2D_Release(back_buffer);
    }
    return result;
}

static HRESULT pc_present_resize(
    JPBPCD3D11Presenter *presenter,
    int width,
    int height)
{
    HRESULT result;

    if (width == presenter->outputWidth &&
        height == presenter->outputHeight &&
        presenter->renderTarget != NULL) {
        return S_OK;
    }
    ID3D11DeviceContext_OMSetRenderTargets(
        presenter->context, 0, NULL, NULL);
    pc_present_release_target(presenter);
    result = IDXGISwapChain_ResizeBuffers(
        presenter->swapChain,
        0,
        (UINT)width,
        (UINT)height,
        DXGI_FORMAT_UNKNOWN,
        0);
    if (FAILED(result)) {
        return result;
    }
    result = pc_present_create_target(presenter);
    if (SUCCEEDED(result)) {
        presenter->outputWidth = width;
        presenter->outputHeight = height;
    }
    return result;
}

static HRESULT pc_present_create_upload(
    JPBPCD3D11Presenter *presenter,
    int width,
    int height)
{
    D3D11_TEXTURE2D_DESC texture_desc;
    D3D11_SHADER_RESOURCE_VIEW_DESC view_desc;
    HRESULT result;

    pc_present_release_upload(presenter);
    memset(&texture_desc, 0, sizeof(texture_desc));
    texture_desc.Width = (UINT)width;
    texture_desc.Height = (UINT)height;
    texture_desc.MipLevels = 1;
    texture_desc.ArraySize = 1;
    texture_desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    texture_desc.SampleDesc.Count = 1;
    texture_desc.Usage = D3D11_USAGE_DYNAMIC;
    texture_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    texture_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    result = ID3D11Device_CreateTexture2D(
        presenter->device,
        &texture_desc,
        NULL,
        &presenter->uploadTexture);
    if (FAILED(result)) {
        return result;
    }
    memset(&view_desc, 0, sizeof(view_desc));
    view_desc.Format = texture_desc.Format;
    view_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    view_desc.Texture2D.MipLevels = 1;
    result = ID3D11Device_CreateShaderResourceView(
        presenter->device,
        (ID3D11Resource *)presenter->uploadTexture,
        &view_desc,
        &presenter->uploadView);
    if (SUCCEEDED(result)) {
        presenter->sourceWidth = width;
        presenter->sourceHeight = height;
    }
    return result;
}

static HRESULT pc_present_create_shaders(JPBPCD3D11Presenter *presenter)
{
    ID3DBlob *vertex_blob = NULL;
    ID3DBlob *pixel_blob = NULL;
    ID3DBlob *composite_blob = NULL;
    ID3DBlob *errors = NULL;
    HRESULT result;

    result = D3DCompile(
        pc_present_shader,
        sizeof(pc_present_shader) - 1,
        "OpenJPBPresent.hlsl",
        NULL,
        NULL,
        "VSMain",
        "vs_4_0",
        D3DCOMPILE_OPTIMIZATION_LEVEL3,
        0,
        &vertex_blob,
        &errors);
    if (errors != NULL) {
        ID3D10Blob_Release(errors);
        errors = NULL;
    }
    if (FAILED(result)) {
        goto cleanup;
    }
    result = D3DCompile(
        pc_present_shader,
        sizeof(pc_present_shader) - 1,
        "OpenJPBPresent.hlsl",
        NULL,
        NULL,
        "PSMain",
        "ps_4_0",
        D3DCOMPILE_OPTIMIZATION_LEVEL3,
        0,
        &pixel_blob,
        &errors);
    if (errors != NULL) {
        ID3D10Blob_Release(errors);
        errors = NULL;
    }
    if (FAILED(result)) {
        goto cleanup;
    }
    result = ID3D11Device_CreateVertexShader(
        presenter->device,
        ID3D10Blob_GetBufferPointer(vertex_blob),
        ID3D10Blob_GetBufferSize(vertex_blob),
        NULL,
        &presenter->vertexShader);
    if (FAILED(result)) {
        goto cleanup;
    }
    result = ID3D11Device_CreatePixelShader(
        presenter->device,
        ID3D10Blob_GetBufferPointer(pixel_blob),
        ID3D10Blob_GetBufferSize(pixel_blob),
        NULL,
        &presenter->pixelShader);
    if (FAILED(result)) {
        goto cleanup;
    }
    result = D3DCompile(
        pc_gameplay_composite_shader,
        sizeof(pc_gameplay_composite_shader) - 1,
        "OpenJPBGameplayComposite.hlsl",
        NULL, NULL, "PSGameplayComposite", "ps_4_0",
        D3DCOMPILE_OPTIMIZATION_LEVEL3, 0,
        &composite_blob, &errors);
    if (errors != NULL) {
        ID3D10Blob_Release(errors);
        errors = NULL;
    }
    if (FAILED(result)) {
        goto cleanup;
    }
    result = ID3D11Device_CreatePixelShader(
        presenter->device,
        ID3D10Blob_GetBufferPointer(composite_blob),
        ID3D10Blob_GetBufferSize(composite_blob),
        NULL, &presenter->gameplayCompositePixelShader);

cleanup:
    if (composite_blob != NULL) ID3D10Blob_Release(composite_blob);
    if (pixel_blob != NULL) ID3D10Blob_Release(pixel_blob);
    if (vertex_blob != NULL) ID3D10Blob_Release(vertex_blob);
    return result;
}

static HRESULT pc_present_create_states(JPBPCD3D11Presenter *presenter)
{
    D3D11_SAMPLER_DESC sampler_desc;
    D3D11_BUFFER_DESC buffer_desc;
    HRESULT result;

    memset(&sampler_desc, 0, sizeof(sampler_desc));
    sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampler_desc.MaxLOD = D3D11_FLOAT32_MAX;
    result = ID3D11Device_CreateSamplerState(
        presenter->device, &sampler_desc, &presenter->sampler);
    if (FAILED(result)) {
        return result;
    }
    memset(&buffer_desc, 0, sizeof(buffer_desc));
    buffer_desc.ByteWidth = sizeof(JPBPCD3D11Constants);
    buffer_desc.Usage = D3D11_USAGE_DYNAMIC;
    buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    buffer_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    return ID3D11Device_CreateBuffer(
        presenter->device, &buffer_desc, NULL, &presenter->constants);
}

static HRESULT pc_present_compile_world_shaders(
    JPBPCD3D11Presenter *presenter)
{
    static const D3D11_INPUT_ELEMENT_DESC elements[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
         offsetof(JPBSoftwareLevelVertex, position),
         D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0,
         offsetof(JPBSoftwareLevelVertex, u),
         D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,
         offsetof(JPBSoftwareLevelVertex, red),
         D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 1, DXGI_FORMAT_R32G32_FLOAT, 0,
         offsetof(JPBSoftwareLevelVertex, uvScrollU),
         D3D11_INPUT_PER_VERTEX_DATA, 0}
    };
    ID3DBlob *vertex_blob = NULL;
    ID3DBlob *pixel_blob = NULL;
    ID3DBlob *errors = NULL;
    HRESULT result;

    result = D3DCompile(
        pc_world_shader, sizeof(pc_world_shader) - 1,
        "OpenJPBWorld.hlsl", NULL, NULL, "VSWorld", "vs_4_0",
        D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, &vertex_blob, &errors);
    if (errors != NULL) {
        ID3D10Blob_Release(errors);
        errors = NULL;
    }
    if (FAILED(result)) goto cleanup;
    result = D3DCompile(
        pc_world_shader, sizeof(pc_world_shader) - 1,
        "OpenJPBWorld.hlsl", NULL, NULL, "PSWorld", "ps_4_0",
        D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, &pixel_blob, &errors);
    if (errors != NULL) {
        ID3D10Blob_Release(errors);
        errors = NULL;
    }
    if (FAILED(result)) goto cleanup;
    result = ID3D11Device_CreateVertexShader(
        presenter->device,
        ID3D10Blob_GetBufferPointer(vertex_blob),
        ID3D10Blob_GetBufferSize(vertex_blob), NULL,
        &presenter->worldVertexShader);
    if (FAILED(result)) goto cleanup;
    result = ID3D11Device_CreatePixelShader(
        presenter->device,
        ID3D10Blob_GetBufferPointer(pixel_blob),
        ID3D10Blob_GetBufferSize(pixel_blob), NULL,
        &presenter->worldPixelShader);
    if (FAILED(result)) goto cleanup;
    result = ID3D11Device_CreateInputLayout(
        presenter->device, elements,
        sizeof(elements) / sizeof(elements[0]),
        ID3D10Blob_GetBufferPointer(vertex_blob),
        ID3D10Blob_GetBufferSize(vertex_blob),
        &presenter->worldInputLayout);

cleanup:
    if (pixel_blob != NULL) ID3D10Blob_Release(pixel_blob);
    if (vertex_blob != NULL) ID3D10Blob_Release(vertex_blob);
    return result;
}

static HRESULT pc_present_compile_model_shaders(
    JPBPCD3D11Presenter *presenter)
{
    static const D3D11_INPUT_ELEMENT_DESC elements[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
         offsetof(JPBSoftwareMaterialVertex, x),
         D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32_FLOAT, 0,
         offsetof(JPBSoftwareMaterialVertex, inverseDepth),
         D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 1, DXGI_FORMAT_R32G32_FLOAT, 0,
         offsetof(JPBSoftwareMaterialVertex, u),
         D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,
         offsetof(JPBSoftwareMaterialVertex, red),
         D3D11_INPUT_PER_VERTEX_DATA, 0}
    };
    static const D3D11_INPUT_ELEMENT_DESC screen_elements[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
         offsetof(JPBSoftwareMaterialVertex, x),
         D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32_FLOAT, 0,
         offsetof(JPBSoftwareMaterialVertex, inverseDepth),
         D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 2, DXGI_FORMAT_R32_FLOAT, 0,
         offsetof(JPBSoftwareMaterialVertex, clipDepth),
         D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 1, DXGI_FORMAT_R32G32_FLOAT, 0,
         offsetof(JPBSoftwareMaterialVertex, u),
         D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,
         offsetof(JPBSoftwareMaterialVertex, red),
         D3D11_INPUT_PER_VERTEX_DATA, 0}
    };
    ID3DBlob *vs = NULL;
    ID3DBlob *screen_vs = NULL;
    ID3DBlob *ps = NULL;
    ID3DBlob *screen_opaque_ps = NULL;
    ID3DBlob *screen_transparent_ps = NULL;
    ID3DBlob *errors = NULL;
    HRESULT result;

    result = D3DCompile(
        pc_model_shader, sizeof(pc_model_shader) - 1,
        "OpenJPBModel.hlsl", NULL, NULL, "VSModel", "vs_4_0",
        D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, &vs, &errors);
    if (errors != NULL) { ID3D10Blob_Release(errors); errors = NULL; }
    if (FAILED(result)) goto cleanup;
    result = D3DCompile(
        pc_model_shader, sizeof(pc_model_shader) - 1,
        "OpenJPBImmediate.hlsl", NULL, NULL,
        "VSImmediate", "vs_4_0",
        D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, &screen_vs, &errors);
    if (errors != NULL) { ID3D10Blob_Release(errors); errors = NULL; }
    if (FAILED(result)) goto cleanup;
    result = D3DCompile(
        pc_model_shader, sizeof(pc_model_shader) - 1,
        "OpenJPBImmediate.hlsl", NULL, NULL,
        "PSImmediateOpaque", "ps_4_0",
        D3DCOMPILE_OPTIMIZATION_LEVEL3, 0,
        &screen_opaque_ps, &errors);
    if (errors != NULL) { ID3D10Blob_Release(errors); errors = NULL; }
    if (FAILED(result)) goto cleanup;
    result = D3DCompile(
        pc_model_shader, sizeof(pc_model_shader) - 1,
        "OpenJPBImmediate.hlsl", NULL, NULL,
        "PSImmediateTransparent", "ps_4_0",
        D3DCOMPILE_OPTIMIZATION_LEVEL3, 0,
        &screen_transparent_ps, &errors);
    if (errors != NULL) { ID3D10Blob_Release(errors); errors = NULL; }
    if (FAILED(result)) goto cleanup;
    result = D3DCompile(
        pc_model_shader, sizeof(pc_model_shader) - 1,
        "OpenJPBModel.hlsl", NULL, NULL, "PSModel", "ps_4_0",
        D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, &ps, &errors);
    if (errors != NULL) { ID3D10Blob_Release(errors); errors = NULL; }
    if (FAILED(result)) goto cleanup;
    result = ID3D11Device_CreatePixelShader(
        presenter->device,
        ID3D10Blob_GetBufferPointer(screen_opaque_ps),
        ID3D10Blob_GetBufferSize(screen_opaque_ps), NULL,
        &presenter->screenOpaquePixelShader);
    if (FAILED(result)) goto cleanup;
    result = ID3D11Device_CreatePixelShader(
        presenter->device,
        ID3D10Blob_GetBufferPointer(screen_transparent_ps),
        ID3D10Blob_GetBufferSize(screen_transparent_ps), NULL,
        &presenter->screenTransparentPixelShader);
    if (FAILED(result)) goto cleanup;
    result = ID3D11Device_CreateVertexShader(
        presenter->device, ID3D10Blob_GetBufferPointer(vs),
        ID3D10Blob_GetBufferSize(vs), NULL,
        &presenter->modelVertexShader);
    if (FAILED(result)) goto cleanup;
    result = ID3D11Device_CreateVertexShader(
        presenter->device, ID3D10Blob_GetBufferPointer(screen_vs),
        ID3D10Blob_GetBufferSize(screen_vs), NULL,
        &presenter->screenVertexShader);
    if (FAILED(result)) goto cleanup;
    result = ID3D11Device_CreatePixelShader(
        presenter->device, ID3D10Blob_GetBufferPointer(ps),
        ID3D10Blob_GetBufferSize(ps), NULL,
        &presenter->modelPixelShader);
    if (FAILED(result)) goto cleanup;
    result = ID3D11Device_CreateInputLayout(
        presenter->device, elements,
        sizeof(elements) / sizeof(elements[0]),
        ID3D10Blob_GetBufferPointer(vs),
        ID3D10Blob_GetBufferSize(vs),
        &presenter->modelInputLayout);
    if (FAILED(result)) goto cleanup;
    result = ID3D11Device_CreateInputLayout(
        presenter->device, screen_elements,
        sizeof(screen_elements) / sizeof(screen_elements[0]),
        ID3D10Blob_GetBufferPointer(screen_vs),
        ID3D10Blob_GetBufferSize(screen_vs),
        &presenter->screenInputLayout);
cleanup:
    if (screen_transparent_ps != NULL) {
        ID3D10Blob_Release(screen_transparent_ps);
    }
    if (screen_opaque_ps != NULL) ID3D10Blob_Release(screen_opaque_ps);
    if (ps != NULL) ID3D10Blob_Release(ps);
    if (vs != NULL) ID3D10Blob_Release(vs);
    if (screen_vs != NULL) ID3D10Blob_Release(screen_vs);
    return result;
}

static HRESULT pc_present_create_model_states(
    JPBPCD3D11Presenter *presenter)
{
    D3D11_BUFFER_DESC buffer_desc;
    D3D11_SAMPLER_DESC sampler_desc;
    HRESULT result;

    memset(&buffer_desc, 0, sizeof(buffer_desc));
    buffer_desc.ByteWidth = 16;
    buffer_desc.Usage = D3D11_USAGE_DYNAMIC;
    buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    buffer_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    result = ID3D11Device_CreateBuffer(
        presenter->device, &buffer_desc, NULL,
        &presenter->modelConstants);
    if (FAILED(result)) return result;
    memset(&sampler_desc, 0, sizeof(sampler_desc));
    sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampler_desc.MaxLOD = D3D11_FLOAT32_MAX;
    result = ID3D11Device_CreateSamplerState(
        presenter->device, &sampler_desc,
        &presenter->modelLinearSampler);
    if (FAILED(result)) return result;
    sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    return ID3D11Device_CreateSamplerState(
        presenter->device, &sampler_desc,
        &presenter->modelPointSampler);
}

static HRESULT pc_present_create_world_states(
    JPBPCD3D11Presenter *presenter)
{
    D3D11_BUFFER_DESC buffer_desc;
    D3D11_SAMPLER_DESC sampler_desc;
    D3D11_RASTERIZER_DESC raster_desc;
    D3D11_DEPTH_STENCIL_DESC depth_desc;
    D3D11_BLEND_DESC blend_desc;
    D3D11_TEXTURE2D_DESC texture_desc;
    D3D11_SUBRESOURCE_DATA texture_data;
    ID3D11Texture2D *white_texture = NULL;
    uint32_t white = UINT32_C(0xffffffff);
    HRESULT result;

    memset(&buffer_desc, 0, sizeof(buffer_desc));
    buffer_desc.ByteWidth = sizeof(JPBPCD3D11WorldConstants);
    buffer_desc.Usage = D3D11_USAGE_DYNAMIC;
    buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    buffer_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    result = ID3D11Device_CreateBuffer(
        presenter->device, &buffer_desc, NULL,
        &presenter->worldConstants);
    if (FAILED(result)) return result;

    memset(&sampler_desc, 0, sizeof(sampler_desc));
    sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    sampler_desc.MaxLOD = D3D11_FLOAT32_MAX;
    result = ID3D11Device_CreateSamplerState(
        presenter->device, &sampler_desc,
        &presenter->worldSampler);
    if (FAILED(result)) return result;

    memset(&raster_desc, 0, sizeof(raster_desc));
    raster_desc.FillMode = D3D11_FILL_SOLID;
    raster_desc.CullMode = D3D11_CULL_NONE;
    raster_desc.DepthClipEnable = TRUE;
    result = ID3D11Device_CreateRasterizerState(
        presenter->device, &raster_desc,
        &presenter->worldRasterizer);
    if (FAILED(result)) return result;
    raster_desc.ScissorEnable = TRUE;
    result = ID3D11Device_CreateRasterizerState(
        presenter->device, &raster_desc,
        &presenter->worldScissorRasterizer);
    if (FAILED(result)) return result;

    memset(&depth_desc, 0, sizeof(depth_desc));
    depth_desc.DepthEnable = TRUE;
    depth_desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    depth_desc.DepthFunc = D3D11_COMPARISON_LESS;
    result = ID3D11Device_CreateDepthStencilState(
        presenter->device, &depth_desc,
        &presenter->worldDepthWrite);
    if (FAILED(result)) return result;
    depth_desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    result = ID3D11Device_CreateDepthStencilState(
        presenter->device, &depth_desc,
        &presenter->worldDepthRead);
    if (FAILED(result)) return result;
    /* Both shipped D3DTransparencyPass PSOs use LESS_EQUAL. */
    depth_desc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
    result = ID3D11Device_CreateDepthStencilState(
        presenter->device, &depth_desc,
        &presenter->screenDepthRead);
    if (FAILED(result)) return result;

    memset(&blend_desc, 0, sizeof(blend_desc));
    blend_desc.IndependentBlendEnable = TRUE;
    blend_desc.RenderTarget[0].RenderTargetWriteMask =
        D3D11_COLOR_WRITE_ENABLE_ALL;
    blend_desc.RenderTarget[1].RenderTargetWriteMask =
        D3D11_COLOR_WRITE_ENABLE_RED;
    result = ID3D11Device_CreateBlendState(
        presenter->device, &blend_desc,
        &presenter->worldOpaqueBlend);
    if (FAILED(result)) return result;
    blend_desc.RenderTarget[0].BlendEnable = TRUE;
    blend_desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blend_desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    blend_desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blend_desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blend_desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    blend_desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    result = ID3D11Device_CreateBlendState(
        presenter->device, &blend_desc,
        &presenter->worldAlphaBlend);
    if (FAILED(result)) return result;
    blend_desc.RenderTarget[1].RenderTargetWriteMask = 0;
    result = ID3D11Device_CreateBlendState(
        presenter->device, &blend_desc,
        &presenter->worldGlassBlend);
    if (FAILED(result)) return result;
    blend_desc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
    result = ID3D11Device_CreateBlendState(
        presenter->device, &blend_desc,
        &presenter->worldAdditiveBlend);
    if (FAILED(result)) return result;

    memset(&texture_desc, 0, sizeof(texture_desc));
    texture_desc.Width = 1;
    texture_desc.Height = 1;
    texture_desc.MipLevels = 1;
    texture_desc.ArraySize = 1;
    texture_desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    texture_desc.SampleDesc.Count = 1;
    texture_desc.Usage = D3D11_USAGE_IMMUTABLE;
    texture_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    memset(&texture_data, 0, sizeof(texture_data));
    texture_data.pSysMem = &white;
    texture_data.SysMemPitch = sizeof(white);
    result = ID3D11Device_CreateTexture2D(
        presenter->device, &texture_desc, &texture_data,
        &white_texture);
    if (SUCCEEDED(result)) {
        result = ID3D11Device_CreateShaderResourceView(
            presenter->device, (ID3D11Resource *)white_texture,
            NULL, &presenter->worldWhiteView);
    }
    if (white_texture != NULL) ID3D11Texture2D_Release(white_texture);
    return result;
}

static HRESULT pc_present_create_world_targets(
    JPBPCD3D11Presenter *presenter, int width, int height)
{
    D3D11_TEXTURE2D_DESC desc;
    HRESULT result;

    if (presenter->worldWidth == width &&
        presenter->worldHeight == height &&
        presenter->worldColorTarget != NULL) {
        return S_OK;
    }
    ID3D11DeviceContext_OMSetRenderTargets(
        presenter->context, 0, NULL, NULL);
    pc_present_release_world_targets(presenter);

    memset(&desc, 0, sizeof(desc));
    desc.Width = (UINT)width;
    desc.Height = (UINT)height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.SampleDesc.Count = 1;
    desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_RENDER_TARGET |
        D3D11_BIND_SHADER_RESOURCE;
    result = ID3D11Device_CreateTexture2D(
        presenter->device, &desc, NULL, &presenter->worldColor);
    if (SUCCEEDED(result)) {
        result = ID3D11Device_CreateRenderTargetView(
            presenter->device,
            (ID3D11Resource *)presenter->worldColor,
            NULL, &presenter->worldColorTarget);
    }
    if (SUCCEEDED(result)) {
        result = ID3D11Device_CreateShaderResourceView(
            presenter->device,
            (ID3D11Resource *)presenter->worldColor,
            NULL, &presenter->worldColorView);
    }
    if (SUCCEEDED(result)) {
        result = ID3D11Device_CreateTexture2D(
            presenter->device, &desc, NULL,
            &presenter->gameplayComposite);
    }
    if (SUCCEEDED(result)) {
        result = ID3D11Device_CreateRenderTargetView(
            presenter->device,
            (ID3D11Resource *)presenter->gameplayComposite,
            NULL, &presenter->gameplayCompositeTarget);
    }
    if (SUCCEEDED(result)) {
        result = ID3D11Device_CreateShaderResourceView(
            presenter->device,
            (ID3D11Resource *)presenter->gameplayComposite,
            NULL, &presenter->gameplayCompositeView);
    }
    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    if (SUCCEEDED(result)) {
        int buffer_index;
        int layer_index;

        for (buffer_index = 0;
             buffer_index < PC_GAMEPLAY_HUD_BUFFER_COUNT &&
                 SUCCEEDED(result);
             ++buffer_index) {
        for (layer_index = 0;
             layer_index < 2 && SUCCEEDED(result);
             ++layer_index) {
            result = ID3D11Device_CreateTexture2D(
                presenter->device, &desc, NULL,
                &presenter->gameplayHud[buffer_index][layer_index]);
            if (SUCCEEDED(result)) {
                result = ID3D11Device_CreateShaderResourceView(
                    presenter->device,
                    (ID3D11Resource *)presenter->gameplayHud[
                        buffer_index][layer_index],
                    NULL, &presenter->gameplayHudView[
                        buffer_index][layer_index]);
            }
        }
        }
    }
    desc.Usage = D3D11_USAGE_STAGING;
    desc.BindFlags = 0;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    if (SUCCEEDED(result)) {
        result = ID3D11Device_CreateTexture2D(
            presenter->device, &desc, NULL,
            &presenter->worldColorReadback);
    }

    desc.Format = DXGI_FORMAT_R32_FLOAT;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_RENDER_TARGET |
        D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = 0;
    if (SUCCEEDED(result)) {
        result = ID3D11Device_CreateTexture2D(
            presenter->device, &desc, NULL,
            &presenter->worldLinearDepth);
    }
    if (SUCCEEDED(result)) {
        result = ID3D11Device_CreateRenderTargetView(
            presenter->device,
            (ID3D11Resource *)presenter->worldLinearDepth,
            NULL, &presenter->worldLinearDepthTarget);
    }
    if (SUCCEEDED(result)) {
        result = ID3D11Device_CreateShaderResourceView(
            presenter->device,
            (ID3D11Resource *)presenter->worldLinearDepth,
            NULL, &presenter->worldLinearDepthView);
    }
    desc.BindFlags = 0;
    if (SUCCEEDED(result)) {
        result = ID3D11Device_CreateTexture2D(
            presenter->device, &desc, NULL,
            &presenter->worldLinearDepthSnapshot);
    }
    desc.Usage = D3D11_USAGE_STAGING;
    desc.BindFlags = 0;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    if (SUCCEEDED(result)) {
        result = ID3D11Device_CreateTexture2D(
            presenter->device, &desc, NULL,
            &presenter->worldLinearDepthReadback);
    }

    desc.Format = DXGI_FORMAT_D32_FLOAT;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    desc.CPUAccessFlags = 0;
    if (SUCCEEDED(result)) {
        result = ID3D11Device_CreateTexture2D(
            presenter->device, &desc, NULL,
            &presenter->worldDepth);
    }
    if (SUCCEEDED(result)) {
        result = ID3D11Device_CreateDepthStencilView(
            presenter->device,
            (ID3D11Resource *)presenter->worldDepth,
            NULL, &presenter->worldDepthTarget);
    }
    if (SUCCEEDED(result)) {
        presenter->worldWidth = width;
        presenter->worldHeight = height;
    }
    return result;
}

static HRESULT pc_present_create_world_mesh(
    JPBPCD3D11Presenter *presenter,
    const JPBSoftwareLevelMesh *mesh)
{
    D3D11_BUFFER_DESC desc;
    D3D11_SUBRESOURCE_DATA data;
    JPBSoftwareLevelVertex *vertices;
    size_t offset = 0;
    size_t batch_index;
    HRESULT result;

    if (presenter->worldMesh == mesh &&
        presenter->worldMeshBatches == mesh->batches &&
        presenter->worldVertexCount == mesh->vertices &&
        presenter->worldVertexBuffer != NULL) {
        return S_OK;
    }
    pc_present_release_world_mesh(presenter);
    if (mesh == NULL || mesh->batches == NULL || mesh->vertices == 0 ||
        mesh->vertices > UINT_MAX / sizeof(*vertices)) {
        return E_INVALIDARG;
    }
    vertices = (JPBSoftwareLevelVertex *)malloc(
        mesh->vertices * sizeof(*vertices));
    if (vertices == NULL) return E_OUTOFMEMORY;
    for (batch_index = 0; batch_index < mesh->batchCount; ++batch_index) {
        const JPBSoftwareLevelBatch *batch = &mesh->batches[batch_index];

        if (batch->vertices == NULL ||
            offset + batch->vertexCount > mesh->vertices) {
            free(vertices);
            return E_INVALIDARG;
        }
        memcpy(vertices + offset, batch->vertices,
               batch->vertexCount * sizeof(*vertices));
        offset += batch->vertexCount;
    }
    if (offset != mesh->vertices) {
        free(vertices);
        return E_INVALIDARG;
    }
    memset(&desc, 0, sizeof(desc));
    desc.ByteWidth = (UINT)(mesh->vertices * sizeof(*vertices));
    desc.Usage = D3D11_USAGE_IMMUTABLE;
    desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    memset(&data, 0, sizeof(data));
    data.pSysMem = vertices;
    result = ID3D11Device_CreateBuffer(
        presenter->device, &desc, &data,
        &presenter->worldVertexBuffer);
    free(vertices);
    if (SUCCEEDED(result)) {
        presenter->worldMesh = mesh;
        presenter->worldMeshBatches = mesh->batches;
        presenter->worldVertexCount = mesh->vertices;
    }
    return result;
}

static ID3D11ShaderResourceView *pc_present_world_texture(
    JPBPCD3D11Presenter *presenter,
    const JPBSoftwareTexture *source)
{
    D3D11_TEXTURE2D_DESC desc;
    D3D11_SUBRESOURCE_DATA data;
    JPBPCD3D11WorldTexture *cached;
    size_t index;
    HRESULT result;

    if (source == NULL || source->pixels == NULL ||
        source->width == 0 || source->height == 0 ||
        source->stridePixels < source->width ||
        source->width > UINT_MAX || source->height > UINT_MAX) {
        return NULL;
    }
    for (index = 0; index < presenter->worldTextureCount; ++index) {
        cached = &presenter->worldTextures[index];
        if (cached->sourcePixels == source->pixels &&
            cached->width == source->width &&
            cached->height == source->height) {
            return cached->view;
        }
    }
    if (presenter->worldTextureCount >= PC_WORLD_TEXTURE_CAPACITY) {
        return NULL;
    }
    cached = &presenter->worldTextures[presenter->worldTextureCount];
    memset(cached, 0, sizeof(*cached));
    memset(&desc, 0, sizeof(desc));
    desc.Width = (UINT)source->width;
    desc.Height = (UINT)source->height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_IMMUTABLE;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    memset(&data, 0, sizeof(data));
    data.pSysMem = source->pixels;
    data.SysMemPitch = (UINT)(
        source->stridePixels * sizeof(uint32_t));
    result = ID3D11Device_CreateTexture2D(
        presenter->device, &desc, &data, &cached->texture);
    if (SUCCEEDED(result)) {
        result = ID3D11Device_CreateShaderResourceView(
            presenter->device, (ID3D11Resource *)cached->texture,
            NULL, &cached->view);
    }
    if (FAILED(result)) {
        if (cached->texture != NULL) {
            ID3D11Texture2D_Release(cached->texture);
            cached->texture = NULL;
        }
        return NULL;
    }
    cached->sourcePixels = source->pixels;
    cached->width = source->width;
    cached->height = source->height;
    ++presenter->worldTextureCount;
    return cached->view;
}

static HRESULT pc_present_read_color(
    JPBPCD3D11Presenter *presenter,
    JPBSoftwareFramebuffer *framebuffer,
    double *seconds)
{
    D3D11_MAPPED_SUBRESOURCE mapped;
    HRESULT result;
    LARGE_INTEGER frequency;
    LARGE_INTEGER started;
    LARGE_INTEGER finished;
    int y;

    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&started);
    result = ID3D11DeviceContext_Map(
        presenter->context,
        (ID3D11Resource *)presenter->worldColorReadback,
        0, D3D11_MAP_READ, 0, &mapped);
    if (FAILED(result)) return result;
    for (y = 0; y < presenter->worldHeight; ++y) {
        memcpy(
            framebuffer->pixels +
                (size_t)y * framebuffer->stridePixels,
            (const uint8_t *)mapped.pData +
                (size_t)y * mapped.RowPitch,
            (size_t)framebuffer->width * sizeof(uint32_t));
    }
    ID3D11DeviceContext_Unmap(
        presenter->context,
        (ID3D11Resource *)presenter->worldColorReadback, 0);
    QueryPerformanceCounter(&finished);
    if (seconds != NULL) {
        *seconds =
            (double)(finished.QuadPart - started.QuadPart) /
            (double)frequency.QuadPart;
    }
    return S_OK;
}

static HRESULT pc_present_read_depth(
    JPBPCD3D11Presenter *presenter,
    JPBSoftwareDepthBuffer *depth_buffer,
    double *seconds)
{
    D3D11_MAPPED_SUBRESOURCE mapped;
    HRESULT result;
    LARGE_INTEGER frequency;
    LARGE_INTEGER started;
    LARGE_INTEGER finished;
    int y;

    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&started);
    result = ID3D11DeviceContext_Map(
        presenter->context,
        (ID3D11Resource *)presenter->worldLinearDepthReadback,
        0, D3D11_MAP_READ, 0, &mapped);
    if (FAILED(result)) return result;
    for (y = 0; y < presenter->worldHeight; ++y) {
        memcpy(
            depth_buffer->values +
                (size_t)y * depth_buffer->strideValues,
            (const uint8_t *)mapped.pData +
                (size_t)y * mapped.RowPitch,
            (size_t)presenter->worldWidth * sizeof(float));
    }
    ID3D11DeviceContext_Unmap(
        presenter->context,
        (ID3D11Resource *)presenter->worldLinearDepthReadback, 0);
    QueryPerformanceCounter(&finished);
    if (seconds != NULL) {
        *seconds =
            (double)(finished.QuadPart - started.QuadPart) /
            (double)frequency.QuadPart;
    }
    return S_OK;
}

static HRESULT pc_present_read_world(
    JPBPCD3D11Presenter *presenter,
    JPBSoftwareFramebuffer *framebuffer,
    JPBSoftwareDepthBuffer *depth_buffer,
    double *color_seconds,
    double *depth_seconds)
{
    HRESULT result = pc_present_read_color(
        presenter, framebuffer, color_seconds);

    return SUCCEEDED(result)
        ? pc_present_read_depth(
              presenter, depth_buffer, depth_seconds)
        : result;
}

int jpb_PCD3D11PresenterBeginModels(
    JPBPCD3D11Presenter *presenter,
    JPBSoftwareFramebuffer *framebuffer,
    JPBSoftwareDepthBuffer *depth_buffer)
{
    (void)framebuffer;
    (void)depth_buffer;
    if (presenter == NULL || presenter->worldColorTarget == NULL ||
        presenter->worldDepthTarget == NULL) {
        return 0;
    }
    presenter->modelVertexCount = 0;
    presenter->modelBatchCount = 0;
    return 1;
}

int jpb_PCD3D11PresenterPrewarmLevel(
    JPBPCD3D11Presenter *presenter,
    const JPBSoftwareLevelMesh *mesh,
    JPBPCPresentTextureResolver resolve_texture,
    void *texture_user_data)
{
    size_t batch_index;

    if (presenter == NULL || mesh == NULL || mesh->batches == NULL ||
        resolve_texture == NULL) {
        return 0;
    }
    for (batch_index = 0; batch_index < mesh->batchCount; ++batch_index) {
        const char *name = mesh->batches[batch_index].textureName;
        JPBSoftwareTexture texture;

        if (name == NULL || name[0] == '\0') continue;
        memset(&texture, 0, sizeof(texture));
        texture.colorOverride = -1;
        if (!resolve_texture(texture_user_data, name, &texture)) {
            presenter->lastError = E_INVALIDARG;
            return 0;
        }
        if (pc_present_world_texture(presenter, &texture) == NULL) {
            presenter->lastError = E_OUTOFMEMORY;
            return 0;
        }
    }
    return 1;
}

static int pc_present_append_triangle(
    JPBPCD3D11Presenter *presenter,
    const JPBSoftwareMaterialVertex *first,
    const JPBSoftwareMaterialVertex *second,
    const JPBSoftwareMaterialVertex *third,
    const JPBSoftwareTexture *texture,
    int material_type)
{
    ID3D11ShaderResourceView *view;
    ID3D11SamplerState *sampler;
    JPBPCD3D11ModelBatch *batch;

    if (presenter == NULL || first == NULL || second == NULL ||
        third == NULL) {
        return 0;
    }
    /* A NULL texture is the renderer's explicit solid-color primitive. */
    view = texture != NULL
        ? pc_present_world_texture(presenter, texture)
        : presenter->worldWhiteView;
    if (view == NULL) return 0;
    sampler = texture != NULL &&
            texture->samplerType == TEXTURESAMPLER_POINTCLAMP
        ? presenter->modelPointSampler
        : presenter->modelLinearSampler;
    if (presenter->modelVertexCount + 3 >
        presenter->modelVertexCapacity) {
        size_t capacity = presenter->modelVertexCapacity != 0
            ? presenter->modelVertexCapacity * 2 : 8192;
        JPBSoftwareMaterialVertex *vertices;

        while (capacity < presenter->modelVertexCount + 3) {
            capacity *= 2;
        }
        vertices = (JPBSoftwareMaterialVertex *)realloc(
            presenter->modelVertices,
            capacity * sizeof(*vertices));
        if (vertices == NULL) return 0;
        presenter->modelVertices = vertices;
        presenter->modelVertexCapacity = capacity;
    }
    if (presenter->modelBatchCount == 0 ||
        presenter->modelBatches[presenter->modelBatchCount - 1]
                .textureView != view ||
        presenter->modelBatches[presenter->modelBatchCount - 1]
                .sampler != sampler ||
        presenter->modelBatches[presenter->modelBatchCount - 1]
                .materialType != material_type) {
        if (presenter->modelBatchCount ==
            presenter->modelBatchCapacity) {
            size_t capacity = presenter->modelBatchCapacity != 0
                ? presenter->modelBatchCapacity * 2 : 256;
            JPBPCD3D11ModelBatch *batches =
                (JPBPCD3D11ModelBatch *)realloc(
                    presenter->modelBatches,
                    capacity * sizeof(*batches));
            if (batches == NULL) return 0;
            presenter->modelBatches = batches;
            presenter->modelBatchCapacity = capacity;
        }
        batch = &presenter->modelBatches[
            presenter->modelBatchCount++];
        batch->firstVertex = (UINT)presenter->modelVertexCount;
        batch->vertexCount = 0;
        batch->textureView = view;
        batch->sampler = sampler;
        batch->materialType = material_type;
    }
    batch = &presenter->modelBatches[presenter->modelBatchCount - 1];
    presenter->modelVertices[presenter->modelVertexCount++] = *first;
    presenter->modelVertices[presenter->modelVertexCount++] = *second;
    presenter->modelVertices[presenter->modelVertexCount++] = *third;
    batch->vertexCount += 3;
    return 1;
}

int jpb_PCD3D11PresenterModelTriangle(
    void *user_data,
    const JPBSoftwareMaterialVertex *first,
    const JPBSoftwareMaterialVertex *second,
    const JPBSoftwareMaterialVertex *third,
    const JPBSoftwareTexture *texture)
{
    return pc_present_append_triangle(
        (JPBPCD3D11Presenter *)user_data,
        first, second, third, texture, -1);
}

int jpb_PCD3D11PresenterScreenPolyTriangle(
    void *user_data,
    const JPBSoftwareMaterialVertex *first,
    const JPBSoftwareMaterialVertex *second,
    const JPBSoftwareMaterialVertex *third,
    const JPBSoftwareTexture *texture)
{
    return pc_present_append_triangle(
        (JPBPCD3D11Presenter *)user_data,
        first, second, third, texture,
        texture != NULL ? texture->materialType : 0);
}

static void pc_present_gameplay_scissor_rect(
    const JPBSoftwareFramebuffer *framebuffer,
    D3D11_RECT *rect)
{
    float viewport_x;
    float viewport_y;
    float viewport_width;
    float viewport_height;
    LONG left;
    LONG top;
    LONG right;
    LONG bottom;

    if (rect == NULL) {
        return;
    }
    memset(rect, 0, sizeof(*rect));
    if (framebuffer == NULL ||
        framebuffer->width <= 0 ||
        framebuffer->height <= 0) {
        return;
    }
    jpb_PcGameplayViewport(
        (float)framebuffer->width,
        (float)framebuffer->height,
        &viewport_x,
        &viewport_y,
        &viewport_width,
        &viewport_height);
    left = (LONG)floorf(viewport_x);
    top = (LONG)floorf(viewport_y);
    right = (LONG)ceilf(viewport_x + viewport_width);
    bottom = (LONG)ceilf(viewport_y + viewport_height);
    if (left < 0) left = 0;
    if (top < 0) top = 0;
    if (right > framebuffer->width) right = (LONG)framebuffer->width;
    if (bottom > framebuffer->height) bottom = (LONG)framebuffer->height;
    if (right < left) right = left;
    if (bottom < top) bottom = top;
    rect->left = left;
    rect->top = top;
    rect->right = right;
    rect->bottom = bottom;
}

static HRESULT pc_present_submit_material_triangles(
    JPBPCD3D11Presenter *presenter,
    JPBSoftwareFramebuffer *framebuffer,
    int screen_polys,
    int gameplay_scissor,
    double *upload_seconds,
    double *submit_seconds)
{
    D3D11_BUFFER_DESC desc;
    D3D11_MAPPED_SUBRESOURCE mapped;
    D3D11_VIEWPORT viewport;
    D3D11_RECT scissor_rect;
    ID3D11RenderTargetView *targets[2];
    ID3D11ShaderResourceView *null_view = NULL;
    UINT stride = sizeof(JPBSoftwareMaterialVertex);
    UINT offset = 0;
    size_t batch_index;
    HRESULT result = S_OK;
    LARGE_INTEGER frequency;
    LARGE_INTEGER started;
    LARGE_INTEGER uploaded;
    LARGE_INTEGER submitted;

    if (presenter == NULL || framebuffer == NULL) return E_INVALIDARG;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&started);
    uploaded = started;
    submitted = started;
    if (presenter->modelVertexCount != 0) {
        if (presenter->modelVertexCount > UINT_MAX / stride) {
            return 0;
        }
        if (presenter->modelVertexCount >
            presenter->modelGpuVertexCapacity) {
            size_t gpu_capacity = presenter->modelVertexCapacity;

            if (gpu_capacity < presenter->modelVertexCount) {
                gpu_capacity = presenter->modelVertexCount;
            }
            if (gpu_capacity > UINT_MAX / stride) {
                return 0;
            }
            if (presenter->modelVertexBuffer != NULL) {
                ID3D11Buffer_Release(presenter->modelVertexBuffer);
                presenter->modelVertexBuffer = NULL;
            }
            memset(&desc, 0, sizeof(desc));
            desc.ByteWidth = (UINT)(gpu_capacity * stride);
            desc.Usage = D3D11_USAGE_DYNAMIC;
            desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
            desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
            result = ID3D11Device_CreateBuffer(
                presenter->device, &desc, NULL,
                &presenter->modelVertexBuffer);
            if (FAILED(result)) goto finish;
            presenter->modelGpuVertexCapacity = gpu_capacity;
        }
        result = ID3D11DeviceContext_Map(
            presenter->context,
            (ID3D11Resource *)presenter->modelVertexBuffer,
            0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        if (FAILED(result)) goto finish;
        memcpy(mapped.pData, presenter->modelVertices,
               presenter->modelVertexCount * stride);
        ID3D11DeviceContext_Unmap(
            presenter->context,
            (ID3D11Resource *)presenter->modelVertexBuffer, 0);
        result = ID3D11DeviceContext_Map(
            presenter->context,
            (ID3D11Resource *)presenter->modelConstants,
            0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        if (FAILED(result)) goto finish;
        ((float *)mapped.pData)[0] = 2.0f / framebuffer->width;
        ((float *)mapped.pData)[1] = 2.0f / framebuffer->height;
        ((float *)mapped.pData)[2] = 0.0f;
        ((float *)mapped.pData)[3] = 0.0f;
        ID3D11DeviceContext_Unmap(
            presenter->context,
            (ID3D11Resource *)presenter->modelConstants, 0);
        QueryPerformanceCounter(&uploaded);
        memset(&viewport, 0, sizeof(viewport));
        viewport.Width = (float)framebuffer->width;
        viewport.Height = (float)framebuffer->height;
        viewport.MaxDepth = 1.0f;
        ID3D11DeviceContext_RSSetViewports(
            presenter->context, 1, &viewport);
        if (gameplay_scissor) {
            pc_present_gameplay_scissor_rect(framebuffer, &scissor_rect);
            ID3D11DeviceContext_RSSetScissorRects(
                presenter->context, 1, &scissor_rect);
        }
        targets[0] = presenter->worldColorTarget;
        targets[1] = presenter->worldLinearDepthTarget;
        ID3D11DeviceContext_OMSetRenderTargets(
            presenter->context, 2, targets,
            presenter->worldDepthTarget);
        ID3D11DeviceContext_OMSetBlendState(
            presenter->context, presenter->worldAlphaBlend,
            NULL, UINT_MAX);
        ID3D11DeviceContext_OMSetDepthStencilState(
            presenter->context, presenter->worldDepthWrite, 0);
        ID3D11DeviceContext_RSSetState(
            presenter->context,
            gameplay_scissor
                ? presenter->worldScissorRasterizer
                : presenter->worldRasterizer);
        ID3D11DeviceContext_IASetInputLayout(
            presenter->context,
            screen_polys
                ? presenter->screenInputLayout
                : presenter->modelInputLayout);
        ID3D11DeviceContext_IASetPrimitiveTopology(
            presenter->context, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        ID3D11DeviceContext_IASetVertexBuffers(
            presenter->context, 0, 1,
            &presenter->modelVertexBuffer, &stride, &offset);
        ID3D11DeviceContext_VSSetShader(
            presenter->context,
            screen_polys
                ? presenter->screenVertexShader
                : presenter->modelVertexShader,
            NULL, 0);
        ID3D11DeviceContext_VSSetConstantBuffers(
            presenter->context, 0, 1, &presenter->modelConstants);
        ID3D11DeviceContext_PSSetShader(
            presenter->context,
            screen_polys
                ? presenter->screenOpaquePixelShader
                : presenter->modelPixelShader,
            NULL, 0);
        for (batch_index = 0;
             batch_index < presenter->modelBatchCount; ++batch_index) {
            JPBPCD3D11ModelBatch *batch =
                &presenter->modelBatches[batch_index];

            if (screen_polys) {
                int transparent = batch->materialType != 0;
                ID3D11BlendState *blend =
                    batch->materialType == 1
                        ? presenter->worldAdditiveBlend
                        : (transparent
                               ? presenter->worldGlassBlend
                               : presenter->worldOpaqueBlend);

                ID3D11DeviceContext_PSSetShader(
                    presenter->context,
                    transparent
                        ? presenter->screenTransparentPixelShader
                        : presenter->screenOpaquePixelShader,
                    NULL, 0);
                ID3D11DeviceContext_OMSetBlendState(
                    presenter->context, blend, NULL, UINT_MAX);
                ID3D11DeviceContext_OMSetDepthStencilState(
                    presenter->context,
                    transparent
                        ? presenter->screenDepthRead
                        : presenter->worldDepthWrite,
                    0);
            }
            ID3D11DeviceContext_PSSetShaderResources(
                presenter->context, 0, 1, &batch->textureView);
            ID3D11DeviceContext_PSSetSamplers(
                presenter->context, 0, 1, &batch->sampler);
            ID3D11DeviceContext_Draw(
                presenter->context, batch->vertexCount,
                batch->firstVertex);
        }
        ID3D11DeviceContext_PSSetShaderResources(
            presenter->context, 0, 1, &null_view);
        ID3D11DeviceContext_OMSetRenderTargets(
            presenter->context, 0, NULL, NULL);
        ID3D11DeviceContext_RSSetState(
            presenter->context, presenter->worldRasterizer);
    }
    QueryPerformanceCounter(&submitted);
 finish:
    if (upload_seconds != NULL) {
        *upload_seconds =
            (double)(uploaded.QuadPart - started.QuadPart) /
            (double)frequency.QuadPart;
    }
    if (submit_seconds != NULL) {
        *submit_seconds =
            (double)(submitted.QuadPart - uploaded.QuadPart) /
            (double)frequency.QuadPart;
    }
    return result;
}

int jpb_PCD3D11PresenterEndModels(
    JPBPCD3D11Presenter *presenter,
    JPBSoftwareFramebuffer *framebuffer,
    JPBSoftwareDepthBuffer *depth_buffer)
{
    double upload_seconds = 0.0;
    double submit_seconds = 0.0;
    HRESULT result;

    if (presenter == NULL || framebuffer == NULL ||
        depth_buffer == NULL) return 0;
    result = pc_present_submit_material_triangles(
        presenter, framebuffer, 0, 1,
        &upload_seconds, &submit_seconds);
    ++presenter->modelTimingFrames;
    presenter->modelUploadSeconds += upload_seconds;
    presenter->modelSubmitSeconds += submit_seconds;
    presenter->lastError = result;
    return SUCCEEDED(result);
}

int jpb_PCD3D11PresenterBeginScreenPolys(
    JPBPCD3D11Presenter *presenter,
    JPBSoftwareFramebuffer *framebuffer,
    JPBSoftwareDepthBuffer *depth_buffer)
{
    (void)framebuffer;
    (void)depth_buffer;
    if (presenter == NULL || presenter->worldColorTarget == NULL ||
        presenter->worldDepthTarget == NULL) {
        return 0;
    }
    presenter->modelVertexCount = 0;
    presenter->modelBatchCount = 0;
    return 1;
}

int jpb_PCD3D11PresenterEndScreenPolys(
    JPBPCD3D11Presenter *presenter,
    JPBSoftwareFramebuffer *framebuffer,
    JPBSoftwareDepthBuffer *depth_buffer)
{
    HRESULT result;
    int y;

    if (presenter == NULL || framebuffer == NULL ||
        depth_buffer == NULL) return 0;
    result = pc_present_submit_material_triangles(
        presenter, framebuffer, 1, 1, NULL, NULL);
    if (SUCCEEDED(result)) {
        for (y = 0; y < framebuffer->height; ++y) {
            memset(
                framebuffer->pixels +
                    (size_t)y * framebuffer->stridePixels,
                0,
                (size_t)framebuffer->width * sizeof(uint32_t));
        }
        presenter->gameplayCompositeReady = 0;
    }
    presenter->lastError = result;
    return SUCCEEDED(result);
}

static HRESULT pc_present_upload_gameplay_layer(
    JPBPCD3D11Presenter *presenter,
    int layer,
    const JPBSoftwareFramebuffer *framebuffer)
{
    D3D11_MAPPED_SUBRESOURCE mapped;
    HRESULT result;
    int y;

    if (presenter == NULL || framebuffer == NULL ||
        layer < 0 || layer >= 2 ||
        framebuffer->width != presenter->worldWidth ||
        framebuffer->height != presenter->worldHeight) {
        return E_INVALIDARG;
    }
    result = ID3D11DeviceContext_Map(
        presenter->context,
        (ID3D11Resource *)presenter->gameplayHud[
            presenter->gameplayHudBuffer][layer],
        0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    if (FAILED(result)) return result;
    for (y = 0; y < framebuffer->height; ++y) {
        memcpy(
            (uint8_t *)mapped.pData + (size_t)y * mapped.RowPitch,
            framebuffer->pixels +
                (size_t)y * framebuffer->stridePixels,
            (size_t)framebuffer->width * sizeof(uint32_t));
    }
    ID3D11DeviceContext_Unmap(
        presenter->context,
        (ID3D11Resource *)presenter->gameplayHud[
            presenter->gameplayHudBuffer][layer], 0);
    return S_OK;
}

static HRESULT pc_present_compose_gameplay_base(
    JPBPCD3D11Presenter *presenter)
{
    D3D11_VIEWPORT viewport;
    ID3D11ShaderResourceView *views[3];
    ID3D11ShaderResourceView *null_views[3] = {NULL, NULL, NULL};

    memset(&viewport, 0, sizeof(viewport));
    viewport.Width = (float)presenter->worldWidth;
    viewport.Height = (float)presenter->worldHeight;
    viewport.MaxDepth = 1.0f;
    views[0] = presenter->worldColorView;
    views[1] = presenter->gameplayHudView[
        presenter->gameplayHudBuffer][0];
    views[2] = presenter->gameplayHudView[
        presenter->gameplayHudBuffer][1];
    ID3D11DeviceContext_OMSetRenderTargets(
        presenter->context, 1,
        &presenter->gameplayCompositeTarget, NULL);
    ID3D11DeviceContext_OMSetBlendState(
        presenter->context, presenter->worldOpaqueBlend,
        NULL, UINT_MAX);
    ID3D11DeviceContext_RSSetViewports(
        presenter->context, 1, &viewport);
    ID3D11DeviceContext_IASetInputLayout(
        presenter->context, NULL);
    ID3D11DeviceContext_IASetPrimitiveTopology(
        presenter->context, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ID3D11DeviceContext_VSSetShader(
        presenter->context, presenter->vertexShader, NULL, 0);
    ID3D11DeviceContext_PSSetShader(
        presenter->context,
        presenter->gameplayCompositePixelShader, NULL, 0);
    ID3D11DeviceContext_PSSetShaderResources(
        presenter->context, 0, 3, views);
    ID3D11DeviceContext_PSSetSamplers(
        presenter->context, 0, 1,
        &presenter->modelPointSampler);
    ID3D11DeviceContext_Draw(presenter->context, 3, 0);
    ID3D11DeviceContext_PSSetShaderResources(
        presenter->context, 0, 3, null_views);
    ID3D11DeviceContext_OMSetRenderTargets(
        presenter->context, 0, NULL, NULL);
    return S_OK;
}

int jpb_PCD3D11PresenterGameplayComposite(
    JPBPCD3D11Presenter *presenter,
    int stage,
    const JPBSoftwareFramebuffer *framebuffer,
    JPBSoftwareRenderStats *stats)
{
    HRESULT result;

    (void)stats;
    if (presenter == NULL || framebuffer == NULL) return 0;
    if (stage == JPB_GAMEPLAY_COMPOSITE_HUD_BLACK ||
        stage == JPB_GAMEPLAY_COMPOSITE_HUD_WHITE) {
        if (stage == JPB_GAMEPLAY_COMPOSITE_HUD_BLACK) {
            presenter->gameplayHudBuffer =
                (presenter->gameplayHudBuffer + 1) %
                PC_GAMEPLAY_HUD_BUFFER_COUNT;
        }
        result = pc_present_upload_gameplay_layer(
            presenter,
            stage == JPB_GAMEPLAY_COMPOSITE_HUD_BLACK ? 0 : 1,
            framebuffer);
    } else if (stage == JPB_GAMEPLAY_COMPOSITE_FINISH) {
        result = pc_present_compose_gameplay_base(presenter);
        presenter->gameplayCompositeReady = SUCCEEDED(result);
    } else {
        result = E_INVALIDARG;
    }
    presenter->lastError = result;
    return SUCCEEDED(result);
}

int jpb_PCD3D11PresenterReadbackGameplay(
    JPBPCD3D11Presenter *presenter,
    JPBSoftwareFramebuffer *framebuffer)
{
    HRESULT result;

    if (presenter == NULL || framebuffer == NULL ||
        !presenter->gameplayCompositeReady) return 0;
    ID3D11DeviceContext_CopyResource(
        presenter->context,
        (ID3D11Resource *)presenter->worldColorReadback,
        (ID3D11Resource *)presenter->gameplayComposite);
    result = pc_present_read_color(presenter, framebuffer, NULL);
    presenter->lastError = result;
    return SUCCEEDED(result);
}

static void pc_present_set_title_vertex(
    JPBSoftwareMaterialVertex *vertex,
    float x,
    float y,
    float u,
    float v,
    CVECTOR color)
{
    memset(vertex, 0, sizeof(*vertex));
    vertex->x = x;
    vertex->y = y;
    vertex->depth = 0.0001f;
    vertex->inverseDepth = 10000.0f;
    vertex->clipDepth = 0.0001f;
    vertex->u = u;
    vertex->v = v;
    vertex->red = (float)color.r;
    vertex->green = (float)color.g;
    vertex->blue = (float)color.b;
    vertex->alpha = (float)color.cd;
}

int jpb_PCD3D11PresenterRenderTitleScreenDraws(
    JPBPCD3D11Presenter *presenter,
    const JPBGameRuntimeScreenDraw *draws,
    size_t draw_count,
    JPBSoftwareFramebuffer *framebuffer)
{
    size_t draw_order[JPB_GAME_RUNTIME_SCREEN_DRAW_CAPACITY];
    LARGE_INTEGER frequency;
    LARGE_INTEGER started;
    LARGE_INTEGER prepared;
    LARGE_INTEGER submitted;
    HRESULT result;
    size_t draw_index;
    double readback_seconds = 0.0;

    if (presenter == NULL || framebuffer == NULL ||
        framebuffer->pixels == NULL || framebuffer->width <= 0 ||
        framebuffer->height <= 0 ||
        framebuffer->stridePixels < framebuffer->width ||
        (draw_count != 0 && draws == NULL) ||
        draw_count > JPB_GAME_RUNTIME_SCREEN_DRAW_CAPACITY) {
        return 0;
    }
    presenter->gameplayCompositeReady = 0;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&started);
    result = pc_present_create_world_targets(
        presenter, framebuffer->width, framebuffer->height);
    if (FAILED(result)) goto finish;

    ID3D11DeviceContext_OMSetRenderTargets(
        presenter->context, 0, NULL, NULL);
    ID3D11DeviceContext_UpdateSubresource(
        presenter->context,
        (ID3D11Resource *)presenter->worldColor,
        0, NULL, framebuffer->pixels,
        (UINT)(framebuffer->stridePixels * sizeof(uint32_t)), 0);
    ID3D11DeviceContext_ClearDepthStencilView(
        presenter->context, presenter->worldDepthTarget,
        D3D11_CLEAR_DEPTH, 1.0f, 0);
    presenter->modelVertexCount = 0;
    presenter->modelBatchCount = 0;

    for (draw_index = 0; draw_index < draw_count; ++draw_index) {
        size_t insertion = draw_index;

        draw_order[draw_index] = draw_index;
        while (insertion > 0) {
            size_t previous = draw_order[insertion - 1];
            size_t current = draw_order[insertion];

            if (draws[previous].layerDepth >=
                draws[current].layerDepth) {
                break;
            }
            draw_order[insertion - 1] = current;
            draw_order[insertion] = previous;
            --insertion;
        }
    }
    for (draw_index = 0; draw_index < draw_count; ++draw_index) {
        const JPBGameRuntimeScreenDraw *draw =
            &draws[draw_order[draw_index]];
        const JPBSoftwareTexture *texture = NULL;
        JPBSoftwareMaterialVertex vertices[4];
        CVECTOR color = draw->color;
        float source_left = 0.0f;
        float source_top = 0.0f;
        float source_right = 1.0f;
        float source_bottom = 1.0f;

        if (draw->destination.left == draw->destination.right ||
            draw->destination.top == draw->destination.bottom) {
            continue;
        }
        if (draw->texture != NULL &&
            draw->texture->texture != NULL) {
            const JPBSoftwareTexture *candidate =
                (const JPBSoftwareTexture *)draw->texture->texture;

            if (candidate->pixels != NULL && candidate->width > 0 &&
                candidate->height > 0 &&
                candidate->stridePixels >= candidate->width) {
                texture = candidate;
            }
        }
        if (texture != NULL) {
            int source_x0 = draw->hasSource ? draw->source.left : 0;
            int source_y0 = draw->hasSource ? draw->source.top : 0;
            int source_x1 = draw->hasSource
                ? draw->source.right : (int)texture->width;
            int source_y1 = draw->hasSource
                ? draw->source.bottom : (int)texture->height;

            source_left = (float)source_x0 / (float)texture->width;
            source_top = (float)source_y0 / (float)texture->height;
            source_right = (float)source_x1 / (float)texture->width;
            source_bottom = (float)source_y1 / (float)texture->height;
            if (color.r == 0 || color.g == 0 ||
                color.b == 0 || color.cd == 0) {
                color.r = UINT8_MAX;
                color.g = UINT8_MAX;
                color.b = UINT8_MAX;
                color.cd = UINT8_MAX;
            }
        }
        pc_present_set_title_vertex(
            &vertices[0],
            (float)draw->destination.left,
            (float)draw->destination.top,
            source_left, source_top, color);
        pc_present_set_title_vertex(
            &vertices[1],
            (float)draw->destination.left,
            (float)draw->destination.bottom,
            source_left, source_bottom, color);
        pc_present_set_title_vertex(
            &vertices[2],
            (float)draw->destination.right,
            (float)draw->destination.top,
            source_right, source_top, color);
        pc_present_set_title_vertex(
            &vertices[3],
            (float)draw->destination.right,
            (float)draw->destination.bottom,
            source_right, source_bottom, color);
        if (!pc_present_append_triangle(
                presenter, &vertices[0], &vertices[1], &vertices[2],
                texture, 2) ||
            !pc_present_append_triangle(
                presenter, &vertices[1], &vertices[3], &vertices[2],
                texture, 2)) {
            result = E_OUTOFMEMORY;
            goto finish;
        }
    }
    QueryPerformanceCounter(&prepared);
    result = pc_present_submit_material_triangles(
        presenter, framebuffer, 1, 0, NULL, NULL);
    QueryPerformanceCounter(&submitted);
    if (SUCCEEDED(result)) {
        ID3D11DeviceContext_CopyResource(
            presenter->context,
            (ID3D11Resource *)presenter->worldColorReadback,
            (ID3D11Resource *)presenter->worldColor);
        result = pc_present_read_color(
            presenter, framebuffer, &readback_seconds);
    }
 finish:
    if (FAILED(result)) {
        QueryPerformanceCounter(&prepared);
        submitted = prepared;
    }
    ++presenter->titleTimingFrames;
    presenter->titlePrepareSeconds +=
        (double)(prepared.QuadPart - started.QuadPart) /
        (double)frequency.QuadPart;
    presenter->titleSubmitSeconds +=
        (double)(submitted.QuadPart - prepared.QuadPart) /
        (double)frequency.QuadPart;
    presenter->titleReadbackSeconds += readback_seconds;
    presenter->lastError = result;
    return SUCCEEDED(result);
}

static void pc_present_describe_adapter(JPBPCD3D11Presenter *presenter)
{
    IDXGIDevice *dxgi_device = NULL;
    IDXGIAdapter *adapter = NULL;
    DXGI_ADAPTER_DESC desc;
    char name[96] = "Direct3D 11 hardware";

    if (SUCCEEDED(ID3D11Device_QueryInterface(
            presenter->device,
            &IID_IDXGIDevice,
            (void **)&dxgi_device)) &&
        SUCCEEDED(IDXGIDevice_GetAdapter(dxgi_device, &adapter)) &&
        SUCCEEDED(IDXGIAdapter_GetDesc(adapter, &desc))) {
        WideCharToMultiByte(
            CP_UTF8, 0, desc.Description, -1,
            name, (int)sizeof(name), NULL, NULL);
    }
    snprintf(
        presenter->description,
        sizeof(presenter->description),
        "D3D11 hardware adapter=%s feature=0x%04x",
        name,
        (unsigned)ID3D11Device_GetFeatureLevel(presenter->device));
    if (adapter != NULL) IDXGIAdapter_Release(adapter);
    if (dxgi_device != NULL) IDXGIDevice_Release(dxgi_device);
}

JPBPCD3D11Presenter *jpb_PCD3D11PresenterCreate(HWND window)
{
    JPBPCD3D11Presenter *presenter;
    DXGI_SWAP_CHAIN_DESC swap_desc;
    D3D_FEATURE_LEVEL requested_levels[] = {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0
    };
    D3D_FEATURE_LEVEL selected_level;
    HRESULT result;

    if (window == NULL) {
        return NULL;
    }
    presenter = (JPBPCD3D11Presenter *)calloc(1, sizeof(*presenter));
    if (presenter == NULL) {
        return NULL;
    }
    presenter->window = window;
    memset(&swap_desc, 0, sizeof(swap_desc));
    swap_desc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    swap_desc.SampleDesc.Count = 1;
    swap_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swap_desc.BufferCount = 2;
    swap_desc.OutputWindow = window;
    swap_desc.Windowed = TRUE;
    swap_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    result = D3D11CreateDeviceAndSwapChain(
        NULL,
        D3D_DRIVER_TYPE_HARDWARE,
        NULL,
        D3D11_CREATE_DEVICE_BGRA_SUPPORT,
        requested_levels,
        sizeof(requested_levels) / sizeof(requested_levels[0]),
        D3D11_SDK_VERSION,
        &swap_desc,
        &presenter->swapChain,
        &presenter->device,
        &selected_level,
        &presenter->context);
    if (result == E_INVALIDARG) {
        result = D3D11CreateDeviceAndSwapChain(
            NULL,
            D3D_DRIVER_TYPE_HARDWARE,
            NULL,
            D3D11_CREATE_DEVICE_BGRA_SUPPORT,
            requested_levels + 1,
            sizeof(requested_levels) / sizeof(requested_levels[0]) - 1,
            D3D11_SDK_VERSION,
            &swap_desc,
            &presenter->swapChain,
            &presenter->device,
            &selected_level,
            &presenter->context);
    }
    presenter->lastError = result;
    if (FAILED(result)) {
        jpb_PCD3D11PresenterDestroy(presenter);
        return NULL;
    }
    result = pc_present_create_target(presenter);
    if (SUCCEEDED(result)) result = pc_present_create_shaders(presenter);
    if (SUCCEEDED(result)) result = pc_present_create_states(presenter);
    if (SUCCEEDED(result)) {
        result = pc_present_compile_world_shaders(presenter);
    }
    if (SUCCEEDED(result)) {
        result = pc_present_create_world_states(presenter);
    }
    if (SUCCEEDED(result)) {
        result = pc_present_compile_model_shaders(presenter);
    }
    if (SUCCEEDED(result)) {
        result = pc_present_create_model_states(presenter);
    }
    presenter->lastError = result;
    if (FAILED(result)) {
        jpb_PCD3D11PresenterDestroy(presenter);
        return NULL;
    }
    pc_present_describe_adapter(presenter);
    return presenter;
}

void jpb_PCD3D11PresenterDestroy(JPBPCD3D11Presenter *presenter)
{
    if (presenter == NULL) {
        return;
    }
    if (presenter->context != NULL) {
        ID3D11DeviceContext_ClearState(presenter->context);
        ID3D11DeviceContext_Flush(presenter->context);
    }
    pc_present_release_world_mesh(presenter);
    pc_present_release_world_targets(presenter);
    free(presenter->modelBatches);
    free(presenter->modelVertices);
    if (presenter->modelVertexBuffer != NULL) ID3D11Buffer_Release(presenter->modelVertexBuffer);
    if (presenter->modelPointSampler != NULL) ID3D11SamplerState_Release(presenter->modelPointSampler);
    if (presenter->modelLinearSampler != NULL) ID3D11SamplerState_Release(presenter->modelLinearSampler);
    if (presenter->modelConstants != NULL) ID3D11Buffer_Release(presenter->modelConstants);
    if (presenter->modelInputLayout != NULL) ID3D11InputLayout_Release(presenter->modelInputLayout);
    if (presenter->screenInputLayout != NULL) ID3D11InputLayout_Release(presenter->screenInputLayout);
    if (presenter->screenTransparentPixelShader != NULL) ID3D11PixelShader_Release(presenter->screenTransparentPixelShader);
    if (presenter->screenOpaquePixelShader != NULL) ID3D11PixelShader_Release(presenter->screenOpaquePixelShader);
    if (presenter->modelPixelShader != NULL) ID3D11PixelShader_Release(presenter->modelPixelShader);
    if (presenter->modelVertexShader != NULL) ID3D11VertexShader_Release(presenter->modelVertexShader);
    if (presenter->screenVertexShader != NULL) ID3D11VertexShader_Release(presenter->screenVertexShader);
    if (presenter->worldWhiteView != NULL) ID3D11ShaderResourceView_Release(presenter->worldWhiteView);
    if (presenter->worldSampler != NULL) ID3D11SamplerState_Release(presenter->worldSampler);
    if (presenter->worldScissorRasterizer != NULL) ID3D11RasterizerState_Release(presenter->worldScissorRasterizer);
    if (presenter->worldRasterizer != NULL) ID3D11RasterizerState_Release(presenter->worldRasterizer);
    if (presenter->screenDepthRead != NULL) ID3D11DepthStencilState_Release(presenter->screenDepthRead);
    if (presenter->worldDepthRead != NULL) ID3D11DepthStencilState_Release(presenter->worldDepthRead);
    if (presenter->worldDepthWrite != NULL) ID3D11DepthStencilState_Release(presenter->worldDepthWrite);
    if (presenter->worldGlassBlend != NULL) ID3D11BlendState_Release(presenter->worldGlassBlend);
    if (presenter->worldAdditiveBlend != NULL) ID3D11BlendState_Release(presenter->worldAdditiveBlend);
    if (presenter->worldAlphaBlend != NULL) ID3D11BlendState_Release(presenter->worldAlphaBlend);
    if (presenter->worldOpaqueBlend != NULL) ID3D11BlendState_Release(presenter->worldOpaqueBlend);
    if (presenter->worldConstants != NULL) ID3D11Buffer_Release(presenter->worldConstants);
    if (presenter->worldInputLayout != NULL) ID3D11InputLayout_Release(presenter->worldInputLayout);
    if (presenter->worldPixelShader != NULL) ID3D11PixelShader_Release(presenter->worldPixelShader);
    if (presenter->worldVertexShader != NULL) ID3D11VertexShader_Release(presenter->worldVertexShader);
    pc_present_release_upload(presenter);
    pc_present_release_target(presenter);
    if (presenter->constants != NULL) ID3D11Buffer_Release(presenter->constants);
    if (presenter->sampler != NULL) ID3D11SamplerState_Release(presenter->sampler);
    if (presenter->pixelShader != NULL) ID3D11PixelShader_Release(presenter->pixelShader);
    if (presenter->gameplayCompositePixelShader != NULL) ID3D11PixelShader_Release(presenter->gameplayCompositePixelShader);
    if (presenter->vertexShader != NULL) ID3D11VertexShader_Release(presenter->vertexShader);
    if (presenter->swapChain != NULL) IDXGISwapChain_Release(presenter->swapChain);
    if (presenter->context != NULL) ID3D11DeviceContext_Release(presenter->context);
    if (presenter->device != NULL) ID3D11Device_Release(presenter->device);
    free(presenter);
}

int jpb_PCD3D11PresenterPresent(
    JPBPCD3D11Presenter *presenter,
    const JPBSoftwareFramebuffer *framebuffer)
{
    RECT client;
    D3D11_MAPPED_SUBRESOURCE mapped;
    D3D11_VIEWPORT viewport;
    JPBPCD3D11Constants *constants;
    ID3D11ShaderResourceView *null_view = NULL;
    ID3D11ShaderResourceView *source_view;
    float clear[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    int client_width;
    int client_height;
    int viewport_width;
    int viewport_height;
    int viewport_x;
    int viewport_y;
    HRESULT result;
    int y;

    if (presenter == NULL || framebuffer == NULL ||
        framebuffer->pixels == NULL || framebuffer->width <= 0 ||
        framebuffer->height <= 0 ||
        framebuffer->stridePixels < framebuffer->width ||
        !GetClientRect(presenter->window, &client)) {
        return 0;
    }
    client_width = client.right - client.left;
    client_height = client.bottom - client.top;
    if (client_width <= 0 || client_height <= 0) {
        return 1;
    }
    result = pc_present_resize(presenter, client_width, client_height);
    if (SUCCEEDED(result) && !presenter->gameplayCompositeReady &&
        (presenter->sourceWidth != framebuffer->width ||
         presenter->sourceHeight != framebuffer->height)) {
        result = pc_present_create_upload(
            presenter, framebuffer->width, framebuffer->height);
    }
    if (FAILED(result)) {
        presenter->lastError = result;
        return 0;
    }
    if (!presenter->gameplayCompositeReady) {
        result = ID3D11DeviceContext_Map(
            presenter->context,
            (ID3D11Resource *)presenter->uploadTexture,
            0,
            D3D11_MAP_WRITE_DISCARD,
            0,
            &mapped);
        if (FAILED(result)) {
            presenter->lastError = result;
            return 0;
        }
        for (y = 0; y < framebuffer->height; ++y) {
            memcpy(
                (uint8_t *)mapped.pData + (size_t)y * mapped.RowPitch,
                framebuffer->pixels +
                    (size_t)y * (size_t)framebuffer->stridePixels,
                (size_t)framebuffer->width * sizeof(uint32_t));
        }
        ID3D11DeviceContext_Unmap(
            presenter->context,
            (ID3D11Resource *)presenter->uploadTexture,
            0);
    }
    result = ID3D11DeviceContext_Map(
        presenter->context,
        (ID3D11Resource *)presenter->constants,
        0,
        D3D11_MAP_WRITE_DISCARD,
        0,
        &mapped);
    if (FAILED(result)) {
        presenter->lastError = result;
        return 0;
    }
    constants = (JPBPCD3D11Constants *)mapped.pData;
    constants->inverseSourceWidth = 1.0f / (float)framebuffer->width;
    constants->inverseSourceHeight = 1.0f / (float)framebuffer->height;
    constants->padding[0] = 0.0f;
    constants->padding[1] = 0.0f;
    ID3D11DeviceContext_Unmap(
        presenter->context,
        (ID3D11Resource *)presenter->constants,
        0);

    viewport_width = client_width;
    viewport_height = client_height;
    if ((int64_t)client_width * framebuffer->height >
        (int64_t)client_height * framebuffer->width) {
        viewport_width = (int)(
            (int64_t)client_height * framebuffer->width /
            framebuffer->height);
    } else {
        viewport_height = (int)(
            (int64_t)client_width * framebuffer->height /
            framebuffer->width);
    }
    viewport_x = (client_width - viewport_width) / 2;
    viewport_y = (client_height - viewport_height) / 2;
    memset(&viewport, 0, sizeof(viewport));
    viewport.TopLeftX = (float)viewport_x;
    viewport.TopLeftY = (float)viewport_y;
    viewport.Width = (float)viewport_width;
    viewport.Height = (float)viewport_height;
    viewport.MaxDepth = 1.0f;

    ID3D11DeviceContext_ClearRenderTargetView(
        presenter->context, presenter->renderTarget, clear);
    ID3D11DeviceContext_OMSetRenderTargets(
        presenter->context, 1, &presenter->renderTarget, NULL);
    ID3D11DeviceContext_RSSetViewports(presenter->context, 1, &viewport);
    ID3D11DeviceContext_IASetInputLayout(presenter->context, NULL);
    ID3D11DeviceContext_IASetPrimitiveTopology(
        presenter->context, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ID3D11DeviceContext_VSSetShader(
        presenter->context, presenter->vertexShader, NULL, 0);
    ID3D11DeviceContext_PSSetShader(
        presenter->context, presenter->pixelShader, NULL, 0);
    source_view = presenter->gameplayCompositeReady
        ? presenter->gameplayCompositeView
        : presenter->uploadView;
    ID3D11DeviceContext_PSSetShaderResources(
        presenter->context, 0, 1, &source_view);
    ID3D11DeviceContext_PSSetSamplers(
        presenter->context, 0, 1, &presenter->sampler);
    ID3D11DeviceContext_PSSetConstantBuffers(
        presenter->context, 0, 1, &presenter->constants);
    ID3D11DeviceContext_Draw(presenter->context, 3, 0);
    ID3D11DeviceContext_PSSetShaderResources(
        presenter->context, 0, 1, &null_view);
    result = IDXGISwapChain_Present(
        presenter->swapChain, 0, DXGI_PRESENT_DO_NOT_WAIT);
    if (result == DXGI_ERROR_WAS_STILL_DRAWING) {
        presenter->lastError = S_OK;
        return 1;
    }
    presenter->lastError = result;
    return SUCCEEDED(result);
}

const char *jpb_PCD3D11PresenterDescription(
    const JPBPCD3D11Presenter *presenter)
{
    return presenter != NULL ? presenter->description : "unavailable";
}

long jpb_PCD3D11PresenterLastError(
    const JPBPCD3D11Presenter *presenter)
{
    return presenter != NULL ? (long)presenter->lastError : (long)E_POINTER;
}

static int pc_present_render_level_range(
    JPBPCD3D11Presenter *presenter,
    const JPBSoftwareLevelMesh *mesh,
    int first_pass,
    int last_pass,
    const JPBSoftwareJpxScene *world_scene,
    MATRIX *view_matrix,
    JPBSoftwareFramebuffer *framebuffer,
    uint32_t clear_color,
    JPBPCPresentTextureResolver resolve_texture,
    void *texture_user_data,
    JPBSoftwareDepthBuffer *depth_buffer,
    JPBSoftwareRenderStats *stats)
{
    ID3D11RenderTargetView *targets[2];
    D3D11_MAPPED_SUBRESOURCE mapped;
    D3D11_VIEWPORT viewport;
    JPBPCD3D11WorldConstants *constants;
    ID3D11ShaderResourceView *null_view = NULL;
    float color_clear[4] = {
        (float)((clear_color >> 16) & 255u) / 255.0f,
        (float)((clear_color >> 8) & 255u) / 255.0f,
        (float)(clear_color & 255u) / 255.0f,
        1.0f
    };
    float depth_clear[4] = {FLT_MAX, 0.0f, 0.0f, 0.0f};
    float gameplay_viewport_x;
    float gameplay_viewport_y;
    float gameplay_viewport_width;
    float gameplay_viewport_height;
    UINT stride = sizeof(JPBSoftwareLevelVertex);
    UINT offset = 0;
    size_t vertex_offset = 0;
    size_t batch_index;
    int pass;
    HRESULT result;
    LARGE_INTEGER frequency;
    LARGE_INTEGER started;
    LARGE_INTEGER prepared;
    LARGE_INTEGER submitted;
    LARGE_INTEGER finished;

    (void)world_scene;
    if (presenter == NULL || mesh == NULL || view_matrix == NULL ||
        framebuffer == NULL || framebuffer->pixels == NULL ||
        framebuffer->width <= 0 || framebuffer->height <= 0 ||
        framebuffer->stridePixels < framebuffer->width ||
        depth_buffer == NULL || depth_buffer->values == NULL ||
        depth_buffer->width < (size_t)framebuffer->width ||
        depth_buffer->height < (size_t)framebuffer->height ||
        depth_buffer->strideValues < (size_t)framebuffer->width) {
        return -1;
    }
    if (first_pass < JPB_LEVEL_FBX_PASS_OPAQUE ||
        last_pass > JPB_LEVEL_FBX_PASS_GLASS ||
        first_pass > last_pass) {
        return -1;
    }
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&started);
    jpb_PcGameplayViewport(
        (float)framebuffer->width,
        (float)framebuffer->height,
        &gameplay_viewport_x,
        &gameplay_viewport_y,
        &gameplay_viewport_width,
        &gameplay_viewport_height);
    result = pc_present_create_world_targets(
        presenter, framebuffer->width, framebuffer->height);
    if (SUCCEEDED(result)) {
        result = pc_present_create_world_mesh(presenter, mesh);
    }
    if (FAILED(result)) {
        presenter->lastError = result;
        return -1;
    }
    QueryPerformanceCounter(&prepared);
    result = ID3D11DeviceContext_Map(
        presenter->context,
        (ID3D11Resource *)presenter->worldConstants,
        0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    if (FAILED(result)) {
        presenter->lastError = result;
        return -1;
    }
    constants = (JPBPCD3D11WorldConstants *)mapped.pData;
    constants->row0[0] = view_matrix->m[0][0];
    constants->row0[1] = view_matrix->m[0][1];
    constants->row0[2] = view_matrix->m[0][2];
    constants->row0[3] = (float)view_matrix->t[0];
    constants->row1[0] = view_matrix->m[1][0];
    constants->row1[1] = view_matrix->m[1][1];
    constants->row1[2] = view_matrix->m[1][2];
    constants->row1[3] = (float)view_matrix->t[1];
    constants->row2[0] = view_matrix->m[2][0];
    constants->row2[1] = view_matrix->m[2][1];
    constants->row2[2] = view_matrix->m[2][2];
    constants->row2[3] = (float)view_matrix->t[2];
    constants->projection[1] =
        1.0f / tanf(0.9250245094299316f * 0.5f);
    constants->projection[0] =
        constants->projection[1] *
        gameplay_viewport_height / gameplay_viewport_width;
    constants->projection[2] = 10000.0f / 9999.0f;
    constants->projection[3] = -10000.0f / 9999.0f;
    memset(constants->transparentPass, 0,
           sizeof(constants->transparentPass));
    constants->uvScrollSpeed[0] = g_levelUVScroll.vx;
    constants->uvScrollSpeed[1] = g_levelUVScroll.vy;
    memset(constants->padding, 0, sizeof(constants->padding));
    ID3D11DeviceContext_Unmap(
        presenter->context,
        (ID3D11Resource *)presenter->worldConstants, 0);

    targets[0] = presenter->worldColorTarget;
    targets[1] = presenter->worldLinearDepthTarget;
    if (first_pass == JPB_LEVEL_FBX_PASS_OPAQUE) {
        ID3D11DeviceContext_ClearRenderTargetView(
            presenter->context, targets[0], color_clear);
        ID3D11DeviceContext_ClearRenderTargetView(
            presenter->context, targets[1], depth_clear);
        ID3D11DeviceContext_ClearDepthStencilView(
            presenter->context, presenter->worldDepthTarget,
            D3D11_CLEAR_DEPTH, 1.0f, 0);
    }
    ID3D11DeviceContext_OMSetRenderTargets(
        presenter->context, 2, targets,
        presenter->worldDepthTarget);
    memset(&viewport, 0, sizeof(viewport));
    viewport.TopLeftX = gameplay_viewport_x;
    viewport.TopLeftY = gameplay_viewport_y;
    viewport.Width = gameplay_viewport_width;
    viewport.Height = gameplay_viewport_height;
    viewport.MaxDepth = 1.0f;
    ID3D11DeviceContext_RSSetViewports(
        presenter->context, 1, &viewport);
    ID3D11DeviceContext_RSSetState(
        presenter->context, presenter->worldRasterizer);
    ID3D11DeviceContext_IASetInputLayout(
        presenter->context, presenter->worldInputLayout);
    ID3D11DeviceContext_IASetPrimitiveTopology(
        presenter->context, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ID3D11DeviceContext_IASetVertexBuffers(
        presenter->context, 0, 1,
        &presenter->worldVertexBuffer, &stride, &offset);
    ID3D11DeviceContext_VSSetShader(
        presenter->context, presenter->worldVertexShader, NULL, 0);
    ID3D11DeviceContext_VSSetConstantBuffers(
        presenter->context, 0, 1, &presenter->worldConstants);
    ID3D11DeviceContext_PSSetShader(
        presenter->context, presenter->worldPixelShader, NULL, 0);
    ID3D11DeviceContext_PSSetConstantBuffers(
        presenter->context, 0, 1, &presenter->worldConstants);
    ID3D11DeviceContext_PSSetSamplers(
        presenter->context, 0, 1, &presenter->worldSampler);

    for (pass = first_pass;
         pass <= last_pass; ++pass) {
        ID3D11BlendState *blend =
            pass == JPB_LEVEL_FBX_PASS_OPAQUE
                ? presenter->worldOpaqueBlend
                : (pass == JPB_LEVEL_FBX_PASS_GLASS
                       ? presenter->worldGlassBlend
                       : presenter->worldAlphaBlend);
        ID3D11DepthStencilState *depth_state =
            pass == JPB_LEVEL_FBX_PASS_GLASS
                ? presenter->worldDepthRead
                : presenter->worldDepthWrite;

        result = ID3D11DeviceContext_Map(
            presenter->context,
            (ID3D11Resource *)presenter->worldConstants,
            0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        if (FAILED(result)) break;
        constants = (JPBPCD3D11WorldConstants *)mapped.pData;
        constants->row0[0] = view_matrix->m[0][0];
        constants->row0[1] = view_matrix->m[0][1];
        constants->row0[2] = view_matrix->m[0][2];
        constants->row0[3] = (float)view_matrix->t[0];
        constants->row1[0] = view_matrix->m[1][0];
        constants->row1[1] = view_matrix->m[1][1];
        constants->row1[2] = view_matrix->m[1][2];
        constants->row1[3] = (float)view_matrix->t[1];
        constants->row2[0] = view_matrix->m[2][0];
        constants->row2[1] = view_matrix->m[2][1];
        constants->row2[2] = view_matrix->m[2][2];
        constants->row2[3] = (float)view_matrix->t[2];
        constants->projection[1] =
            1.0f / tanf(0.9250245094299316f * 0.5f);
        constants->projection[0] = constants->projection[1] *
            gameplay_viewport_height / gameplay_viewport_width;
        constants->projection[2] = 10000.0f / 9999.0f;
        constants->projection[3] = -10000.0f / 9999.0f;
        constants->transparentPass[0] =
            pass == JPB_LEVEL_FBX_PASS_OPAQUE ? 0.0f : 1.0f;
        constants->transparentPass[1] = 0.0f;
        constants->transparentPass[2] = 0.0f;
        constants->transparentPass[3] = 0.0f;
        ID3D11DeviceContext_Unmap(
            presenter->context,
            (ID3D11Resource *)presenter->worldConstants, 0);
        ID3D11DeviceContext_OMSetBlendState(
            presenter->context, blend, NULL, UINT_MAX);
        ID3D11DeviceContext_OMSetDepthStencilState(
            presenter->context, depth_state, 0);
        vertex_offset = 0;
        for (batch_index = 0;
             batch_index < mesh->batchCount; ++batch_index) {
            const JPBSoftwareLevelBatch *batch =
                &mesh->batches[batch_index];
            JPBSoftwareTexture texture;
            ID3D11ShaderResourceView *view =
                presenter->worldWhiteView;

            if ((int)batch->pass == pass &&
                batch->vertices != NULL &&
                batch->vertexCount >= 3 &&
                batch->vertexCount % 3 == 0 &&
                batch->meshName != NULL &&
                jpb_ShouldDrawFbxMesh(
                    mesh->levelIndex, batch->pass,
                    batch->meshCount, batch->meshIndex,
                    batch->meshName)) {
                memset(&texture, 0, sizeof(texture));
                texture.colorOverride = -1;
                if (batch->textureName != NULL &&
                    batch->textureName[0] != '\0') {
                    if (resolve_texture == NULL ||
                        !resolve_texture(
                            texture_user_data,
                            batch->textureName,
                            &texture)) {
                        result = E_INVALIDARG;
                        break;
                    }
                    view = pc_present_world_texture(
                        presenter, &texture);
                }
                if (view == NULL) {
                    result = E_OUTOFMEMORY;
                    break;
                }
                ID3D11DeviceContext_PSSetShaderResources(
                    presenter->context, 0, 1, &view);
                ID3D11DeviceContext_Draw(
                    presenter->context,
                    (UINT)batch->vertexCount,
                    (UINT)vertex_offset);
                if (stats != NULL) {
                    stats->triangles += batch->vertexCount / 3;
                    if (pass != JPB_LEVEL_FBX_PASS_OPAQUE) {
                        stats->levelTransparentTriangles +=
                            batch->vertexCount / 3;
                        if (pass == JPB_LEVEL_FBX_PASS_GLASS) {
                            stats->levelGlassTriangles +=
                                batch->vertexCount / 3;
                        }
                    }
                }
            } else if ((int)batch->pass == pass && stats != NULL) {
                stats->levelCulledTriangles +=
                    batch->vertexCount / 3;
            }
            vertex_offset += batch->vertexCount;
        }
        if (FAILED(result)) break;
    }
    ID3D11DeviceContext_PSSetShaderResources(
        presenter->context, 0, 1, &null_view);
    ID3D11DeviceContext_OMSetRenderTargets(
        presenter->context, 0, NULL, NULL);
    if (FAILED(result)) {
        presenter->lastError = result;
        return -1;
    }
    if (last_pass == JPB_LEVEL_FBX_PASS_GLASS) {
        ID3D11DeviceContext_CopyResource(
            presenter->context,
            (ID3D11Resource *)presenter->worldLinearDepthSnapshot,
            (ID3D11Resource *)presenter->worldLinearDepth);
    }
    QueryPerformanceCounter(&submitted);
    QueryPerformanceCounter(&finished);
    if (last_pass == JPB_LEVEL_FBX_PASS_GLASS) {
        ++presenter->worldTimingFrames;
    }
    presenter->worldPrepareSeconds +=
        (double)(prepared.QuadPart - started.QuadPart) /
        (double)frequency.QuadPart;
    presenter->worldSubmitSeconds +=
        (double)(submitted.QuadPart - prepared.QuadPart) /
        (double)frequency.QuadPart;
    presenter->worldReadbackSeconds +=
        (double)(finished.QuadPart - submitted.QuadPart) /
        (double)frequency.QuadPart;
    presenter->lastError = result;
    return SUCCEEDED(result) ? 0 : -1;
}

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
    JPBSoftwareRenderStats *stats)
{
    return pc_present_render_level_range(
        presenter,
        mesh,
        (int)pass,
        (int)pass,
        world_scene,
        view_matrix,
        framebuffer,
        clear_color,
        resolve_texture,
        texture_user_data,
        depth_buffer,
        stats);
}

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
    JPBSoftwareRenderStats *stats)
{
    return pc_present_render_level_range(
        presenter,
        mesh,
        JPB_LEVEL_FBX_PASS_OPAQUE,
        JPB_LEVEL_FBX_PASS_GLASS,
        world_scene,
        view_matrix,
        framebuffer,
        clear_color,
        resolve_texture,
        texture_user_data,
        depth_buffer,
        stats);
}

int jpb_PCD3D11PresenterFinalWorldCoverage(
    JPBPCD3D11Presenter *presenter,
    size_t *covered_pixels)
{
    D3D11_MAPPED_SUBRESOURCE mapped;
    size_t covered = 0;
    int y;
    HRESULT result;

    if (presenter == NULL || covered_pixels == NULL ||
        presenter->worldLinearDepthSnapshot == NULL ||
        presenter->worldLinearDepthReadback == NULL ||
        presenter->worldWidth <= 0 || presenter->worldHeight <= 0) {
        return 0;
    }
    ID3D11DeviceContext_CopyResource(
        presenter->context,
        (ID3D11Resource *)presenter->worldLinearDepthReadback,
        (ID3D11Resource *)presenter->worldLinearDepthSnapshot);
    result = ID3D11DeviceContext_Map(
        presenter->context,
        (ID3D11Resource *)presenter->worldLinearDepthReadback,
        0, D3D11_MAP_READ, 0, &mapped);
    if (FAILED(result)) {
        presenter->lastError = result;
        return 0;
    }
    for (y = 0; y < presenter->worldHeight; ++y) {
        const float *row = (const float *)(
            (const uint8_t *)mapped.pData +
            (size_t)y * mapped.RowPitch);
        int x;

        for (x = 0; x < presenter->worldWidth; ++x) {
            if (row[x] < FLT_MAX) ++covered;
        }
    }
    ID3D11DeviceContext_Unmap(
        presenter->context,
        (ID3D11Resource *)presenter->worldLinearDepthReadback, 0);
    *covered_pixels = covered;
    presenter->lastError = S_OK;
    return 1;
}

void jpb_PCD3D11PresenterWorldTiming(
    const JPBPCD3D11Presenter *presenter,
    unsigned *frames,
    double *prepare_seconds,
    double *submit_seconds,
    double *readback_seconds)
{
    if (frames != NULL) {
        *frames = presenter != NULL ? presenter->worldTimingFrames : 0;
    }
    if (prepare_seconds != NULL) {
        *prepare_seconds = presenter != NULL
            ? presenter->worldPrepareSeconds : 0.0;
    }
    if (submit_seconds != NULL) {
        *submit_seconds = presenter != NULL
            ? presenter->worldSubmitSeconds : 0.0;
    }
    if (readback_seconds != NULL) {
        *readback_seconds = presenter != NULL
            ? presenter->worldReadbackSeconds : 0.0;
    }
}

void jpb_PCD3D11PresenterModelTiming(
    const JPBPCD3D11Presenter *presenter,
    unsigned *frames,
    double *upload_seconds,
    double *submit_seconds,
    double *color_readback_seconds,
    double *depth_readback_seconds)
{
    if (frames != NULL) {
        *frames = presenter != NULL ? presenter->modelTimingFrames : 0;
    }
    if (upload_seconds != NULL) {
        *upload_seconds = presenter != NULL
            ? presenter->modelUploadSeconds : 0.0;
    }
    if (submit_seconds != NULL) {
        *submit_seconds = presenter != NULL
            ? presenter->modelSubmitSeconds : 0.0;
    }
    if (color_readback_seconds != NULL) {
        *color_readback_seconds = presenter != NULL
            ? presenter->modelColorReadbackSeconds : 0.0;
    }
    if (depth_readback_seconds != NULL) {
        *depth_readback_seconds = presenter != NULL
            ? presenter->modelDepthReadbackSeconds : 0.0;
    }
}

void jpb_PCD3D11PresenterTitleTiming(
    const JPBPCD3D11Presenter *presenter,
    unsigned *frames,
    double *prepare_seconds,
    double *submit_seconds,
    double *readback_seconds)
{
    if (frames != NULL) {
        *frames = presenter != NULL ? presenter->titleTimingFrames : 0;
    }
    if (prepare_seconds != NULL) {
        *prepare_seconds = presenter != NULL
            ? presenter->titlePrepareSeconds : 0.0;
    }
    if (submit_seconds != NULL) {
        *submit_seconds = presenter != NULL
            ? presenter->titleSubmitSeconds : 0.0;
    }
    if (readback_seconds != NULL) {
        *readback_seconds = presenter != NULL
            ? presenter->titleReadbackSeconds : 0.0;
    }
}
