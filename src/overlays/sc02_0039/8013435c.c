/* The terrain edge test selects its clearance from the face's Y normal. */
#include "mine_terrain_plane.h"

s32 func_8013435C(s16 *point, s16 *normal, s32 offset, s16 *face_normal)
{
    s32 limit = -0x2f00;
    if (face_normal[1] < -0xe00) limit = -0x1800;
    return mine_terrain_plane_value(point, normal, offset) < limit;
}
