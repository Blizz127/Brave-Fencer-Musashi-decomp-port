/* func_80156A88 - owner re-derivation from retail assembly (clean input set).
 * For each of 3 table entries with bit 0 set, dispatches the masked command
 * word to func_80147324 / func_8014CA88 / func_8014CA14. void. */
#include "psx_types.h"
extern u32 D_801150E0[];
extern u32 D_801151E0[3];
extern void func_80147324(s32 a);
extern s32 func_8014CA88(u32 a, u32 b);
extern s32 func_8014CA14(u32 a, u32 b);

void func_80156A88(s32 unused, u8 *s)
{
    u32 *p;
    u32 *q;
    u32 i;
    u32 m;
    u32 f2;
    u32 f1;
    u32 v;

    v = *(u32 *)(s + 0xC);
    m = v & 0xF0FFFFFF;
    if (v == 0x80000000) {
        return;
    }
    i = 0;
    f1 = v & 0x1000000;
    f2 = v & 0x2000000;
    p = D_801150E0;
    q = D_801151E0;
    for (; i < 3; i++, q++) {
        if ((*q & 1) != 0) {
            if (f1 != 0) {
                func_80147324(m & 0xFFFF);
            } else if (f2 != 0) {
                func_8014CA88(*p, m);
            } else {
                func_8014CA14(*p, m);
            }
        }
        p++;
    }
}

