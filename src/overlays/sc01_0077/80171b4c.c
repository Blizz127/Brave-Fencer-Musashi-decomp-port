/* Owner re-derivation from retail assembly (clean input set). */
/* Choose the direction word from the camera delta and the linked heading.
 * Compare the previous byte counter with the low byte of n on each call. */
#include "opening_event.h"

extern u32 D_801151D4;
extern s32 func_8004CFEC(s32 dx, s32 dz);
extern s32 func_80146D90(void *o);

s32 func_80171B4C(ScenePlayer *o, s32 n)
{
    SceneDirectionCamera *cam = OPENING_OBJECT(SceneDirectionCamera, D_801151D4);
    s32 dx;
    s32 dz;
    s32 m;
    s32 ang;
    s32 d;
    s32 v;

    o->direction_mode = 0x41;
    dx = (s32)((u32)cam->target_x - (u32)cam->x);
    dz = (s32)((u32)cam->target_z - (u32)cam->z);
    m = n;
    ang = func_8004CFEC(dx, dz);
    d = (OPENING_OBJECT(SceneDirectionLink, o->link)->heading - (s32)(((u32)ang + 0x800u) & 0xFFFu) + 0x100) & 0xE00;
    switch (d / 0x200) {
    case 0:
        v = 0x1000;
        break;
    case 1:
        v = 0x3000;
        break;
    case 2:
        v = 0x2000;
        break;
    case 3:
        v = 0x6000;
        break;
    case 4:
        v = 0x4000;
        break;
    case 5:
        v = 0xC000;
        break;
    case 6:
        v = 0x8000;
        break;
    case 7:
        v = 0x9000;
        break;
    default:
        goto count;
    }
    o->direction_bits = v;
count:
    if (o->event_progress.counter++ == (u8)m) {
        func_80146D90(o);
        return 1;
    }
    return 0;
}
