#include "jpb/bmd.h"
#include "jpb/globalarrays.h"
#include "jpb/loader.h"
#include "jpb/material.h"
#include "jpb/model.h"
#include "jpb/resources.h"
#include "jpb/texture.h"

#include <stdio.h>
#include <string.h>

#define CHECK(condition)                                                     \
    do {                                                                     \
        if (!(condition)) {                                                  \
            fprintf(                                                         \
                stderr,                                                      \
                "CHECK failed at %s:%d: %s\n",                               \
                __FILE__,                                                    \
                __LINE__,                                                    \
                #condition);                                                 \
            return 1;                                                        \
        }                                                                    \
    } while (0)

static void write_u32(uint8_t *bytes, uint32_t value)
{
    bytes[0] = (uint8_t)value;
    bytes[1] = (uint8_t)(value >> 8);
    bytes[2] = (uint8_t)(value >> 16);
    bytes[3] = (uint8_t)(value >> 24);
}

static int model_texture_token;
static char model_texture_path[JPB_RESOURCE_PATH_CAPACITY];

static void *load_model_texture(
    void *user_data,
    const char *filename,
    unsigned option,
    int material_type,
    int16_t *width,
    int16_t *height)
{
    (void)user_data;
    (void)option;
    (void)material_type;
    strncpy(
        model_texture_path,
        filename,
        sizeof(model_texture_path) - 1);
    model_texture_path[sizeof(model_texture_path) - 1] = '\0';
    *width = 128;
    *height = 64;
    return &model_texture_token;
}

static void make_model_archive(
    uint8_t *archive, size_t archive_size)
{
    geomData *records;

    memset(archive, 0, archive_size);
    write_u32(archive, (uint32_t)(archive_size - 4));
    records = (geomData *)(void *)(archive + 4);
    memcpy(records[1].name, "root", sizeof("root"));
    records[1].id = NODE_DYNAMIC | 1;
    records[1].trans.vx = 10;
    records[1].trans.vy = 20;
    records[1].trans.vz = 30;
    records[1].numChildren = 2;
    records[1].pVertex = 4;
    records[1].pNormal = 8;
    records[1].pUV = 12;
    records[1].pColor = 16;
    records[1].pIndex = 20;
    memcpy(
        records[1].t.Texture,
        "hero_saber.bmp",
        sizeof("hero_saber.bmp"));
    records[1].aChildren[0] = 2;
    records[1].aChildren[1] = 4;
    memcpy(records[2].name, "torso", sizeof("torso"));
    records[2].id = NODE_DYNAMIC | 2;
    records[2].numChildren = 1;
    records[2].aChildren[0] = 3;
    memcpy(records[3].name, "head", sizeof("head"));
    records[3].id = NODE_DYNAMIC | 3;
    memcpy(records[4].name, "leg", sizeof("leg"));
    records[4].id = NODE_DYNAMIC | 4;
}

static int test_registry_and_hierarchy(void)
{
    uint8_t archive[4 + 5 * sizeof(geomData)];
    JPBBmdView view;
    modelObject *model;
    modelObject *reused;
    Mnode *root;
    geomData *records;
    JPBBmdGeometryView geometry;

    make_model_archive(archive, sizeof(archive));
    CHECK(jpb_BmdInspect(
              archive, sizeof(archive), &view) == JPB_BMD_OK);
    records = (geomData *)(void *)view.payload;
    LevelSelect = 0;
    pointerRegistry_Reset();
    model_InitModels();
    jpb_ModelSetGeometryBounds(
        view.payload, view.payload_size);
    model = model_gInitModelRoot(
        (geomData *)(void *)view.payload,
        "hero",
        3);
    CHECK(model == &maModelSpace[3].Model);
    CHECK(model->pRootNode == &maModelSpace[3].aNodes[0]);
    CHECK(strcmp(model->sModelName, "hero") == 0);
    CHECK(model->v3Scale.vx == 0x2000);
    CHECK(model->v3Scale.vy == 0x2000);
    CHECK(model->v3Scale.vz == 0x2000);
    CHECK(mNumRegisteredModels == 1);
    CHECK(mReuseModel == 0);

    root = model->pRootNode;
    CHECK(root->id == (modelNodeId)(NODE_DYNAMIC | 1));
    CHECK(root->v3Translation.vx == 10);
    CHECK(root->v3Translation.vy == 20);
    CHECK(root->v3Translation.vz == 30);
    CHECK(root->numChildNodes == 2);
    CHECK(root->aChildNode == &maModelSpace[3].aNodes[1]);
    CHECK(root->aChildNode[0].id ==
          (modelNodeId)(NODE_DYNAMIC | 2));
    CHECK(root->aChildNode[0].aChildNode ==
          &maModelSpace[3].aNodes[3]);
    CHECK(root->aChildNode[0].aChildNode[0].id ==
          (modelNodeId)(NODE_DYNAMIC | 3));
    CHECK(root->aChildNode[1].id ==
          (modelNodeId)(NODE_DYNAMIC | 4));
    CHECK(root->pGeomData == view.root);
    CHECK((model->idMask & (1u << 1)) != 0);
    CHECK((model->idMask & (1u << 4)) != 0);
    CHECK(coll_GetNode(3, 1) == root);
    CHECK(coll_GetNode(3, 3) ==
          &root->aChildNode[0].aChildNode[0]);
    CHECK(records[1].pVertex == 0);
    CHECK(records[1].pNormal == 0);
    CHECK(records[1].pUV == 0);
    CHECK(records[1].pColor == 0);
    CHECK(records[1].pIndex == 0);
    CHECK(getPtr(0, JPB_POINTER_ARRAY_VERTEX) ==
          view.payload + 4);
    CHECK(getPtr(0, JPB_POINTER_ARRAY_NORMAL) ==
          view.payload + 8);
    CHECK(getPtr(0, JPB_POINTER_ARRAY_UV) ==
          view.payload + 12);
    CHECK(getPtr(0, JPB_POINTER_ARRAY_COLOR) ==
          view.payload + 16);
    CHECK(getPtr(0, JPB_POINTER_ARRAY_INDEX) ==
          view.payload + 20);
    CHECK(strcmp(records[1].t.Texture, "transabr.bmp") == 0);
    view.geometry_streams_relocated = 1;
    CHECK(jpb_BmdGetGeometry(
              &view, &records[1], &geometry) == JPB_BMD_OK);
    CHECK((const void *)geometry.packed_vertices ==
          getPtr(0, JPB_POINTER_ARRAY_VERTEX));

    reused = model_gInitModelRoot(
        (geomData *)(void *)view.payload,
        "hero",
        4);
    CHECK(reused == &maModelSpace[4].Model);
    CHECK(reused->pRootNode != NULL);
    CHECK(mNumRegisteredModels == 1);
    CHECK(mReuseModel == 1);
    CHECK(model_GetModel(JPB_MODEL_REGISTRY_CAPACITY) == NULL);
    CHECK(model_GetNodes(4, JPB_MODEL_NODE_CAPACITY + 1) == NULL);
    return 0;
}

static int test_bounded_bad_child(void)
{
    uint8_t archive[4 + 5 * sizeof(geomData)];
    JPBBmdView view;
    geomData *records;

    make_model_archive(archive, sizeof(archive));
    CHECK(jpb_BmdInspect(
              archive, sizeof(archive), &view) == JPB_BMD_OK);
    records = (geomData *)(void *)view.payload;
    records[2].aChildren[0] = 99;
    model_InitModels();
    jpb_ModelSetGeometryBounds(
        view.payload, view.payload_size);
    CHECK(model_gInitModelRoot(
              (geomData *)(void *)view.payload,
              "bad",
              -1) == NULL);
    CHECK(maTempModelSpace.Model.pRootNode == NULL);
    return 0;
}

static int test_live_material_relocation(void)
{
    uint8_t archive[4 + 5 * sizeof(geomData)];
    JPBBmdView view;
    geomData *records;
    modelObject *model;
    _Material *material;

    make_model_archive(archive, sizeof(archive));
    CHECK(jpb_BmdInspect(
              archive, sizeof(archive), &view) == JPB_BMD_OK);
    records = (geomData *)(void *)view.payload;
    memset(g_material, 0, sizeof(g_material));
    model_texture_path[0] = '\0';
    CHECK(jpb_ResourceSetBasePath("C:/game") == 1);
    jpb_TextureSetPlatformHooks(
        load_model_texture, NULL, NULL);
    LevelSelect = 0;
    pointerRegistry_Reset();
    model_InitModels();
    jpb_ModelSetGeometryBounds(
        view.payload, view.payload_size);
    model = model_gInitModelRoot(
        records, "material_hero", 0);
    CHECK(model != NULL);
    material = (_Material *)(uintptr_t)records[1].t.TextureID;
    CHECK(material == &g_material[0]);
    CHECK(material->type == TT_PLAYERCHAR);
    CHECK(material->texture == &model_texture_token);
    CHECK(material->iw == 128);
    CHECK(material->ih == 64);
    CHECK(strcmp(
              model_texture_path,
              "C:/game/res/model/tga/transabr.tga") == 0);
    view.geometry_streams_relocated = 1;
    view.material_handles_relocated = 1;
    CHECK(jpb_BmdGetMaterial(
              &view, &records[1]) == material);
    texture_Flush((unsigned)TT_ANY);
    jpb_TextureSetPlatformHooks(NULL, NULL, NULL);
    return 0;
}

static int test_texture_tracker_and_suffix(void)
{
    TextureTracker tracker[2];
    uint32_t name[2] = {
        UINT32_C(0x11223344), UINT32_C(0x55667788)
    };
    _Material material;
    unsigned char fed_name[] = "FED12";
    unsigned char level_name[] = "level7";
    unsigned char unknown_name[] = "unknown2";
    unsigned char no_number[] = "fed";

    memset(tracker, 0xff, sizeof(tracker));
    memset(&material, 0, sizeof(material));
    CHECK(EndsWith("transabr.bmp", "sabr.bmp"));
    CHECK(!EndsWith("saber", "saber.bmp"));
    CHECK(!EndsWith(NULL, "x"));
    CHECK(levTexParseFarce(fed_name) == UINT32_C(0x8000000c));
    CHECK(levTexParseFarce(level_name) == UINT32_C(0x80000007));
    CHECK(levTexParseFarce(unknown_name) == 0);
    CHECK(levTexParseFarce(no_number) == 0);
    CHECK(levTexParseFarce(NULL) == 0);
    addTexTrack(&tracker[0], name, &material);
    CHECK(tracker[0].name[0] == name[0]);
    CHECK(tracker[0].name[1] == name[1]);
    CHECK(tracker[0].th == &material);
    CHECK(tracker[1].th == NULL);
    texTrack[0].th = &material;
    resetTexTrack();
    CHECK(texTrack[0].th == NULL);
    return 0;
}

static int test_texture_packer(void)
{
    unsigned x;
    unsigned y;
    unsigned row;

    LevelSelect = 0;
    resetTexturePacker();
    for (row = 0; row < JPB_MODEL_PACK_TRACKER_CAPACITY; ++row) {
        CHECK(packTracker[row] == 0xc0);
    }
    CHECK(pack_getVRAM(8, 8, &x, &y));
    CHECK(x == 0xb8);
    CHECK(y == 0);
    for (row = 0; row < 8; ++row) {
        CHECK(packTracker[row] == 0xb8);
    }
    CHECK(pack_getVRAM(8, 8, &x, &y));
    CHECK(x == 0xb8);
    CHECK(y == 8);

    LevelSelect = 1;
    resetTexturePacker();
    CHECK(packTracker[0] == 0xe0);
    CHECK(packTracker[0x7f] == 0xe0);
    CHECK(packTracker[0x80] == 0x100);
    CHECK(pack_getVRAM(8, 8, &x, &y));
    CHECK(x == 0xf8);
    CHECK(y == 0x80);
    packFlag = 0;
    x = 9;
    y = 9;
    CHECK(!pack_getVRAM(8, 8, &x, &y));
    CHECK(x == 0);
    CHECK(y == 0);
    return 0;
}

int main(void)
{
    int result = 0;

    result |= test_registry_and_hierarchy();
    result |= test_bounded_bad_child();
    result |= test_live_material_relocation();
    result |= test_texture_tracker_and_suffix();
    result |= test_texture_packer();
    if (result == 0) {
        puts("model tests passed");
    }
    return result;
}
