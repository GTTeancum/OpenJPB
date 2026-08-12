/*
 * Bounded portable view of the original BMD geomData archive and the
 * structural portion of model_gInitModelRoot/model_MakeNode.
 *
 * geomData and the model procedure names come directly from the matched PDB.
 * jpb_BmdInspect and jpb_BmdBuildModel are descriptive safety boundaries, not
 * original symbols. Texture/material realization remains outside this seam.
 */

#include "jpb/bmd.h"
#include "jpb/globalarrays.h"
#include "jpb/io.h"

#include <limits.h>
#include <string.h>

typedef struct JPBBmdBuildState {
    const JPBBmdView *view;
    Mnode *nodes;
    size_t node_capacity;
    size_t next_node;
    int collision_player;
    modelObject *model;
} JPBBmdBuildState;

static uint32_t bmd_read_u32(const uint8_t *bytes)
{
    return (uint32_t)bytes[0] |
           ((uint32_t)bytes[1] << 8) |
           ((uint32_t)bytes[2] << 16) |
           ((uint32_t)bytes[3] << 24);
}

static int bmd_record_fits(
    uint32_t record_index, uint32_t payload_size)
{
    size_t offset = (size_t)record_index * sizeof(geomData);

    return offset <= payload_size &&
           sizeof(geomData) <= (size_t)payload_size - offset;
}

static geomData *bmd_record(
    const JPBBmdView *view, uint32_t record_index)
{
    return (geomData *)(
        view->payload +
        (size_t)record_index * sizeof(geomData));
}

static int bmd_string_terminated(const char *text, size_t capacity)
{
    return memchr(text, '\0', capacity) != NULL;
}

static int bmd_offset_valid(int32_t offset, uint32_t payload_size)
{
    return offset == 0 ||
           (offset > 0 &&
            (uint32_t)offset <= payload_size &&
            ((uint32_t)offset & 3u) == 0);
}

static int bmd_range_fits(
    int32_t offset, size_t bytes, uint32_t payload_size)
{
    return offset >= 0 &&
           (uint32_t)offset <= payload_size &&
           bytes <= (size_t)payload_size - (uint32_t)offset;
}

static const uint8_t *bmd_stream_data(
    const JPBBmdView *view,
    int32_t stored_reference,
    int pointer_type,
    size_t bytes)
{
    const uint8_t *data;
    uintptr_t payload_address;
    uintptr_t data_address;
    size_t offset;

    if (view == NULL || view->payload == NULL) {
        return NULL;
    }
    if (view->geometry_streams_relocated) {
        data = (const uint8_t *)getPtr(
            stored_reference, pointer_type);
        if (data == NULL) {
            return NULL;
        }
    } else {
        if (!bmd_range_fits(
                stored_reference, bytes, view->payload_size)) {
            return NULL;
        }
        data = view->payload + (uint32_t)stored_reference;
    }
    payload_address = (uintptr_t)view->payload;
    data_address = (uintptr_t)data;
    if (data_address < payload_address) {
        return NULL;
    }
    offset = (size_t)(data_address - payload_address);
    if (offset > view->payload_size ||
        bytes > (size_t)view->payload_size - offset) {
        return NULL;
    }
    return data;
}

static int bmd_face_vertex_index(
    int16_t stored_index, size_t vertex_count, size_t *index)
{
    int value = stored_index;

    if (value == INT16_MIN) {
        return 0;
    }
    if (value < 0) {
        value = -value;
    }
    if ((size_t)value >= vertex_count) {
        return 0;
    }
    if (index != NULL) {
        *index = (size_t)value;
    }
    return 1;
}

static int32_t bmd_child_index(
    const geomData *node, int child)
{
    if (child < JPB_BMD_STORED_CHILD_CAPACITY) {
        return node->aChildren[child];
    }
    return node->pVertNormals;
}

