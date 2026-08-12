#ifndef JPB_VECTORS_H
#define JPB_VECTORS_H

#include <stdint.h>

#include "jpb/fmath.h"

#ifdef __cplusplus
extern "C" {
#endif

MATRIX *InvertMatrix(MATRIX *src, MATRIX *dst);
int32_t vec_AreaTriangle(VECTOR *a, VECTOR *b, VECTOR *c);
int32_t vec_DistPoint2Line(VECTOR *p, VECTOR *a, VECTOR *b);
uint32_t vec_Distance2DLV(VECTOR *v0, VECTOR *v1);
uint32_t vec_DistanceLV(VECTOR *v0, VECTOR *v1);
uint32_t vec_DistanceSV(_svector *v0, _svector *v1);
uint32_t vec_DistanceSVLV(_svector *v0, VECTOR *v1);
MATRIX *vec_IdentMatrix(MATRIX *m);
int vec_InvRotVectorLV(_svector *rot, VECTOR *src, VECTOR *dest);
uint32_t vec_LengthLV(VECTOR *v0);
void vec_LinearCombine(
    _svector *s_point, VECTOR *w_point, _svector *light, int16_t t);
int vec_PointNearSegment(int dist, VECTOR *p, VECTOR *a, VECTOR *b);
void vec_Polar2Rect(VECTOR *polar, VECTOR *dest);
uint32_t vec_QuickDistanceLV(VECTOR *v0, VECTOR *v1);
uint32_t vec_QuickRangeCheck(VECTOR *v0, VECTOR *v1, int range);
uint32_t vec_QuickRangeCheckFV(FVECTOR *v0, FVECTOR *v1, float range);
uint32_t vec_RangeCheck(VECTOR *v0, VECTOR *v1, int range);
int32_t vec_RotFromNormal(_svector *rot, VECTOR *tp);
int32_t vec_RotFromNormalF(_svector *rot, FVECTOR *tp);
int32_t vec_RotFromNormalS(_svector *rot, _svector *tp);
int vec_RotVectorLV(_svector *rot, VECTOR *src, VECTOR *dest);
int vec_RotVectorY(int y, VECTOR *src, VECTOR *dest);
void vec_ScaleVector(_svector *vector, int scale);
void vec_VectorNormalLV(VECTOR *vector, VECTOR *normal);
void vec_VectorNormalSV(VECTOR *vector, _svector *normal);
void vec_gDefinePlane(VECTOR *p, VECTOR *q, VECTOR *r, Plane *plane);
void vec_gDefinePlaneNormal(VECTOR *normal, VECTOR *p, Plane *plane);
void vec_gProject2Plane(
    _svector *s_point, VECTOR *w_point, _svector *light, Plane *plane);
void vec_gProject2PlaneF12(
    _svector *s_point, VECTOR *w_point, VECTOR *light, Plane *plane);

#ifdef __cplusplus
}
#endif

#endif
