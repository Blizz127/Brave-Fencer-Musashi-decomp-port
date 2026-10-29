/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

typedef struct {
    u16 x, y, z, d, e, f, pad, h;
    s32 i;
} L143BDC;

extern void func_8012C51C(L143BDC *, s32);

void func_80143BDC(u16 *p) {
    L143BDC s;
    s.x = p[0];
    s.y = p[1];
    s.z = p[2];
    s.d = 0x1C;
    s.e = 1;
    s.f = 0;
    s.i = 0;
    s.h = 0;
    func_8012C51C(&s, 0);
}
