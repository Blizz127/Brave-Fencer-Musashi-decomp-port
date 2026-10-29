/* Owner re-derivation from retail assembly (clean input set). */
/* Clears a global halfword then calls a helper. */
#include "psx_types.h"

extern s16 D_800A4EF0;
extern void func_80031D70(void);

void func_8002F620(void) {
    D_800A4EF0 = 0;
    func_80031D70();
}
