#ifndef MINE_TERRAIN_PLANE_H
#define MINE_TERRAIN_PLANE_H

#include "psx_types.h"

static __inline__ s32 mine_terrain_plane_value(const s16 *point, const s16 *normal, s32 offset)
{
#if defined(__mips__) && defined(__GNUC__) && __GNUC__ == 2
    /* The target uses wrapping addu for the signed plane accumulator. */
    return point[0] * normal[0] + point[1] * normal[1] + point[2] * normal[2] + offset;
#else
    u32 value = (u32)(point[0] * normal[0]) + (u32)(point[1] * normal[1]) +
                (u32)(point[2] * normal[2]) + (u32)offset;
    /* Interpret the low word as a signed distance after all four terms. */
    return value <= 0x7fffffffu ? (s32)value : -1 - (s32)(~value);
#endif
}
#endif
