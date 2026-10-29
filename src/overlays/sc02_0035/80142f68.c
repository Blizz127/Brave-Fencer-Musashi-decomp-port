/* Owner re-derivation from retail assembly (clean input set). */
/* Same body as the main_0012 twin at this address. */
#include "psx_types.h"

extern s32 func_80142DB8(u8 *o);
extern s32 func_8012C0EC(u8 *o);
extern void func_80142BB4(u8 *o, s32 a, s32 b);
extern void func_80142C9C(u8 *o);
extern s32 func_8012BF10(u8 *o, s32 a);
extern void func_8012AD44(u8 *o, s32 a);

void func_80142F68(u8 *o) {
    if (func_80142DB8(o) == 1) {
        func_80142BB4(o, 1, 3);
    } else if (*(s16 *)(o + 0x100) != 0) {
        if (func_8012C0EC(o) != 0) {
            func_80142BB4(o, 0, 3);
        }
    } else {
        func_80142C9C(o);
        if (func_8012BF10(o, 0x30) != 0) {
            func_8012AD44(o, 2);
        }
    }
}
