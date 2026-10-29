/* Owner re-derivation from retail assembly (clean input set). */
/* Four-state step function: advances the state on poll results and returns 1 when the current state's condition is clear. */
#include "psx_types.h"

extern s32 D_800747EC;
extern s32 D_800747E8;
extern s32 D_800747E4;
extern s32 D_800BA1B4;
extern s32 D_800AE708;
extern s32 func_80034B98(void);
extern void func_8002D4C8(s32, s32);

s32 func_8001BE30(void)
{
    s32 r;
    s32 x;

    switch (D_800747EC) {
    case 0:
        r = func_80034B98();
        if (D_800BA1B4 == 5) {
            if (D_800747E4 != 0) {
                func_8002D4C8(0x16, 0);
            }
            D_800BA1B4 = 0;
            D_800747E8 = 0;
            D_800747E4 = 0;
            D_800747EC = 3;
            goto zero;
        }
        if (r == 1 || r == 4 || r == 2) {
            func_8002D4C8(0x16, 0);
            D_800747EC = D_800747EC + 1;
            goto zero;
        }
        if (D_800AE708 == 0) {
            goto one;
        }
        D_800747EC = 2;
        goto zero;
    case 1:
        x = func_80034B98();
        break;
    case 2:
        x = D_800AE708;
        break;
    case 3:
        x = D_800747E8;
        break;
    default:
        goto zero;
    }
    if (x != 0) {
        goto zero;
    }
one:
    return 1;
zero:
    return 0;
}
