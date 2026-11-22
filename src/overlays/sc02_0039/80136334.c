/* Intersect a movement segment with one Z face of a box. */
#include "scene_box.h"

/* The segment, box and scratch points can overlap by one halfword. */
#if BFM_MINE_DIRECT_GUEST_ADDRESS && defined(__GNUC__) && __GNUC__ == 2
#define AXIS_ACCESS
#else
#define AXIS_ACCESS volatile
#endif
#define BOX_BOUNDS(pointer) ((AXIS_ACCESS SceneSurfaceBox *)(pointer))
#define BOX_MIDPOINT ((AXIS_ACCESS SceneProbeVector *)&D_80126720)


s32 func_80136334(SceneSurfaceBox *box, s32 flags, s16 face_z)
{
    /* The original leaf still reserves an eight-byte frame. */
    u8 workspace[8];
    AXIS_ACCESS SceneProbeVector *near_point, *movement, *hit;
    s32 denominator, distance, delta;
    u16 mode = (u16)flags;
    (void)workspace;
#if !BFM_MINE_DIRECT_GUEST_ADDRESS
    if (box == 0 || (uintptr_t)box % _Alignof(SceneSurfaceBox))
        bfm_model_box_invalid_bounds();
#endif
    if (!(flags & 1)) {
        delta = face_z - (s16)box_point(D_8017F68C)->z;
        distance = delta;
        denominator = -(s16)box_point(D_8017F698)->z;
    } else {
        denominator = (s16)box_point(D_8017F698)->z;
        distance = (s16)box_point(D_8017F68C)->z - face_z;
        delta = -distance;
    }
    movement = box_point(D_8017F698);
    near_point = box_point(D_8017F68C);
    hit = box_point(D_8017F694);
#if !BFM_MINE_DIRECT_GUEST_ADDRESS
    if (denominator == 0)
        bfm_model_box_divide_by_zero();
#endif
    hit->x = near_point->x + (u32)((-distance * (s16)movement->x) / denominator);
    hit->y = near_point->y + (u32)((-distance * (s16)movement->y) / denominator);
    hit->z = near_point->z + delta;
    if ((s16)hit->x < BOX_BOUNDS(box)->bounds[0] || (s16)hit->x > BOX_BOUNDS(box)->bounds[1]) return 0;
    if ((s16)hit->y < BOX_BOUNDS(box)->bounds[2] || (s16)hit->y > BOX_BOUNDS(box)->bounds[3]) return 0;
    if (mode & 0x8000) {
        AXIS_ACCESS SceneProbeVector *far_point = box_point(D_8017F690);
        hit->x = far_point->x;
        hit->y = far_point->y;
    }
    D_801152A8.y = 0;
    D_801152A8.x = 0;
    if (mode & 1) {
        box_point(D_8017F694)->z = face_z + 2;
        D_801152A8.z = 0xfff;
    } else {
        *(s16 *)&D_801152A8.z = -0xfff;
        box_point(D_8017F694)->z = face_z - 2;
    }
    BOX_MIDPOINT->x = (BOX_BOUNDS(box)->bounds[0] + BOX_BOUNDS(box)->bounds[1]) >> 1;
    BOX_MIDPOINT->y = (BOX_BOUNDS(box)->bounds[2] + BOX_BOUNDS(box)->bounds[3]) >> 1;
    BOX_MIDPOINT->z = (BOX_BOUNDS(box)->bounds[4] + BOX_BOUNDS(box)->bounds[5]) >> 1;
    return 1;
}
