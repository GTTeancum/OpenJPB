/*
 * REVIEWED RECONSTRUCTION of the dependency-free model registry and model
 * hierarchy portion of W:\SWJediPowerBattles\Work\model.c.
 *
 * Provenance:
 *   direct   - procedure/global/type names and layouts from game.pdb;
 *   assembly - branches, constants, mutation order, and array extents checked
 *              at exact RVAs 0xD97B0 through 0xDA5AA in matched game.exe;
 *   inferred - jpb_ModelSetGeometryBounds is the bounded portable replacement
 *              for the retail loader's trusted relocated BMD arena.
 *
 * The original model_MakeNode stream relocation and _LoadTexture material
 * replacement are restored through the exact addPtr and _Material owners.
 * Headless inspection keeps texture names only when no platform texture hook
 * is active; the live PC runtime installs that narrow hook before model load.
 *
 * PDB module: 0056
 * Object: W:\SWJediPowerBattles\winver\obj\x64\Steam_Release\model.obj
 * Primary source: W:\SWJediPowerBattles\Work\model.c
 */

#include "jpb/model.h"

#include "jpb/bmd.h"
#include "jpb/globalarrays.h"
#include "jpb/level_world.h"
#include "jpb/loader.h"
#include "jpb/material.h"
#include "jpb/resources.h"
#include "jpb/texture.h"

#include <ctype.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* Exact PDB globals and extents. */
int32_t mModelID = -1;
geomData *mpGeomArray;
uint8_t packFlag = 1;
char maRegisteredModels
    [JPB_MODEL_REGISTRY_CAPACITY]
    [JPB_MODEL_REGISTERED_NAME_CAPACITY];
int32_t mNumRegisteredModels;
int32_t mReuseModel;
uint16_t packTracker[JPB_MODEL_PACK_TRACKER_CAPACITY];
modelSpace maTempModelSpace;
modelSpace xmaModelSpace[JPB_MODEL_REGISTRY_CAPACITY];
modelSpace *maModelSpace = xmaModelSpace;
TextureTracker texTrack[JPB_MODEL_TEXTURE_TRACKER_CAPACITY];
modelObject *mObject;

/* Exact model.c file-local PDB global at matched-PC RVA 0x53A390. */
static int32_t jediloading;

static uint8_t *jpb_model_geometry_base;
static size_t jpb_model_geometry_size;
static int jpb_model_build_failed;

void jpb_ModelSetGeometryBounds(void *base, size_t size)
{
    jpb_model_geometry_base = (uint8_t *)base;
    jpb_model_geometry_size = size;
}

static int model_geometry_record_valid(const geomData *record)
{
    uintptr_t base_address;
    uintptr_t record_address;
    size_t offset;

    if (record == NULL) {
        return 0;
    }
    if (jpb_model_geometry_base == NULL ||
        jpb_model_geometry_size == 0) {
        return 1;
    }
    base_address = (uintptr_t)jpb_model_geometry_base;
    record_address = (uintptr_t)record;
    if (record_address < base_address) {
        return 0;
    }
    offset = (size_t)(record_address - base_address);
    return offset <= jpb_model_geometry_size &&
        sizeof(*record) <= jpb_model_geometry_size - offset;
}

static int32_t model_child_index(
    const geomData *node, int child)
{
    if (child < JPB_BMD_STORED_CHILD_CAPACITY) {
        return node->aChildren[child];
    }
    return node->pVertNormals;
}

static int model_relocate_geometry_stream(
    int32_t *stored_reference, int pointer_type)
{
    uintptr_t base_address;
    uintptr_t stream_address;
    size_t offset;
    int index;

    if (stored_reference == NULL || mpGeomArray == NULL ||
        *stored_reference < 0) {
        return 0;
    }
    offset = (size_t)*stored_reference;
    if (jpb_model_geometry_base != NULL &&
        jpb_model_geometry_size != 0 &&
        offset > jpb_model_geometry_size) {
        return 0;
    }
    base_address = (uintptr_t)mpGeomArray;
    if (offset > UINTPTR_MAX - base_address) {
        return 0;
    }
    stream_address = base_address + offset;
    index = addPtr((void *)stream_address, pointer_type);
    if (index == INT_MAX) {
        return 0;
    }
    *stored_reference = index;
    return 1;
}

