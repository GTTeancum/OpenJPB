/* nxdk/pbkit backend. Shared code supplies clipped/projected triangles.
   This backend is under construction; material pass parity and HUD composition
   must be verified before it replaces the software diagnostic renderer. */
#include "gpu.h"
#include "jpb/level_world.h"
#include "jpb/level.h"
#include "jpb/material.h"
#include "jpb/texture.h"
#include "jpb/portable_text.h"
#include <pbkit/pbkit.h>
#include <hal/video.h>
#include <xboxkrnl/xboxkrnl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <hal/debug.h>

#define JPB_XBOX_PUSHBUFFER_SIZE (512u * 1024u)
#define JPB_XBOX_MODEL_BC1 1
#define JPB_XBOX_TRIANGLE_PROFILE 0
#define MASK(mask, value) (((value) << (__builtin_ffs(mask)-1)) & (mask))
typedef struct XboxTexture {
    const uint32_t *source;
    uint32_t *pixels;
    unsigned width, height, format,pitch;
    unsigned source_width,source_height;
    int linear;
    int zero_rgb;
} XboxTexture;
static XboxTexture textures[512];
static unsigned texture_count;
volatile unsigned jpb_XboxTextureCount;
volatile unsigned jpb_XboxRawTextureBytes;
/* At most 512 resident textures; a 1024-slot table keeps triangle lookups
   bounded without moving the contiguous GPU allocations. Values are index+1. */
static uint16_t texture_hash[1024];
static XboxTexture *last_texture;
static const char *bc1_level_name="fed";
static float texture_v_scale(const XboxTexture *entry)
{
    if(entry->linear)return (float)entry->height;
    /* Native XEMU captures show that DXT1 V spans two payload lengths over
       normalized 0..1. World uploads include a second copy; half-range V
       selects the expected tile after per-triangle UV rebasing. */
    return ((entry->format>>8)&0xff)==NV097_SET_TEXTURE_FORMAT_COLOR_L_DXT1_A1R5G5B5
        ?0.5f:1.0f;
}
volatile unsigned jpb_XboxBc1WorldCalls;
volatile unsigned jpb_XboxBc1CacheHits;
volatile unsigned jpb_XboxBc1Eligible;
volatile unsigned jpb_XboxBc1OpenFailures;
volatile unsigned jpb_XboxBc1Loads;
volatile unsigned jpb_XboxBc1ModelLoads,jpb_XboxBc1ModelOpenFailures;
volatile unsigned jpb_XboxBc1LastMaterialType;
volatile unsigned jpb_XboxGpuStage;
volatile unsigned jpb_XboxGpuLastPass;
static uint32_t white = 0xffffffff;
static unsigned submitted_batches, submitted_vertices;
static int model_pass;
static float output_x_per_canvas=2.0f,output_y_per_canvas=2.0f;
#define JPB_XBOX_MODEL_BATCH_VERTICES 384
#define JPB_XBOX_MODEL_ARRAY_BYTES (384u * 1024u)
typedef struct XboxModelArrayVertex {
    float position[4];
    float color[4];
    float uv[4];
} XboxModelArrayVertex;
static XboxModelArrayVertex *model_array_vertices;
static unsigned model_array_count;
static unsigned model_array_enabled;
volatile unsigned jpb_XboxModelArrayActive;
volatile unsigned jpb_XboxModelArrayWaits;
static JPBSoftwareMaterialVertex model_batch_vertices[JPB_XBOX_MODEL_BATCH_VERTICES];
static unsigned model_batch_count;
static XboxTexture *model_batch_texture;
static int model_batch_sampler,model_batch_transparency;
volatile unsigned jpb_XboxModelBatchFlushes;
static void flush_model_batch(void);
static uint32_t *model_command_end;
static unsigned model_command_batches;
static const XboxTexture *triangle_state_texture;
static int triangle_state_sampler,triangle_state_transparency,triangle_state_model_pass;
volatile unsigned jpb_XboxGpuStateChanges;
static const uint32_t *glow_source;
static const JPBSoftwareTexture *glow_texture;
static unsigned glow_scan_frame=~0u;
volatile unsigned jpb_XboxGlowScanCount;
volatile unsigned jpb_XboxGpuTriangleCalls;
volatile unsigned jpb_XboxModelTriangleUs,jpb_XboxEffectsTriangleUs;
static uint64_t model_triangle_ticks,effects_triangle_ticks;
volatile unsigned jpb_XboxModelBatchUs;
static uint64_t model_batch_ticks;
static unsigned glow_triangles;
static float glow_u,glow_v;
static int glow_type;
static int diagnostics_overlay;
extern volatile unsigned jpb_XboxSmokeFrame;
unsigned jpb_XboxGlowCaptureCount;
JPBSoftwareMaterialVertex jpb_XboxGlowCaptureVertices[384];
extern void jpb_XboxAudioStats(void);

/* pbkit does not wrap its 512 KiB command buffer automatically. A frame
   containing many immediate triangles can exceed it. Reset only between
   complete blocks; pb_reset waits for DMA GET to reach the buffer head and
   preserves the GPU's render state. Leave room for pbkit's internal commands. */
static uint32_t *command_base;
static unsigned command_resets;
static uint32_t *bounded_begin(void)
{
    uint32_t *p=pb_begin();
    if(!command_base || p<command_base)command_base=p;
    return p;
}
static void bounded_end(uint32_t *end)
{
    unsigned used=(unsigned)(end-command_base)*sizeof(uint32_t);
    pb_end(end);
    if(used>=JPB_XBOX_PUSHBUFFER_SIZE-16384) {
        jpb_XboxGpuStage=90;
        pb_reset();
        jpb_XboxGpuStage=91;
        command_base=NULL;
        ++command_resets;
    }
}
#define pb_begin bounded_begin
#define pb_end bounded_end

static void commit_model_commands(void)
{
    if(model_command_end){
        pb_end(model_command_end);
        model_command_end=NULL;
        model_command_batches=0;
    }
}


/* Rectangular Morton layout interleaves only the bits present in each axis. */
static unsigned swizzle_offset(unsigned x, unsigned y, unsigned w, unsigned h)
{
    unsigned out = 0, destination_bit = 1;
    for (unsigned bit = 1; bit < w || bit < h; bit <<= 1) {
        if (bit < w) { if (x & bit) out |= destination_bit; destination_bit <<= 1; }
        if (bit < h) { if (y & bit) out |= destination_bit; destination_bit <<= 1; }
    }
    return out;
}

static int load_bc1_texture(XboxTexture *entry, const char *name, int model)
{
    const char *base=name;
    const char *slash=strrchr(name,'/');
    const char *backslash=strrchr(name,'\\');
    if(slash && slash+1>base)base=slash+1;
    if(backslash && backslash+1>base)base=backslash+1;
    const char *dot=strrchr(base,'.');
    unsigned stem=(unsigned)(dot?dot-base:strlen(base));
    if(!stem || stem>120)return 0;
    char path[192];
    if(model)snprintf(path,sizeof(path),"D:\\res\\MODEL\\tga\\%.*s.xbt",(int)stem,base);
    else snprintf(path,sizeof(path),"D:\\res\\level\\jpx\\%s\\%.*s.xbt",
        bc1_level_name,(int)stem,base);
    FILE *file=fopen(path,"rb");
    if(!file){
        if(model)++jpb_XboxBc1ModelOpenFailures;
        else ++jpb_XboxBc1OpenFailures;
        return 0;
    }
    struct {char magic[4];uint32_t width,height,bytes;} header;
    int valid=fread(&header,sizeof(header),1,file)==1 &&
        memcmp(header.magic,"XBT1",4)==0 &&
        header.width>=4 && header.width<=1024 &&
        header.height>=4 && header.height<=1024 &&
        !(header.width&(header.width-1)) &&
        !(header.height&(header.height-1)) &&
        header.bytes==header.width*header.height/2;
    if(!valid){fclose(file);return 0;}
    uint32_t allocation_bytes=model?header.bytes:header.bytes*2;
    uint32_t *pixels=MmAllocateContiguousMemoryEx(allocation_bytes,0,0x03ffafff,0,
        PAGE_READWRITE|PAGE_WRITECOMBINE);
    if(!pixels){fclose(file);return 0;}
    if(fread(pixels,1,header.bytes,file)!=header.bytes){
        fclose(file);MmFreeContiguousMemory(pixels);return 0;
    }
    fclose(file);
    /* XEMU's DXT1 sampling traverses two payload lengths over normalized
       V=0..1. Keep a second copy adjacent so half-range V retains native
       repeat addressing for authored world UVs outside 0..1. */
    if(!model)memcpy((uint8_t *)pixels+header.bytes,pixels,header.bytes);
    /* The allocation is write-combined. Model textures can be submitted in
       the same frame as the file read, so drain CPU writes before NV2A sees
       the texture address in the push buffer. */
    __builtin_ia32_sfence();
    unsigned logw=0,logh=0;
    while((1u<<logw)<header.width)++logw;
    while((1u<<logh)<header.height)++logh;
    entry->pixels=pixels;
    entry->width=header.width;
    entry->height=header.height;
    entry->format=1|(2<<4)|(NV097_SET_TEXTURE_FORMAT_COLOR_L_DXT1_A1R5G5B5<<8)|
        (1<<16)|(logw<<20)|(logh<<24);
    entry->pitch=0;
    entry->linear=0;
    entry->zero_rgb=0;
    ++jpb_XboxBc1Loads;
    if(model)++jpb_XboxBc1ModelLoads;
    return 1;
}

