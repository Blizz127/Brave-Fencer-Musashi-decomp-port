/* Owner re-derivation from retail assembly (clean input set). */
/* Mode zero selects scene tables by event flag. Mode one installs all five. */
#include "psx_types.h"

extern u8 D_80183D70[];
extern u8 D_80183DA0[];
extern u8 D_80183DF0[];
extern u8 D_80183E20[];
extern u8 D_80183E50[];
extern u8 func_80029178(s32 a);
extern void func_80146014(u8 *p);

void func_80145EE8(s32 mode)
{
    switch (mode) {
    case 0:
        if (func_80029178(0x1F) != 0) {
            func_80146014(D_80183D70);
        }
        if (func_80029178(0x21) != 0) {
            func_80146014(D_80183DA0);
        }
        if (func_80029178(0x1E) != 0) {
            func_80146014(D_80183DF0);
        }
        if (func_80029178(0x1D) != 0) {
            func_80146014(D_80183E20);
        }
        if (func_80029178(0x20) != 0) {
            func_80146014(D_80183E50);
        }
        break;
    case 1:
        func_80146014(D_80183D70);
        func_80146014(D_80183DA0);
        func_80146014(D_80183DF0);
        func_80146014(D_80183E20);
        func_80146014(D_80183E50);
        break;
    }
}
