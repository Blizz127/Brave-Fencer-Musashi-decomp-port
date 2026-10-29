/* Owner re-derivation from retail assembly (clean input set). */
/* Same body as the main_0012 twin at this address. */
#include "psx_types.h"

extern void func_80131E00(u8 *o, s32 a);
extern s32 func_80131CA8(u8 *o, s32 a);
extern void func_8002A04C(u8 *o);
extern void func_8012C218(u8 *o);

s32 func_80131D68(u8 *o, u8 k) {
    if (k == 2 || k == 0x1B) {
        func_80131E00(o, 0x12);
        return 1;
    }
    if (k == 0x1A || k == 0x1C) {
        if (func_80131CA8(o, 0x11) != 0) {
            return 1;
        }
        if (*(u32 *)(o + 0x78) != 0) {
            func_8002A04C(o);
        }
        func_8012C218(o);
        return 1;
    }
    return 0;
}
