#ifndef JPB_D3DTEXTR_H
#define JPB_D3DTEXTR_H

#include "jpb/texture2d.h"

#include <d3d12.h>
#include <dxgi.h>
#include <windows.h>
#include <wrl/client.h>

#include <cstddef>
#include <cstdint>

struct SDL_Texture;
class CD3DFramework12;

class Texture final : public PHL::Texture2D {
public:
    Texture(char *name, DWORD stage, DWORD flags);
    Texture(
        std::uint64_t width,
        std::uint64_t height,
        PHL::TextureFormat format);
    ~Texture() override;

    HRESULT LoadImageData(int flip);
    HRESULT LoadBitmapFile(char *path);
    HRESULT LoadTimFile(char *path);
    HRESULT LoadTargaFile(char *path, int flip);
    HRESULT LoadPNGFile(int flip);
    HRESULT Restore(CD3DFramework12 *framework);
    void Invalidate();
    HRESULT CopyBitmapToSurface(
        ID3D12Device *device,
        ID3D12GraphicsCommandList *command_list);
    HRESULT CopyRGBADataToSurface(
        ID3D12Device *device,
        ID3D12GraphicsCommandList *command_list);
    HRESULT CreateTextureResource();
    HRESULT CreateSRVHeap(CD3DFramework12 *framework);

    void UpdateTexture(void *data, std::uint64_t size) override;
    void *GetNativeResource() override;
    void SetDebugName(const char *name) override;

    static std::uint64_t GetBitsPerPixel(DXGI_FORMAT format);

    Texture *next;
    Texture *prev;
    char m_strName[256];
    DWORD m_dwWidth;
    DWORD m_dwHeight;
    DWORD m_dwStage;
    DWORD m_dwBPP;
    DWORD m_dwFlags;
    BOOL m_bHasAlpha;
    DWORD m_dwPitch;
    int m_nIndex;
    int m_tpfDesired;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_pTextureResource;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_pSRVHeap;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_pUploadBuffer;
    HBITMAP m_hbmBitmap;
    unsigned char *m_pRGBAData;
    SDL_Texture *m_pSDLTexture;
    void *m_pDescriptor;
    bool m_isTransparent;
};

Texture *CreateEmptyTexture(
    char *name,
    DWORD width,
    DWORD height,
    DWORD stage,
    DWORD flags);
Texture *CreateNonTexturedTexture(int type);
Texture *CreateTextureFromFile(
    char *name,
    DWORD stage,
    DWORD flags,
    int flip,
    int desired_format,
    int type,
    CD3DFramework12 *framework);
void DeleteAllTextures();
HRESULT InvalidateAllTextures();
HRESULT RestoreAllTextures(CD3DFramework12 *framework);
void SetTexturePath(char *path);

std::uint64_t GetRequiredIntermediateSize(
    ID3D12Resource *destination_resource,
    UINT first_subresource,
    UINT subresource_count);
std::uint64_t UpdateSubresources(
    ID3D12GraphicsCommandList *command_list,
    ID3D12Resource *destination_resource,
    ID3D12Resource *intermediate_resource,
    std::uint64_t intermediate_offset,
    UINT first_subresource,
    UINT subresource_count,
    D3D12_SUBRESOURCE_DATA *source_data);
std::uint64_t UpdateSubresources(
    ID3D12GraphicsCommandList *command_list,
    ID3D12Resource *destination_resource,
    ID3D12Resource *intermediate_resource,
    UINT first_subresource,
    UINT subresource_count,
    std::uint64_t required_size,
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT *layouts,
    UINT *row_counts,
    std::uint64_t *row_sizes,
    D3D12_SUBRESOURCE_DATA *source_data);
void WaitForGpuTexture(CD3DFramework12 *framework);

const char *jpb_D3DTexturePathForTest();

static_assert(sizeof(Texture) == 416, "Texture PDB size changed");
static_assert(offsetof(Texture, next) == 40, "Texture.next offset changed");
static_assert(offsetof(Texture, m_strName) == 56,
              "Texture name offset changed");
static_assert(offsetof(Texture, m_dwWidth) == 312,
              "Texture width offset changed");
static_assert(offsetof(Texture, m_pTextureResource) == 352,
              "Texture resource offset changed");
static_assert(offsetof(Texture, m_hbmBitmap) == 376,
              "Texture bitmap offset changed");
static_assert(offsetof(Texture, m_pDescriptor) == 400,
              "Texture descriptor offset changed");
static_assert(offsetof(Texture, m_isTransparent) == 408,
              "Texture transparency offset changed");

#endif
