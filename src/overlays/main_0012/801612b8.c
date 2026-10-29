/* Owner re-derivation from retail assembly (clean input set). */
/* Runs func_8014E934 unless bit 0 of the flag is set, returns func_8014CC28 | func_8014F3E8. */
#include "psx_types.h"

extern void func_8014E934(void *o);
extern s32 func_8014CC28(void *o);
extern s32 func_8014F3E8(void *o);

s32 func_801612B8(void *o, s32 flag) {
    s32 a;
    if (!(flag & 1)) {
        func_8014E934(o);
    }
    a = func_8014CC28(o);
    a |= func_8014F3E8(o);
    return a;
}
