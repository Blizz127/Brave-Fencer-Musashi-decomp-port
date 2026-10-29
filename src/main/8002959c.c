/* Owner re-derivation from retail assembly (clean input set). */
/* Calls func_8002A544(0x32) and bumps a global counter. */
#include "psx_types.h"

extern s32 D_80078E88;
extern void func_8002A544(s32);

void func_8002959C(void) {
    func_8002A544(0x32);
    D_80078E88 = D_80078E88 + 1;
}
