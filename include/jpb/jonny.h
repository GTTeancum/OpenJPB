#ifndef JPB_JONNY_H
#define JPB_JONNY_H

#include "jpb/fmath.h"
#include "jpb/material.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

extern char *jonnylevel;
extern int32_t *leveldata;
extern _Material *leveltexture[4];
extern int32_t *texturebase;
extern int32_t *colorbase;
extern int32_t *vertbase;
extern int32_t mapyend;
extern uint8_t gaScratch[2048];
extern int32_t *eventlist_start;
extern int32_t *eventlist_next;
extern int32_t *eventlist_end;

/* Portable archive-lifetime guard for persistent collision contacts. */
int jpb_LevelDataContains(const void *data, size_t size);
void jpb_LevelDataClearBounds(void);

/* Three-byte return stubs in the matched retail jonny.obj. */
void calc_frustrum(MATRIX *matrix, int distance);
void band_lights(void);
void contraband_cluts(void *cluts);
void contraband_lights(void *mapbase);
void jon_texscroll(void *mapbase, int frame_step);
void set_camera(MATRIX *matrix);

struct _jheightstuff;

int BlockBuster(
    int32_t *mapbase,
    int hitforce,
    int cubeshite);
int ExtraCharacterEnvironmentEffectExceptions(void);
int HitsHit(
    int32_t *mapbase,
    int32_t *entry,
    int hitforce,
    int cubeshite,
    int32_t *coords);
void MTV(int cubeshite);
void clear_eventlist(void);
void restore_events(int32_t *mapbase);
int32_t *jon_getlibpart(
    int32_t *cube,
    VECTOR *cubeorg,
    int32_t *mapbase);
int32_t *jon_getlibpartfloat(
    FVECTOR *output,
    int32_t *cube,
    FVECTOR *cubeorg,
    int32_t *mapbase,
    int32_t *numverts);
int32_t *jon_getlibpartint32_t(
    int32_t *cube,
    VECTOR *cubeorg,
    int32_t *mapbase);
int jon_otagpos(int x, int y, int z);
int jon_plumbgeneral(
    _svector *verts,
    _svector *norms,
    unsigned *index,
    int npolys,
    VECTOR *pos);
void *jpb_render(
    MATRIX *cammat,
    void *mapbase,
    void *pbuf,
    int globaltimer,
    void *otagbase,
    int showmask,
    int minx,
    int maxx,
    int miny,
    int maxy,
    int subdisable);
void makecull(
    FVECTOR4 *planes,
    MATRIX *camera,
    FVECTOR *camera_position,
    float clip_radius,
    float screen_width,
    float screen_height,
    float screen_distance,
    float far_clip);
/*
 * Inferred test/integration facade for the exact file-local
 * intersec_WankCheck procedure.
 */
int jpb_JonnyWankCheck(
    _svector *verts,
    int32_t **polyptr,
    VECTOR *pos);
int jon_plumbline(
    void *mapbase,
    void *pointspace,
    VECTOR *pos,
    int minheight,
    struct _jheightstuff *results);
void spack_frustrum(
    MATRIX *cammat,
    _svector *fuckoff,
    VECTOR *camtwat);
void spackdivver_frustrum(
    MATRIX *cammat,
    _svector *fuckoff,
    VECTOR *camtwat);

typedef void (*JPBJonnyPostLoadHook)(void);

void file_SetJonnyPostLoadHooks(
    JPBJonnyPostLoadHook clear_events,
    JPBJonnyPostLoadHook initialize_uvs);

#ifdef __cplusplus
}
#endif

#endif
