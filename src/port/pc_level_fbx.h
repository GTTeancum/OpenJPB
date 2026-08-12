#ifndef JPB_PC_LEVEL_FBX_H
#define JPB_PC_LEVEL_FBX_H

#include "jpb/software_renderer.h"

#include <stddef.h>

typedef struct JPBPcFbxLevel {
    JPBSoftwareLevelMesh mesh;
    JPBSoftwareLevelBatch *batches;
    JPBSoftwareLevelVertex *vertices;
    char (*textureNames)[256];
    char (*meshNames)[128];
} JPBPcFbxLevel;

int jpb_PCLoadFbxLevel(
    const char *path,
    int level_index,
    JPBPcFbxLevel *level,
    char *error_text,
    size_t error_text_capacity);
void jpb_PCFreeFbxLevel(JPBPcFbxLevel *level);

#endif
