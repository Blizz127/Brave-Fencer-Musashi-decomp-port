/* Owner re-derivation from retail assembly (clean input set). */
/* Four-state sequence on D_801270CC: waits for the position and button state, starts the three scripted callbacks in turn, and counts D_801270D4 while it is nonzero. */
#include "psx_types.h"

extern s32 D_801270CC[];
extern s32 D_801270D4;
extern u16 D_80126B66;
extern u8 D_80078EC1;
extern u8 D_8017E1D4[];
extern u8 D_8017E748[];
extern u8 D_8017E868[];
extern void func_8016EDEC(u8 *cb, s32 arg);

void func_8017F290(void)
{
    switch (D_801270CC[0]) {
    case 0:
        if ((s16)D_80126B66 >= 0x600) {
            if ((D_80078EC1 & 0x7F) == 8) {
                func_8016EDEC(D_8017E1D4, 0x1000000);
                D_801270CC[0] = 2;
            } else {
                func_8016EDEC(D_8017E1D4, 0x1000000);
                D_801270CC[0] = 1;
            }
        }
        break;
    case 1:
        if ((D_80078EC1 & 0x7F) == 8) {
            func_8016EDEC(D_8017E748, 0x1000000);
            D_801270CC[0] = 2;
        }
        break;
    case 2:
        if (D_801270D4 != 0) {
            D_801270CC[0] = 3;
            D_801270D4 = D_801270D4 + 1;
            func_8016EDEC(D_8017E868, 0x1000000);
        }
        break;
    case 3:
        if (D_801270D4 != 0) {
            D_801270D4 = D_801270D4 + 1;
        }
        break;
    }
}