static int bmd_validate_signed_faces(
    const JPBBmdView *view,
    JPBBmdGeometryView *geometry,
    int32_t offset,
    size_t *corner_count)
{
    size_t bytes =
        geometry->face_count * sizeof(JPBBmdFaceRecord);
    const uint8_t *face_data;
    size_t total_corners = 0;
    size_t face;

    face_data = bmd_stream_data(
        view, offset, JPB_POINTER_ARRAY_INDEX, bytes);
    if (face_data == NULL) {
        return 0;
    }
    geometry->face_encoding = JPB_BMD_FACE_SIGNED_16;
    geometry->faces = (const JPBBmdFaceRecord *)face_data;
    geometry->packed_faces = NULL;
    for (face = 0; face < geometry->face_count; ++face) {
        const JPBBmdFaceRecord *record = &geometry->faces[face];
        size_t corners =
            record->vertex[3] == INT16_MAX ? 3 : 4;
        size_t corner;

        for (corner = 0; corner < corners; ++corner) {
            if (!bmd_face_vertex_index(
                    record->vertex[corner],
                    geometry->total_vertex_count,
                    NULL)) {
                return 0;
            }
        }
        total_corners += corners;
    }
    *corner_count = total_corners;
    return 1;
}

static int bmd_validate_packed_faces(
    const JPBBmdView *view,
    JPBBmdGeometryView *geometry,
    int32_t offset,
    size_t *corner_count)
{
    size_t bytes =
        geometry->face_count * sizeof(JPBBmdPackedFaceRecord);
    const uint8_t *face_data;
    size_t total_corners = 0;
    size_t face;

    face_data = bmd_stream_data(
        view, offset, JPB_POINTER_ARRAY_INDEX, bytes);
    if (face_data == NULL) {
        return 0;
    }
    geometry->face_encoding = JPB_BMD_FACE_PACKED_8;
    geometry->faces = NULL;
    geometry->packed_faces =
        (const JPBBmdPackedFaceRecord *)face_data;
    for (face = 0; face < geometry->face_count; ++face) {
        const JPBBmdPackedFaceRecord *record =
            &geometry->packed_faces[face];
        size_t corners =
            record->vertex[3] == UINT8_MAX ? 3 : 4;
        size_t corner;

        for (corner = 0; corner < corners; ++corner) {
            if ((size_t)record->vertex[corner] >=
                geometry->total_vertex_count) {
                return 0;
            }
        }
        total_corners += corners;
    }
    *corner_count = total_corners;
    return 1;
}

JPBBmdResult jpb_BmdInspect(
    void *buffer, size_t buffer_size, JPBBmdView *view)
{
    uint8_t *file_data = (uint8_t *)buffer;
    uint32_t payload_size;
    uint32_t stack[JPB_BMD_NODE_CAPACITY];
    uint32_t visited[JPB_BMD_NODE_CAPACITY];
    size_t stack_count = 0;
    size_t visited_count = 0;

    if (buffer == NULL || view == NULL) {
        return JPB_BMD_INVALID_ARGUMENT;
    }
    memset(view, 0, sizeof(*view));
    if (buffer_size < 4 + 2 * sizeof(geomData)) {
        return JPB_BMD_TRUNCATED;
    }
    payload_size = bmd_read_u32(file_data);
    if (payload_size != buffer_size - 4) {
        return JPB_BMD_INVALID_SIZE;
    }
    view->file_data = file_data;
    view->file_size = buffer_size;
    view->payload = file_data + 4;
    view->payload_size = payload_size;
    if (((uintptr_t)view->payload & 3u) != 0 ||
        !bmd_record_fits(1, payload_size)) {
        memset(view, 0, sizeof(*view));
        return JPB_BMD_INVALID_LAYOUT;
    }

    stack[stack_count++] = 1;
    while (stack_count != 0) {
        uint32_t record_index = stack[--stack_count];
        geomData *node;
        size_t prior;
        int child;

        if (!bmd_record_fits(record_index, payload_size)) {
            memset(view, 0, sizeof(*view));
            return JPB_BMD_INVALID_LAYOUT;
        }
        for (prior = 0; prior < visited_count; ++prior) {
            if (visited[prior] == record_index) {
                memset(view, 0, sizeof(*view));
                return JPB_BMD_INVALID_LAYOUT;
            }
        }
        if (visited_count >= JPB_BMD_NODE_CAPACITY) {
            memset(view, 0, sizeof(*view));
            return JPB_BMD_INVALID_LAYOUT;
        }
        visited[visited_count++] = record_index;
        node = bmd_record(view, record_index);
        if (!bmd_string_terminated(node->name, sizeof(node->name)) ||
            !bmd_string_terminated(
                node->t.Texture, sizeof(node->t.Texture)) ||
            node->numChildren < 0 ||
            node->numChildren > JPB_BMD_CHILD_CAPACITY ||
            (node->id & 0xffff0000u) != 0 ||
            !bmd_offset_valid(node->pVertex, payload_size) ||
            !bmd_offset_valid(node->pNormal, payload_size) ||
            !bmd_offset_valid(node->pUV, payload_size) ||
            !bmd_offset_valid(node->pColor, payload_size) ||
            !bmd_offset_valid(node->pIndex, payload_size) ||
            (node->numChildren < JPB_BMD_CHILD_CAPACITY &&
             !bmd_offset_valid(
                 node->pVertNormals, payload_size))) {
            memset(view, 0, sizeof(*view));
            return JPB_BMD_INVALID_LAYOUT;
        }
        if (stack_count + (size_t)node->numChildren >
            JPB_BMD_NODE_CAPACITY) {
            memset(view, 0, sizeof(*view));
            return JPB_BMD_INVALID_LAYOUT;
        }
        for (child = node->numChildren - 1; child >= 0; --child) {
            int32_t child_index =
                bmd_child_index(node, child);

            if (child_index < 1) {
                memset(view, 0, sizeof(*view));
                return JPB_BMD_INVALID_LAYOUT;
            }
            stack[stack_count++] = (uint32_t)child_index;
        }
    }

    view->root = bmd_record(view, 1);
    view->node_count = visited_count;
    return JPB_BMD_OK;
}

