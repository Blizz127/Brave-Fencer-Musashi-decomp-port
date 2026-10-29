/* func_8014E98C [8014E98C,8014EA4C) - owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

typedef struct {
    s16 x;
    s16 y;
    s16 z;
    s16 w;
} V8;

/* callers read v0: yes (stores it); callee func_8014EA4C matches owner prototype. */
extern s32 func_8014EA4C(u8 *obj, V8 *probe, V8 *out, s32 pass);

s32 func_8014E98C(u8 *o) {
    V8 a;
    V8 b;
    s32 r;
    s32 i;

    r = 0;
    i = 2;
    if ((*(u32 *)(o + 0x44) & 0x80000) == 0 && (*(u32 *)(o + 0x1F8) & 0x4000000) == 0) {
        do {
            a.x = *(u16 *)(o + 0x88);
            a.y = *(u16 *)(o + 0x8A) - 0x10;
            a.z = *(u16 *)(o + 0x8C);
            b.x = *(u16 *)(o + 6);
            b.y = *(u16 *)(o + 0xA) - 0x10;
            b.z = *(u16 *)(o + 0xE);
            i--;
            r |= func_8014EA4C(o, &a, &b, i);
        } while (i != 0);
    }
    return r;
}
