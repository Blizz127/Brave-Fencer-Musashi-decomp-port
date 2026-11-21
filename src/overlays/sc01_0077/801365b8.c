/* Intersect the moving segment with one X boundary of the collision box. */
#include "forest_floor.h"

/* The segment, box and scratch points can overlap by one halfword. */
#if defined(__mips__) && defined(__GNUC__) && __GNUC__ == 2
#define AXIS_ACCESS
#else
#define AXIS_ACCESS volatile
#endif
#define BOX_BOUNDS(pointer) ((AXIS_ACCESS FloorBox *)(pointer))
#define BOX_NORMAL ((AXIS_ACCESS FloorPoint *)&D_801152A8)
#define BOX_MIDPOINT ((AXIS_ACCESS FloorPoint *)&D_80126720)

s32 func_801365B8(FloorBox *box, s32 flags, s16 plane)
{
    s32 distance;
    s32 denominator;
    s32 x_offset;
    u16 mode = (u16)flags;
    AXIS_ACCESS FloorPoint *near_point;
    AXIS_ACCESS FloorPoint *contact;
    AXIS_ACCESS FloorPoint *movement;

    if ((flags & 1) == 0) {
        distance = plane - (s16)D_801870AC->x;
        x_offset = distance;
        denominator = -(s16)D_801870B8->x;
    } else {
        denominator = (s16)D_801870B8->x;
        distance = (s16)D_801870AC->x - plane;
        x_offset = -distance;
    }
    near_point = D_801870AC;
    contact = D_801870B4;
    movement = D_801870B8;
    contact->x = near_point->x + x_offset;
    contact->y = near_point->y + (u32)((-distance * (s16)movement->y) / denominator);
    contact->z = near_point->z + (u32)((-distance * (s16)movement->z) / denominator);
    if ((s16)contact->z < BOX_BOUNDS(box)->zlo || (s16)contact->z > BOX_BOUNDS(box)->zhi ||
        (s16)contact->y < BOX_BOUNDS(box)->ylo || (s16)contact->y > BOX_BOUNDS(box)->yhi)
        return 0;
    if (mode & 0x8000) {
        AXIS_ACCESS FloorPoint *far_point = D_801870B0;
        contact->y = far_point->y;
        contact->z = far_point->z;
    }
    BOX_NORMAL->y = BOX_NORMAL->z = 0;
    if (mode & 1) {
        BOX_NORMAL->x = 4095;
        ((AXIS_ACCESS FloorPoint *)D_801870B4)->x = plane + 2;
    } else {
        *(AXIS_ACCESS s16 *)&D_801152A8.x = -4095;
        ((AXIS_ACCESS FloorPoint *)D_801870B4)->x = plane - 2;
    }
    BOX_MIDPOINT->x = (BOX_BOUNDS(box)->xlo + BOX_BOUNDS(box)->xhi) >> 1;
    BOX_MIDPOINT->y = (BOX_BOUNDS(box)->ylo + BOX_BOUNDS(box)->yhi) >> 1;
    BOX_MIDPOINT->z = (BOX_BOUNDS(box)->zlo + BOX_BOUNDS(box)->zhi) >> 1;
    return 1;
}
