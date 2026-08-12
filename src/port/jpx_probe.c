/*
 * Dependency-light PC integration probe for the preprocessed JPX world mesh.
 */

#include "jpb/jpx.h"
#include "jpb/level_world.h"
#include "jpb/transparent_texture_database.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct ProbeStats {
    size_t sites;
    size_t markedSites;
    size_t staticNodes;
    size_t vertices;
    uint32_t minStride;
    uint32_t maxStride;
    size_t materialSites[JPB_JPX_MAX_MATERIALS];
    size_t materialVertices[JPB_JPX_MAX_MATERIALS];
    size_t materialTriangles[JPB_JPX_MAX_MATERIALS];
    float materialMinX[JPB_JPX_MAX_MATERIALS];
    float materialMinZ[JPB_JPX_MAX_MATERIALS];
    float materialMinY[JPB_JPX_MAX_MATERIALS];
    float materialMaxX[JPB_JPX_MAX_MATERIALS];
    float materialMaxZ[JPB_JPX_MAX_MATERIALS];
    float materialMaxY[JPB_JPX_MAX_MATERIALS];
    int dumpMaterial;
} ProbeStats;

static int resolve_probe_material(
    const char *level_name,
    const char *material_name,
    int8_t list_type,
    JPBJpxDescriptor *descriptor,
    void *user_data)
{
    uint32_t hash = UINT32_C(2166136261);
    size_t index;

    (void)level_name;
    (void)user_data;
    while (*material_name != '\0') {
        hash ^= (uint8_t)*material_name++;
        hash *= UINT32_C(16777619);
    }
    hash ^= (uint8_t)list_type;
    for (index = 0; index < sizeof(descriptor->bytes); ++index) {
        descriptor->bytes[index] =
            (uint8_t)(hash >> ((index & 3) * 8));
    }
    return 0;
}

static int inspect_probe_site(
    const JPBJpxPatchSite *site, void *user_data)
{
    ProbeStats *stats = (ProbeStats *)user_data;
    uint32_t material = site->materialIndex;
    size_t site_index = stats->sites;
    float min_x = 0.0f;
    float min_z = 0.0f;
    float min_y = 0.0f;
    float max_x = 0.0f;
    float max_z = 0.0f;
    float max_y = 0.0f;
    uint16_t vertex_index;

    if (stats->sites == 0 || site->stride < stats->minStride) {
        stats->minStride = site->stride;
    }
    if (site->stride > stats->maxStride) {
        stats->maxStride = site->stride;
    }
    ++stats->sites;
    stats->vertices += site->vertexCount;
    if (site->hasStripHeadMarker) {
        ++stats->markedSites;
    }
    ++stats->materialSites[material];
    stats->materialVertices[material] += site->vertexCount;
    if (site->vertexCount >= 3) {
        stats->materialTriangles[material] += site->vertexCount - 2;
    }
    for (vertex_index = 0;
         vertex_index < site->vertexCount;
         ++vertex_index) {
        JPBJpxVertex vertex;

        if (jpx_DecodeVertex(site, vertex_index, &vertex) != JPB_JPX_OK) {
            return 1;
        }
        if (vertex_index == 0) {
            min_x = max_x = vertex.x;
            min_z = max_z = vertex.z;
            min_y = max_y = vertex.y;
        } else {
            if (vertex.x < min_x) min_x = vertex.x;
            if (vertex.z < min_z) min_z = vertex.z;
            if (vertex.y < min_y) min_y = vertex.y;
            if (vertex.x > max_x) max_x = vertex.x;
            if (vertex.z > max_z) max_z = vertex.z;
            if (vertex.y > max_y) max_y = vertex.y;
        }
        if (stats->materialVertices[material] == site->vertexCount &&
            vertex_index == 0) {
            stats->materialMinX[material] =
                stats->materialMaxX[material] = vertex.x;
            stats->materialMinZ[material] =
                stats->materialMaxZ[material] = vertex.z;
            stats->materialMinY[material] =
                stats->materialMaxY[material] = vertex.y;
        } else {
            if (vertex.x < stats->materialMinX[material])
                stats->materialMinX[material] = vertex.x;
            if (vertex.z < stats->materialMinZ[material])
                stats->materialMinZ[material] = vertex.z;
            if (vertex.y < stats->materialMinY[material])
                stats->materialMinY[material] = vertex.y;
            if (vertex.x > stats->materialMaxX[material])
                stats->materialMaxX[material] = vertex.x;
            if (vertex.z > stats->materialMaxZ[material])
                stats->materialMaxZ[material] = vertex.z;
            if (vertex.y > stats->materialMaxY[material])
                stats->materialMaxY[material] = vertex.y;
        }
    }
    if (stats->dumpMaterial == (int)material) {
        printf(
            "site[%zu] material=%u vertices=%u triangles=%u "
            "bounds=%.6f,%.6f,%.6f..%.6f,%.6f,%.6f\n",
            site_index,
            (unsigned)material,
            (unsigned)site->vertexCount,
            site->vertexCount >= 3
                ? (unsigned)site->vertexCount - 2u : 0u,
            min_x,
            min_z,
            min_y,
            max_x,
            max_z,
            max_y);
    }
    return 0;
}

