/* Owner re-derivation from retail assembly (clean input set). */
/* Seeds the two probe points D_80184A60/D_80184A64 from the point at a0 (y offset -4 and +0x2FC), runs func_801345F8 and on a hit lowers a0's y to D_80184A68's y minus 2 and returns a 0x2000/0x4000/0x8000 class from the first probe's w field; returns 0 on a miss. */
#include "psx_types.h"

typedef struct {
    u16 x, y, z;
    s16 w;
} Pt;

extern Pt *D_80184A60;
extern Pt *D_80184A64;
extern Pt *D_80184A68;
extern s32 D_801BD608;
extern u16 D_801BD618;
extern s32 func_801345F8(s32 a, Pt *b, Pt *c);

u32 func_80134510(Pt *p)
{
    u32 r;
    Pt *b = D_80184A64;
    Pt *a = D_80184A60;

    u16 x = p->x;
    u16 y;
    u16 z;

    p->w = 0;
    a->x = x;
    b->x = x;
    y = p->y;
    a->y = y - 4;
    b->y = y + 0x2FC;
    z = p->z;
    a->z = z;
    b->z = z;
    r = 0;
    if (func_801345F8(D_801BD608, a, b)) {
        s32 w;
        p->y = D_80184A68->y - 2;
        w = D_80184A60->w;
        if (w >= -0xBCB) {
            if (w < -0x578) {
                r = 0x4000;
            } else {
                r = 0x8000;
            }
        } else {
            r = 0x2000;
        }
        D_80184A60->w = D_801BD618;
    }
    return r;
}