static int model_prepare_geometry(geomData *geometry)
{
    char resolved_texture[JPB_RESOURCE_PATH_CAPACITY];
    const char *resource_path;
    const uint8_t *colors;
    _Material *material;
    TT_TEXTYPE texture_type;
    char *extension;
    unsigned option = 0;
    size_t texture_length;

    if (!model_relocate_geometry_stream(
            &geometry->pVertex, JPB_POINTER_ARRAY_VERTEX) ||
        !model_relocate_geometry_stream(
            &geometry->pNormal, JPB_POINTER_ARRAY_NORMAL) ||
        !model_relocate_geometry_stream(
            &geometry->pUV, JPB_POINTER_ARRAY_UV) ||
        !model_relocate_geometry_stream(
            &geometry->pIndex, JPB_POINTER_ARRAY_INDEX) ||
        !model_relocate_geometry_stream(
            &geometry->pColor, JPB_POINTER_ARRAY_COLOR)) {
        return 0;
    }
    if (memchr(
            geometry->t.Texture,
            '\0',
            sizeof(geometry->t.Texture)) == NULL) {
        return 0;
    }
    texture_length = strlen(geometry->t.Texture);
    if (texture_length >= 2 &&
        (EndsWith(geometry->t.Texture, "saber.bmp") ||
         EndsWith(geometry->t.Texture, "sabr.bmp"))) {
        memcpy(
            geometry->t.Texture,
            "transabr.bmp",
            sizeof("transabr.bmp"));
    }
    if (texture_length < 2 || !jpb_TextureHasLoadHook()) {
        return 1;
    }

    resource_path = resource_getPath(
        geometry->t.Texture, JPB_RESOURCE_MODEL_TEXTURE);
    if (resource_path == NULL ||
        strlen(resource_path) >= sizeof(resolved_texture)) {
        return 0;
    }
    memcpy(
        resolved_texture,
        resource_path,
        strlen(resource_path) + 1);
    extension = strstr(resolved_texture, "bmp");
    if (extension != NULL) {
        memcpy(extension, "tga", sizeof("tga") - 1);
    }
    extension = strstr(resolved_texture, ".tga");
    if (extension != NULL) {
        memcpy(extension, ".tga", sizeof(".tga"));
    }
    colors = (const uint8_t *)getPtr(
        geometry->pColor, JPB_POINTER_ARRAY_COLOR);
    if (colors != NULL) {
        option = colors[3] & 2u;
    }
    texture_type =
        jediloading ? TT_PLAYERCHAR : TT_NONPLAYERCHAR;
    material = _LoadTexture(
        resolved_texture, texture_type, option);
    geometry->t.TextureID = (uint64_t)(uintptr_t)material;
    return 1;
}

static geomData *model_geometry_record(int32_t index)
{
    size_t offset;

    if (mpGeomArray == NULL || index < 0 ||
        (size_t)index > SIZE_MAX / sizeof(geomData)) {
        return NULL;
    }
    offset = (size_t)index * sizeof(geomData);
    if (jpb_model_geometry_base != NULL &&
        jpb_model_geometry_size != 0 &&
        (uint8_t *)mpGeomArray == jpb_model_geometry_base &&
        (offset > jpb_model_geometry_size ||
         sizeof(geomData) > jpb_model_geometry_size - offset)) {
        return NULL;
    }
    return (geomData *)((uint8_t *)mpGeomArray + offset);
}

