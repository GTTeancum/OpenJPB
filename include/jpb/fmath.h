#ifndef JPB_FMATH_H
#define JPB_FMATH_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    JPB_FIXED_SHIFT = 12,
    JPB_FIXED_ONE = 1 << JPB_FIXED_SHIFT
};

/*
 * Direct PDB types from the matched x64 build. Microsoft long/short fields
 * are represented explicitly so the layouts remain identical on host and
 * original-Xbox targets.
 */
typedef struct VECTOR {
    int32_t vx;
    int32_t vy;
    int32_t vz;
    int32_t pad;
} VECTOR;

typedef struct _svector {
    int16_t vx;
    int16_t vy;
    int16_t vz;
    int16_t pad;
} _svector;

typedef _svector SVECTOR;

typedef struct _sfvector {
    float vx;
    float vy;
    float vz;
    int16_t pad;
} _sfvector;

typedef _sfvector SFVECTOR;

typedef struct MATRIX {
    float m[3][3];
    int32_t t[3];
} MATRIX;

typedef struct FMATRIX {
    float m[3][3];
    float t[3];
} FMATRIX;

typedef struct FVECTOR {
    float vx;
    float vy;
    float vz;
} FVECTOR;

typedef struct FVECTOR4 {
    float vx;
    float vy;
    float vz;
    float vw;
} FVECTOR4;

typedef struct CVECTOR {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t cd;
} CVECTOR;

typedef struct Plane {
    VECTOR plane_normal;
    int32_t plane_const;
} Plane;

void SetGTETransLV(VECTOR *t);
void SetTransformMatrix(MATRIX *m);
void ApplyMatrixMany10Bit(
    int *input, _svector *output, int n, int shifter);
void ApplyMatrixMany10BitFV(
    int *input, FVECTOR *output, int n, int shifter);
void ApplyMatrixMany10BitFVnormalize(
    int *input, FVECTOR *output, int n, int shifter);
void ApplyMatrixMany10BitLong(
    int *input, VECTOR *output, int n, int shifter);
void ApplyMatrixMany10BitStride(
    int *input,
    _svector *output,
    int n,
    int shifter,
    int stride);
void ApplyMatrixManyFV(FVECTOR *input, FVECTOR *output, int n);
void ApplyMatrixManySV(
    _svector *input, _svector *output, int n);
void FindSinCos(int a, float *s, float *c);
int PerspectiveTransform(
    MATRIX *matrix, _svector *source, FVECTOR *destination);
int PerspectiveTransformFV(
    MATRIX *matrix, FVECTOR *source, FVECTOR *destination);
int PerspectiveTransformLV(
    MATRIX *matrix, VECTOR *source, FVECTOR *destination);
int PerspectiveTransformManyFV(
    MATRIX *matrix, FVECTOR *source, FVECTOR *destination, int count);
int PerspectiveTransformOLD(
    MATRIX *matrix, _svector *source, FVECTOR *destination);
int RotTransPersFloat(
    MATRIX *matrix, FVECTOR *input, FVECTOR *output, int n);
int RotTransPersSFV(
    MATRIX *matrix, _sfvector *input, FVECTOR *output, int n);
int SquareRoot0(int a);
int SquareRoot12(int a);
int TransformPoints(_svector *points, int *results, int n);
int TransformPointsFV(
    _svector *points, FVECTOR *results, int n);
int VectorNormal(VECTOR *v0, VECTOR *v1);
int VectorNormalS(VECTOR *v0, _svector *v1);
void XRotMatrix(MATRIX *m, float angle);
void YRotMatrix(MATRIX *m, float angle);
void ZRotMatrix(MATRIX *m, float angle);
VECTOR *fApplyMatrix(MATRIX *m, _svector *v0, VECTOR *v1);
_svector *fApplyMatrixSV(
    MATRIX *m, _svector *v0, _svector *v1);
_sfvector *fApplyMatrixSFV(
    MATRIX *m, _sfvector *v0, _sfvector *v1);
FVECTOR *fApplyMatrixFV(MATRIX *m, FVECTOR *v0, FVECTOR *v1);
VECTOR *fApplyMatrixLV(MATRIX *m, VECTOR *v0, VECTOR *v1);
MATRIX *fMulMatrix(MATRIX *m0, MATRIX *m1);
MATRIX *fMulMatrix0(MATRIX *m0, MATRIX *m1, MATRIX *r);
int fRotTransPers(
    MATRIX *matrix,
    _svector *source,
    FVECTOR *destination,
    int count);
MATRIX *fRotMatrix(_svector *r, MATRIX *m);
MATRIX *fRotMatrixX(int a, MATRIX *m);
MATRIX *fRotMatrixY(int a, MATRIX *m);
MATRIX *fRotMatrixZ(int a, MATRIX *m);
MATRIX *fRotMatrixZYX(_svector *r, MATRIX *m);
MATRIX *fScaleMatrix(MATRIX *m, VECTOR *v);
MATRIX *fTransMatrix(MATRIX *m, VECTOR *v);
MATRIX *fTransposeMatrix(MATRIX *m0, MATRIX *m1);
float getscreenz(MATRIX *m, VECTOR *v);
int normalize(int a, int b, int c, _svector *d);
int normalize_l(int x, int y, int z, VECTOR *r);
int normalize_lvector(VECTOR *a, VECTOR *b);
int ratan2(int y, int x);
int rcos(int a);
int rsin(int a);

