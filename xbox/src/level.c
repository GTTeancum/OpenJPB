#include "gpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <math.h>
#include <xboxkrnl/xboxkrnl.h>

static JPBSoftwareLevelMesh mesh;
static JPBSoftwareLevelBatch *batches;
static char (*names)[384];
/* GPU color input accepts four D3D unsigned bytes. Keep position, UV and
   animated UV offsets as full floats, reducing the resident XLV stride from
   44 to 32 bytes without changing geometry or texture coordinates. */
typedef struct PackedLevelVertex {
    FVECTOR position;
    float u,v;
    uint8_t color[4];
    float uvScrollU,uvScrollV;
} PackedLevelVertex;
static void *vertices;
static int packed_vertices;
volatile unsigned jpb_XboxLevelResidentVertexBytes;
volatile unsigned jpb_XboxLevelLoadStage;
volatile unsigned jpb_XboxLevelLoadBatch;
volatile unsigned jpb_XboxLevelFreePagesBeforeVertices;
volatile unsigned jpb_XboxLevelFreePagesAfterVertices;
static uint16_t *indices;
static unsigned *index_starts;
typedef struct ChunkBounds { float low[3], high[3]; } ChunkBounds;
static ChunkBounds *bounds;
static unsigned *bound_starts;
/* The game renderer is single-threaded. Prepare the six linear view-frustum
 * planes once per level pass, then test each chunk's most-inside AABB corner. */
static float clip_planes[6][4];
static float depth_plane[4];
_Static_assert(sizeof(JPBSoftwareLevelVertex) == 44, "XLV vertex layout");
_Static_assert(sizeof(PackedLevelVertex) == 32, "Xbox level vertex layout");
_Static_assert(offsetof(PackedLevelVertex,color) == 20, "Xbox color offset");

static uint8_t pack_color(float value)
{
    if(value<=0)return 0;
    if(value>=255)return 255;
    return (uint8_t)(value+0.5f);
}

int jpb_XboxLevelUsesPackedVertices(void) { return packed_vertices; }

void jpb_XboxLevelDecodeVertex(const JPBSoftwareLevelBatch *batch,
    unsigned index, JPBSoftwareLevelVertex *out)
{
    if (!packed_vertices) { *out=batch->vertices[index]; return; }
    const PackedLevelVertex *v=((const PackedLevelVertex *)batch->vertices)+index;
    out->position=v->position;
    out->u=v->u; out->v=v->v;
    out->red=v->color[0];out->green=v->color[1];
    out->blue=v->color[2];out->alpha=v->color[3];
    out->uvScrollU=v->uvScrollU;out->uvScrollV=v->uvScrollV;
}

