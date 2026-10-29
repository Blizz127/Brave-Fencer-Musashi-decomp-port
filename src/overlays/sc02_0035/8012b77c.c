/* Owner re-derivation from retail assembly (clean input set). */
/* Returns a packed pair: the 12-bit heading from point q to point p (high half) and an angle derived from the rotated offsets (low half). */
#include "psx_types.h"

typedef struct {
    u32 lo : 16;
    u32 hi : 16;
} Pair32;

extern s32 func_8004CFEC(s32 a, s32 b);
extern s32 func_80047948(s32 ang);
extern s32 func_8004787C(s32 ang);

Pair32 func_8012B77C(s16 *p, s16 *q) {
    Pair32 r;
    s32 dx = q[5] - p[5];
    s32 dy = q[1] - p[1];
    s32 dz = q[3] - p[3];
    s32 ang = (func_8004CFEC(-dx, dy) - 0x400) & 0xFFF;
    s32 s;
    s32 c;
    r.hi = ang;
    s = func_80047948(ang);
    c = func_8004787C(ang);
    dx = (dx * s + dy * c) >> 12;
    r.lo = func_8004CFEC(dz, -dx);
    return r;
}
