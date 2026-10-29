/* Owner re-derivation from retail assembly (clean input set). */
/* Same body as the main_0012 twin at this address. */
#include "psx_types.h"

extern s32 func_8012BEE8(u8 *o);
extern void func_80130D48(u8 *o);
extern void func_801319E0(u8 *o);
extern void func_80131CA8(u8 *o, s32 a);

void func_8012FF98(u8 *o) {
    u8 *p;
    if (*(u32 *)(o + 0xB4) & 0x20) {
        p = (u8 *)*(u32 *)(o + 0x20);
        if (p != 0) {
            *(u32 *)(p + 4) ^= 0x80000000;
        }
        if (func_8012BEE8(o) != 0) {
            func_80130D48(o);
            func_801319E0(o);
        }
    }
    func_80131CA8(o, 0x10);
}