static XboxTexture *resolve(const JPBSoftwareTexture *texture,const char *world_name)
{
    const uint32_t *source = texture ? texture->pixels : &white;
    unsigned width = texture ? texture->width : 1;
    unsigned height = texture ? texture->height : 1;
    unsigned stride = texture ? texture->stridePixels : 1;
    unsigned logw = 0, logh = 0;
    if(world_name){
        ++jpb_XboxBc1WorldCalls;
        jpb_XboxBc1LastMaterialType=texture?texture->materialType:~0u;
        if(texture && texture->materialType==0)++jpb_XboxBc1Eligible;
    }
    if (last_texture && last_texture->source == source &&
        last_texture->source_width == width &&
        last_texture->source_height == height) {
        if (world_name) ++jpb_XboxBc1CacheHits;
        return last_texture;
    }
    uintptr_t key=(uintptr_t)source>>4;
    unsigned bucket=(unsigned)(key^(key>>11)^width*2654435761u^
        height*2246822519u)&1023u;
    while(texture_hash[bucket]){
        XboxTexture *cached=&textures[texture_hash[bucket]-1];
        if (cached->source == source && cached->source_width == width &&
            cached->source_height == height){
            if(world_name)++jpb_XboxBc1CacheHits;
            last_texture=cached;
            return cached;
        }
        bucket=(bucket+1)&1023u;
    }
    int linear=width<4 || height<4 ||
        (texture && texture->materialType!=0);
    if (!source || !width || !height || width>2048 || height>2048 ||
        (!linear && ((width & (width-1)) || (height & (height-1)))) ||
        texture_count == 512)
        return NULL;
    XboxTexture *entry = &textures[texture_count];
    const char *bc1_name=NULL;
    int bc1_model=0;
    if(world_name && texture && texture->materialType==0)bc1_name=world_name;
    else if(JPB_XBOX_MODEL_BC1 && model_pass && texture && texture->materialType==0 &&
            texture->sourceName){
        bc1_name=texture->sourceName;
        bc1_model=1;
    }
    if(bc1_name && load_bc1_texture(entry,bc1_name,bc1_model)) {
        entry->source=source;
        entry->source_width=width;
        entry->source_height=height;
        texture_hash[bucket]=(uint16_t)(texture_count+1);
        ++texture_count;
        jpb_XboxTextureCount=texture_count;
        last_texture=entry;
        return entry;
    }
    /* Translucent cards expose incorrect alpha sampling through the
       swizzled path in XEMU; linear A8R8G8B8 reproduces authored falloff.
       It uses pixel-space UVs below, unlike the swizzled normalized path. */
    entry->linear=linear;
    entry->pitch=entry->linear?((width*4+63)&~63u):width*4;
    entry->pixels = MmAllocateContiguousMemoryEx(entry->pitch * height, 0, 0x03ffafff, 0,
        PAGE_READWRITE | PAGE_WRITECOMBINE);
    if (!entry->pixels) return NULL;
    jpb_XboxRawTextureBytes+=entry->pitch*height;
    entry->zero_rgb=1;
    for (unsigned y = 0; y < height; ++y)
        for (unsigned x = 0; x < width; ++x) {
            entry->pixels[entry->linear?y*(entry->pitch/4)+x:swizzle_offset(x, y, width, height)] = source[y * stride + x];
            if(source[y*stride+x]&0xffffff)entry->zero_rgb=0;
        }
    while ((1u << logw) < width) ++logw;
    while ((1u << logh) < height) ++logh;
    entry->format = 1 | (2 << 4) | (NV097_SET_TEXTURE_FORMAT_COLOR_SZ_A8R8G8B8 << 8) |
        (1 << 16) | (logw << 20) | (logh << 24);
    if(entry->linear)entry->format=1|(2<<4)|(NV097_SET_TEXTURE_FORMAT_COLOR_LU_IMAGE_A8R8G8B8<<8)|(1<<16);
    entry->source = source; entry->width = width; entry->height = height;
    entry->source_width=width;entry->source_height=height;
    texture_hash[bucket]=(uint16_t)(texture_count+1);
    ++texture_count;
    jpb_XboxTextureCount=texture_count;
    last_texture=entry;
    return entry;
}

int jpb_XboxGpuInit(void)
{
    memset(texture_hash,0,sizeof(texture_hash));
    texture_count=0;
    jpb_XboxTextureCount=0;
    jpb_XboxRawTextureBytes=0;
    last_texture=NULL;
    FILE *model_array_flag=fopen("D:\\xbox-model-vertex-array.txt","rb");
    model_array_enabled=0;
    if(model_array_flag){
        char option[16]={0};
        if(fgets(option,sizeof(option),model_array_flag) &&
           strncmp(option,"enable",6)==0)model_array_enabled=1;
        fclose(model_array_flag);
    }
    if(model_array_enabled){
        model_array_vertices=MmAllocateContiguousMemoryEx(
            JPB_XBOX_MODEL_ARRAY_BYTES,0,0x03ffafff,0,
            PAGE_READWRITE | PAGE_WRITECOMBINE);
        if(!model_array_vertices)model_array_enabled=0;
    }
    jpb_XboxModelArrayActive=model_array_enabled;
    FILE *diagnostics=fopen("D:\\openjpb-diagnostics.flag","rb");
    diagnostics_overlay=diagnostics!=NULL;
    if(diagnostics)fclose(diagnostics);
    uint32_t shader[] = {
#include "gpu.inl"
    };
    pb_size(JPB_XBOX_PUSHBUFFER_SIZE);
    int result = pb_init();
    if (result) return result;
    uint32_t *p = pb_begin();
    p = pb_push1(p, NV097_SET_TRANSFORM_PROGRAM_START, 0);
    p = pb_push1(p, NV097_SET_TRANSFORM_EXECUTION_MODE,
        MASK(NV097_SET_TRANSFORM_EXECUTION_MODE_MODE, NV097_SET_TRANSFORM_EXECUTION_MODE_MODE_PROGRAM) |
        MASK(NV097_SET_TRANSFORM_EXECUTION_MODE_RANGE_MODE, NV097_SET_TRANSFORM_EXECUTION_MODE_RANGE_MODE_PRIV));
    p = pb_push1(p, NV097_SET_TRANSFORM_PROGRAM_CXT_WRITE_EN, 0);
    p = pb_push1(p, NV097_SET_TRANSFORM_PROGRAM_LOAD, 0);
    pb_end(p);
    for (unsigned i = 0; i < sizeof(shader)/sizeof(shader[0]); i += 4) {
        p = pb_begin();
        pb_push(p++, NV097_SET_TRANSFORM_PROGRAM, 4);
        memcpy(p, shader+i, 16); p += 4;
        pb_end(p);
    }
    {
        uint32_t level_shader[] = {
#include "level.inl"
        };
        p = pb_begin();
        p = pb_push1(p, NV097_SET_TRANSFORM_PROGRAM_LOAD, 32);
        pb_end(p);
        for (unsigned i = 0; i < sizeof(level_shader)/4; i += 4) {
            p = pb_begin();
            pb_push(p++, NV097_SET_TRANSFORM_PROGRAM, 4);
            memcpy(p, level_shader+i,16); p += 4;
            pb_end(p);
        }
    }
    p = pb_begin();
#include "fragment.inl"
    p = pb_push1(p, NV097_SET_CULL_FACE_ENABLE, 0);
    p = pb_push1(p, NV097_SET_DEPTH_TEST_ENABLE, 1);
    p = pb_push1(p, NV097_SET_DEPTH_MASK, 1);
    p = pb_push1(p, NV097_SET_DEPTH_FUNC, NV097_SET_DEPTH_FUNC_V_LEQUAL);
    pb_end(p);
    pb_show_front_screen();
    return 0;
}

