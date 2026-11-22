#include "floor_plane_probe.h"

s32 func_80134A28(const FloorPlane *plane, const SceneProbeVector *point, s32 offset)
{
    u32 x = (u32)(plane->v[0] * point->x);
    u32 y = (u32)(plane->v[1] * point->y);
    u32 z = (u32)(plane->v[2] * point->z);
    return (s32)(x + y + z + (u32)offset);
}
