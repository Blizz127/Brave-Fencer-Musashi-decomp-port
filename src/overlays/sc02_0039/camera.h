#ifndef BFM_SC02_0039_CAMERA_H
#define BFM_SC02_0039_CAMERA_H
#include "geometry.h"

typedef struct {
    SpatialPoint position;
    s16 padding;
} SceneVector;

typedef struct {
    s32 x, y, z;
} WideVector;

typedef struct FacingActor {
    u8 callback_address[4]; /* Provider stores a little-endian guest address. */
    u8 mode;
    u8 phase;
    u8 reserved6[2];
    s32 distance, target_distance;
    s32 depth, phase_value;
    SceneVector rotation, target_rotation;
    SpatialPoint offset, target_offset;
    u8 reserved34[8];
    WideVector camera, focus;
    s32 frame_component;
    s32 view_parameter;
    s32 view[7];
    s32 published_parameter;
    s32 view_offset[7];
    u8 reserved98[8];
    s16 facing;
} FacingActor;

typedef void (*SceneCameraHandler)(FacingActor *);

extern void func_8017D0D4(FacingActor *actor);
extern s32 func_8017D01C(void);
extern void func_8017D2B8(FacingActor *actor, s16 facing, SceneVector *position);
#endif
