/* Owner re-derivation from retail assembly and locally generated m2c. */
#include "psx_types.h"

extern u8 D_800AF7C4;
extern u8 D_800AF7C3;
extern u8 D_800AF7C2;
extern void func_80014998(void);

void func_80014960(void) {
    D_800AF7C4 = 0;
    D_800AF7C3 = 0;
    D_800AF7C2 = 0;
    func_80014998();
}
