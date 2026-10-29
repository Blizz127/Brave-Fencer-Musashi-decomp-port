/* Owner re-derivation from retail assembly (clean input set). */
/* Same body as the main_0012 twin at this address. */
#include "psx_types.h"

extern s32 func_80142DB8(u8 *);
extern s32 func_80142D38(u8 *);
extern void func_80142BB4(u8 *, s32, s32);

void func_801428CC(u8 *o) {
    if (func_80142DB8(o) == 1) {
        func_80142BB4(o, 1, 3);
    } else if (func_80142D38(o) == 1) {
        func_80142BB4(o, 0, 3);
    }
}
