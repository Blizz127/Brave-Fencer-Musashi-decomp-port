/* Check a segment against a mine collision box and publish the selected contact. */
#include "scene_box.h"

#if BFM_MINE_DIRECT_GUEST_ADDRESS
extern u16 D_801152AA;
#define POINT(ptr) (ptr)
#else
#define D_801152AA D_801152A8.y
/* A direction store may overlap a subsequent halfword read of an endpoint. */
#define POINT(ptr) ((volatile SceneProbeVector *)(ptr))
#endif
extern u32 D_801150D8;
/* Resident VectorNormalSS service. */
extern s32 func_80047E98(SceneProbeVector *input, SceneProbeVector *output);

s32 func_80135EB0(SceneSurfaceBox *box, s32 flags)
{
#if BFM_MINE_DIRECT_GUEST_ADDRESS
    register s32 mode __asm__("$19") = flags;
#else
    s32 mode = flags;
#endif
    u32 side, height, far_side, far_height;

#if !BFM_MINE_DIRECT_GUEST_ADDRESS
    if (box == 0 || (uintptr_t)box % _Alignof(SceneSurfaceBox))
        bfm_model_box_invalid_bounds();
#endif
    if ((s16)box_point(D_8017F68C)->x < box->bounds[0]) side = 1;
    else if ((s16)box_point(D_8017F68C)->x > box->bounds[1]) side = 2;
    else side = 0;
    if ((s16)box_point(D_8017F68C)->z < box->bounds[4]) side |= 4;
    else if ((s16)box_point(D_8017F68C)->z > box->bounds[5]) side |= 8;
    if ((s16)box_point(D_8017F68C)->y < box->bounds[2]) height = 16;
    else if ((s16)box_point(D_8017F68C)->y > box->bounds[3]) height = 32;
    else height = 0;

    if (!(side | height)) {
        u16 x, z;
        s32 center_x, center_z;
        x = POINT(box_point(D_8017F68C))->x;
        POINT(box_point(D_8017F694))->x = x;
        center_x = (box->bounds[0] + box->bounds[1]) >> 1;
        D_801152AA = 0;
#if BFM_MINE_DIRECT_GUEST_ADDRESS
        /* The separate linker label for y occupies the second halfword of the normal. */
        __asm__ volatile ("" : : "m" (D_801152AA), "r" (x));
#endif
        D_80126720.x = center_x;
        D_801152A8.x = x - center_x;
        z = POINT(box_point(D_8017F68C))->z;
        POINT(box_point(D_8017F694))->z = z;
        center_z = (box->bounds[4] + box->bounds[5]) >> 1;
        D_80126720.z = center_z;
        D_801152A8.z = z - center_z;
        POINT(box_point(D_8017F694))->y = POINT(box_point(D_8017F68C))->y;
        D_80126720.y = (box->bounds[2] + box->bounds[3]) >> 1;
        func_80047E98(&D_801152A8, &D_801152A8);
        D_801150D8 |= 1;
        return 1;
    }

    if ((s16)box_point(D_8017F690)->x < box->bounds[0]) far_side = 1;
    else if ((s16)box_point(D_8017F690)->x > box->bounds[1]) far_side = 2;
    else far_side = 0;
    if ((s16)box_point(D_8017F690)->z < box->bounds[4]) far_side |= 4;
    else if ((s16)box_point(D_8017F690)->z > box->bounds[5]) far_side |= 8;
    if ((s16)box_point(D_8017F690)->y < box->bounds[2]) far_height = 16;
    else if ((s16)box_point(D_8017F690)->y > box->bounds[3]) far_height = 32;
    else far_height = 0;
    if ((side | height) & (far_side | far_height)) return 0;

    POINT(box_point(D_8017F698))->x = POINT(box_point(D_8017F690))->x - POINT(box_point(D_8017F68C))->x;
    POINT(box_point(D_8017F698))->y = POINT(box_point(D_8017F690))->y - POINT(box_point(D_8017F68C))->y;
    POINT(box_point(D_8017F698))->z = POINT(box_point(D_8017F690))->z - POINT(box_point(D_8017F68C))->z;
    switch (side) {
    case 4:
        if (func_80136334(box, (s16)mode, box->bounds[4])) goto hit;
        if (height) break;
        return 0;
    case 1:
        if (func_801365B8(box, (s16)mode, box->bounds[0])) goto hit;
        break;
    case 2:
        if (func_801365B8(box, (s16)(mode | 1), box->bounds[1])) goto hit;
        break;
    case 5:
        if (func_80136334(box, (s16)mode, box->bounds[4])) goto hit;
        if (func_801365B8(box, (s16)mode, box->bounds[0])) goto hit;
        break;
    case 6:
        if (func_80136334(box, (s16)mode, box->bounds[4])) goto hit;
        if (func_801365B8(box, (s16)(mode | 1), box->bounds[1])) goto hit;
        break;
    case 9:
        if (func_801365B8(box, (s16)mode, box->bounds[0])) goto hit;
        /* fall through */
    case 8:
        if (func_80136334(box, (s16)(mode | 1), box->bounds[5])) goto hit;
        if (height) break;
    miss:
        return 0;
    case 10:
        if (func_801365B8(box, (s16)(mode | 1), box->bounds[1])) goto hit;
        if (func_80136334(box, (s16)(mode | 1), box->bounds[5])) goto hit;
        break;
    case 0:
        break;
    default:
        goto hit;
    }
    if (!height) goto miss;
    if (height == 16) {
        if (!func_80136824(box, (s16)mode, box->bounds[2])) goto miss;
    } else {
        if (!func_80136824(box, (s16)(mode | 1), box->bounds[3])) goto miss;
    }
hit:
    return 1;
}
