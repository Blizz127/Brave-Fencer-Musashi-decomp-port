/* Owner re-derivation from retail assembly (clean input set). */
/* Choose the direction word from the camera delta and the linked heading.
 * Compare the previous byte counter with the low byte of n on each call. */
#include "psx_types.h"

/* Camera target; only the position words used here. */
typedef struct {
    u8 pad00[0x5C];
    s32 f5C;
    u8 pad60[4];
    s32 f64;
    s32 f68;
    u8 pad6C[4];
    s32 f70;
} Cam;

/* Linked object; only its heading. */
typedef struct {
    u8 pad00[0x12];
    s16 heading;
} Link;

typedef struct {
    u8 pad00[0x20];
    u32 link;
    u8 pad24[0xA9 - 0x24];
    u8 bA9;
    u16 fAA;
    u8 padAC[0x20C - 0xAC];
    u8 cnt;
} Obj;

extern u32 D_801151D4;
/* The original caller consumes the full return register. The host angle
 * provider has a signed halfword result. Only its low twelve bits are used. */
#if defined(__mips__) && defined(__GNUC__) && __GNUC__ == 2
typedef u32 uintptr_t;
extern s32 func_8004CFEC(s32 dx, s32 dz);
#else
#include <stdint.h>
extern s16 func_8004CFEC(s32 dx, s32 dz);
#endif
extern s32 func_80146D90(void *o);

s32 func_80171B4C(Obj *o, s32 n)
{
    Cam *cam = (Cam *)(uintptr_t)D_801151D4;
    s32 dx;
    s32 dz;
    s32 m;
    s32 ang;
    s32 d;
    s32 v;

    o->bA9 = 0x41;
    dx = (s32)((u32)cam->f68 - (u32)cam->f5C);
    dz = (s32)((u32)cam->f70 - (u32)cam->f64);
    m = n;
    ang = func_8004CFEC(dx, dz);
    d = (((Link *)(uintptr_t)o->link)->heading - (s32)(((u32)ang + 0x800u) & 0xFFFu) + 0x100) & 0xE00;
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
    o->fAA = v;
count:
    if (o->cnt++ == (u8)m) {
        func_80146D90(o);
        return 1;
    }
    return 0;
}
