/* Intersect a segment with a horizontal box face, then classify its X/Z extent. */
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
typedef AXIS_ACCESS FloorPoint AxisPoint;

s32 func_80136824(FloorBox *box, s32 flags, s16 plane)
{
    s16 mode = (s16)flags;
    s32 distance, denominator, offset_y;
    AxisPoint *start, *movement, *hit;
    if ((mode & 1) == 0) {
        offset_y = plane - (s16)D_801870AC->y;
        distance = offset_y;
        denominator = -(s16)D_801870B8->y;
    } else {
        distance = (s16)D_801870AC->y - plane;
        denominator = (s16)D_801870B8->y;
        offset_y = -distance;
    }
    movement = D_801870B8;
    start = D_801870AC;
    hit = D_801870B4;
    hit->x = start->x + (u32)(-distance * (s16)movement->x / denominator);
    hit->y = start->y + offset_y;
    hit->z = start->z + (u32)(-distance * (s16)movement->z / denominator);
    if ((s16)hit->x < BOX_BOUNDS(box)->xlo || (s16)hit->x > BOX_BOUNDS(box)->xhi ||
        (s16)hit->z < BOX_BOUNDS(box)->zlo || (s16)hit->z > BOX_BOUNDS(box)->zhi)
        return 0;
    if (mode & 0x8000) {
        AxisPoint *far_point = D_801870B0;
        hit->x = far_point->x;
        hit->z = far_point->z;
    }
    BOX_NORMAL->z = 0;
    BOX_NORMAL->x = 0;
    if (mode & 1) {
        BOX_NORMAL->y = 0xFFF;
        ((AxisPoint *)D_801870B4)->y = plane + 2;
    } else {
        *(AXIS_ACCESS s16 *)&BOX_NORMAL->y = -0xFFF;
        ((AxisPoint *)D_801870B4)->y = plane - 2;
    }
    BOX_MIDPOINT->x = (BOX_BOUNDS(box)->xlo + BOX_BOUNDS(box)->xhi) >> 1;
    BOX_MIDPOINT->y = (BOX_BOUNDS(box)->ylo + BOX_BOUNDS(box)->yhi) >> 1;
    BOX_MIDPOINT->z = (BOX_BOUNDS(box)->zlo + BOX_BOUNDS(box)->zhi) >> 1;
    return 1;
}
