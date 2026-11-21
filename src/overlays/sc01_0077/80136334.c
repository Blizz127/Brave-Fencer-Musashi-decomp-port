/* Intersect a movement segment with one Z face of a box. */
#include "forest_floor.h"

/* The segment, box and scratch points can overlap by one halfword. */
#if defined(__mips__) && defined(__GNUC__) && __GNUC__ == 2
#define AXIS_ACCESS
#else
#define AXIS_ACCESS volatile
#endif
#define BOX_BOUNDS(pointer) ((AXIS_ACCESS FloorBox *)(pointer))
#define BOX_MIDPOINT ((AXIS_ACCESS FloorPoint *)&D_80126720)


s32 func_80136334(FloorBox *box, s32 flags, s16 face_z)
{
    /* The original leaf still reserves an eight-byte frame. */
    u8 workspace[8];
    AXIS_ACCESS FloorPoint *near_point, *movement, *hit;
    s32 denominator, distance, delta;
    u16 mode = (u16)flags;
    (void)workspace;
    if (!(flags & 1)) {
        delta = face_z - (s16)D_801870AC->z;
        distance = delta;
        denominator = -(s16)D_801870B8->z;
    } else {
        denominator = (s16)D_801870B8->z;
        distance = (s16)D_801870AC->z - face_z;
        delta = -distance;
    }
    movement = D_801870B8;
    near_point = D_801870AC;
    hit = D_801870B4;
    hit->x = near_point->x + (u32)((-distance * (s16)movement->x) / denominator);
    hit->y = near_point->y + (u32)((-distance * (s16)movement->y) / denominator);
    hit->z = near_point->z + delta;
    if ((s16)hit->x < BOX_BOUNDS(box)->xlo || (s16)hit->x > BOX_BOUNDS(box)->xhi) return 0;
    if ((s16)hit->y < BOX_BOUNDS(box)->ylo || (s16)hit->y > BOX_BOUNDS(box)->yhi) return 0;
    if (mode & 0x8000) {
        AXIS_ACCESS FloorPoint *far_point = D_801870B0;
        hit->x = far_point->x;
        hit->y = far_point->y;
    }
    D_801152A8.y = 0;
    D_801152A8.x = 0;
    if (mode & 1) {
        D_801870B4->z = face_z + 2;
        D_801152A8.z = 0xfff;
    } else {
        *(s16 *)&D_801152A8.z = -0xfff;
        D_801870B4->z = face_z - 2;
    }
    BOX_MIDPOINT->x = (BOX_BOUNDS(box)->xlo + BOX_BOUNDS(box)->xhi) >> 1;
    BOX_MIDPOINT->y = (BOX_BOUNDS(box)->ylo + BOX_BOUNDS(box)->yhi) >> 1;
    BOX_MIDPOINT->z = (BOX_BOUNDS(box)->zlo + BOX_BOUNDS(box)->zhi) >> 1;
    return 1;
}
