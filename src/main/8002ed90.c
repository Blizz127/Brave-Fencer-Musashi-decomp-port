/* Owner re-derivation from retail assembly (clean input set). */
/* Runs func_8002EC10, then if bit 2 of D_800A4EA2 is set calls func_800385C0 with D_800A4E9A and clears the flags. */
#include "psx_types.h"

extern u16 D_800A4EA2;
extern s16 D_800A4E9A;
extern void func_8002EC10(void);
extern void func_800385C0(s32 a);

void func_8002ED90(void) {
    u16 *flags;

    func_8002EC10();
    flags = &D_800A4EA2;
    if (*flags & 4) {
        func_800385C0(D_800A4E9A);
        *flags = 0;
    }
}
