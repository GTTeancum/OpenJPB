/* Offline Blender bridge. Coordinates and topology use the runtime readers;
 * this process never initializes a window or executes gameplay scripts. */
#include "pc_level_fbx.h"
#include "jpb/filesys.h"
#include "jpb/globalarrays.h"
#include "jpb/memory.h"
#include "jpb/world.h"
#include "jpb/jonny.h"
#include "jpb/level_world.h"
#include "jpb/loader.h"
#include "jpb/pwrup.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static FILE *out;
static void string(const char *s, size_t max)
{
    size_t i; fputc('"',out);
    for(i=0;s && i<max && s[i];++i) {
        unsigned char c=(unsigned char)s[i];
        if(c=='"'||c=='\\') fprintf(out,"\\%c",c);
        else if(c<32||c>=127) fprintf(out,"\\u%04x",c);
        else fputc(c,out);
    }
    fputc('"',out);
}
static void vector(rdVECTOR v) { fprintf(out,"[%d,%d,%d]",v.vx,v.vy,v.vz); }
static void collision_face(FVECTOR *v,int n,int *first,ptrdiff_t offset,uint32_t flags,int camera,int trigger)
{
    int i; if(!*first) fputc(',',out); *first=0;
    fprintf(out,"{\"offset\":%td,\"flags\":%u,\"camera\":%d,\"trigger\":%d,\"vertices\":[",offset,flags,camera,trigger);
    for(i=0;i<n;i++) fprintf(out,"%s[%.9g,%.9g,%.9g]",i?",":"",v[i].vx,v[i].vy,v[i].vz);
    fputs("]}",out);
}
/* Same cell/library traversal and polygon order as intersec_RayCast. */
static int collision(void)
{
    int x,z,first=1; unsigned char *fat_seen;
    if(!leveldata) return 1;
    if(mapyend<0||mapyend>4096||!jpb_LevelDataContains(leveldata,(size_t)mapyend*1024)) {
        fprintf(stderr,"Invalid collision grid: %d rows\n",mapyend);return 0;
    }
    fat_seen=(unsigned char*)calloc(256,1); if(!fat_seen)return 0;
    for(z=0;z<mapyend;z++) for(x=0;x<256;x++) {
        int32_t cell=leveldata[x+z*256],*cube; int chain=0;
        if(cell>=0)continue;
        cube=leveldata+((uint32_t)cell&0x1ffff);
        for(;;) {
            uint32_t c; int32_t *next,*entry;
            FVECTOR origin; if(++chain>256||!jpb_LevelDataContains(cube,4))goto bad;
            c=(uint32_t)*cube; next=cube+1+((c>>26)&15);
            if(!jpb_LevelDataContains(cube,(size_t)(next-cube)*4))goto bad;
            origin.vx=(float)(0x8000-x*256); origin.vy=(float)((c&127)*256); origin.vz=(float)(z*256-0x7f00);
            if(next==cube+1) {
                unsigned id=(c>>14)&255; int32_t *fat=leveldata+(leveldata[-4]>>11)+id*9;
                if(!jpb_LevelDataContains(fat,36))goto bad;
                if(!fat_seen[id] && !((uint32_t)fat[0]&0x40000000)) {
                    FVECTOR v[4]; int i;
                    for(i=0;i<4;i++) {
                        uint16_t a; int16_t b,y;
                        memcpy(&a,(char*)fat+20+i*4,2); memcpy(&b,(char*)fat+22+i*4,2);
                        memcpy(&y,(char*)fat+12+i*2,2);
                        /* jon_plumbline's signed 16-bit XYZ decoding. The
                         * raycast reconstruction has a different scratch-axis
                         * representation; do not interpret its packed words
                         * as authoring-space heights. */
                        v[i].vx=(float)(int16_t)(0x8100-a);
                        v[i].vy=(float)y;v[i].vz=(float)(int16_t)(b-0x7f00);
                    }
                    collision_face(v,4,&first,fat-leveldata,(uint32_t)fat[0],((unsigned char*)fat)[7]&127,((uint32_t)fat[1]>>8)&255);
                    fat_seen[id]=1;
                }
            } else for(entry=cube+2;entry<next;entry+=((uint32_t)*entry>>30)+1) {
                int32_t *lib=leveldata+(uint16_t)*entry,*poly; int count,steps=0;
                FVECTOR points[40],v[4]; const uint8_t *packed;
                if(!jpb_LevelDataContains(lib,8))goto bad;
                packed=(const uint8_t*)leveldata+(((uint32_t)lib[0]&65535)+(uint32_t)((leveldata[-1]>>2)*2))*2U;
                if(!jpb_LevelDataContains(packed,(((uint32_t)lib[0]>>16)&31)*2))goto bad;
                jon_getlibpartfloat(points,entry,&origin,leveldata,&count);
                for(poly=lib+2;;poly+=2) {
                    uint32_t a,b; int ids[4],n,i;
                    if(++steps>256||!jpb_LevelDataContains(poly,8))goto bad;
                    a=(uint32_t)poly[0];b=(uint32_t)poly[1]; n=4-((b>>20)&1);
                    ids[0]=b&31;ids[1]=(b>>5)&31;ids[2]=(b>>10)&31;ids[3]=(b>>15)&31;
                    if(n==4){int tmp=ids[2];ids[2]=ids[3];ids[3]=tmp;}
                    if(!(b&0xc0000000)) {
                        for(i=0;i<n;i++){if(ids[i]>=count)goto bad;v[i]=points[ids[i]];}
                        collision_face(v,n,&first,poly-leveldata,b,((unsigned char*)cube)[7]&127,((uint32_t)cube[1]>>8)&255);
                    }
                    if(a&0xc0000000)break;
                }
            }
            if(c&0x40000000)break; cube=next;
        }
    }
    free(fat_seen); return 1;
bad: free(fat_seen); fputs("Invalid collision record\n",stderr);return 0;
}
int main(int argc,char **argv)
{
    JPBPcFbxLevel fbx={0}; WorldData world={0}; char error[512];
    JPBSoftwareOwnedLevelMesh jpx_mesh={0};uint8_t *jpx_storage=NULL;
    const JPBSoftwareLevelMesh *visual=&fbx.mesh;
    int index,i,j,size=0; void *archive=NULL; uint8_t *cursor,*original=NULL;
    if(argc!=5){fputs("usage: jpb_level_import visual.fbx|- gameplay.j3d|- level-index output.json\n",stderr);return 2;}
    index=atoi(argv[3]);
    if(index<0)index=jpb_LevelIndexFromPath(strcmp(argv[1],"-")?argv[1]:argv[2]);
    if(index<0||index>=JPB_LEVEL_COUNT){fputs("Unknown level: supply the source level index\n",stderr);return 2;}
    if(strcmp(argv[1],"-")) {
        const char *ext=strrchr(argv[1],'.');
        if(ext && !_stricmp(ext,".jpx")) {
            JPBJpxLoadConfig config={0};JPBJpxView view;JPBSoftwareJpxScene scene;
            jpx_storage=(uint8_t*)malloc(JPB_JPX_REFERENCE_WORLD_CAPACITY);
            config.storage=jpx_storage;config.storageCapacity=JPB_JPX_REFERENCE_WORLD_CAPACITY;config.levelName=sLevelNames[index];
            if(!jpx_storage||jpx_LoadFile(argv[1],&config,&view)!=JPB_JPX_OK||
               jpb_SoftwarePrepareJpxLevelScene(&view,index,&scene)!=JPB_SOFTWARE_RENDER_OK||
               jpb_SoftwareBuildJpxLevelMesh(&scene,&jpx_mesh)!=JPB_SOFTWARE_RENDER_OK)return 3;
            visual=&jpx_mesh.mesh;
        } else if(!jpb_PCLoadFbxLevel(argv[1],index,&fbx,error,sizeof(error))){fprintf(stderr,"%s\n",error);return 3;}
    }
    gpWorld=&world;pointerRegistry_Reset();memory_InitMemorySystem();
    if(strcmp(argv[2],"-")) {
        archive=file_LoadFile2PoolFunc(argv[2],&size,MEMORY_POOL_ANY,__LINE__,__FILE__);
        if(!archive||size<=0)return 4;
        original=(uint8_t*)malloc((size_t)size);if(!original)return 4;
        memcpy(original,archive,(size_t)size);
        if(file_RelocateChunks(archive,(size_t)size,&cursor)!=JPB_CHUNKS_OK)return 4;
    }
    out=fopen(argv[4],"wb");if(!out)return 5;
    fprintf(out,"{\"version\":1,\"level_index\":%d,\"actor_models\":{",index);
    for(i=0;i<JPB_ACTOR_NAME_COUNT;i++){if(i)fputc(',',out);string(sObiNames[i],256);fputc(':',out);string(sModelNames[model_anim_table[i].modelID],256);}
    fputs("},\"level_name\":",out);string(sLevelNames[index],256);
    fputs(",\"pickup_models\":[",out);
    for(i=0;i<17;i++){if(i)fputc(',',out);string(powerUpFiles[i],256);}
    fputs("],\"pickup_scales\":[",out);
    for(i=0;i<17;i++)fprintf(out,"%s%d",i?",":"",powerUpScales[i]);
    fputs("],\"meshes\":[",out);
    for(i=0;i<(int)visual->batchCount;i++) {
        const JPBSoftwareLevelBatch *b=&visual->batches[i];size_t k;
        fprintf(out,"%s{\"name\":",i?",":"");string(b->meshName,128);
        fputs(",\"texture\":",out);string(b->textureName,256);
        fprintf(out,",\"pass\":%d,\"mesh_index\":%zu,\"vertices\":[",b->pass,b->meshIndex);
        for(k=0;k<b->vertexCount;k++) {
            const JPBSoftwareLevelVertex *v=&b->vertices[k];
            fprintf(out,"%s[%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g]",k?",":"",v->position.vx,v->position.vy,v->position.vz,v->u,v->v,v->red,v->green,v->blue,v->alpha);
        }
        fputs("]}",out);
    }
    fputs("],\"collision\":[",out);if(!collision()){fclose(out);remove(argv[4]);return 6;}
    fputs("],\"world_start\":",out);vector(world.start);
    fputs(",\"player_starts\":[",out);
    for(i=0;i<2;i++){rdVECTOR p={(128-startPos[index][i].vx)*256,startPos[index][i].vz*256,(startPos[index][i].vy-127)*256};if(i)fputc(',',out);vector(p);}
    fputc(']',out);
    fputs(",\"placements\":[",out);
    for(i=0;i<world.nEnemy;i++) {
        wsl_BAP_PLACEMENT *p=world.apEnemy[i];
        const wsl_BAP_PLACEMENT *raw=(const wsl_BAP_PLACEMENT*)(original+((uint8_t*)p-(uint8_t*)archive));
        fprintf(out,"%s{\"id\":%d,\"name\":",i?",":"",i);string(p->aName,12);
        fprintf(out,",\"actor_id\":%d,\"ai_id\":%d,\"enemy_id\":%d,\"active_flags\":%u,\"position\":",p->actorNum,p->aiNum,p->enemyID,p->aiDf.activeFlags);vector(p->loc);
        fputs(",\"authored_position\":",out);vector(raw->loc);
        fputs(",\"defaults_hex\":\"",out);for(j=0;j<sizeof(raw->aiDf);j++)fprintf(out,"%02x",((const unsigned char*)&raw->aiDf)[j]);
        fputs("\",\"links\":[",out);for(j=0;j<p->nLink&&j<8;j++)fprintf(out,"%s%u",j?",":"",p->links[j]);
        fputs("],\"waypoints\":[",out);for(j=0;j<p->nWaypnt;j++){fprintf(out,"%s{\"flags\":%d,\"position\":",j?",":"",p->wayPoints[j].flags);vector(p->wayPoints[j].loc);fputc('}',out);}
        fputs("]}",out);
    }
    fputs("],\"actors\":[",out);for(i=0;i<world.nActor;i++){if(i)fputc(',',out);string(world.apActorNames[i],256);}
    fputs("],\"powerups\":[",out);
    for(i=0;i<world.nPowerups;i++){wsl_Powerup *p=&world.pPowerups[i];fprintf(out,"%s{\"type\":%u,\"rate\":%u,\"data\":%u,\"position\":[%d,%d,%d]}",i?",":"",p->type,p->rate,p->data,p->pos.vx,p->pos.vy,p->pos.vz);}
    fputs("],\"cameras\":[",out);
    if(world.pObiDolly)for(i=0;i<32;i++){BAP_CAMERADOLLY *c=&world.pObiDolly[i];fprintf(out,"%s{\"id\":%d,\"flags\":%u,\"pitch\":%d,\"yaw\":%d,\"slack\":[%d,%d,%d],\"off\":[%d,%d,%d],\"offset\":",i?",":"",i,c->flags,c->pitch,c->yaw,c->slackx,c->slacky,c->slackz,c->offx,c->offy,c->offz);vector(c->offset);fputc('}',out);}
    fputs("],\"scripts\":[",out);
    for(i=0;i<world.nAI;i++) {
        BAP_AI *a=world.apAI[i];int n=a->numNodes-a->numAvailable;
        int vars=(a->bSize-(int)offsetof(BAP_AI,aiNodes)-n*(int)sizeof(BAP_AINODE))/4;
        UDATA *values=getPtr(a->pVars,JPB_POINTER_ARRAY_AI);
        fprintf(out,"%s{\"id\":%d,\"nodes\":[",i?",":"",i);
        for(j=0;j<n;j++){BAP_AINODE *v=&a->aiNodes[j];fprintf(out,"%s[%d,%d,%d,%d,%u]",j?",":"",v->iParent,v->iChild,v->iSibling,v->opcode,v->vx.ui);}
        fputs("],\"variables\":[",out);for(j=0;values&&j<vars;j++)fprintf(out,"%s%u",j?",":"",values[j].ui);fputs("]}",out);
    }
    fputs("],\"animation_map\":[",out);
    for(i=0;i<world.nAnimMap;i++)fprintf(out,"%s%d",i?",":"",world.animMapEnemies[i]);
    fputs("],\"animations\":[",out);
    for(i=0;i<world.nADef;i++) {
        wsl_BT_ANIMDEF *a=world.animDef[i];
        fprintf(out,"%s{\"id\":%d,\"type\":%d,\"number\":%d,\"flags\":%d,\"fps\":%d,\"frames\":%d,\"nodes\":[",i?",":"",i,a->type,a->num,a->flags,a->fps,a->numFrames);
        for(j=0;j<a->totalNodes;j++) {
            wsl_BT_ANIMNODE *n=&a->aNodes[j];int k;
            fprintf(out,"%s{\"id\":%d,\"number\":%d,\"used\":%d,\"flags\":%d,\"level\":%d,\"parent\":%d,\"child\":%d,\"sibling\":%d,\"next\":%d,\"speed\":%d,\"keys\":[",j?",":"",j,n->num,n->used,n->flags,n->level,n->iParent,n->iChild,n->iSibling,n->iNext,n->nodeSpeed);
            for(k=0;k<n->numEntries&&k<8;k++) {
                wsl_BT_ANIMENTRY *e=&n->aEntry[k];
                fprintf(out,"%s{\"frame\":%d,\"number\":%d,\"flags\":%d,\"position\":",k?",":"",e->frame,e->num,e->flags);vector(e->xyz);
                fputs(",\"rotation\":",out);vector(e->pyr);fputs(",\"scale\":",out);vector(e->scale);fputc('}',out);
            }
            fputs("]}",out);
        }
        fputs("]}",out);
    }
    fputs("],\"library_tags\":[",out);
    for(i=0;i<world.numTags;i++) {
        wsl_libTags *t=&world.pLibTags[i];
        fprintf(out,"%s{\"id\":%d,\"gmi1\":%u,\"flip\":%u,\"flags\":%u,\"alpha\":%u,\"back_side\":%u,\"event\":%u,\"z_adjust\":%d,\"bgi1\":%u,\"bgi2\":%u,\"x_light\":%d}",i?",":"",i,t->gmi1,t->flip,t->flags,t->alpha,t->bSide,t->event,t->zAdjust,t->bgi1,t->bgi2,t->xLight);
    }
    fprintf(out,"],\"animation_definition_count\":%d}",world.nADef);
    i=ferror(out);if(fclose(out))i=1;jpb_PCFreeFbxLevel(&fbx);
    jpb_SoftwareFreeOwnedLevelMesh(&jpx_mesh);free(jpx_storage);free(original);return i?7:0;
}
