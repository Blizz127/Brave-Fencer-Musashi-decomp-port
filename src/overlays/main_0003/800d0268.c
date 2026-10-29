/* Owner re-derivation from retail assembly (clean input set). */
/* Menu pad handler: right/left (0x4000/0x1000) cycle the selection D_800EC894 through 0..2 with a sound cue, 0x800 confirms, then refreshes via func_800D0D6C. */
#include "psx_types.h"

extern u8 D_800AF630[];
extern s16 D_800EC894;
extern u16 func_800149E0(s32 port);
extern void func_8002D4C8(s32 id, s32 a);
extern void func_800167B8(s32 a);
extern void func_800118AC(void);
extern void func_800D0D6C(void);

void func_800D0268(void) {
    register u8 *unused = D_800AF630;
    u16 pad;

    pad = func_800149E0(0);
    if (pad & 0x4000) {
        func_8002D4C8(0x9C1, 0);
        D_800EC894++;
        if (D_800EC894 >= 3) {
            D_800EC894 = 0;
        }
    } else if (pad & 0x1000) {
        func_8002D4C8(0x9C1, 0);
        D_800EC894--;
        if (D_800EC894 < 0) {
            D_800EC894 = 2;
        }
    }

    switch (D_800EC894) {
    case 0:
        if (pad & 0x800) {
            func_8002D4C8(0x9C0, 0);
            func_800167B8(4);
            func_800118AC();
        }
        break;
    case 1:
    case 2:
        if (pad & 0x800) {
            func_8002D4C8(0x9C0, 0);
            func_800167B8(4);
            func_800118AC();
        }
        break;
    }
    func_800D0D6C();
}