const JPBSoftwareLevelMesh *jpb_XboxLoadLevel(const char *path)
{
    jpb_XboxLevelLoadStage=1;
    uint32_t header[5];
    FILE *file = fopen(path, "rb");
    if (!file) return NULL;
    if (fread(header, sizeof(header), 1, file) != 1 || header[0] != 0x564c584a ||
        (header[1] != 1 && header[1] != 2) || header[2] >= 32 || header[3] > 4096 || header[4] > 1000000)
        goto fail;
    uint32_t unique_total=header[4];
    if(header[1]==2 && (fread(&unique_total,4,1,file)!=1 || unique_total>header[4]))goto fail;
    FILE *gpu_flag=fopen("D:\\xbox-gpu.txt","rb");
    packed_vertices=gpu_flag!=NULL;
    if(gpu_flag)fclose(gpu_flag);
    batches = calloc(header[3], sizeof(*batches));
    names = calloc(header[3], sizeof(*names));
    {
        MM_STATISTICS memory={0};
        memory.Length=sizeof(memory);
        MmQueryStatistics(&memory);
        jpb_XboxLevelFreePagesBeforeVertices=memory.AvailablePages;
    }
    jpb_XboxLevelLoadStage=2;
    vertices = MmAllocateContiguousMemoryEx(
        (size_t)unique_total*(packed_vertices?sizeof(PackedLevelVertex):44),
        0, 0x03ffafff, 0, PAGE_READWRITE | PAGE_WRITECOMBINE);
    jpb_XboxLevelResidentVertexBytes=(unsigned)((size_t)unique_total*
        (packed_vertices?sizeof(PackedLevelVertex):44));
    {
        MM_STATISTICS memory={0};
        memory.Length=sizeof(memory);
        MmQueryStatistics(&memory);
        jpb_XboxLevelFreePagesAfterVertices=memory.AvailablePages;
    }
    if(!vertices)goto fail;
    jpb_XboxLevelLoadStage=3;
    if(header[1]==2) {
        indices=malloc(header[4]*sizeof(*indices));
        index_starts=calloc(header[3],sizeof(*index_starts));
        if(!indices || !index_starts)goto fail;
    }
    bounds=calloc(header[4]/252+header[3],sizeof(*bounds));
    bound_starts=calloc(header[3],sizeof(*bound_starts));
    if (!batches || !names || !vertices || !bounds || !bound_starts) goto fail;
    unsigned offset = 0;
    unsigned index_offset=0;
    unsigned bound_count=0;
    for (unsigned i = 0; i < header[3]; ++i) {
        jpb_XboxLevelLoadStage=4;
        jpb_XboxLevelLoadBatch=i;
        uint32_t data[4];
        if (fread(data, sizeof(data), 1, file) != 1 || data[0] > header[4]-index_offset ||
            data[0]%3 || data[1]>2 || data[2]>=data[3]){
            jpb_XboxLevelLoadStage=41;goto fail;
        }
        uint32_t unique_count=data[0];
        if(header[1]==2 && (fread(&unique_count,4,1,file)!=1 || unique_count>65536))goto fail;
        if(unique_count>unique_total-offset || fread(names[i],384,1,file)!=1 ||
            names[i][255] || names[i][383]) {jpb_XboxLevelLoadStage=42;goto fail;}
        if(packed_vertices) {
            JPBSoftwareLevelVertex source[64];
            PackedLevelVertex *target=((PackedLevelVertex *)vertices)+offset;
            for(unsigned first=0;first<unique_count;first+=64) {
                unsigned count=unique_count-first;
                if(count>64)count=64;
                if(fread(source,44,count,file)!=count) {
                    jpb_XboxLevelLoadStage=44;goto fail;
                }
                for(unsigned j=0;j<count;++j) {
                    const JPBSoftwareLevelVertex *v=&source[j];
                    PackedLevelVertex *out=&target[first+j];
                    out->position=v->position;
                    out->u=v->u;out->v=v->v;
                    out->color[0]=pack_color(v->red);
                    out->color[1]=pack_color(v->green);
                    out->color[2]=pack_color(v->blue);
                    out->color[3]=pack_color(v->alpha);
                    out->uvScrollU=v->uvScrollU;
                    out->uvScrollV=v->uvScrollV;
                }
            }
        } else if(fread(((JPBSoftwareLevelVertex *)vertices)+offset,
            44,unique_count,file)!=unique_count) {jpb_XboxLevelLoadStage=45;goto fail;}
        if(indices) {
            index_starts[i]=index_offset;
            if(fread(indices+index_offset,2,data[0],file)!=data[0]){
                jpb_XboxLevelLoadStage=46;goto fail;
            }
            for(unsigned j=0;j<data[0];++j)if(indices[index_offset+j]>=unique_count){
                jpb_XboxLevelLoadStage=47;goto fail;
            }
        }
        batches[i].vertices = packed_vertices
            ? (const JPBSoftwareLevelVertex *)(((PackedLevelVertex *)vertices)+offset)
            : ((JPBSoftwareLevelVertex *)vertices)+offset;
        batches[i].vertexCount = data[0];
        batches[i].pass = (JPBLevelFbxMeshPass)data[1];
        batches[i].meshIndex = data[2]; batches[i].meshCount = data[3];
        batches[i].textureName = names[i]; batches[i].meshName = names[i]+256;
        bound_starts[i]=bound_count;
        for(unsigned first=0;first<data[0];first+=252) {
            ChunkBounds *box=&bounds[bound_count++];
            unsigned count=data[0]-first; if(count>252)count=252;
            for(unsigned v=0;v<count;++v) {
                unsigned local=indices?indices[index_offset+first+v]:first+v;
                const float *position=packed_vertices
                    ? (const float *)&((PackedLevelVertex *)vertices)[offset+local]
                    : (const float *)&((JPBSoftwareLevelVertex *)vertices)[offset+local];
                for(unsigned axis=0;axis<3;++axis) {
                    float value=position[axis];
                    if(!v || value<box->low[axis])box->low[axis]=value;
                    if(!v || value>box->high[axis])box->high[axis]=value;
                }
            }
        }
        offset += unique_count;
        index_offset += data[0];
    }
    if (offset != unique_total || index_offset!=header[4] || fgetc(file)!=EOF) goto fail;
    fclose(file);
    mesh.batches=batches; mesh.batchCount=header[3]; mesh.levelIndex=header[2];
    mesh.vertices=index_offset; mesh.triangles=index_offset/3;
    jpb_XboxLevelLoadStage=5;
    return &mesh;
fail:
    fclose(file);
    if (vertices) MmFreeContiguousMemory(vertices);
    free(batches); free(names); free(bounds); free(bound_starts);
    free(indices); free(index_starts); indices=NULL; index_starts=NULL;
    bounds=NULL; bound_starts=NULL;
    vertices=NULL; batches=NULL; names=NULL;
    return NULL;
}