void jpb_XboxGpuBegin(void)
{
    VIDEO_MODE output_mode=XVideoGetMode();
    output_x_per_canvas=(float)output_mode.width/JPB_XBOX_CANVAS_WIDTH;
    output_y_per_canvas=(float)output_mode.height/JPB_XBOX_CANVAS_HEIGHT;
    flush_model_batch();
    commit_model_commands();
    jpb_XboxGpuStage=10;
    submitted_batches=submitted_vertices=0;
    glow_triangles=0;
    model_triangle_ticks=effects_triangle_ticks=model_batch_ticks=0;
    command_resets=0;
    model_array_count=0;
    jpb_XboxModelArrayWaits=0;
    triangle_state_texture=NULL;
    /* pb_finished() queues a VBlank swap and manages its three back buffers.
       Waiting for another VBlank here adds a full refresh after slow frames. */
    pb_reset();
    command_base=NULL;
    pb_target_back_buffer();
    pb_erase_depth_stencil_buffer(0, 0, output_mode.width, output_mode.height);
    pb_fill(0, 0, output_mode.width, output_mode.height, 0);
    uint32_t *p=pb_begin();
    /* pb_target_back_buffer enables perspective depth (W buffering). Our
       vertices already contain projected Z, matching the PC depth buffer;
       interpolating it with W again breaks world/model occlusion. */
    p=pb_push1(p,NV097_SET_CONTROL0,
        NV097_SET_CONTROL0_TEXTURE_PERSPECTIVE_ENABLE |
        NV097_SET_CONTROL0_STENCIL_WRITE_ENABLE);
    p=pb_push1(p,NV097_SET_DEPTH_TEST_ENABLE,1);
    p=pb_push1(p,NV097_SET_DEPTH_MASK,1);
    pb_end(p);
}

int jpb_XboxGpuModelBegin(void *user, JPBSoftwareFramebuffer *fb, JPBSoftwareDepthBuffer *depth)
{
    flush_model_batch();
    commit_model_commands();
    jpb_XboxGpuStage=20;
    (void)user; (void)fb; (void)depth;
    model_pass=1;
    triangle_state_texture=NULL;
    uint32_t *p=pb_begin();
    p=pb_push1(p,NV097_SET_DEPTH_TEST_ENABLE,1);
    p=pb_push1(p,NV097_SET_DEPTH_MASK,1);
    p=pb_push1(p,NV097_SET_BLEND_ENABLE,1);
    p=pb_push1(p,NV097_SET_BLEND_FUNC_SFACTOR,NV097_SET_BLEND_FUNC_SFACTOR_V_SRC_ALPHA);
    p=pb_push1(p,NV097_SET_BLEND_FUNC_DFACTOR,NV097_SET_BLEND_FUNC_DFACTOR_V_ONE_MINUS_SRC_ALPHA);
    pb_end(p);
    return 1;
}

int jpb_XboxGpuEffectsBegin(void *user, JPBSoftwareFramebuffer *fb, JPBSoftwareDepthBuffer *depth)
{
    jpb_XboxGpuStage=30;
    jpb_XboxGpuModelBegin(user,fb,depth);
    model_pass=0;
    uint32_t *p=pb_begin();
    p=pb_push1(p,NV097_SET_DEPTH_MASK,0);
    pb_end(p);
    return 1;
}

void jpb_XboxGpuPresent(void)
{
    flush_model_batch();
    commit_model_commands();
    uint64_t frequency=KeQueryPerformanceFrequency();
    if(frequency){
        jpb_XboxModelTriangleUs=(unsigned)(model_triangle_ticks*1000000u/frequency);
        jpb_XboxEffectsTriangleUs=(unsigned)(effects_triangle_ticks*1000000u/frequency);
        jpb_XboxModelBatchUs=(unsigned)(model_batch_ticks*1000000u/frequency);
    }
    while (pb_busy()) {}
    while (pb_finished()) {}
}

void jpb_XboxGpuStats(unsigned frame, unsigned elapsed, unsigned pad, int motion)
{
    if(!diagnostics_overlay)return;
    pb_erase_text_screen();
    pb_print("OpenJPB nxdk GPU diagnostic\nF %u T %u ms pad %04x motion %d\n",
        frame, elapsed, pad, motion);
}

void jpb_XboxGpuProfile(const JPBGameRuntime *runtime)
{
    if(!diagnostics_overlay)return;
    MM_STATISTICS memory={0};
    memory.Length=sizeof(memory);
    MmQueryStatistics(&memory);
    pb_print("ms frame %u world %u models %u effects %u hud %u\n",
        (unsigned)(runtime->profileLastFrameSeconds*1000),
        (unsigned)(runtime->profileLastWorldSeconds*1000),
        (unsigned)(runtime->profileLastModelsSeconds*1000),
        (unsigned)(runtime->profileLastEffectsSeconds*1000),
        (unsigned)(runtime->profileLastHudSeconds*1000));
    pb_print("level batches %u vertices %u command wraps %u\n",submitted_batches,submitted_vertices,command_resets);
    pb_print("HUD upload %u finish %u replay %u\n",
        (unsigned)(runtime->profileLastCompositeUploadSeconds*1000),
        (unsigned)(runtime->profileLastCompositeFinishSeconds*1000),
        (unsigned)(runtime->profileLastHudReplaySeconds*1000));
    pb_print("RAM free %lu KiB textures %u damage events %u\n",
        memory.AvailablePages*4,texture_count,runtime->enemyDamageProcessedCount);
    jpb_XboxAudioStats();
    pb_print("Glow triangles %u type %d UV %d/%d\n",glow_triangles,glow_type,(int)(glow_u*1000),(int)(glow_v*1000));
    /* Text submission is internal to pbkit; give it a fresh buffer. */
    pb_reset();
    command_base=NULL;
    pb_draw_text_screen();
}

