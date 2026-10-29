#include "psx_types.h"

extern s32 func_8017248C(u8 *o, s16 *p);
extern s32 func_8004CFEC(s32 a, s32 b);
extern void func_80171EC8(u8 *o);
extern void func_80146D90(u8 *o);

s32 func_80171D78(u8 *o, s16 *p) {
    s32 r;
    s32 a;
    r = func_8017248C(o, p);
    if (r != 0) {
        func_80146D90(o);
        return r;
    }
    a = func_8004CFEC(p[0] - *(s16 *)(o + 6), p[2] - *(s16 *)(o + 0xE));
    *(s16 *)(*(u32 *)(o + 0x20) + 0x12) = (a + 0x800) & 0xFFF;
    func_80171EC8(o);
    return 0;
}
