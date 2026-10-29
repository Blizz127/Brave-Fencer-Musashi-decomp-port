/* Owner re-derivation from retail assembly (clean input set). */
/* Steps the byte script at D_801BD680: counts down D_801BD685, otherwise latches the signed byte at the current index into D_801269D4/D_801269C8, loads the next count and advances D_801BD684; a zero count clears the script. */
#include "psx_types.h"

extern s8 *D_801BD680;
extern u8 D_801BD684;
extern u8 D_801BD685;
extern s32 D_801269D4;
extern s32 D_801269C8;

void func_8013CA14(void)
{
    u32 idx;
    s32 v;
    u8 cnt;

    if (*(u32 *)&D_801BD680 != 0) {
        cnt = D_801BD685;
        if (cnt == 0) {
            idx = D_801BD684;
            v = D_801BD680[idx * 2];
            D_801269C8 = D_801269D4 = v;
            cnt = D_801BD680[idx * 2 + 1];
            D_801BD684 = idx + 1;
            D_801BD685 = cnt;
            if (cnt == 0) {
                D_801269C8 = D_801269D4 = 0;
                D_801BD684 = 0;
                *(u32 *)&D_801BD680 = 0;
            }
        } else {
            D_801BD685 = cnt - 1;
        }
    }
}
