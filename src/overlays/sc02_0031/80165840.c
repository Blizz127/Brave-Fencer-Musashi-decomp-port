/* Owner re-derivation from retail assembly (clean input set). */
/* If D_80078EC3 is set, clear it and run func_80165874(void); else do nothing. */
#include "psx_types.h"

extern u8 D_80078EC3;
extern void func_80165874(void);

void func_80165840(void)
{
    if (D_80078EC3 != 0) {
        D_80078EC3 = 0;
        func_80165874();
    }
}
