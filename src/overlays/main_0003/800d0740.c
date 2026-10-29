/* Owner re-derivation from retail assembly (clean input set). */
/* Menu step: polls input, moves a 0-2 cursor on up/down, fires a select action on confirm, and counts down a timer. */
#include "psx_types.h"

extern u8 D_800AF630[];
extern u16 D_800EC888;
extern s32 D_800EC88C;
extern s16 D_800EC894;
extern s16 D_800EC898;
extern void func_800D12D0(void);
extern u16 func_800149E0(s32);
extern void func_8002D4C8(s32, s32);
extern void func_800167B8(s32);
extern void func_800118AC(void);

void func_800D0740(void) {
    register u8 *base = D_800AF630;
    s32 moved;

    func_800D12D0();
    D_800EC888 = func_800149E0(0);
    moved = 0;
    if ((D_800EC888 & 0x4000) != 0) {
        func_8002D4C8(0x9C1, 0);
        moved = 1;
        D_800EC894++;
        if (D_800EC894 >= 3) {
            D_800EC894 = 0;
        }
    } else if ((D_800EC888 & 0x1000) != 0) {
        func_8002D4C8(0x9C1, 0);
        moved = 1;
        D_800EC894--;
        if (D_800EC894 < 0) {
            D_800EC894 = 2;
        }
    }
    D_800EC898 = 0;
    switch (D_800EC894) {
    case 0:
        if ((D_800EC888 & 0x800) != 0) {
            func_8002D4C8(0x9C0, 0);
            func_800167B8(4);
            func_800118AC();
        }
        break;
    case 1:
    case 2:
        if ((D_800EC888 & 0x800) != 0) {
            func_8002D4C8(0x9C0, 0);
            func_800167B8(4);
            func_800118AC();
        }
        break;
    }
    if (D_800EC88C > 0) {
        if (--D_800EC88C == 0) {
            func_800167B8(4);
            D_800EC894 = 4;
            func_800118AC();
        }
    }
}
