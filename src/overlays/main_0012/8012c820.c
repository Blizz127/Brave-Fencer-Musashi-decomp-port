/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

extern s32 func_8012C890(void *o, u8 *p, s32 a);
extern u8 D_80120194[];

s32 func_8012C820(u8 *o) {
    u8 *base = D_80120194;
    u8 *p = base + 0x6480;
    if (p != base) {
        do {
            if (*(u16 *)p == 0) {
                goto found;
            }
            p -= 0x10C;
        } while (p != base);
    }
    p = 0;
found:
    if (p == 0) {
        return 0;
    }
    *(u16 *)(o + 0xA) |= 0x8000;
    return func_8012C890(o, p, 0);
}