/* Reference RVA 0xD97B0, 95 bytes. */
int EndsWith(const char *str, const char *suffix)
{
    size_t string_length;
    size_t suffix_length;

    if (str == NULL || suffix == NULL) {
        return 0;
    }
    string_length = strlen(str);
    suffix_length = strlen(suffix);
    return suffix_length <= string_length &&
        strncmp(
            str + string_length - suffix_length,
            suffix,
            suffix_length) == 0;
}

/* Reference RVA 0xD9810, 23 bytes. */
void addTexTrack(
    TextureTracker *tt,
    uint32_t *name,
    _Material *pTextureHandle)
{
    if (tt == NULL || name == NULL) {
        return;
    }
    tt->name[0] = name[0];
    tt->name[1] = name[1];
    tt->th = pTextureHandle;
    tt[1].th = NULL;
}

/* RVA 0xD9830 remains a debug-console-only gap: console_NodeCommand. */

/* Reference RVA 0xD9A80, 404 bytes. */
unsigned levTexParseFarce(unsigned char *name)
{
    unsigned char tn[32];
    size_t name_length;
    unsigned loop1;
    unsigned loop2 = 0;
    unsigned flag = 0;
    unsigned num;

    if (name == NULL) {
        return 0;
    }
    name_length = strlen((const char *)name);
    if (name_length >= sizeof(tn)) {
        return 0;
    }
    memcpy(tn, name, name_length + 1);
    for (loop1 = 0; tn[loop1] != '\0'; ++loop1) {
        tn[loop1] = (unsigned char)tolower(tn[loop1]);
        if (tn[loop1] >= '0' && tn[loop1] <= '9') {
            flag = 1;
            loop2 = loop1;
            break;
        }
    }
    if (!flag) {
        return 0;
    }
    num = (unsigned)strtoul(
        (const char *)&tn[loop2], NULL, 10);
    tn[loop2] = '\0';
    for (loop1 = 0; loop1 < 16; ++loop1) {
        if (strcmp(
                (const char *)tn,
                sLevelNames[loop1]) == 0) {
            return num | UINT32_C(0x80000000);
        }
    }
    if (strncmp((const char *)tn, "level", 5) == 0) {
        return num | UINT32_C(0x80000000);
    }
    return 0;
}

/* Reference RVA 0xD9C20, 88 bytes. */
modelObject *model_GetModel(int id)
{
    if (id < 0) {
        maTempModelSpace.curNode = 0;
        return &maTempModelSpace.Model;
    }
    if ((unsigned)id >= JPB_MODEL_REGISTRY_CAPACITY) {
        return NULL;
    }
    memset(&maModelSpace[id], 0, sizeof(maModelSpace[id]));
    return &maModelSpace[id].Model;
}

/* Reference RVA 0xD9C80, 107 bytes. */
Mnode *model_GetNodes(int id, int num)
{
    modelSpace *space;
    Mnode *nodes;

    if (num < 0) {
        return NULL;
    }
    if (id < 0) {
        space = &maTempModelSpace;
    } else {
        if ((unsigned)id >= JPB_MODEL_REGISTRY_CAPACITY) {
            return NULL;
        }
        space = &maModelSpace[id];
    }
    if ((unsigned)num > JPB_MODEL_NODE_CAPACITY ||
        space->curNode < 0 ||
        space->curNode > JPB_MODEL_NODE_CAPACITY - num) {
        return NULL;
    }
    nodes = &space->aNodes[space->curNode];
    space->curNode += num;
    if (id >= 0 && num != 0) {
        nodes->flags = 0;
        nodes->time = 0;
    }
    return nodes;
}

/* Reference RVA 0xD9CF0, 158 bytes. */
void model_InitModels(void)
{
    memset(maRegisteredModels, 0, sizeof(maRegisteredModels));
    maModelSpace = xmaModelSpace;
    memset(xmaModelSpace, 0, sizeof(xmaModelSpace));
    mModelID = -1;
    mNumRegisteredModels = 0;
    mReuseModel = 0;
    mpGeomArray = NULL;
    mObject = NULL;
    packFlag = 1;
    resetTexturePacker();
}

