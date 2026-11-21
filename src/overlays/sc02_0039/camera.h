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

typedef struct {
    u8 reserved0[5];
    u8 phase;
    u8 reserved6[2];
    s32 distance, target_distance;
    s32 depth, phase_value;
    SceneVector rotation, target_rotation;
    SpatialPoint offset, target_offset;
    u8 reserved34[8];
    WideVector camera, focus;
    u8 reserved54[0x4c];
    s16 facing;
} FacingActor;

extern void func_8017D2B8(FacingActor *actor, s16 facing, SceneVector *position);
#endif