static void flush_model_batch(void)
{
    if(!model_batch_count){
        commit_model_commands();
        return;
    }
    uint64_t batch_started=KeQueryPerformanceCounter();
    if(model_array_enabled &&
       model_array_count+model_batch_count>
           JPB_XBOX_MODEL_ARRAY_BYTES/sizeof(XboxModelArrayVertex)){
        commit_model_commands();
        while(pb_busy()){}
        ++jpb_XboxModelArrayWaits;
        model_array_count=0;
    }
    XboxTexture *entry=model_batch_texture;
    int sampler=model_batch_sampler;
    int transparency=model_batch_transparency;
    int state_changed=entry!=triangle_state_texture ||
        sampler!=triangle_state_sampler ||
        transparency!=triangle_state_transparency ||
        triangle_state_model_pass!=1;
    uint32_t *p=model_command_end?model_command_end:pb_begin();
    if(state_changed){
        ++jpb_XboxGpuStateChanges;
        triangle_state_texture=entry;
        triangle_state_sampler=sampler;
        triangle_state_transparency=transparency;
        triangle_state_model_pass=1;
        p=pb_push1(p,NV097_SET_TRANSFORM_PROGRAM_START,0);
        p=pb_push2(p,NV097_SET_TEXTURE_OFFSET,
            (uint32_t)entry->pixels&0x03ffffff,entry->format);
        p=pb_push1(p,NV097_SET_TEXTURE_CONTROL1,entry->pitch<<16);
        p=pb_push1(p,NV097_SET_TEXTURE_IMAGE_RECT,
            (entry->width<<16)|entry->height);
        p=pb_push1(p,NV097_SET_TEXTURE_ADDRESS,
            (sampler==TEXTURESAMPLER_LINEARCLAMP ||
             sampler==TEXTURSAMPLER_POINTCLAMP)?0x00050505:0x00010101);
        p=pb_push1(p,NV097_SET_TEXTURE_CONTROL0,0x4003ffc0);
        p=pb_push1(p,NV097_SET_TEXTURE_FILTER,
            sampler==TEXTURSAMPLER_POINTCLAMP?0x01012000:0x02022000);
    }
    const float u_scale=entry->linear?(float)entry->width:1.0f;
    const float v_scale=texture_v_scale(entry);
    if(model_array_enabled){
        XboxModelArrayVertex *base=model_array_vertices+model_array_count;
        for(unsigned i=0;i<model_batch_count;++i){
            const JPBSoftwareMaterialVertex *v=&model_batch_vertices[i];
            XboxModelArrayVertex *out=base+i;
            float inverse=v->inverseDepth>0?v->inverseDepth:1;
            float z=1.00010001f-1.00010001f*(inverse/10240.0f);
            int tint=v->red>0 && v->green>0 && v->blue>0 && v->alpha>0;
            out->position[0]=v->x*output_x_per_canvas;
            out->position[1]=v->y*output_y_per_canvas;
            out->position[2]=z*16777215;
            out->position[3]=1;
            out->color[0]=tint?v->red/255.0f:1;
            out->color[1]=tint?v->green/255.0f:1;
            out->color[2]=tint?v->blue/255.0f:1;
            out->color[3]=1;
            out->uv[0]=v->u*inverse*u_scale;
            out->uv[1]=v->v*inverse*v_scale;
            out->uv[2]=0;
            out->uv[3]=inverse;
        }
        model_array_count+=model_batch_count;
        const uintptr_t address=(uintptr_t)base;
        static const unsigned attributes[3]={0,3,9};
        static const unsigned offsets[3]={0,16,32};
        for(unsigned i=0;i<3;++i){
            p=pb_push1(p,NV097_SET_VERTEX_DATA_ARRAY_FORMAT+attributes[i]*4,
                NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE_F |
                (4<<4) | (sizeof(XboxModelArrayVertex)<<8));
            p=pb_push1(p,NV097_SET_VERTEX_DATA_ARRAY_OFFSET+attributes[i]*4,
                (address+offsets[i])&0x03ffffff);
        }
        p=pb_push1(p,NV097_SET_BEGIN_END,NV097_SET_BEGIN_END_OP_TRIANGLES);
        p=pb_push1(p,0x40000000|NV097_DRAW_ARRAYS,
            (model_batch_count-1)<<24);
        p=pb_push1(p,NV097_SET_BEGIN_END,NV097_SET_BEGIN_END_OP_END);
    }else{
        p=pb_push1(p,NV097_SET_BEGIN_END,NV097_SET_BEGIN_END_OP_TRIANGLES);
        for(unsigned i=0;i<model_batch_count;++i){
            const JPBSoftwareMaterialVertex *v=&model_batch_vertices[i];
            float inverse=v->inverseDepth>0?v->inverseDepth:1;
            float z=1.00010001f-
                1.00010001f*(inverse/10240.0f);
            int tint=v->red>0 && v->green>0 && v->blue>0 && v->alpha>0;
            float r=tint?v->red/255.0f:1;
            float g=tint?v->green/255.0f:1;
            float blue=tint?v->blue/255.0f:1;
            p=pb_push4f(p,NV097_SET_VERTEX_DATA4F_M+3*16,r,g,blue,1);
            p=pb_push4f(p,NV097_SET_VERTEX_DATA4F_M+9*16,
                v->u*inverse*u_scale,
                v->v*inverse*v_scale,0,inverse);
            p=pb_push4f(p,NV097_SET_VERTEX_DATA4F_M,
                v->x*output_x_per_canvas,
                v->y*output_y_per_canvas,z*16777215,1);
        }
        p=pb_push1(p,NV097_SET_BEGIN_END,NV097_SET_BEGIN_END_OP_END);
    }
    model_command_end=p;
    ++model_command_batches;
    if(model_command_batches>=8 ||
       (unsigned)(p-command_base)*sizeof(uint32_t)>=
           JPB_XBOX_PUSHBUFFER_SIZE-40000){
        pb_end(p);
        model_command_end=NULL;
        model_command_batches=0;
        if((unsigned)(p-command_base)*sizeof(uint32_t)>=
           JPB_XBOX_PUSHBUFFER_SIZE-40000){
            pb_reset();
            command_base=NULL;
            ++command_resets;
        }
    }
    model_batch_count=0;
    ++jpb_XboxModelBatchFlushes;
    model_batch_ticks+=KeQueryPerformanceCounter()-batch_started;
}

int jpb_XboxGpuTriangle(void *unused, const JPBSoftwareMaterialVertex *a,
    const JPBSoftwareMaterialVertex *b, const JPBSoftwareMaterialVertex *c,
    const JPBSoftwareTexture *texture)
{
    jpb_XboxGpuStage=40;
    ++jpb_XboxGpuTriangleCalls;
#if JPB_XBOX_TRIANGLE_PROFILE
    uint64_t sink_started=KeQueryPerformanceCounter();
#define JPB_TRIANGLE_END() do { \
    uint64_t ticks=KeQueryPerformanceCounter()-sink_started; \
    if(model_pass)model_triangle_ticks+=ticks; \
    else effects_triangle_ticks+=ticks; \
} while(0)
#else
#define JPB_TRIANGLE_END() ((void)0)
#endif
    (void)unused;
    if(!glow_source && glow_scan_frame!=jpb_XboxSmokeFrame){
        glow_scan_frame=jpb_XboxSmokeFrame;
        ++jpb_XboxGlowScanCount;
        for(unsigned i=0;i<JPB_TEXTURE_MATERIAL_CAPACITY;++i)
            if(g_material[i].texture && strstr(g_material[i].filename,"a_glow")) {
                glow_texture=(JPBSoftwareTexture *)g_material[i].texture;
                glow_source=glow_texture->pixels;
                break;
            }
    }
    XboxTexture *entry = resolve(texture,NULL);
    if (!entry) {
        JPB_TRIANGLE_END();
        return 0;
    }
    if(texture && texture->pixels==glow_source) {
        ++glow_triangles;glow_type=texture->materialType;glow_u=a->u;glow_v=a->v;
        if(jpb_XboxSmokeFrame==650 && jpb_XboxGlowCaptureCount+3<=384) {
            jpb_XboxGlowCaptureVertices[jpb_XboxGlowCaptureCount++]=*a;
            jpb_XboxGlowCaptureVertices[jpb_XboxGlowCaptureCount++]=*b;
            jpb_XboxGlowCaptureVertices[jpb_XboxGlowCaptureCount++]=*c;
        }
    }
    /* PSModel discards sampled RGB==0. Entirely black masks (transabr)
       therefore emit no fragments, including no depth writes. */
    if(model_pass && entry->zero_rgb){
        JPB_TRIANGLE_END();
        return 1;
    }
    int transparency=texture?texture->materialType:0;
    int sampler=texture?texture->samplerType:0;
    if(model_pass){
        if(model_batch_count &&
           (entry!=model_batch_texture || sampler!=model_batch_sampler ||
            transparency!=model_batch_transparency))flush_model_batch();
        if(!model_batch_count){
            model_batch_texture=entry;
            model_batch_sampler=sampler;
            model_batch_transparency=transparency;
        }
        model_batch_vertices[model_batch_count++]=*a;
        model_batch_vertices[model_batch_count++]=*b;
        model_batch_vertices[model_batch_count++]=*c;
        if(model_batch_count==JPB_XBOX_MODEL_BATCH_VERTICES)flush_model_batch();
        JPB_TRIANGLE_END();
        return 1;
    }
    int state_changed=entry!=triangle_state_texture ||
        sampler!=triangle_state_sampler ||
        transparency!=triangle_state_transparency ||
        model_pass!=triangle_state_model_pass;
    uint32_t *p = pb_begin();
    if(state_changed){
    ++jpb_XboxGpuStateChanges;
    triangle_state_texture=entry;
    triangle_state_sampler=sampler;
    triangle_state_transparency=transparency;
    triangle_state_model_pass=model_pass;
    p = pb_push1(p, NV097_SET_TRANSFORM_PROGRAM_START, 0);
    p = pb_push2(p, NV097_SET_TEXTURE_OFFSET, (uint32_t)entry->pixels & 0x03ffffff, entry->format);
    p = pb_push1(p,NV097_SET_TEXTURE_CONTROL1,entry->pitch<<16);
    p = pb_push1(p,NV097_SET_TEXTURE_IMAGE_RECT,(entry->width<<16)|entry->height);
    if(!model_pass) {
        p=pb_push1(p,NV097_SET_DEPTH_MASK,transparency==0);
        p=pb_push1(p,NV097_SET_BLEND_ENABLE,transparency!=0);
        p=pb_push1(p,NV097_SET_BLEND_FUNC_SFACTOR,NV097_SET_BLEND_FUNC_SFACTOR_V_SRC_ALPHA);
        p=pb_push1(p,NV097_SET_BLEND_FUNC_DFACTOR,transparency==1?
            NV097_SET_BLEND_FUNC_DFACTOR_V_ONE:NV097_SET_BLEND_FUNC_DFACTOR_V_ONE_MINUS_SRC_ALPHA);
    }
    /* nxdk's NV2A sample identifies mode 5 as clamp-to-edge. Mode 3 is
       GL-style clamp, which reads border color near u/v = 1; D3D clamp
       samplers in the PC renderer use the edge texel instead. */
    p = pb_push1(p, NV097_SET_TEXTURE_ADDRESS, texture &&
        (texture->samplerType==TEXTURESAMPLER_LINEARCLAMP || texture->samplerType==TEXTURSAMPLER_POINTCLAMP)?0x00050505:0x00010101);
    p = pb_push1(p, NV097_SET_TEXTURE_CONTROL0, 0x4003ffc0);
    p = pb_push1(p, NV097_SET_TEXTURE_FILTER, texture && texture->samplerType==TEXTURSAMPLER_POINTCLAMP?0x01012000:0x02022000);
    }
    p = pb_push1(p, NV097_SET_BEGIN_END, NV097_SET_BEGIN_END_OP_TRIANGLES);
    const JPBSoftwareMaterialVertex *vertices[] = {a,b,c};
    for (unsigned i = 0; i < 3; ++i) {
        const JPBSoftwareMaterialVertex *v = vertices[i];
        /* PC VSImmediate emits W=1: its screen-space effects interpolate UVs
           affinely. Only VSModel carries camera W for perspective correction. */
        float inverse = model_pass && v->inverseDepth > 0 ? v->inverseDepth : 1;
        /* VSImmediate already supplies clip depth; only VSModel reconstructs
           it from camera depth. Re-projecting immediate effects moves them
           to the wrong depth relative to actors and the world. */
        float z = model_pass
            ? 1.00010001f - 1.00010001f / fmaxf(v->depth * 10240.0f, 1.0f)
            : v->clipDepth;
        int tint = !model_pass || (v->red>0 && v->green>0 && v->blue>0 && v->alpha>0);
        float r=tint?v->red/255.0f:1;
        float g=tint?v->green/255.0f:1;
        float blue=tint?v->blue/255.0f:1;
        if(!model_pass && transparency==0) {
            r=fmaxf(r,0.1f); g=fmaxf(g,0.1f); blue=fmaxf(blue,0.1f);
        }
        p = pb_push4f(p, NV097_SET_VERTEX_DATA4F_M + 3*16,
            r,g,blue,model_pass?1:v->alpha/255.0f);
        p = pb_push4f(p, NV097_SET_VERTEX_DATA4F_M + 9*16,
            v->u*inverse*(entry->linear?entry->width:1),
            v->v*inverse*texture_v_scale(entry), 0, inverse);
        p = pb_push4f(p, NV097_SET_VERTEX_DATA4F_M,
            v->x*output_x_per_canvas,v->y*output_y_per_canvas,
            z*16777215, 1);
    }
    p = pb_push1(p, NV097_SET_BEGIN_END, NV097_SET_BEGIN_END_OP_END);
    pb_end(p);
    JPB_TRIANGLE_END();
    return 1;
