/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

extern void func_80154150(u8 *, s32);
extern void func_801553A8(u8 *);
extern void func_80146CA0(u8 *);

void func_8015C030(u8 *o, s16 *p) {
    *(s32 *)(o + 0x234) = p[0] << 7;
    *(s32 *)(o + 0x238) = 0x80000;
    *(s32 *)(o + 0x23C) = p[2] << 7;
    func_80154150(o, 0x11);
    func_801553A8(o);
    func_80146CA0(o);
}