JPBBmdResult jpb_BmdGetGeometry(
    const JPBBmdView *view,
    const geomData *geometry,
    JPBBmdGeometryView *geometry_view)
{
    const uint8_t *geometry_bytes;
    const uint8_t *vertex_data;
    const uint8_t *uv_data;
    const uint8_t *normal_data;
    const uint8_t *color_data;
    size_t vertex_bytes;
    size_t uv_bytes;
    size_t corner_count = 0;

    if (view == NULL || view->payload == NULL ||
        geometry == NULL || geometry_view == NULL) {
        return JPB_BMD_INVALID_ARGUMENT;
    }
    memset(geometry_view, 0, sizeof(*geometry_view));
    geometry_bytes = (const uint8_t *)geometry;
    if (geometry_bytes < view->payload ||
        (size_t)(geometry_bytes - view->payload) >
            view->payload_size ||
        sizeof(*geometry) >
            (size_t)view->payload_size -
                (size_t)(geometry_bytes - view->payload) ||
        ((uintptr_t)geometry & 3u) != 0 ||
        geometry->numFaces < 0 ||
        geometry->numVerts < 0 ||
        geometry->numShareVerts < 0) {
        return JPB_BMD_INVALID_LAYOUT;
    }
    if ((size_t)geometry->numVerts >
            SIZE_MAX / (3 * sizeof(uint32_t)) ||
        (size_t)geometry->numFaces >
            SIZE_MAX / sizeof(JPBBmdFaceRecord) ||
        (size_t)geometry->numFaces >
            SIZE_MAX / sizeof(faceUV) ||
        (size_t)geometry->numVerts > SIZE_MAX / 3 ||
        (size_t)geometry->numShareVerts >
            SIZE_MAX - (size_t)geometry->numVerts * 3) {
        return JPB_BMD_INVALID_LAYOUT;
    }
    vertex_bytes =
        (size_t)geometry->numVerts *
        3 * sizeof(uint32_t);
    uv_bytes =
        (size_t)geometry->numFaces *
        sizeof(faceUV);
    vertex_data = bmd_stream_data(
        view,
        geometry->pVertex,
        JPB_POINTER_ARRAY_VERTEX,
        vertex_bytes);
    uv_data = bmd_stream_data(
        view,
        geometry->pUV,
        JPB_POINTER_ARRAY_UV,
        uv_bytes);
    if (vertex_data == NULL || uv_data == NULL) {
        return JPB_BMD_INVALID_LAYOUT;
    }

    geometry_view->geometry = geometry;
    geometry_view->packed_vertices =
        (const uint32_t *)vertex_data;
    geometry_view->shared_vertex_count =
        (size_t)geometry->numShareVerts;
    geometry_view->local_vertex_count =
        (size_t)geometry->numVerts * 3;
    geometry_view->total_vertex_count =
        geometry_view->shared_vertex_count +
        geometry_view->local_vertex_count;
    geometry_view->face_count =
        (size_t)geometry->numFaces;
    geometry_view->face_uvs = (const faceUV *)uv_data;

    if (!bmd_validate_signed_faces(
            view,
            geometry_view,
            geometry->pIndex,
            &corner_count) &&
        !bmd_validate_packed_faces(
            view,
            geometry_view,
            geometry->pIndex,
            &corner_count)) {
        return JPB_BMD_INVALID_LAYOUT;
    }
    normal_data = bmd_stream_data(
        view,
        geometry->pNormal,
        JPB_POINTER_ARRAY_NORMAL,
        corner_count * sizeof(uint32_t));
    color_data = bmd_stream_data(
        view,
        geometry->pColor,
        JPB_POINTER_ARRAY_COLOR,
        corner_count * sizeof(CVECTOR));
    if (normal_data == NULL || color_data == NULL) {
        return JPB_BMD_INVALID_LAYOUT;
    }
    geometry_view->packed_normals =
        (const uint32_t *)normal_data;
    geometry_view->colors = (const CVECTOR *)color_data;
    geometry_view->corner_count = corner_count;
    return JPB_BMD_OK;
}