#undef JPB_TRIANGLE_END
}

/* The NV2A vertex program emits screen coordinates, after division by W.
   Clip triangles before that division, as the PC's D3D clip stage does.
   Otherwise triangles crossing/behind the eye create stretched polygons. */
static float level_depth(const JPBSoftwareLevelVertex *v,const MATRIX *view)
{
    return v->position.vx*view->m[2][0]+v->position.vy*view->m[2][1]+
        v->position.vz*view->m[2][2]+view->t[2];
}

static unsigned clip_level_plane(const JPBSoftwareLevelVertex *in,unsigned n,
    JPBSoftwareLevelVertex *out,const MATRIX *view,float plane,int far_plane)
{
    unsigned written=0;
    for(unsigned i=0;i<n;++i) {
        const JPBSoftwareLevelVertex *a=&in[(i+n-1)%n],*b=&in[i];
        float da=level_depth(a,view)-plane,db=level_depth(b,view)-plane;
        if(far_plane) {da=-da;db=-db;}
        if((da>=0)!=(db>=0)) {
            float t=da/(da-db);
            float av[11],bv[11],result[11];
            memcpy(av,a,sizeof(av));memcpy(bv,b,sizeof(bv));
            for(unsigned k=0;k<11;++k)result[k]=av[k]+t*(bv[k]-av[k]);
            memcpy(&out[written++],result,sizeof(result));
        }
        if(db>=0)out[written++]=*b;
    }
    return written;
}

static void draw_clipped_level(const JPBSoftwareLevelBatch *batch,const uint16_t *indices,
    unsigned first,unsigned count,const MATRIX *view)
{
    for(unsigned i=0;i<count;i+=3) {
        JPBSoftwareLevelVertex source[3],near_poly[6],poly[6];
        for(unsigned j=0;j<3;++j)
            jpb_XboxLevelDecodeVertex(batch,
                indices?indices[first+i+j]:first+i+j,&source[j]);
        unsigned n=clip_level_plane(source,3,near_poly,view,1,0);
        n=clip_level_plane(near_poly,n,poly,view,10000,1);
        for(unsigned j=1;j+1<n;++j) {
            const JPBSoftwareLevelVertex *tri[3]={&poly[0],&poly[j],&poly[j+1]};
            uint32_t *p=pb_begin();
            p=pb_push1(p,NV097_SET_BEGIN_END,NV097_SET_BEGIN_END_OP_TRIANGLES);
            for(unsigned k=0;k<3;++k) {
                const JPBSoftwareLevelVertex *v=tri[k];
                p=pb_push4f(p,NV097_SET_VERTEX_DATA4F_M+3*16,v->red,v->green,v->blue,v->alpha);
                p=pb_push4f(p,NV097_SET_VERTEX_DATA4F_M+9*16,v->u,v->v,0,1);
                p=pb_push4f(p,NV097_SET_VERTEX_DATA4F_M+10*16,v->uvScrollU,v->uvScrollV,0,1);
                p=pb_push4f(p,NV097_SET_VERTEX_DATA4F_M,v->position.vx,v->position.vy,v->position.vz,1);
            }
            p=pb_push1(p,NV097_SET_BEGIN_END,NV097_SET_BEGIN_END_OP_END);
            pb_end(p);
            submitted_vertices+=3;
        }
    }
}

void jpb_XboxGpuTransparencyProbe(void)
{
    /* Show the loaded glow texture over dark gray when available; otherwise
       use the four-color upload probe. Opt-in diagnostic only. */
    static uint32_t probe_pixels[2*2],backdrop_pixels[2*2];
    static int initialized;
    if(!initialized) {
        static const uint32_t colors[4]={0,0xff0000ff,0x80ff0000,0xff00ff00};
        for(unsigned y=0;y<2;++y)for(unsigned x=0;x<2;++x) {
            probe_pixels[y*2+x]=colors[x+2*y];
            backdrop_pixels[y*2+x]=0xff202020;
        }
        initialized=1;
    }
    JPBSoftwareTexture backdrop={0},probe={0};
    backdrop.pixels=backdrop_pixels;backdrop.width=backdrop.height=backdrop.stridePixels=2;
    probe.pixels=probe_pixels;probe.width=probe.height=probe.stridePixels=2;
    if(glow_texture)probe=*glow_texture;
    probe.materialType=2;probe.samplerType=TEXTURSAMPLER_POINTCLAMP;
    JPBSoftwareMaterialVertex v[4]={0};
    for(unsigned i=0;i<4;++i) {
        v[i].x=220+(i&1)*80;v[i].y=150+(i>>1)*80;
        v[i].inverseDepth=1;v[i].u=i&1;v[i].v=i>>1;
        v[i].red=v[i].green=v[i].blue=v[i].alpha=255;
    }
    jpb_XboxGpuEffectsBegin(NULL,NULL,NULL);
    jpb_XboxGpuTriangle(NULL,&v[0],&v[1],&v[2],&backdrop);
    jpb_XboxGpuTriangle(NULL,&v[2],&v[1],&v[3],&backdrop);
    jpb_XboxGpuTriangle(NULL,&v[0],&v[1],&v[2],&probe);
    jpb_XboxGpuTriangle(NULL,&v[2],&v[1],&v[3],&probe);
}