#ifdef JPB_FMATH_TESTING
const MATRIX *jpb_fmath_test_transform_matrix(void);
#endif

#ifdef __cplusplus
}
#endif

#if defined(__cplusplus)
#define JPB_FMATH_STATIC_ASSERT(condition, message) static_assert(condition, message)
#else
#define JPB_FMATH_STATIC_ASSERT(condition, message) _Static_assert(condition, message)
#endif

JPB_FMATH_STATIC_ASSERT(sizeof(int) == 4, "PDB int must remain 32-bit");

JPB_FMATH_STATIC_ASSERT(sizeof(VECTOR) == 16, "VECTOR layout changed");
JPB_FMATH_STATIC_ASSERT(offsetof(VECTOR, vx) == 0, "VECTOR.vx layout changed");
JPB_FMATH_STATIC_ASSERT(offsetof(VECTOR, vy) == 4, "VECTOR.vy layout changed");
JPB_FMATH_STATIC_ASSERT(offsetof(VECTOR, vz) == 8, "VECTOR.vz layout changed");
JPB_FMATH_STATIC_ASSERT(offsetof(VECTOR, pad) == 12, "VECTOR.pad layout changed");

JPB_FMATH_STATIC_ASSERT(sizeof(_svector) == 8, "_svector layout changed");
JPB_FMATH_STATIC_ASSERT(offsetof(_svector, vx) == 0, "_svector.vx layout changed");
JPB_FMATH_STATIC_ASSERT(offsetof(_svector, vy) == 2, "_svector.vy layout changed");
JPB_FMATH_STATIC_ASSERT(offsetof(_svector, vz) == 4, "_svector.vz layout changed");
JPB_FMATH_STATIC_ASSERT(offsetof(_svector, pad) == 6, "_svector.pad layout changed");

JPB_FMATH_STATIC_ASSERT(sizeof(_sfvector) == 16, "_sfvector layout changed");
JPB_FMATH_STATIC_ASSERT(offsetof(_sfvector, vx) == 0, "_sfvector.vx layout changed");
JPB_FMATH_STATIC_ASSERT(offsetof(_sfvector, vy) == 4, "_sfvector.vy layout changed");
JPB_FMATH_STATIC_ASSERT(offsetof(_sfvector, vz) == 8, "_sfvector.vz layout changed");
JPB_FMATH_STATIC_ASSERT(offsetof(_sfvector, pad) == 12, "_sfvector.pad layout changed");

JPB_FMATH_STATIC_ASSERT(sizeof(MATRIX) == 48, "MATRIX layout changed");
JPB_FMATH_STATIC_ASSERT(offsetof(MATRIX, m) == 0, "MATRIX.m layout changed");
JPB_FMATH_STATIC_ASSERT(offsetof(MATRIX, t) == 36, "MATRIX.t layout changed");

JPB_FMATH_STATIC_ASSERT(
    sizeof(FVECTOR4) == 16,
    "FVECTOR4 must match PDB type 0x1238");
JPB_FMATH_STATIC_ASSERT(
    offsetof(FVECTOR4, vw) == 12,
    "FVECTOR4.vw layout changed");

JPB_FMATH_STATIC_ASSERT(sizeof(FMATRIX) == 48, "FMATRIX layout changed");
JPB_FMATH_STATIC_ASSERT(
    offsetof(FMATRIX, m) == 0, "FMATRIX.m layout changed");
JPB_FMATH_STATIC_ASSERT(
    offsetof(FMATRIX, t) == 36, "FMATRIX.t layout changed");

JPB_FMATH_STATIC_ASSERT(sizeof(FVECTOR) == 12, "FVECTOR layout changed");
JPB_FMATH_STATIC_ASSERT(offsetof(FVECTOR, vx) == 0, "FVECTOR.vx layout changed");
JPB_FMATH_STATIC_ASSERT(offsetof(FVECTOR, vy) == 4, "FVECTOR.vy layout changed");
JPB_FMATH_STATIC_ASSERT(offsetof(FVECTOR, vz) == 8, "FVECTOR.vz layout changed");

JPB_FMATH_STATIC_ASSERT(sizeof(Plane) == 20, "Plane layout changed");
JPB_FMATH_STATIC_ASSERT(
    offsetof(Plane, plane_normal) == 0, "Plane.plane_normal layout changed");
JPB_FMATH_STATIC_ASSERT(
    offsetof(Plane, plane_const) == 16, "Plane.plane_const layout changed");

#undef JPB_FMATH_STATIC_ASSERT

#endif
