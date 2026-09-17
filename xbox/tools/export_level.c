/* Offline asset preparation on the host. Game XBE remains nxdk-only.
 * Reuse the exact PC FBX import, then serialize pointer-free little-endian
 * records. No simplification, coordinate guesses, or texture reassignment. */
#include "pc_level_fbx.h"
#include "jpb/level_world.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static int word(FILE *f, uint32_t value) { return fwrite(&value, 4, 1, f) == 1; }
int main(int argc, char **argv)
{
    JPBPcFbxLevel level = {0};
    char error[512];
    if (argc != 4) { fprintf(stderr, "usage: export_level input.fbx level-index output.xlv\n"); return 2; }
    int index = atoi(argv[2]);
    int inferred = jpb_LevelIndexFromPath(argv[1]);
    if (inferred != JPB_LEVEL_INDEX_NONE && inferred != index) {
        fprintf(stderr, "level index mismatch: path resolves to %d, requested %d\n", inferred, index);
        return 2;
    }
    if (!jpb_PCLoadFbxLevel(argv[1], index, &level, error, sizeof(error))) {
        fprintf(stderr, "%s\n", error); return 3;
    }
    FILE *file = fopen(argv[3], "wb");
    if (!file) { jpb_PCFreeFbxLevel(&level); return 4; }
    int ok = word(file, 0x564c584a) && word(file, 1) && word(file, index) &&
        word(file, (uint32_t)level.mesh.batchCount) && word(file, (uint32_t)level.mesh.vertices);
    for (size_t i = 0; ok && i < level.mesh.batchCount; ++i) {
        const JPBSoftwareLevelBatch *b = &level.mesh.batches[i];
        char texture[256] = {0}, name[128] = {0};
        snprintf(texture, sizeof(texture), "%s", b->textureName);
        snprintf(name, sizeof(name), "%s", b->meshName);
        ok = word(file, (uint32_t)b->vertexCount) && word(file, b->pass) &&
            word(file, (uint32_t)b->meshIndex) && word(file, (uint32_t)b->meshCount) &&
            fwrite(texture, sizeof(texture), 1, file) == 1 && fwrite(name, sizeof(name), 1, file) == 1;
        for (size_t j = 0; ok && j < b->vertexCount; ++j) {
            const JPBSoftwareLevelVertex *v = &b->vertices[j];
            float data[11] = {v->position.vx, v->position.vy, v->position.vz,
                v->u, v->v, v->red, v->green, v->blue, v->alpha, v->uvScrollU, v->uvScrollV};
            ok = fwrite(data, sizeof(data), 1, file) == 1;
        }
    }
    if (fclose(file) != 0) ok = 0;
    printf("batches=%zu vertices=%zu triangles=%zu status=%s\n", level.mesh.batchCount,
        level.mesh.vertices, level.mesh.triangles, ok ? "written" : "failed");
    jpb_PCFreeFbxLevel(&level);
    if (!ok) { remove(argv[3]); return 5; }
    return 0;
}