/* Reference RVA 0xD9D90, 705 bytes; hierarchy/registry portion. */
void model_MakeNode(
    Mnode *pNew, geomData *pNode, char *modelName)
{
    int child;

    (void)modelName;
    if (pNew == NULL || !model_geometry_record_valid(pNode) ||
        mObject == NULL || pNode->numChildren < 0 ||
        pNode->numChildren > JPB_BMD_CHILD_CAPACITY) {
        jpb_model_build_failed = 1;
        return;
    }

    pNew->pGeomData = pNode;
    pNew->id = (modelNodeId)pNode->id;

    if (!mReuseModel && !model_prepare_geometry(pNode)) {
        jpb_model_build_failed = 1;
        return;
    }
    mObject->idMask |=
        UINT32_C(1) << ((uint32_t)pNew->id & 31u);
    if (mModelID >= 0) {
        coll_gRegisterNode(mModelID, pNew);
    }
    pNew->v3Translation.vx = pNode->trans.vx;
    pNew->v3Translation.vy = pNode->trans.vy;
    pNew->v3Translation.vz = pNode->trans.vz;
    pNew->numChildNodes = (int16_t)pNode->numChildren;
    if (pNode->numChildren == 0) {
        pNew->aChildNode = NULL;
        return;
    }
    pNew->aChildNode = model_GetNodes(
        mModelID, pNode->numChildren);
    if (pNew->aChildNode == NULL) {
        jpb_model_build_failed = 1;
        return;
    }
    for (child = 0; child < pNode->numChildren; ++child) {
        geomData *child_node = model_geometry_record(
            model_child_index(pNode, child));

        if (child_node == NULL) {
            jpb_model_build_failed = 1;
            return;
        }
        model_MakeNode(
            &pNew->aChildNode[child],
            child_node,
            modelName);
        if (jpb_model_build_failed) {
            return;
        }
    }
}

/* Reference RVA 0xDA060, 158 bytes. */
int model_RegisterModel(char *name)
{
    int index;

    if (name == NULL) {
        return -1;
    }
    for (index = 0; index < mNumRegisteredModels; ++index) {
        if (strcmp(name, maRegisteredModels[index]) == 0) {
            return 1;
        }
    }
    if (mNumRegisteredModels >= JPB_MODEL_REGISTRY_CAPACITY) {
        return -1;
    }
    strncpy(
        maRegisteredModels[mNumRegisteredModels],
        name,
        JPB_MODEL_REGISTERED_NAME_CAPACITY - 1);
    maRegisteredModels[mNumRegisteredModels]
        [JPB_MODEL_REGISTERED_NAME_CAPACITY - 1] = '\0';
    ++mNumRegisteredModels;
    return 0;
}

/* Reference RVA 0xDA100, 450 bytes. */
modelObject *model_gInitModelRoot(
    geomData *pRoot, char *name, int id)
{
    Mnode *root_node;
    int registration;

    if (pRoot == NULL || name == NULL ||
        (id >= 0 &&
         (unsigned)id >= JPB_MODEL_REGISTRY_CAPACITY)) {
        return NULL;
    }
    mModelID = id;
    mpGeomArray = pRoot;
    if (jpb_model_geometry_base != NULL &&
        (uint8_t *)pRoot != jpb_model_geometry_base) {
        return NULL;
    }
    mObject = model_GetModel(id);
    if (mObject == NULL) {
        return NULL;
    }
    mObject->v3Scale.vx = 0x2000;
    mObject->v3Scale.vy = 0x2000;
    mObject->v3Scale.vz = 0x2000;
    strncpy(mObject->sModelName, name, sizeof(mObject->sModelName) - 1);
    mObject->sModelName[sizeof(mObject->sModelName) - 1] = '\0';

    registration = model_RegisterModel(mObject->sModelName);
    if (registration < 0) {
        return NULL;
    }
    mReuseModel = registration;
    root_node = model_GetNodes(id, 1);
    if (root_node == NULL) {
        return NULL;
    }
    mObject->pRootNode = root_node;
    if (id >= 0) {
        coll_ResetPlayerCollision(id);
    }
    jediloading = id < 2;
    jpb_model_build_failed = 0;
    model_MakeNode(
        root_node,
        model_geometry_record(1),
        mObject->sModelName);
    if (jpb_model_build_failed) {
        mObject->pRootNode = NULL;
        return NULL;
    }
    return mObject;
}

