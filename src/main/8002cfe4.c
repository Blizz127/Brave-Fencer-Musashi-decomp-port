/* Owner re-derivation from retail assembly (clean input set). */
/* Decrements a global counter if nonzero, then runs three update helpers. */
#include "psx_types.h"

extern s16 D_800A4EFC;
extern void func_80030F80(s16 *p);
extern void func_8002D320(void);
extern void func_80031BE0(void);

void func_8002CFE4(void) {
    s16 *p;

    p = &D_800A4EFC;
    if (*p != 0) {
        *p -= 1;
    }
    func_80030F80(p);
    func_8002D320();
    func_80031BE0();
}
