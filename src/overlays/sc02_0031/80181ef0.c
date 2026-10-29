/* Owner re-derivation from retail assembly (clean input set). */
/* Scene-state check: true when the byte at D_80078EBA is 4 and the byte at D_80078EB1 is in 7..11. */
#include "psx_types.h"
extern u8 D_80078EBA;
extern u8 D_80078EB1;
s32 func_80181EF0(void)
{
    if (D_80078EBA == 4 && (u32)(D_80078EB1 - 7) < 5) {
        return 1;
    }
    return 0;
}