/* Reference RVA 0xDA2D0, 83 bytes. */
int pack_checkPage(
    uint16_t *page, unsigned width, unsigned height)
{
    unsigned aligned;
    unsigned row;

    if (page == NULL || width == 0 || height == 0) {
        return 0;
    }
    aligned = page[0];
    while ((aligned & (width - 1u)) != 0) {
        if (aligned < 8) {
            return 0;
        }
        aligned -= 8;
    }
    for (row = 0; row < height; ++row) {
        if (page[row] < width || page[row] < aligned) {
            return 0;
        }
    }
    return 1;
}

/* Reference RVA 0xDA330, 496 bytes. */
int pack_getVRAM(
    unsigned width, unsigned height, unsigned *x, unsigned *y)
{
    unsigned packed_width;
    unsigned packed_height;
    unsigned best_row = 0;
    unsigned best_edge = 0;
    unsigned row;
    int found = 0;

    if (x == NULL || y == NULL) {
        return 0;
    }
    *x = 0;
    *y = 0;
    if (packFlag == 0) {
        return 0;
    }
    packed_width = width < 8 ? 8 : width;
    if ((packed_width & 1u) != 0) {
        ++packed_width;
    }
    if ((height & 7u) != 0) {
        height += 8u - (height & 7u);
    }
    packed_height = height;
    if ((packed_height & 1u) != 0) {
        ++packed_height;
    }
    if (packed_height == 0 ||
        packed_height >= JPB_MODEL_PACK_TRACKER_CAPACITY) {
        return 0;
    }
    for (row = 0;
         row + packed_height <= JPB_MODEL_PACK_TRACKER_CAPACITY;
         ++row) {
        if (pack_checkPage(
                &packTracker[row],
                packed_width,
                packed_height)) {
            if (!found || packTracker[row] > best_edge) {
                found = 1;
                best_row = row;
                best_edge = packTracker[row];
            }
        }
    }
    if (!found) {
        return 0;
    }
    while ((best_edge & (packed_width - 1u)) != 0) {
        best_edge -= 8;
    }
    packTracker[best_row] = (uint16_t)best_edge;
    *x = best_edge - packed_width;
    *y = best_row;
    pack_page(
        &packTracker[best_row],
        packed_width,
        packed_height);
    return 1;
}

/* Reference RVA 0xDA520, 34 bytes. */
void pack_page(
    uint16_t *page, unsigned width, unsigned height)
{
    uint16_t edge;
    unsigned row;

    if (page == NULL || height == 0) {
        return;
    }
    edge = page[0];
    for (row = 0; row < height; ++row) {
        page[row] = (uint16_t)(edge - width);
    }
}

/* Reference RVA 0xDA550, 12 bytes. */
void resetTexTrack(void)
{
    texTrack[0].th = NULL;
}

/* Reference RVA 0xDA560, 74 bytes. */
void resetTexturePacker(void)
{
    unsigned row;

    packFlag = 1;
    for (row = 0; row < JPB_MODEL_PACK_TRACKER_CAPACITY; ++row) {
        if (LevelSelect == 0) {
            packTracker[row] = 0xc0;
        } else if (row < 0x80) {
            packTracker[row] = 0xe0;
        } else {
            packTracker[row] = 0x100;
        }
    }
}