void jpb_BmdDecodePackedVertex(
    uint32_t packed_vertex, FVECTOR *vertex)
{
    if (vertex == NULL) {
        return;
    }
    vertex->vx =
        (float)((int32_t)(packed_vertex << 22) >> 22);
    vertex->vy =
        (float)((int32_t)(packed_vertex << 12) >> 22);
    vertex->vz =
        (float)((int32_t)(packed_vertex << 2) >> 22);
}

size_t jpb_BmdFaceCornerCount(
    const JPBBmdGeometryView *geometry, size_t face)
{
    if (geometry == NULL || face >= geometry->face_count) {
        return 0;
    }
    if (geometry->face_encoding == JPB_BMD_FACE_SIGNED_16 &&
        geometry->faces != NULL) {
        return geometry->faces[face].vertex[3] ==
                   INT16_MAX
                   ? 3
                   : 4;
    }
    if (geometry->face_encoding == JPB_BMD_FACE_PACKED_8 &&
        geometry->packed_faces != NULL) {
        return geometry->packed_faces[face].vertex[3] ==
                   UINT8_MAX
                   ? 3
                   : 4;
    }
    return 0;
}

int jpb_BmdFaceVertexIndex(
    const JPBBmdGeometryView *geometry,
    size_t face,
    size_t corner,
    size_t *vertex_index)
{
    size_t corners = jpb_BmdFaceCornerCount(geometry, face);

    if (vertex_index == NULL || corner >= corners) {
        return 0;
    }
    if (geometry->face_encoding == JPB_BMD_FACE_SIGNED_16) {
        return bmd_face_vertex_index(
            geometry->faces[face].vertex[corner],
            geometry->total_vertex_count,
            vertex_index);
    }
    if (geometry->face_encoding == JPB_BMD_FACE_PACKED_8) {
        size_t index =
            geometry->packed_faces[face].vertex[corner];

        if (index >= geometry->total_vertex_count) {
            return 0;
        }
        *vertex_index = index;
        return 1;
    }
    return 0;
}

const pairUV *jpb_BmdFaceUv(
    const JPBBmdGeometryView *geometry,
    size_t face,
    size_t corner)
{
    size_t corners = jpb_BmdFaceCornerCount(geometry, face);

    if (geometry == NULL || geometry->face_uvs == NULL ||
        corner >= corners) {
        return NULL;
    }
    return &geometry->face_uvs[face].uv[corner];
}

const CVECTOR *jpb_BmdFaceColor(
    const JPBBmdGeometryView *geometry,
    size_t face,
    size_t corner)
{
    size_t color_offset = 0;
    size_t prior_face;
    size_t corners = jpb_BmdFaceCornerCount(geometry, face);

    if (geometry == NULL || geometry->colors == NULL ||
        corner >= corners) {
        return NULL;
    }
    for (prior_face = 0; prior_face < face; ++prior_face) {
        color_offset +=
            jpb_BmdFaceCornerCount(geometry, prior_face);
    }
    if (color_offset + corner >= geometry->corner_count) {
        return NULL;
    }
    return &geometry->colors[color_offset + corner];
}

struct _Material *jpb_BmdGetMaterial(
    const JPBBmdView *view, const geomData *geometry)
{
    if (view == NULL || geometry == NULL ||
        !view->material_handles_relocated) {
        return NULL;
    }
    return (struct _Material *)(uintptr_t)geometry->t.TextureID;
}