static int inspect_probe_spatial_node(
    const JPBJpxSpatialNode *node, void *user_data)
{
    ProbeStats *stats = (ProbeStats *)user_data;

    (void)node;
    ++stats->staticNodes;
    return 0;
}

int main(int argc, char **argv)
{
    JPBJpxLoadConfig config = {0};
    JPBJpxDescriptor *descriptors;
    JPBJpxView view;
    uint8_t *buffer;
    ProbeStats stats = {0};
    int result;
    int level_index;
    size_t opaque_materials = 0;
    size_t transparent_materials = 0;
    size_t glass_materials = 0;
    size_t fbx_material_matches = 0;
    uint16_t index;

    stats.dumpMaterial = -1;
    if (argc != 2 && argc != 4) {
        fprintf(
            stderr,
            "usage: %s <world.jpx> [--sites <material-index>]\n",
            argv[0]);
        return 2;
    }
    if (argc == 4) {
        char *end = NULL;
        long value;

        if (strcmp(argv[2], "--sites") != 0) {
            fprintf(stderr, "unknown option: %s\n", argv[2]);
            return 2;
        }
        value = strtol(argv[3], &end, 10);
        if (end == argv[3] || *end != '\0' || value < 0 ||
            value >= JPB_JPX_MAX_MATERIALS) {
            fprintf(stderr, "invalid material index: %s\n", argv[3]);
            return 2;
        }
        stats.dumpMaterial = (int)value;
    }
    buffer = (uint8_t *)malloc(JPB_JPX_REFERENCE_WORLD_CAPACITY);
    descriptors = (JPBJpxDescriptor *)calloc(
        JPB_JPX_MAX_MATERIALS, sizeof(*descriptors));
    if (buffer == NULL || descriptors == NULL) {
        free(descriptors);
        free(buffer);
        fputs("could not allocate JPX probe storage\n", stderr);
        return 3;
    }
    config.storage = buffer;
    config.storageCapacity = JPB_JPX_REFERENCE_WORLD_CAPACITY;
    config.descriptors = descriptors;
    config.descriptorCapacity = JPB_JPX_MAX_MATERIALS;
    level_index = jpb_LevelIndexFromPath(argv[1]);
    config.levelName = level_index != JPB_LEVEL_INDEX_NONE
                           ? sLevelNames[level_index]
                           : "probe";
    config.resolveMaterial = resolve_probe_material;
    config.visitPatchSite = inspect_probe_site;
    config.visitSpatialNode = inspect_probe_spatial_node;
    config.progress = NULL;
    config.userData = &stats;
    result = jpx_LoadFile(argv[1], &config, &view);
    if (result != JPB_JPX_OK) {
        fprintf(stderr, "JPX load failed: status=%d\n", result);
        free(descriptors);
        free(buffer);
        return 5;
    }

    printf(
        "loaded=%zu materials=%u world_offset=%u first_strip=%u "
        "patch_sites=%zu vertices=%zu marked=%zu static_nodes=%zu "
        "stride=%u..%u\n",
        view.size,
        (unsigned)view.numMaterials,
        (unsigned)view.worldOffset,
        (unsigned)view.firstStripOffset,
        view.stripCount,
        stats.vertices,
        stats.markedSites,
        stats.staticNodes,
        (unsigned)stats.minStride,
        (unsigned)stats.maxStride);
    if (level_index != JPB_LEVEL_INDEX_NONE) {
        for (index = 0; index < view.numMaterials; ++index) {
            if (jpb_IsTextureTransparentForJpxMirror(
                    jpx_GetMaterialName(&view, index), level_index)) {
                ++fbx_material_matches;
            }
        }
    }
    for (index = 0; index < view.numMaterials; ++index) {
        const char *name = jpx_GetMaterialName(&view, index);
        int8_t list_type = jpx_GetMaterialListType(&view, index);
        int transparent =
            fbx_material_matches != 0
                ? jpb_IsTextureTransparentForJpxMirror(
                      name, level_index)
                : list_type == 8;
        int glass =
            transparent &&
            (fbx_material_matches != 0
                 ? jpb_IsTextureGlassForJpxMirror(name, level_index)
                 : jpb_IsGlassTextureForJpxMirror(name));

        if (glass) {
            ++glass_materials;
        } else if (transparent) {
            ++transparent_materials;
        } else {
            ++opaque_materials;
        }

        printf(
            "material[%u]=%s list_type=%d pass=%s "
            "sites=%zu vertices=%zu triangles=%zu "
            "bounds=%.3f,%.3f,%.3f..%.3f,%.3f,%.3f\n",
            (unsigned)index,
            name != NULL ? name : "<unused>",
            (int)list_type,
            glass ? "glass" :
            (transparent ? "transparent" : "opaque"),
            stats.materialSites[index],
            stats.materialVertices[index],
            stats.materialTriangles[index],
            stats.materialMinX[index],
            stats.materialMinZ[index],
            stats.materialMinY[index],
            stats.materialMaxX[index],
            stats.materialMaxZ[index],
            stats.materialMaxY[index]);
    }
    printf(
        "passes: opaque=%zu transparent=%zu glass=%zu "
        "fbx_matches=%zu level=%d\n",
        opaque_materials,
        transparent_materials,
        glass_materials,
        fbx_material_matches,
        level_index);
    free(descriptors);
    free(buffer);
    return 0;
}