int jpb_XboxGpuLevel(void *user, const JPBSoftwareLevelMesh *mesh, JPBLevelFbxMeshPass pass,
    const JPBSoftwareJpxScene *world, MATRIX *view, JPBSoftwareFramebuffer *fb, uint32_t clear,
    JPBSoftwareTextureResolver resolver, void *textures_user, JPBSoftwareDepthBuffer *depth,
    JPBSoftwareRenderStats *stats)
{
    flush_model_batch();
    commit_model_commands();
    jpb_XboxGpuStage=50+(unsigned)pass;
    jpb_XboxGpuLastPass=(unsigned)pass;
    (void)user;
    if (mesh) {
        const int packed=jpb_XboxLevelUsesPackedVertices();
        if(mesh->levelIndex>=0 && mesh->levelIndex<JPB_LEVEL_NAME_COUNT)
            bc1_level_name=sLevelNames[mesh->levelIndex];
        uint32_t *p = pb_begin();
        float constants[28];
        for (unsigned row=0; row<3; ++row) {
            for (unsigned col=0; col<3; ++col) constants[row*4+col]=view->m[row][col];
            constants[row*4+3]=(float)view->t[row];
        }
        constants[13]=1.0f/tanf(0.9250245094299316f*0.5f);
        constants[12]=constants[13]*(float)fb->height/(float)fb->width;
        constants[14]=10000.0f/9999.0f; constants[15]=-constants[14];
        constants[16]=g_levelUVScroll.vx; constants[17]=g_levelUVScroll.vy;
        constants[18]=constants[19]=0;
        /* cgc literal constants emitted in level.inl, c5/c6. */
        constants[20]=0; constants[21]=1;
        constants[22]=(float)XVideoGetMode().width*0.5f;
        constants[23]=(float)XVideoGetMode().height*0.5f;
        constants[24]=16777215; constants[25]=0.1f;
        constants[26]=constants[27]=0;
        jpb_XboxLevelSetFrustum(view,constants[12],constants[13]);
        p=pb_push1(p,NV097_SET_TRANSFORM_PROGRAM_START,32);
        p=pb_push1(p,NV097_SET_TRANSFORM_CONSTANT_LOAD,96);
        pb_push(p++,NV097_SET_TRANSFORM_CONSTANT,28); memcpy(p,constants,sizeof(constants)); p+=28;
        p=pb_push1(p,NV097_SET_TRANSFORM_CONSTANT_LOAD,104);
        p=pb_push4f(p,NV097_SET_TRANSFORM_CONSTANT,
            packed?1.0f:1.0f/255.0f,0,0,0);
        p=pb_push1(p,NV097_SET_DEPTH_MASK,pass!=JPB_LEVEL_FBX_PASS_GLASS);
        p=pb_push1(p,NV097_SET_BLEND_ENABLE,pass!=JPB_LEVEL_FBX_PASS_OPAQUE);
        p=pb_push1(p,NV097_SET_BLEND_FUNC_SFACTOR,NV097_SET_BLEND_FUNC_SFACTOR_V_SRC_ALPHA);
        p=pb_push1(p,NV097_SET_BLEND_FUNC_DFACTOR,NV097_SET_BLEND_FUNC_DFACTOR_V_ONE_MINUS_SRC_ALPHA);
        pb_end(p);
        for (unsigned i=0;i<mesh->batchCount;++i) {
            const JPBSoftwareLevelBatch *batch=&mesh->batches[i];
            if (batch->pass!=pass || !jpb_ShouldDrawFbxMesh(mesh->levelIndex,pass,batch->meshCount,batch->meshIndex,batch->meshName)) continue;
            /* Do not decode/upload materials for an entirely invisible batch. */
            int visible=0;
            for(unsigned first=0;first<batch->vertexCount;first+=252)
                if(jpb_XboxLevelChunkVisible(i,first)) { visible=1; break; }
            if(!visible)continue;
            JPBSoftwareTexture texture={0};
            if (batch->textureName[0] && !resolver(textures_user,batch->textureName,&texture)) {
                debugPrint("GPU texture lookup failed: %s\n",batch->textureName);
                return JPB_SOFTWARE_RENDER_INVALID_ARGUMENT;
            }
            XboxTexture *entry=resolve(batch->textureName[0] ? &texture : NULL,
                pass==JPB_LEVEL_FBX_PASS_OPAQUE?batch->textureName:NULL);
            if (!entry) {
                debugPrint("GPU upload failed: %s %ux%u cache=%u\n",batch->textureName,
                    (unsigned)texture.width,(unsigned)texture.height,texture_count);
                return JPB_SOFTWARE_RENDER_INVALID_ARGUMENT;
            }
            ++submitted_batches;
            p=pb_begin();
            p=pb_push2(p,NV097_SET_TEXTURE_OFFSET,(uint32_t)entry->pixels&0x03ffffff,entry->format);
            p=pb_push1(p,NV097_SET_TEXTURE_CONTROL1,entry->pitch<<16);
            p=pb_push1(p,NV097_SET_TRANSFORM_CONSTANT_LOAD,103);
            p=pb_push4f(p,NV097_SET_TRANSFORM_CONSTANT,
                entry->linear?entry->width:1,texture_v_scale(entry),0,0);
            p=pb_push1(p,NV097_SET_TEXTURE_IMAGE_RECT,(entry->width<<16)|entry->height);
            p=pb_push1(p,NV097_SET_TEXTURE_ADDRESS,0x00010101);
            p=pb_push1(p,NV097_SET_TEXTURE_CONTROL0,0x4003ffc0);
            p=pb_push1(p,NV097_SET_TEXTURE_FILTER,0x02022000);
            pb_end(p);
            /* Offset each draw's base, avoiding the hardware's 16-bit index limit. */
            for (unsigned first=0;first<batch->vertexCount;) {
                unsigned count=batch->vertexCount-first; if(count>252)count=252;
                if(!jpb_XboxLevelChunkVisible(i,first)) {
                    first+=count;
                    continue;
                }
                const uint16_t *indices=jpb_XboxLevelIndices(i);
                if(jpb_XboxLevelChunkNeedsDepthClip(i,first)) {
                    draw_clipped_level(batch,indices,first,count,view);
                    first+=count;
                    continue;
                }
                submitted_vertices+=count;
                unsigned stride=packed?32:44;
                uintptr_t base=(uintptr_t)batch->vertices+(indices?0:first)*stride;
                static const unsigned attributes[4]={0,3,9,10};
                static const unsigned sizes[4]={3,4,2,2};
                static const unsigned offsets[4]={0,20,12,36};
                p=pb_begin();
                for(unsigned a=0;a<4;++a) {
                    unsigned format_type=packed && a==1
                        ?NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE_UB_OGL
                        :NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE_F;
                    unsigned offset=packed && a==3?24:offsets[a];
                    p=pb_push1(p,NV097_SET_VERTEX_DATA_ARRAY_FORMAT+attributes[a]*4,
                        format_type | (sizes[a]<<4) | (stride<<8));
                    p=pb_push1(p,NV097_SET_VERTEX_DATA_ARRAY_OFFSET+attributes[a]*4,(base+offset)&0x03ffffff);
                }
                pb_end(p);
                /* pbkit limits each begin/end block to 128 DWORDs. Keep
                   vertex bindings separate and index draws at <=240 indices
                   (125 DWORDs including primitive begin/end). */
                if(indices) {
                    for(unsigned start=0;start<count;) {
                        unsigned n=count-start; if(n>240)n=240;
                        unsigned j=0;
                        p=pb_begin();
                        p=pb_push1(p,NV097_SET_BEGIN_END,NV097_SET_BEGIN_END_OP_TRIANGLES);
                        if(n>=2) {
                            pb_push(p++,0x40000000|NV097_ARRAY_ELEMENT16,n/2);
                            for(;j+1<n;j+=2)
                                *p++=indices[first+start+j]|((uint32_t)indices[first+start+j+1]<<16);
                        }
                        if(j<n)p=pb_push1(p,NV097_ARRAY_ELEMENT32,indices[first+start+j]);
                        p=pb_push1(p,NV097_SET_BEGIN_END,NV097_SET_BEGIN_END_OP_END);
                        pb_end(p);
                        start+=n;
                    }
                } else {
                    p=pb_begin();
                    p=pb_push1(p,NV097_SET_BEGIN_END,NV097_SET_BEGIN_END_OP_TRIANGLES);
                    p=pb_push1(p,0x40000000|NV097_DRAW_ARRAYS,(count-1)<<24);
                    p=pb_push1(p,NV097_SET_BEGIN_END,NV097_SET_BEGIN_END_OP_END);
                    pb_end(p);
                }
                first+=count;
            }
        }
        jpb_XboxGpuStage=59;
        return JPB_SOFTWARE_RENDER_OK;
    }
    return jpb_SoftwareRenderJpxMaterializedToSink(world, view, fb, clear, resolver,
        textures_user, depth, jpb_XboxGpuTriangle, NULL, stats);
}

