/* Owner re-derivation from retail assembly (clean input set). */
/* Calls func_80165A20 with the argument when D_80078EC1 is 3, 0xC or 0x18. */
#include "psx_types.h"

extern u8 D_80078EC1;
extern void func_80165A20(s32 x);

void func_80165718(s32 x) {
    switch (D_80078EC1) {
    case 3:
    case 0xC:
    case 0x18:
        func_80165A20(x);
        break;
    }
}