JPBBmdResult jpb_BmdLoadFile(
    const char *path,
    void *storage,
    size_t storage_capacity,
    JPBBmdView *view)
{
    JPBFileHandle file = 0;
    uint64_t file_size;

    if (view != NULL) {
        memset(view, 0, sizeof(*view));
    }
    if (path == NULL || storage == NULL || view == NULL) {
        return JPB_BMD_INVALID_ARGUMENT;
    }
    if (!file_OPEN((char *)path, &file)) {
        return JPB_BMD_IO_ERROR;
    }
    file_size = file_GETSIZE(&file);
    if (file_size > storage_capacity ||
        file_size > INT32_MAX) {
        (void)file_CLOSE(&file);
        return JPB_BMD_STORAGE_TOO_SMALL;
    }
    if (file_READ(
            &file,
            (char *)storage,
            (int32_t)file_size,
            JPB_FILE_READ_STREAM) != file_size) {
        (void)file_CLOSE(&file);
        return JPB_BMD_IO_ERROR;
    }
    (void)file_CLOSE(&file);
    return jpb_BmdInspect(storage, (size_t)file_size, view);
}

static JPBBmdResult bmd_build_node(
    JPBBmdBuildState *state,
    Mnode *destination,
    geomData *source,
    Mnode *parent)
{
    size_t child_base;
    int child;

    memset(destination, 0, sizeof(*destination));
    destination->id = (modelNodeId)source->id;
    destination->v3Translation.vx = source->trans.vx;
    destination->v3Translation.vy = source->trans.vy;
    destination->v3Translation.vz = source->trans.vz;
    destination->numChildNodes = (int16_t)source->numChildren;
    destination->pParent = parent;
    destination->pGeomData = source;
    state->model->idMask |=
        1u << ((uint32_t)destination->id & 31u);
    if (state->collision_player >= 0) {
        coll_gRegisterNode(
            state->collision_player, destination);
    }
    if (source->numChildren == 0) {
        return JPB_BMD_OK;
    }
    if (state->next_node + (size_t)source->numChildren >
        state->node_capacity) {
        return JPB_BMD_NODE_STORAGE_TOO_SMALL;
    }
    child_base = state->next_node;
    state->next_node += (size_t)source->numChildren;
    destination->aChildNode = &state->nodes[child_base];
    for (child = 0; child < source->numChildren; ++child) {
        JPBBmdResult result = bmd_build_node(
            state,
            &destination->aChildNode[child],
            bmd_record(
                state->view,
                (uint32_t)bmd_child_index(
                    source, child)),
            destination);

        if (result != JPB_BMD_OK) {
            return result;
        }
    }
    return JPB_BMD_OK;
}

JPBBmdResult jpb_BmdBuildModel(
    const JPBBmdView *view,
    modelObject *model,
    Mnode *nodes,
    size_t node_capacity,
    int collision_player)
{
    JPBBmdBuildState state;
    JPBBmdResult result;

    if (view == NULL || view->root == NULL ||
        model == NULL || nodes == NULL) {
        return JPB_BMD_INVALID_ARGUMENT;
    }
    if (node_capacity < view->node_count) {
        return JPB_BMD_NODE_STORAGE_TOO_SMALL;
    }
    memset(nodes, 0, node_capacity * sizeof(*nodes));
    model->pRootNode = NULL;
    /*
     * Exact model_gInitModelRoot stores at RVA 0xDA179..0xDA18D:
     * all three fixed-point model scale components are initialized to 0x2000.
     */
    model->v3Scale.vx = 2 * JPB_FIXED_ONE;
    model->v3Scale.vy = 2 * JPB_FIXED_ONE;
    model->v3Scale.vz = 2 * JPB_FIXED_ONE;
    model->idMask = 0;
    if (collision_player >= 0) {
        coll_ResetPlayerCollision(collision_player);
    }
    memset(&state, 0, sizeof(state));
    state.view = view;
    state.nodes = nodes;
    state.node_capacity = node_capacity;
    state.next_node = 1;
    state.collision_player = collision_player;
    state.model = model;
    result = bmd_build_node(
        &state, &nodes[0], view->root, NULL);
    if (result != JPB_BMD_OK) {
        if (collision_player >= 0) {
            coll_ResetPlayerCollision(collision_player);
        }
        memset(nodes, 0, node_capacity * sizeof(*nodes));
        return result;
    }
    model->pRootNode = &nodes[0];
    return JPB_BMD_OK;
}
