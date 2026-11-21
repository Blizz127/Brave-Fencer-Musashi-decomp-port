/* Signed plane products fit in 32 bits individually. Accumulate with the original wraparound. */
#include "forest_floor.h"

s32 func_80134A28(FloorPoint *normal, FloorPoint *point, s32 distance)
{
    u32 x = (u32)((s16)normal->x * (s16)point->x);
    u32 y = (u32)((s16)normal->y * (s16)point->y);
    u32 z = (u32)((s16)normal->z * (s16)point->z);
    return (s32)(x + y + z + (u32)distance);
}