/* Match PC PSGameplayComposite exactly: black + world*(white-black).
   Two fixed-function blend passes avoid a world framebuffer readback. */
int jpb_XboxGpuComposite(void *user, enum JPBGameRuntimeGameplayCompositeStage stage,
    const JPBSoftwareFramebuffer *fb, JPBSoftwareRenderStats *stats)
{
    flush_model_batch();
    commit_model_commands();
    jpb_XboxGpuStage=70+(unsigned)stage;
    static uint32_t *black, *gain;
    static uint32_t *black_cpu;
    static int canvas_width,canvas_height;
    static unsigned canvas_pitch;
    VIDEO_MODE output_mode=XVideoGetMode();
    (void)user; (void)stats;
    if (!fb || fb->width<=0 || fb->height<=0 ||
        fb->width>1024 || fb->height>768 ||
        output_mode.width<=0 || output_mode.height<=0)return 0;
    if(canvas_width!=fb->width || canvas_height!=fb->height){
        while(pb_busy()){}
        if(black)MmFreeContiguousMemory(black);
        if(gain)MmFreeContiguousMemory(gain);
        free(black_cpu);
        black=gain=black_cpu=NULL;
        canvas_width=fb->width;
        canvas_height=fb->height;
        canvas_pitch=((unsigned)fb->width*4+63u)&~63u;
        black_cpu=malloc((size_t)fb->width*fb->height*sizeof(uint32_t));
        black=MmAllocateContiguousMemoryEx(canvas_pitch*fb->height,0,
            0x03ffafff,0,PAGE_READWRITE|PAGE_WRITECOMBINE);
        gain=MmAllocateContiguousMemoryEx(canvas_pitch*fb->height,0,
            0x03ffafff,0,PAGE_READWRITE|PAGE_WRITECOMBINE);
    }
    if (!black || !gain || !black_cpu) return 0;
    if (stage!=JPB_GAMEPLAY_COMPOSITE_FINISH) {
        jpb_XboxGpuStage=80+(unsigned)stage;
        while(pb_busy()) {}
        jpb_XboxGpuStage=85+(unsigned)stage;
        for (unsigned y=0;y<(unsigned)fb->height;++y)
            for(unsigned x=0;x<(unsigned)fb->width;++x) {
            unsigned offset=y*(unsigned)fb->width+x;
            unsigned gpu_offset=y*(canvas_pitch/4)+x;
            uint32_t color=fb->pixels[y*fb->stridePixels+x];
            if(stage==JPB_GAMEPLAY_COMPOSITE_HUD_BLACK) {
                black_cpu[offset]=color;
                black[gpu_offset]=color|0xff000000;
            }
            else {
                uint32_t value=0xff000000;
                for(unsigned shift=0;shift<24;shift+=8) {
                    int difference=((color>>shift)&255)-((black_cpu[offset]>>shift)&255);
                    value|=(unsigned)(difference>0?difference:0)<<shift;
                }
                gain[gpu_offset]=value;
            }
        }
        return 1;
    }
    uint32_t *p=pb_begin();
    p=pb_push1(p,NV097_SET_TRANSFORM_PROGRAM_START,0);
    p=pb_push1(p,NV097_SET_DEPTH_TEST_ENABLE,0);
    p=pb_push1(p,NV097_SET_DEPTH_MASK,0);
    p=pb_push1(p,NV097_SET_ALPHA_TEST_ENABLE,0);
    p=pb_push1(p,NV097_SET_BLEND_ENABLE,1);
    p=pb_push1(p,NV097_SET_TEXTURE_ADDRESS,0x00050505);
    p=pb_push1(p,NV097_SET_TEXTURE_CONTROL0,0x4003ffc0);
    p=pb_push1(p,NV097_SET_TEXTURE_FILTER,0x02022000);
    p=pb_push1(p,NV097_SET_TEXTURE_CONTROL1,canvas_pitch<<16);
    p=pb_push1(p,NV097_SET_TEXTURE_IMAGE_RECT,
        ((unsigned)fb->width<<16)|(unsigned)fb->height);
    pb_end(p);
    for(unsigned pass=0;pass<2;++pass) {
        p=pb_begin();
        p=pb_push2(p,NV097_SET_TEXTURE_OFFSET,(uint32_t)(pass?black:gain)&0x03ffffff,
            1|(2<<4)|(NV097_SET_TEXTURE_FORMAT_COLOR_LU_IMAGE_A8R8G8B8<<8)|(1<<16));
        p=pb_push1(p,NV097_SET_BLEND_FUNC_SFACTOR,pass?NV097_SET_BLEND_FUNC_SFACTOR_V_ONE:NV097_SET_BLEND_FUNC_SFACTOR_V_ZERO);
        p=pb_push1(p,NV097_SET_BLEND_FUNC_DFACTOR,pass?NV097_SET_BLEND_FUNC_DFACTOR_V_ONE:NV097_SET_BLEND_FUNC_DFACTOR_V_SRC_COLOR);
        p=pb_push1(p,NV097_SET_BEGIN_END,NV097_SET_BEGIN_END_OP_TRIANGLE_STRIP);
        for(unsigned i=0;i<4;++i) {
            p=pb_push4f(p,NV097_SET_VERTEX_DATA4F_M+3*16,1,1,1,1);
            p=pb_push4f(p,NV097_SET_VERTEX_DATA4F_M+9*16,
                (i&1)?(float)fb->width:0,
                (i&2)?(float)fb->height:0,0,1);
            p=pb_push4f(p,NV097_SET_VERTEX_DATA4F_M,
                (i&1)?(float)output_mode.width:0,
                (i&2)?(float)output_mode.height:0,0,1);
        }
        p=pb_push1(p,NV097_SET_BEGIN_END,NV097_SET_BEGIN_END_OP_END);
        pb_end(p);
    }
    return 1;
}

/* Draw HUD source textures at the active output viewport. The software
   composite still handles text; card coordinates come from the logical
   canvas, which can be widened independently for a future 16:9 mode. */
