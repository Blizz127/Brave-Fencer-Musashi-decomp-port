/* Owner re-derivation from retail assembly (clean input set). */
/* Same body as the main_0012 twin at this address. */
#include "psx_types.h"

extern s16 D_80126CBA;
extern u8 D_80126B58[];
s32 func_80013478(void *, void *);

s32 func_8012BCCC(u8 *a) {
    u8 *base = D_80126B58;
    s32 r;
    if (D_80126CBA == 0) {
        r = func_80013478(a + 4, base + 0x15C);
    } else {
        r = 0x7FFFFFFF;
    }
    return r;
}
