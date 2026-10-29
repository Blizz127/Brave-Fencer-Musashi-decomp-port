/* Owner re-derivation from retail assembly (clean input set). */
/* If func_801487F4(o)&8 is set, calls func_80161208(o); else dispatches o through
 * D_8018730C[o->0 (u16)] then calls func_80160A74(o). */
#include "psx_types.h"

extern u16 func_801487F4(u8 *o);
extern void (*D_8018730C[])(u8 *);
extern void func_80160A74(u8 *o);
extern s32 func_80161208(u8 *o);

void func_801609B8(u8 *o)
{
    if ((func_801487F4(o) & 8) == 0) {
        D_8018730C[*(u16 *)(o + 0)](o);
        func_80160A74(o);
    } else {
        func_80161208(o);
    }
}
