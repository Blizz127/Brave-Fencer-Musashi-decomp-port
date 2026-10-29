/* Owner re-derivation from retail assembly (clean input set). */
/* Same body as the main_0012 twin at this address. */
#include "psx_types.h"

extern u16 func_8014CA70(void *o, s32 i);
extern void func_80147364(s32, s32);
extern s32 D_8012707C;

/* a0/a1 are forwarded untouched to func_8014CA70 (retail reads entry $a0,$a1). */
s32 func_8014CA14(void *o, s32 i) {
    u16 r = func_8014CA70(o, i);
    if (r == 0) {
        return 1;
    }
    if (r != D_8012707C) {
        func_80147364(r, 0);
        return 0;
    }
    D_8012707C = 0;
    return 0;
}
