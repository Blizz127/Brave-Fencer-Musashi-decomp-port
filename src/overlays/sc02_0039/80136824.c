/* Intersect a segment with a horizontal box face, then classify its X/Z extent. */
#include "scene_box.h"

/* The segment, box and scratch points can overlap by one halfword. */
#if defined(__mips__) && defined(__GNUC__) && __GNUC__ == 2
#define AXIS_ACCESS
#else
#define AXIS_ACCESS volatile
#endif
#define BOX_BOUNDS(pointer) ((AXIS_ACCESS SceneSurfaceBox *)(pointer))
#define BOX_NORMAL ((AXIS_ACCESS SceneProbeVector *)&D_801152A8)
#define BOX_MIDPOINT ((AXIS_ACCESS SceneProbeVector *)&D_80126720)
typedef AXIS_ACCESS SceneProbeVector AxisPoint;

s32 func_80136824(SceneSurfaceBox *box, s32 flags, s16 plane)
{
    s16 mode = (s16)flags;
    s32 distance, denominator, offset_y;
    AxisPoint *start, *movement, *hit;
#ifndef __mips__
    if (box == 0 || (uintptr_t)box % _Alignof(SceneSurfaceBox))
        bfm_model_box_invalid_bounds();
#endif
    if ((mode & 1) == 0) {
        offset_y = plane - (s16)box_point(D_8017F68C)->y;
        distance = offset_y;
        denominator = -(s16)box_point(D_8017F698)->y;
    } else {
        distance = (s16)box_point(D_8017F68C)->y - plane;
        denominator = (s16)box_point(D_8017F698)->y;
        offset_y = -distance;
    }
    movement = box_point(D_8017F698);
    start = box_point(D_8017F68C);
    hit = box_point(D_8017F694);
#ifndef __mips__
    if (denominator == 0)
        bfm_model_box_divide_by_zero();
#endif
    hit->x = start->x + (u32)(-distance * (s16)movement->x / denominator);
    hit->y = start->y + offset_y;
    hit->z = start->z + (u32)(-distance * (s16)movement->z / denominator);
    if ((s16)hit->x < BOX_BOUNDS(box)->bounds[0] || (s16)hit->x > BOX_BOUNDS(box)->bounds[1] ||
        (s16)hit->z < BOX_BOUNDS(box)->bounds[4] || (s16)hit->z > BOX_BOUNDS(box)->bounds[5])
        return 0;
    if (mode & 0x8000) {
        AxisPoint *far_point = box_point(D_8017F690);
        hit->x = far_point->x;
        hit->z = far_point->z;
    }
    BOX_NORMAL->z = 0;
    BOX_NORMAL->x = 0;
    if (mode & 1) {
        BOX_NORMAL->y = 0xFFF;
        ((AxisPoint *)box_point(D_8017F694))->y = plane + 2;
    } else {
        *(AXIS_ACCESS s16 *)&BOX_NORMAL->y = -0xFFF;
        ((AxisPoint *)box_point(D_8017F694))->y = plane - 2;
    }
    BOX_MIDPOINT->x = (BOX_BOUNDS(box)->bounds[0] + BOX_BOUNDS(box)->bounds[1]) >> 1;
    BOX_MIDPOINT->y = (BOX_BOUNDS(box)->bounds[2] + BOX_BOUNDS(box)->bounds[3]) >> 1;
    BOX_MIDPOINT->z = (BOX_BOUNDS(box)->bounds[4] + BOX_BOUNDS(box)->bounds[5]) >> 1;
    return 1;
}