int jpb_XboxGpuHudScreenDraws(void *user,
    const JPBGameRuntimeScreenDraw *draws, size_t draw_count,
    JPBSoftwareFramebuffer *framebuffer)
{
    size_t order[JPB_GAME_RUNTIME_SCREEN_DRAW_CAPACITY];
    VIDEO_MODE output_mode=XVideoGetMode();
    (void)user;
    if(draw_count>JPB_GAME_RUNTIME_SCREEN_DRAW_CAPACITY ||
       framebuffer==NULL || framebuffer->width<=0 ||
       framebuffer->height<=0 || output_mode.width<=0 ||
       output_mode.height<=0)return 0;
    flush_model_batch();
    commit_model_commands();
    model_pass=0;
    triangle_state_texture=NULL;
    uint32_t *p=pb_begin();
    p=pb_push1(p,NV097_SET_DEPTH_TEST_ENABLE,0);
    p=pb_push1(p,NV097_SET_DEPTH_MASK,0);
    p=pb_push1(p,NV097_SET_BLEND_ENABLE,1);
    pb_end(p);
    for(size_t i=0;i<draw_count;++i){
        size_t at=i;
        order[at]=i;
        while(at>0 && draws[order[at-1]].layerDepth<
            draws[order[at]].layerDepth){
            size_t swap=order[at-1];
            order[at-1]=order[at];
            order[at]=swap;
            --at;
        }
    }
    for(size_t item=0;item<draw_count;++item){
        const JPBGameRuntimeScreenDraw *draw=&draws[order[item]];
        JPBSoftwareTexture white_texture={0};
        const JPBSoftwareTexture *source=NULL;
        JPBSoftwareTexture texture;
        float left=(float)draw->destination.left;
        float top=(float)draw->destination.top;
        float right=(float)draw->destination.right;
        float bottom=(float)draw->destination.bottom;
        if(right<=left || bottom<=top)continue;
        if(draw->texture && draw->texture->texture)
            source=(const JPBSoftwareTexture *)draw->texture->texture;
        if(!source){
            white_texture.pixels=&white;
            white_texture.width=white_texture.height=
                white_texture.stridePixels=1;
            white_texture.samplerType=TEXTURESAMPLER_LINEARCLAMP;
            source=&white_texture;
        }
        if(!source->pixels || !source->width || !source->height)continue;
        texture=*source;
        texture.materialType=2;
        if(left<0)left=0;
        if(top<0)top=0;
        if(right>framebuffer->width)right=framebuffer->width;
        if(bottom>framebuffer->height)bottom=framebuffer->height;
        if(draw->hasScissor){
            if(left<draw->scissor.left)left=draw->scissor.left;
            if(top<draw->scissor.top)top=draw->scissor.top;
            if(right>draw->scissor.right)right=draw->scissor.right;
            if(bottom>draw->scissor.bottom)bottom=draw->scissor.bottom;
        }
        if(left>=right || top>=bottom)continue;
        float src_left=draw->hasSource?draw->source.left:0;
        float src_top=draw->hasSource?draw->source.top:0;
        float src_right=draw->hasSource?draw->source.right:(float)source->width;
        float src_bottom=draw->hasSource?draw->source.bottom:(float)source->height;
        float width=(float)(draw->destination.right-draw->destination.left);
        float height=(float)(draw->destination.bottom-draw->destination.top);
        float u0=(src_left+(src_right-src_left)*
            (left-draw->destination.left)/width)/(float)source->width;
        float u1=(src_left+(src_right-src_left)*
            (right-draw->destination.left)/width)/(float)source->width;
        float v0=(src_top+(src_bottom-src_top)*
            (top-draw->destination.top)/height)/(float)source->height;
        float v1=(src_top+(src_bottom-src_top)*
            (bottom-draw->destination.top)/height)/(float)source->height;
        float red=draw->color.r,green=draw->color.g,
            blue=draw->color.b,alpha=draw->color.cd;
        if(draw->texture && (!red || !green || !blue || !alpha))
            red=green=blue=alpha=255;
        JPBSoftwareMaterialVertex vertices[4]={0};
        vertices[0].x=vertices[2].x=left;
        vertices[1].x=vertices[3].x=right;
        vertices[0].y=vertices[1].y=top;
        vertices[2].y=vertices[3].y=bottom;
        vertices[0].u=vertices[2].u=u0;
        vertices[1].u=vertices[3].u=u1;
        vertices[0].v=vertices[1].v=v0;
        vertices[2].v=vertices[3].v=v1;
        for(unsigned i=0;i<4;++i){
            vertices[i].red=red;
            vertices[i].green=green;
            vertices[i].blue=blue;
            vertices[i].alpha=alpha;
        }
        if(!jpb_XboxGpuTriangle(NULL,&vertices[0],&vertices[1],
            &vertices[2],&texture) ||
           !jpb_XboxGpuTriangle(NULL,&vertices[2],&vertices[1],
            &vertices[3],&texture)){
            debugPrint("HUD quad failed %s %ux%u\n",
                draw->texture?draw->texture->filename:"solid",
                (unsigned)source->width,(unsigned)source->height);
            return 0;
        }
    }
    return 1;
}

typedef struct XboxHudTextContext {
    int viewport_width;
    int viewport_height;
    float horizontal_scale;
} XboxHudTextContext;

static int xbox_hud_glyph(void *user, const JPBSoftwareTexture *texture,
    float left, float top, float right, float bottom,
    float u0, float v0, float u1, float v1, uint32_t color)
{
    XboxHudTextContext *context=(XboxHudTextContext *)user;
    JPBSoftwareMaterialVertex vertices[4]={0};
    JPBSoftwareTexture material=*texture;
    left*=context->horizontal_scale;
    right*=context->horizontal_scale;
    /* SDL_ttf has already rasterized antialiased coverage at the output
       pixel size. Bilinear sampling a second time softens every edge. */
    material.samplerType=TEXTURSAMPLER_POINTCLAMP;
    if(left<0){u0+=(u1-u0)*(-left)/(right-left);left=0;}
    if(top<0){v0+=(v1-v0)*(-top)/(bottom-top);top=0;}
    if(right>context->viewport_width){
        u1-=(u1-u0)*(right-context->viewport_width)/(right-left);
        right=(float)context->viewport_width;
    }
    if(bottom>context->viewport_height){
        v1-=(v1-v0)*(bottom-context->viewport_height)/(bottom-top);
        bottom=(float)context->viewport_height;
    }
    if(left>=right || top>=bottom)return 1;
    material.materialType=2;
    vertices[0].x=vertices[2].x=left/output_x_per_canvas;
    vertices[1].x=vertices[3].x=right/output_x_per_canvas;
    vertices[0].y=vertices[1].y=top/output_y_per_canvas;
    vertices[2].y=vertices[3].y=bottom/output_y_per_canvas;
    vertices[0].u=vertices[2].u=u0;
    vertices[1].u=vertices[3].u=u1;
    vertices[0].v=vertices[1].v=v0;
    vertices[2].v=vertices[3].v=v1;
    for(unsigned i=0;i<4;++i){
        vertices[i].red=color&255u;
        vertices[i].green=(color>>8)&255u;
        vertices[i].blue=(color>>16)&255u;
        vertices[i].alpha=(color>>24)&255u;
    }
    return jpb_XboxGpuTriangle(NULL,&vertices[0],&vertices[1],
        &vertices[2],&material) &&
        jpb_XboxGpuTriangle(NULL,&vertices[2],&vertices[1],
        &vertices[3],&material);
}

int jpb_XboxGpuHudTextDraws(void *user,
    const JPBGameRuntimeTextDraw *draws, size_t draw_count,
    JPBSoftwareFramebuffer *framebuffer)
{
    VIDEO_MODE output_mode=XVideoGetMode();
    XboxHudTextContext context;
    (void)user;
    if(!framebuffer || framebuffer->width<=0 || framebuffer->height<=0 ||
       output_mode.width<=0 || output_mode.height<=0 ||
       draw_count>JPB_GAME_RUNTIME_TEXT_DRAW_CAPACITY)return 0;
    context.viewport_width=output_mode.width;
    context.viewport_height=output_mode.height;
    /* 720x480 widescreen uses non-square display pixels. Glyph bitmaps are
       rasterized at the vertical output density, then narrowed in raw pixels
       so they regain square proportions when displayed as 16:9. At 720p the
       ratio is one and no correction is applied. */
    context.horizontal_scale=
        ((float)output_mode.width/(float)output_mode.height)/(16.0f/9.0f);
    float sx=(float)output_mode.width/(float)framebuffer->width;
    float sy=(float)output_mode.height/(float)framebuffer->height;
    triangle_state_texture=NULL;
    model_pass=0;
    uint32_t *p=pb_begin();
    p=pb_push1(p,NV097_SET_DEPTH_TEST_ENABLE,0);
    p=pb_push1(p,NV097_SET_DEPTH_MASK,0);
    p=pb_push1(p,NV097_SET_BLEND_ENABLE,1);
    pb_end(p);
    for(size_t i=0;i<draw_count;++i){
        const JPBGameRuntimeTextDraw *draw=&draws[i];
        int size=(int)lroundf((float)draw->pointSize*sy);
        if(size<1)size=1;
        if(!jpb_PortableTextEmitUiPointSize(draw->text,draw->color,
            draw->mode,(int)lroundf(draw->x*sx/context.horizontal_scale),
            (int)lroundf(draw->y*sy),size,draw->fontStyle,
            OptionStruct.Language,draw->clipEnabled,
            (int)lroundf(draw->clipLeft*sx/context.horizontal_scale),
            (int)lroundf(draw->clipTop*sy),
            (int)lroundf(draw->clipRight*sx/context.horizontal_scale),
            (int)lroundf(draw->clipBottom*sy),
            xbox_hud_glyph,&context))return 0;
    }
    return 1;
}
