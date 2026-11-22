/* Intersect the moving segment with one X boundary of the collision box. */
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

s32 func_801365B8(SceneSurfaceBox *box, s32 flags, s16 plane)
{
    s32 distance;
    s32 denominator;
    s32 x_offset;
    u16 mode = (u16)flags;
    AXIS_ACCESS SceneProbeVector *near_point;
    AXIS_ACCESS SceneProbeVector *contact;
    AXIS_ACCESS SceneProbeVector *movement;

#ifndef __mips__
    if (box == 0 || (uintptr_t)box % _Alignof(SceneSurfaceBox))
        bfm_model_box_invalid_bounds();
#endif
    if ((flags & 1) == 0) {
        distance = plane - (s16)box_point(D_8017F68C)->x;
        x_offset = distance;
        denominator = -(s16)box_point(D_8017F698)->x;
    } else {
        denominator = (s16)box_point(D_8017F698)->x;
        distance = (s16)box_point(D_8017F68C)->x - plane;
        x_offset = -distance;
    }
    near_point = box_point(D_8017F68C);
    contact = box_point(D_8017F694);
    movement = box_point(D_8017F698);
    contact->x = near_point->x + x_offset;
#ifndef __mips__
    if (denominator == 0)
        bfm_model_box_divide_by_zero();
#endif
    contact->y = near_point->y + (u32)((-distance * (s16)movement->y) / denominator);
    contact->z = near_point->z + (u32)((-distance * (s16)movement->z) / denominator);
    if ((s16)contact->z < BOX_BOUNDS(box)->bounds[4] || (s16)contact->z > BOX_BOUNDS(box)->bounds[5] ||
        (s16)contact->y < BOX_BOUNDS(box)->bounds[2] || (s16)contact->y > BOX_BOUNDS(box)->bounds[3])
        return 0;
    if (mode & 0x8000) {
        AXIS_ACCESS SceneProbeVector *far_point = box_point(D_8017F690);
        contact->y = far_point->y;
        contact->z = far_point->z;
    }
    BOX_NORMAL->y = BOX_NORMAL->z = 0;
    if (mode & 1) {
        BOX_NORMAL->x = 4095;
        ((AXIS_ACCESS SceneProbeVector *)box_point(D_8017F694))->x = plane + 2;
    } else {
        *(AXIS_ACCESS s16 *)&D_801152A8.x = -4095;
        ((AXIS_ACCESS SceneProbeVector *)box_point(D_8017F694))->x = plane - 2;
    }
    BOX_MIDPOINT->x = (BOX_BOUNDS(box)->bounds[0] + BOX_BOUNDS(box)->bounds[1]) >> 1;
    BOX_MIDPOINT->y = (BOX_BOUNDS(box)->bounds[2] + BOX_BOUNDS(box)->bounds[3]) >> 1;
    BOX_MIDPOINT->z = (BOX_BOUNDS(box)->bounds[4] + BOX_BOUNDS(box)->bounds[5]) >> 1;
    return 1;
}
