/* Owner re-derivation from retail assembly (clean input set). */
/* Same body as the main_0012 twin at this address. */
#include "psx_types.h"

typedef struct {
    u16 f0;
    u16 f2;
    u16 f4;
    u16 f6;
    s32 f8;
    s32 fC;
    s32 f10;
} Args8014AD30;
extern s32 func_80146AFC(Args8014AD30 *);

/* Returns retail's leftover v0: func_80146AFC's result from the final call. */
s32 func_8014AD30(s32 a, u16 *v, s32 b, s32 c) {
    Args8014AD30 t;
    u16 x = v[0];
    u16 y = v[1];
    u16 z = v[2];
    t.f0 = 0x1D;
    t.f8 = a;
    t.fC = b;
    t.f10 = c;
    t.f2 = x;
    t.f4 = y;
    t.f6 = z;
    return func_80146AFC(&t);
}
