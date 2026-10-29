/* Owner re-derivation from retail assembly (clean input set). */
/* Faces an object toward the camera target D_801151D4: marks it (byte 0xA9 = 0x41), takes the
 * heading from the camera delta through func_8004CFEC, quantises its difference to the linked
 * object's heading (s16 at 0x12 of the object at +0x20) into eighths of the circle and stores the
 * matching animation word at 0xAA. Counts calls in the byte at 0x20C and, when the count reaches
 * the low byte of `n`, runs func_80146D90 and returns 1; otherwise returns 0. */
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
extern s32 func_8004CFEC(s32 dx, s32 dz);
extern void func_80146D90(Obj *o);

s32 func_80171B4C(Obj *o, s32 n)
{
    Cam *cam = (Cam *)D_801151D4;
    s32 dx;
    s32 dz;
    s32 m;
    s32 ang;
    s32 d;
    s32 v;

    o->bA9 = 0x41;
    dx = cam->f68 - cam->f5C;
    dz = cam->f70 - cam->f64;
    m = n;
    ang = func_8004CFEC(dx, dz);
    d = (((Link *)o->link)->heading - ((ang + 0x800) & 0xFFF) + 0x100) & 0xE00;
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
