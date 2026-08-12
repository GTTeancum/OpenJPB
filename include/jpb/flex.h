#ifndef JPB_FLEX_H
#define JPB_FLEX_H

#include "jpb/fmath.h"

#ifdef __cplusplus
extern "C" {
#endif

float VectorNormalize(FVECTOR *v);
float VectorNormalize2(FVECTOR *v, FVECTOR *w);
float VectorNormalize3(float x, float y, float z, FVECTOR *v);
_svector *CROSS(_svector *left, _svector *right, _svector *output);
_svector *CROSS12(_svector *left, _svector *right, _svector *output);
_svector *CROSSnormal(_svector *left, _svector *right, _svector *output);
int DOT(_svector *left, _svector *right);
int DOT12(_svector *left, _svector *right);
int32_t distance_squared(int x, int y, int z);
int flexabs(int value);
int flexdiv(int numerator, int denominator);
int flexmul(int left, int right);
int flexmul12(int a, int b);
float fvectorpointlinesquared(
    FVECTOR *p0,
    FVECTOR *p1,
    FVECTOR *point,
    float *dist);
_svector *intersec_2dlines(_svector *left, _svector *right);
int32_t mul4105(int32_t value);
int32_t normalize_s(
    int32_t x,
    int32_t y,
    int32_t z,
    _svector *output);
int normalize_svector(_svector *input, _svector *output);
int32_t normalize_vector(
    int32_t x,
    int32_t y,
    int32_t z,
    VECTOR *output);
void unpack10bitnormal(int32_t packed, _svector *output);
int vecSqr(_svector *vector);
int vecSqr12(_svector *vector);
_svector *vecadd(_svector *left, _svector *right, _svector *output);
int veclength(_svector *vector);
_svector *vecnegate(_svector *vector, _svector *output);
_svector *vecnormalize(_svector *vector);
_svector *vecnormalize2(_svector *vector, _svector *output);
_svector *vecnormalizevec(_svector *vector, _svector *output);
_svector *vecoffset(_svector *vector, int x, int y, int z);
int vecpointlinesquared(
    _svector *p0,
    _svector *p1,
    _svector *point,
    void *dist);
_svector *vecscale(_svector *vector, int scale, _svector *output);
_svector *vecsub(_svector *left, _svector *right, _svector *output);

#ifdef __cplusplus
}
#endif

#endif
