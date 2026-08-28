/*
 * Bounded portable JPX inspection and loading utilities.
 *
 * The reference reads into a fixed 5 MiB arena, resolves material names, and
 * replaces every 16-byte strip header with data from the selected renderer
 * object. Storage and renderer bindings are caller-owned here, keeping D3D,
 * FBX, and desktop container types out of the game-facing interface.
 */

#include "jpb/io.h"
#include "jpb/jpx.h"

#include <string.h>

_Static_assert(sizeof(JPBJpxMaterialDef) == 4, "MATHEAD must be 4 bytes");
_Static_assert(sizeof(JPBJpxBinHeader) == 4102,
               "_BINHEADER must be 4102 bytes");

static uint16_t jpx_read_u16(const uint8_t *bytes)
{
    return (uint16_t)bytes[0] | (uint16_t)((uint16_t)bytes[1] << 8);
}

static int16_t jpx_read_i16(const uint8_t *bytes)
{
    return (int16_t)jpx_read_u16(bytes);
}

static uint32_t jpx_read_u32(const uint8_t *bytes)
{
    return (uint32_t)bytes[0] |
           ((uint32_t)bytes[1] << 8) |
           ((uint32_t)bytes[2] << 16) |
           ((uint32_t)bytes[3] << 24);
}

static float jpx_read_f32(const uint8_t *bytes)
{
    float value;

    memcpy(&value, bytes, sizeof(value));
    return value;
}

static const uint8_t *jpx_material_record(
    const JPBJpxView *view, uint16_t material_index)
{
    return view->data + 6 +
           (size_t)material_index * sizeof(JPBJpxMaterialDef);
}

const char *jpx_GetMaterialName(
    const JPBJpxView *view, uint16_t material_index)
{
    uint16_t name_offset;

    if (view == NULL || view->data == NULL ||
        material_index >= view->numMaterials) {
        return NULL;
    }
    name_offset = jpx_read_u16(jpx_material_record(view, material_index));
    if (name_offset == 0 || name_offset >= view->worldOffset) {
        return NULL;
    }
    return (const char *)view->data + name_offset;
}

int8_t jpx_GetMaterialListType(
    const JPBJpxView *view, uint16_t material_index)
{
    if (view == NULL || view->data == NULL ||
        material_index >= view->numMaterials) {
        return 0;
    }
    return (int8_t)jpx_material_record(view, material_index)[2];
}

