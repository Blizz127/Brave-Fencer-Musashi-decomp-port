/* Owner re-derivation from retail assembly (clean input set). */
/* Returns retail's leftover v0 (see ledger). */
#include "psx_types.h"

extern s32 func_80161208(u8 *o);
extern s32 func_80146CA0(u8 *o);

s32 func_80160C28(u8 *o) {
    s32 r;
    u8 *p;
    u8 *q;
    r = func_80161208(o);
    if (r == 0) {
        *(s32 *)(o + 0x238) += 0x1C000;
        p = (u8 *)*(u32 *)(o + 0x20);
        *(u16 *)(p + 0x10) = (*(u16 *)(p + 0x10) - *(u16 *)(o + 0x23A)) & 0xFFF;
        q = (u8 *)*(u32 *)(o + 0x20);
        r = *(s16 *)(q + 0x10) < 0xC00;
        if (r) {
            *(s16 *)(q + 0x10) = 0xC00;
            *(s16 *)(o + 0xB8) = 1;
            r = func_80146CA0(o);
        }
    }
    return r;
}
