/* Check a segment against a forest collision box and publish the selected contact. */
#include "forest_floor.h"

#if defined(__mips__)
extern u16 D_801152AA;
#define POINT(ptr) (ptr)
#else
#define D_801152AA D_801152A8.y
/* A direction store may overlap a subsequent halfword read of an endpoint. */
#define POINT(ptr) ((volatile FloorPoint *)(ptr))
#endif
extern u32 D_801150D8;
/* Resident VectorNormalSS service. */
extern void func_80047E98(FloorPoint *input, FloorPoint *output);

s32 func_80135EB0(FloorBox *box, s32 flags)
{
#ifdef __mips__
    register s32 mode __asm__("$19") = flags;
#else
    s32 mode = flags;
#endif
    u32 side, height, far_side, far_height;

    if ((s16)D_801870AC->x < box->xlo) side = 1;
    else if ((s16)D_801870AC->x > box->xhi) side = 2;
    else side = 0;
    if ((s16)D_801870AC->z < box->zlo) side |= 4;
    else if ((s16)D_801870AC->z > box->zhi) side |= 8;
    if ((s16)D_801870AC->y < box->ylo) height = 16;
    else if ((s16)D_801870AC->y > box->yhi) height = 32;
    else height = 0;

    if (!(side | height)) {
        u16 x, z;
        s32 center_x, center_z;
        x = POINT(D_801870AC)->x;
        POINT(D_801870B4)->x = x;
        center_x = (box->xlo + box->xhi) >> 1;
        D_801152AA = 0;
#ifdef __mips__
        /* The separate linker label for y occupies the second halfword of the normal. */
        __asm__ volatile ("" : : "m" (D_801152AA), "r" (x));
#endif
        D_80126720.x = center_x;
        D_801152A8.x = x - center_x;
        z = POINT(D_801870AC)->z;
        POINT(D_801870B4)->z = z;
        center_z = (box->zlo + box->zhi) >> 1;
        D_80126720.z = center_z;
        D_801152A8.z = z - center_z;
        POINT(D_801870B4)->y = POINT(D_801870AC)->y;
        D_80126720.y = (box->ylo + box->yhi) >> 1;
        func_80047E98(&D_801152A8, &D_801152A8);
        D_801150D8 |= 1;
        return 1;
    }

    if ((s16)D_801870B0->x < box->xlo) far_side = 1;
    else if ((s16)D_801870B0->x > box->xhi) far_side = 2;
    else far_side = 0;
    if ((s16)D_801870B0->z < box->zlo) far_side |= 4;
    else if ((s16)D_801870B0->z > box->zhi) far_side |= 8;
    if ((s16)D_801870B0->y < box->ylo) far_height = 16;
    else if ((s16)D_801870B0->y > box->yhi) far_height = 32;
    else far_height = 0;
    if ((side | height) & (far_side | far_height)) return 0;

    POINT(D_801870B8)->x = POINT(D_801870B0)->x - POINT(D_801870AC)->x;
    POINT(D_801870B8)->y = POINT(D_801870B0)->y - POINT(D_801870AC)->y;
    POINT(D_801870B8)->z = POINT(D_801870B0)->z - POINT(D_801870AC)->z;
    switch (side) {
    case 4:
        if (func_80136334(box, (s16)mode, box->zlo)) goto hit;
        if (height) break;
        return 0;
    case 1:
        if (func_801365B8(box, (s16)mode, box->xlo)) goto hit;
        break;
    case 2:
        if (func_801365B8(box, (s16)(mode | 1), box->xhi)) goto hit;
        break;
    case 5:
        if (func_80136334(box, (s16)mode, box->zlo)) goto hit;
        if (func_801365B8(box, (s16)mode, box->xlo)) goto hit;
        break;
    case 6:
        if (func_80136334(box, (s16)mode, box->zlo)) goto hit;
        if (func_801365B8(box, (s16)(mode | 1), box->xhi)) goto hit;
        break;
    case 9:
        if (func_801365B8(box, (s16)mode, box->xlo)) goto hit;
        /* fall through */
    case 8:
        if (func_80136334(box, (s16)(mode | 1), box->zhi)) goto hit;
        if (height) break;
    miss:
        return 0;
    case 10:
        if (func_801365B8(box, (s16)(mode | 1), box->xhi)) goto hit;
        if (func_80136334(box, (s16)(mode | 1), box->zhi)) goto hit;
        break;
    case 0:
        break;
    default:
        goto hit;
    }
    if (!height) goto miss;
    if (height == 16) {
        if (!func_80136824(box, (s16)mode, box->ylo)) goto miss;
    } else {
        if (!func_80136824(box, (s16)(mode | 1), box->yhi)) goto miss;
    }
hit:
    return 1;
}
