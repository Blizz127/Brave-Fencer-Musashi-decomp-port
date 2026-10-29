/* Owner re-derivation from retail assembly (clean input set). */
/* If D_80078EC5 bit 0x80 is set, calls func_80165B28(void); else dispatches through the
 * D_801878D4 table indexed by D_80078EC1 & 0x7F (a0 is never used so no args are set up). */
#include "psx_types.h"

extern u8 D_80078EC5;
extern void func_80165B28(void);
extern u8 D_80078EC1;
extern void (*D_801878D4[])(void);

void func_80165580(void)
{
    if ((D_80078EC5 & 0x80) != 0) {
        func_80165B28();
    } else {
        D_801878D4[D_80078EC1 & 0x7F]();
    }
}
