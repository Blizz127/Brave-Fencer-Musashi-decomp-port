/* Owner re-derivation from retail assembly (clean input set). */
/* Small timer/state machine tick. State 1 counts D_801BD6B4 up to 0x23B then latches
 * D_801BD698 = 1 and switches the state (D_801BD6B0) to 2. State 2 decrements D_801BD698;
 * on reaching 0 it re-latches D_801BD698 = 2, runs func_8013D9B0(2), and advances the state
 * to 3 once D_801BD6A4 reads clear. Any other state (0, or >= 3) is a no-op. */
#include "psx_types.h"

extern s32 D_801BD6B0;
extern s32 D_801BD6B4;
extern s32 D_801BD698;
extern s32 D_801BD6A4;
extern void func_8013D9B0(s32 a);

void func_8013D064(void)
{
    s32 state;

    state = D_801BD6B0;
    if (state == 0) {
        return;
    }

    if (state != 2) {
        if (state < 3) {
            if (state == 1) {
                s32 v;

                v = D_801BD6B4 + 1;
                D_801BD6B4 = v;
                if (v < 0x23B) {
                    return;
                }
                D_801BD698 = state;
                D_801BD6B0 = 2;
            }
        }
        return;
    }

    {
        s32 c;

        c = D_801BD698 - 1;
        D_801BD698 = c;
        if (c != 0) {
            return;
        }
        D_801BD698 = state;
        func_8013D9B0(2);
        if (D_801BD6A4 != 0) {
            return;
        }
        D_801BD6B0 = D_801BD6B0 + 1;
    }
}
