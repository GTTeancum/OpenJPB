#ifndef JPB_CAMERA_H
#define JPB_CAMERA_H

#include "jpb/fmath.h"
#include "jpb/scene.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Direct PDB type 0x121A from the matched x64 build. This record contains no
 * pointers, so fixed-width fields preserve the same layout on PC and Xbox.
 */
typedef struct Camera {
    int32_t viewType;
    VECTOR campos;
    VECTOR angle;
    _svector focus;
    VECTOR angleDest;
    _svector focusDest;
    uint32_t cameraTimer;
    uint32_t userData;
} Camera;

typedef struct JPBCameraSelectionDiagnostics {
    int valid;
    int previousDolly;
    int candidateDolly;
    int cameraType;
    int height;
    int boxMask;
    int player0Active;
    int player1Active;
    int player0Clip;
    int player1Clip;
    int offscreen;
    int accepted;
    int distances0[5];
    int distances1[5];
    _svector testFocus;
    VECTOR testAngle;
} JPBCameraSelectionDiagnostics;

enum {
    JPB_CAMERA_VIEW_RELATIVE_FOCUS = 0x0200,
    JPB_CAMERA_VIEW_ABSOLUTE_FOCUS = 0x0800
};

extern Camera gCamera;
extern int gGlobalFrameRate;
extern int screenshake;
extern int screenshakeamplitude;
extern int mCameraAngleDest;
extern int newcameraflag;
extern VECTOR streetcampos;
extern VECTOR uberPos;
extern int uberXRange;
extern int uberZRange;
extern int uberLock;
extern int cameraYaw;
extern _svector cameraFacing;
extern _svector cameraLocation;
extern VECTOR cameraposition;

int16_t camLerp(int16_t input, int16_t target, double amount);
double lerp(double x, double y, double amount);
int map(
    int x,
    int input_minimum,
    int input_maximum,
    int output_minimum,
    int output_maximum);
int mapClamped(
    int x,
    int input_minimum,
    int input_maximum,
    int output_minimum,
    int output_maximum);
double mapDouble(
    double x,
    double input_minimum,
    double input_maximum,
    double output_minimum,
    double output_maximum);
void camera_Camera2ViewVector(
    Camera *camera, sceneGeometryEnv *environment);
Camera *camera_GetCamera(VECTOR *angle, _svector *focus);
int camera_GetCurrentCameraType(void);
void camera_GetLeadDiagnostics(_svector *lead_out, int32_t *camera_lead);
void camera_GetSelectionDiagnostics(
    JPBCameraSelectionDiagnostics *diagnostics);
void camera_GetLocation(VECTOR *location);
int camera_GetViewType(void);
void camera_RestoreCameras(void);
void camera_ScrollCamera(int dx, int dz);
int camera_SetCameraPos(int camera_type);
void camera_SetCameras(void);
void camera_SetCurrentCameraType(int type);
void camera_SetFocusedCameraFocus(
    int camera_type, VECTOR *destination, int time);
void camera_SetShake(int amplitude);
void camera_SetViewType(int viewType);
void camera_SnapCamera(Camera *camera);
void camera_gGetLocation(VECTOR *location);
int console_CamerasCommand(
    int argument_count,
    char **string_arguments,
    int *integer_arguments,
    float *float_arguments);

#ifdef __cplusplus
}
#endif

#if defined(__cplusplus)
#define JPB_CAMERA_STATIC_ASSERT(condition, message) static_assert(condition, message)
#else
#define JPB_CAMERA_STATIC_ASSERT(condition, message) _Static_assert(condition, message)
#endif

JPB_CAMERA_STATIC_ASSERT(sizeof(Camera) == 76, "Camera layout changed");
JPB_CAMERA_STATIC_ASSERT(
    offsetof(Camera, viewType) == 0, "Camera.viewType layout changed");
JPB_CAMERA_STATIC_ASSERT(
    offsetof(Camera, campos) == 4, "Camera.campos layout changed");
JPB_CAMERA_STATIC_ASSERT(
    offsetof(Camera, angle) == 20, "Camera.angle layout changed");
JPB_CAMERA_STATIC_ASSERT(
    offsetof(Camera, focus) == 36, "Camera.focus layout changed");
JPB_CAMERA_STATIC_ASSERT(
    offsetof(Camera, angleDest) == 44, "Camera.angleDest layout changed");
JPB_CAMERA_STATIC_ASSERT(
    offsetof(Camera, focusDest) == 60, "Camera.focusDest layout changed");
JPB_CAMERA_STATIC_ASSERT(
    offsetof(Camera, cameraTimer) == 68, "Camera.cameraTimer layout changed");
JPB_CAMERA_STATIC_ASSERT(
    offsetof(Camera, userData) == 72, "Camera.userData layout changed");

#undef JPB_CAMERA_STATIC_ASSERT

#endif
