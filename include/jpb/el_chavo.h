#ifndef JPB_EL_CHAVO_H
#define JPB_EL_CHAVO_H

#include "jpb/d3dapp.h"

#include <cstddef>

struct SDL_Surface;
struct ufbx_scene;
struct SCREENRECT;
struct CVECTOR;

struct _linked_poly {
    D3DTLVERTEX vert[4];
    Texture *texture;
    _linked_poly *next;
    int nvert;
};

class el_chavo : public CD3DApplication {
public:
    el_chavo();
    ~el_chavo();

    HRESULT OneTimeSceneInit() override;
    HRESULT InitDeviceObjects() override;
    HRESULT DeleteDeviceObjects() override;
    HRESULT FinalCleanup() override;
    void OnKeyUp(int key) override;

    void StartPoly(int vertex_count, _Material *material);
    void SetVert(
        int vertex,
        float x,
        float y,
        float z,
        unsigned long argb,
        float tu,
        float tv);
    void EndPoly();
    void NoScaleEndPoly();
    Texture *LoadTexture(
        char *filename, unsigned long option, int tpf, int type);
    void InitTransPolys();
    void PlotTransPolys();
    void ApplyLevelTransformation(
        MATRIX *world_matrix,
        float level_scale_x,
        float level_scale_y,
        float level_scale_z);
    void ApplyProjection(FVECTOR *vertices);
    void InitFBXLevelData(ufbx_scene *scene);
    void InitFBXTextureData(char *filename, int texture_index);
    void CleanupFBXData(std::vector<FBX_MESH *> &meshes);
    void DrawLine2d(
        int x1, int y1, int x2, int y2, unsigned long color);
    void DrawLine(
        int x1,
        int y1,
        int z1,
        int x2,
        int y2,
        int z2,
        unsigned long color);
    void DrawSphere(
        int x,
        int y,
        int z,
        int radius,
        unsigned long color);
    void DrawUITextUTF16(
        unsigned short *utf_text,
        SCREENRECT destination,
        int font_style,
        int size,
        CVECTOR color);
    void DrawUITextUTF16Depth(
        unsigned short *utf_text,
        SCREENRECT destination,
        int font_style,
        int size,
        CVECTOR color,
        float depth);
    void DrawUITextUTF16_3D(
        unsigned short *utf_text,
        float x,
        float y,
        float z,
        int font_style,
        int size,
        unsigned long color);
    void renderLoadProgress(int progress);
    void renderVideoFrame(SDL_Surface *surface);

    D3DTLVERTEX polyarray[4];
    int nv;
    int clip[4];
    Texture *currenttexture;
    Texture *defaulttexture;
    int curTexIndex;
    D3DMATERIAL7 m;
    _linked_poly trans_polygon[4096];
    _linked_poly *otag[1024];
    _linked_poly *currentpoly;
    int currentpolyistrans;
    int numtranspolys;
};

extern el_chavo chavo;

static_assert(sizeof(D3DTLVERTEX) == 32,
              "D3DTLVERTEX PDB size changed");
static_assert(sizeof(_linked_poly) == 152,
              "_linked_poly PDB size changed");
static_assert(offsetof(_linked_poly, texture) == 128,
              "_linked_poly texture offset changed");
static_assert(offsetof(_linked_poly, next) == 136,
              "_linked_poly next offset changed");
static_assert(offsetof(_linked_poly, nvert) == 144,
              "_linked_poly vertex-count offset changed");
static_assert(sizeof(el_chavo) == 0x3AAA80,
              "el_chavo PDB size changed");
static_assert(offsetof(el_chavo, polyarray) == 0x310980,
              "el_chavo polygon-array offset changed");
static_assert(offsetof(el_chavo, nv) == 0x310A00,
              "el_chavo vertex-count offset changed");
static_assert(offsetof(el_chavo, clip) == 0x310A04,
              "el_chavo clip offset changed");
static_assert(offsetof(el_chavo, currenttexture) == 0x310A18,
              "el_chavo current-texture offset changed");
static_assert(offsetof(el_chavo, defaulttexture) == 0x310A20,
              "el_chavo default-texture offset changed");
static_assert(offsetof(el_chavo, curTexIndex) == 0x310A28,
              "el_chavo texture-index offset changed");
static_assert(offsetof(el_chavo, m) == 0x310A2C,
              "el_chavo material offset changed");
static_assert(offsetof(el_chavo, trans_polygon) == 0x310A70,
              "el_chavo transparent-polygon offset changed");
static_assert(offsetof(el_chavo, otag) == 0x3A8A70,
              "el_chavo ordering-table offset changed");
static_assert(offsetof(el_chavo, currentpoly) == 0x3AAA70,
              "el_chavo current-polygon offset changed");
static_assert(offsetof(el_chavo, currentpolyistrans) == 0x3AAA78,
              "el_chavo transparency-state offset changed");
static_assert(offsetof(el_chavo, numtranspolys) == 0x3AAA7C,
              "el_chavo transparent-polygon-count offset changed");

#endif
