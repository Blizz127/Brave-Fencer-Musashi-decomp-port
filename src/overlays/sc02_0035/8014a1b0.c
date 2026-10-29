/* Owner re-derivation from retail assembly (clean input set). */
/* Same body as the main_0012 twin at this address. */
#include "psx_types.h"

extern void func_80015978(void *a, void *b);
extern u16 func_80156370(s32 a);
extern s32 func_8014C4AC(void *a, s32 b, s32 c, void *d, s32 e);

/* Returns retail's leftover v0: func_8014C4AC's result from the final call. */
s32 func_8014A1B0(u8 *o, void *p) {
    s16 v[4];
    func_80015978(o + 4, v);
    v[3] = 0;
    return func_8014C4AC(p, 10, func_80156370(10), v, *(s16 *)(*(u8 **)(o + 0x20) + 0x12));
}