int jpx_Inspect(void *data, size_t size, JPBJpxView *view)
{
    JPBJpxView result;
    uint8_t *bytes = (uint8_t *)data;
    size_t material_table_end;
    size_t cursor;
    uint16_t material_index;

    if (view == NULL) {
        return JPB_JPX_INVALID_HEADER;
    }
    memset(&result, 0, sizeof(result));
    *view = result;
    if (bytes == NULL || size < 6) {
        return JPB_JPX_TRUNCATED;
    }

    result.data = bytes;
    result.size = size;
    result.numMaterials = jpx_read_u16(bytes);
    result.worldOffset = jpx_read_u16(bytes + 2);
    result.firstStripOffset = jpx_read_u16(bytes + 4);
    if (result.numMaterials > JPB_JPX_MAX_MATERIALS) {
        return JPB_JPX_INVALID_HEADER;
    }
    material_table_end =
        6 + (size_t)result.numMaterials * sizeof(JPBJpxMaterialDef);
    if (material_table_end > size ||
        result.worldOffset < material_table_end ||
        result.worldOffset > size ||
        result.firstStripOffset > size - result.worldOffset) {
        return JPB_JPX_INVALID_HEADER;
    }

    for (material_index = 0;
         material_index < result.numMaterials;
         ++material_index) {
        uint16_t name_offset =
            jpx_read_u16(
                bytes + 6 +
                (size_t)material_index * sizeof(JPBJpxMaterialDef));

        /*
         * InitJPX treats zero as an unused material slot and skips its texture
         * load. Nonzero names must live in the serialized header/name region.
         */
        if (name_offset != 0 &&
            (name_offset < material_table_end ||
             name_offset >= result.worldOffset ||
             memchr(
                 bytes + name_offset,
                 '\0',
                 (size_t)result.worldOffset - name_offset) == NULL)) {
            return JPB_JPX_INVALID_MATERIAL;
        }
    }

    cursor = (size_t)result.worldOffset + result.firstStripOffset;
    for (;;) {
        uint32_t stride;
        uint32_t strip_material;
        uint16_t vertex_count;
        size_t vertex_bytes;
        size_t following_metadata_size;

        if (cursor > size || size - cursor < sizeof(uint32_t)) {
            return JPB_JPX_TRUNCATED;
        }
        stride = jpx_read_u32(bytes + cursor);
        if (stride == 0) {
            break;
        }
        if (size - cursor < JPB_JPX_DESCRIPTOR_SIZE ||
            stride < JPB_JPX_DESCRIPTOR_SIZE ||
            stride > size - cursor) {
            return JPB_JPX_INVALID_STRIP;
        }
        strip_material = jpx_read_u32(bytes + cursor + sizeof(uint32_t));
        if (strip_material >= result.numMaterials) {
            return JPB_JPX_INVALID_STRIP;
        }
        if (cursor < (size_t)result.worldOffset + sizeof(uint32_t)) {
            return JPB_JPX_INVALID_STRIP;
        }
        vertex_count =
            (uint16_t)(jpx_read_u32(bytes + cursor - 4) >> 16);
        vertex_bytes = (size_t)vertex_count * JPB_JPX_VERTEX_SIZE;
        if (vertex_bytes > stride - JPB_JPX_DESCRIPTOR_SIZE) {
            return JPB_JPX_INVALID_STRIP;
        }
        following_metadata_size =
            stride - JPB_JPX_DESCRIPTOR_SIZE - vertex_bytes;
        if (following_metadata_size != 4 &&
            following_metadata_size != 8 &&
            following_metadata_size != 28) {
            return JPB_JPX_INVALID_STRIP;
        }
        cursor += stride;
        ++result.stripCount;
    }

    *view = result;
    return JPB_JPX_OK;
}

int jpx_ForEachPatchSite(
    const JPBJpxView *view,
    JPBJpxPatchSiteVisitor visitor,
    void *user_data)
{
    static const uint8_t strip_head_marker[8] = {
        'S', 'T', 'R', 'P', 'H', 'E', 'A', 'D'
    };
    size_t cursor;
    size_t site_index;

    if (view == NULL || view->data == NULL || visitor == NULL) {
        return JPB_JPX_INVALID_HEADER;
    }
    cursor = (size_t)view->worldOffset + view->firstStripOffset;
    for (site_index = 0; site_index < view->stripCount; ++site_index) {
        JPBJpxPatchSite site;

        if (cursor > view->size ||
            view->size - cursor < JPB_JPX_DESCRIPTOR_SIZE) {
            return JPB_JPX_TRUNCATED;
        }
        site.data = view->data + cursor;
        site.offset = cursor;
        site.stride = jpx_read_u32(site.data);
        site.materialIndex =
            jpx_read_u32(site.data + sizeof(uint32_t));
        if (cursor < (size_t)view->worldOffset + sizeof(uint32_t)) {
            return JPB_JPX_INVALID_STRIP;
        }
        site.vertexData = site.data + JPB_JPX_DESCRIPTOR_SIZE;
        site.vertexCount =
            (uint16_t)(jpx_read_u32(site.data - 4) >> 16);
        if (site.stride < JPB_JPX_DESCRIPTOR_SIZE ||
            site.stride > view->size - cursor ||
            site.materialIndex >= view->numMaterials) {
            return JPB_JPX_INVALID_STRIP;
        }
        if ((size_t)site.vertexCount * JPB_JPX_VERTEX_SIZE >
            site.stride - JPB_JPX_DESCRIPTOR_SIZE) {
            return JPB_JPX_INVALID_STRIP;
        }
        site.followingMetadataSize = (uint16_t)(
            site.stride -
            JPB_JPX_DESCRIPTOR_SIZE -
            (size_t)site.vertexCount * JPB_JPX_VERTEX_SIZE);
        if (site.followingMetadataSize != 4 &&
            site.followingMetadataSize != 8 &&
            site.followingMetadataSize != 28) {
            return JPB_JPX_INVALID_STRIP;
        }
        site.hasStripHeadMarker =
            memcmp(
                site.data + 2 * sizeof(uint32_t),
                strip_head_marker,
                sizeof(strip_head_marker)) == 0;
        if (visitor(&site, user_data) != 0) {
            return JPB_JPX_VISITOR_FAILED;
        }
        cursor += site.stride;
    }
    return JPB_JPX_OK;
}

