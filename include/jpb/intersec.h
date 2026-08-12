#ifndef JPB_INTERSEC_H
#define JPB_INTERSEC_H

#include "jpb/fmath.h"
#include "jpb/objroot.h"
#include "jpb/player.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _solid _solid;
typedef struct playerObject playerObject;

/* Exact matched-PC PDB enum type 0x1415. */
typedef enum _hit_amount {
    NO_HIT = 0,
    SMALL_HIT = 1,
    MEDIUM_HIT = 2,
    LARGE_HIT = 3
} _hit_amount;

int HitSomething(
    VECTOR *pos,
    int *cube,
    int *entry,
    int *poly,
    int type,
    int force);
float LineAndPlane(
    FVECTOR4 *plane,
    FVECTOR *start,
    FVECTOR *dir,
    float length,
    int padding);
int MoveObject(
    _mvector *movement,
    VECTOR *curpos,
    _hit_amount force);
int MoveObjectNormal(
    _mvector *movement,
    VECTOR *curpos,
    _hit_amount force,
    _svector *normal);
int RaycastCheck(
    FVECTOR *startpos,
    FVECTOR *direction,
    float length,
    int **cube,
    int **entry,
    int **poly,
    int *len,
    FVECTOR *hitpoint);
int RaycastCheckSV(
    _svector *startpos,
    _svector *direction,
    int length,
    int **cube,
    int **entry,
    int **poly,
    int *len,
    _svector *hitpoint);

int cliptofrustrum(
    FVECTOR4 *frustrum,
    FVECTOR *pos,
    int radius,
    int *distances);
int cliptofrustrumSV(
    FVECTOR4 *frustrum,
    _svector *pos,
    int radius,
    int *distances);
int intersec_FindWalkHeight(
    VECTOR *pos,
    VECTOR *normal,
    objectRoot *object,
    unsigned flags);
int intersec_FindWalkHeightFV(
    FVECTOR *pos,
    VECTOR *normal,
    objectRoot *object,
    unsigned flags);
int intersec_FindWalkHeightSV(
    _svector *pos,
    VECTOR *normal,
    objectRoot *object,
    unsigned flags);
int zapcheck(
    playerObject *player,
    _svector *start,
    _svector *end,
    int damage,
    playerObject *hitter,
    int extraradius);

/*
 * Inferred inspection facade for intersec.c's exact file-local
 * `whichsolid`; it is distinct from physics.c's same-named file static.
 */
_solid *jpb_IntersecGetWhichSolid(void);

#ifdef __cplusplus
}
#endif

#endif
