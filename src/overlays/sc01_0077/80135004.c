/* Test the segment from its starting cell and, when needed, from its ending cell. */
#include "forest_floor.h"

#ifdef __mips__
typedef FloorPoint SegmentPoint;
#else
/* Partially overlapping halfword locations retain the original access order. */
typedef volatile FloorPoint SegmentPoint;
#endif

s32 func_80135004(u16 flags, FloorPoint *from, FloorPoint *to)
{
    s32 mode = flags;
    SegmentPoint *origin = from, *destination = to;
    SegmentPoint *near_point = D_801870AC, *far_point = D_801870B0;
    SegmentPoint *motion = D_801870B8, *contact;
    u16 far_value, near_value, z;

    far_value = far_point->x = destination->x;
    near_value = near_point->x = origin->x;
    motion->x = far_value - near_value;
    far_value = far_point->y = destination->y;
    near_value = near_point->y = origin->y;
    motion->y = far_value - near_value;
    far_value = far_point->z = destination->z;
    near_value = near_point->z = origin->z;
    motion->z = far_value - near_value;
    D_801D9500 = 0;
    if (func_80134A74(mode, D_801870AC->x, D_801870AC->z, D_801D94F0)) {
hit:
        contact = D_801870B4;
        destination->x = contact->x;
        destination->y = contact->y;
        z = contact->z;
        destination->w = (s16)D_801D9500;
        destination->z = z;
        return 1;
    }
    if (((D_801870AC->x & 0xff80) != (D_801870B0->x & 0xff80) ||
         (D_801870AC->z & 0xff80) != (D_801870B0->z & 0xff80)) &&
        func_80134A74(mode, D_801870B0->x, D_801870B0->z, D_801D94F0))
        goto hit;
    return 0;
}