int jpx_DecodeVertex(
    const JPBJpxPatchSite *site,
    uint16_t vertex_index,
    JPBJpxVertex *vertex)
{
    const uint8_t *data;

    if (site == NULL || site->vertexData == NULL ||
        vertex == NULL || vertex_index >= site->vertexCount) {
        return JPB_JPX_INVALID_STRIP;
    }
    data =
        site->vertexData + (size_t)vertex_index * JPB_JPX_VERTEX_SIZE;
    vertex->data = data;
    vertex->x = (float)jpx_read_i16(data) / 128.0f;
    vertex->z = (float)jpx_read_i16(data + 2) / 128.0f;
    vertex->y = jpx_read_f32(data + 4);
    vertex->rawU = jpx_read_i16(data + 8);
    vertex->rawV = jpx_read_i16(data + 10);
    vertex->u = (float)vertex->rawU / 4096.0f;
    vertex->v = (float)vertex->rawV / 4096.0f;
    vertex->attributes = jpx_read_u32(data + 12);
    return JPB_JPX_OK;
}

int jpx_ForEachStaticSpatialNode(
    const JPBJpxView *view,
    JPBJpxSpatialNodeVisitor visitor,
    void *user_data)
{
    size_t cursor;
    size_t site_index;

    if (view == NULL || view->data == NULL || visitor == NULL) {
        return JPB_JPX_INVALID_HEADER;
    }
    cursor = (size_t)view->worldOffset + view->firstStripOffset;
    for (site_index = 0; site_index < view->stripCount; ++site_index) {
        uint32_t stride;

        if (cursor > view->size ||
            view->size - cursor < JPB_JPX_DESCRIPTOR_SIZE) {
            return JPB_JPX_TRUNCATED;
        }
        stride = jpx_read_u32(view->data + cursor);
        if (stride < JPB_JPX_DESCRIPTOR_SIZE ||
            stride > view->size - cursor) {
            return JPB_JPX_INVALID_STRIP;
        }
        if (cursor >= (size_t)view->worldOffset + 28) {
            const uint8_t *header = view->data + cursor - 28;
            uint32_t node_flags = jpx_read_u32(header + 20);
            uint32_t vertex_flags = jpx_read_u32(header + 24);

            /*
             * Static JPX spatial headers are the evidence-backed 28-byte
             * form: tag -1, flag word 1, and a zero low flag half. Other
             * shipped forms are dynamic/special records and stay opaque.
             */
            if (jpx_read_u16(header) == UINT16_C(0xFFFF) &&
                node_flags == 1 &&
                (vertex_flags & UINT32_C(0xFFFF)) == 0) {
                JPBJpxSpatialNode node;

                node.data = header;
                node.offset = cursor - 28;
                node.index = jpx_read_u16(header + 2);
                node.x = jpx_read_f32(header + 4);
                node.z = jpx_read_f32(header + 8);
                node.y = jpx_read_f32(header + 12);
                node.radius = jpx_read_f32(header + 16);
                node.vertexCount =
                    (uint16_t)(vertex_flags >> 16);
                if (visitor(&node, user_data) != 0) {
                    return JPB_JPX_VISITOR_FAILED;
                }
            }
        }
        cursor += stride;
    }
    return JPB_JPX_OK;
}