const uint16_t *jpb_XboxLevelIndices(unsigned batch)
{
    return indices?indices+index_starts[batch]:NULL;
}

void jpb_XboxLevelSetFrustum(const MATRIX *view, float sx, float sy)
{
    for(unsigned axis=0;axis<3;++axis) {
        float x=view->m[0][axis]*sx;
        float y=view->m[1][axis]*sy;
        float z=view->m[2][axis];
        clip_planes[0][axis]= x+z;
        clip_planes[1][axis]=-x+z;
        clip_planes[2][axis]= y+z;
        clip_planes[3][axis]=-y+z;
        clip_planes[4][axis]= z;
        clip_planes[5][axis]=-z;
        depth_plane[axis]=z;
    }
    float x=(float)view->t[0]*sx;
    float y=(float)view->t[1]*sy;
    float z=(float)view->t[2];
    clip_planes[0][3]= x+z;
    clip_planes[1][3]=-x+z;
    clip_planes[2][3]= y+z;
    clip_planes[3][3]=-y+z;
    clip_planes[4][3]= z-1.0f;
    clip_planes[5][3]=10000.0f-z;
    depth_plane[3]=z;
}

int jpb_XboxLevelChunkVisible(unsigned batch, unsigned first)
{
    const ChunkBounds *box=&bounds[bound_starts[batch]+first/252];
    for(unsigned plane=0;plane<6;++plane) {
        const float *p=clip_planes[plane];
        float best=p[3];
        for(unsigned axis=0;axis<3;++axis)
            best+=p[axis]*(p[axis]>=0.0f?box->high[axis]:box->low[axis]);
        /* A small outward margin only increases work at the boundary; it
         * cannot discard geometry that the original eight-corner test kept. */
        if(best < -0.01f)return 0;
    }
    return 1;
}

int jpb_XboxLevelChunkNeedsDepthClip(unsigned batch, unsigned first)
{
    const ChunkBounds *box=&bounds[bound_starts[batch]+first/252];
    float nearest=depth_plane[3],farthest=depth_plane[3];
    for(unsigned axis=0;axis<3;++axis) {
        float z=depth_plane[axis];
        nearest+=z*(z>=0.0f?box->low[axis]:box->high[axis]);
        farthest+=z*(z>=0.0f?box->high[axis]:box->low[axis]);
    }
    return nearest<1.01f || farthest>9999.99f;
}
