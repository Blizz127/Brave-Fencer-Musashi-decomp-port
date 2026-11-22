/* Evaluate the signed side of a terrain face at this point. */
#include "mine_terrain_plane.h"

s32 func_80134310(s16 *point, s16 *normal, s32 offset)
{
    return mine_terrain_plane_value(point, normal, offset);
}