static int jpx_patch_materials_with_progress(
    const JPBJpxView *view,
    const JPBJpxDescriptor *descriptors,
    size_t descriptor_count,
    JPBJpxProgressHook progress,
    void *user_data)
{
    size_t cursor;
    size_t strip_index;

    if (view == NULL || view->data == NULL ||
        descriptors == NULL ||
        descriptor_count < view->numMaterials) {
        return JPB_JPX_INVALID_MATERIAL;
    }
    cursor = (size_t)view->worldOffset + view->firstStripOffset;
    for (strip_index = 0; strip_index < view->stripCount; ++strip_index) {
        uint32_t stride;
        uint32_t material;

        if (cursor > view->size ||
            view->size - cursor < JPB_JPX_DESCRIPTOR_SIZE) {
            return JPB_JPX_TRUNCATED;
        }
        stride = jpx_read_u32(view->data + cursor);
        material =
            jpx_read_u32(view->data + cursor + sizeof(uint32_t));
        if (stride < JPB_JPX_DESCRIPTOR_SIZE ||
            stride > view->size - cursor ||
            material >= descriptor_count) {
            return JPB_JPX_INVALID_STRIP;
        }
        if (progress != NULL) {
            progress(100, user_data);
        }

        /*
         * Save the stride before replacing all four words, matching the
         * recovered InitJPX loop's ordering.
         */
        memcpy(
            view->data + cursor,
            descriptors[material].bytes,
            JPB_JPX_DESCRIPTOR_SIZE);
        cursor += stride;
    }
    return JPB_JPX_OK;
}

int jpx_PatchMaterials(
    const JPBJpxView *view,
    const JPBJpxDescriptor *descriptors,
    size_t descriptor_count)
{
    return jpx_patch_materials_with_progress(
        view, descriptors, descriptor_count, NULL, NULL);
}

int jpx_LoadFile(
    const char *path,
    const JPBJpxLoadConfig *config,
    JPBJpxView *view)
{
    JPBFileHandle file = 0;
    uint64_t file_size;
    uint16_t material_index;
    int result;

    if (view != NULL) {
        memset(view, 0, sizeof(*view));
    }
    if (path == NULL || config == NULL || view == NULL ||
        config->storage == NULL) {
        return JPB_JPX_INVALID_HEADER;
    }
    if (!file_OPEN((char *)path, &file)) {
        return JPB_JPX_IO_ERROR;
    }
    file_size = file_GETSIZE(&file);
    if (file_size > config->storageCapacity ||
        file_size > INT32_MAX) {
        (void)file_CLOSE(&file);
        return JPB_JPX_STORAGE_TOO_SMALL;
    }
    if (file_READ(
            &file,
            (char *)config->storage,
            (int32_t)file_size,
            JPB_FILE_READ_STREAM) != file_size) {
        (void)file_CLOSE(&file);
        return JPB_JPX_IO_ERROR;
    }
    (void)file_CLOSE(&file);

    result = jpx_Inspect(config->storage, (size_t)file_size, view);
    if (result != JPB_JPX_OK) {
        return result;
    }
    if (config->visitSpatialNode != NULL) {
        result = jpx_ForEachStaticSpatialNode(
            view, config->visitSpatialNode, config->userData);
        if (result != JPB_JPX_OK) {
            return result;
        }
    }
    if (config->resolveMaterial == NULL) {
        if (config->visitPatchSite != NULL) {
            return jpx_ForEachPatchSite(
                view, config->visitPatchSite, config->userData);
        }
        return JPB_JPX_OK;
    }
    if (config->descriptors == NULL ||
        config->descriptorCapacity < view->numMaterials) {
        return JPB_JPX_STORAGE_TOO_SMALL;
    }
    memset(
        config->descriptors,
        0,
        (size_t)view->numMaterials * sizeof(*config->descriptors));
    for (material_index = 0;
         material_index < view->numMaterials;
         ++material_index) {
        const char *material_name =
            jpx_GetMaterialName(view, material_index);

        if (config->progress != NULL) {
            config->progress(100, config->userData);
        }
        if (material_name != NULL &&
            config->resolveMaterial(
                config->levelName,
                material_name,
                jpx_GetMaterialListType(view, material_index),
                &config->descriptors[material_index],
                config->userData) != 0) {
            return JPB_JPX_MATERIAL_FAILED;
        }
    }
    if (config->visitPatchSite != NULL) {
        result = jpx_ForEachPatchSite(
            view, config->visitPatchSite, config->userData);
        if (result != JPB_JPX_OK) {
            return result;
        }
    }
    return jpx_patch_materials_with_progress(
        view,
        config->descriptors,
        config->descriptorCapacity,
        config->progress,
        config->userData);
}
