/* Owner re-derivation from retail assembly (clean input set). */
/* Maps a 0-11 selector to a table index (9-20, else 0), clears a pending state of 5, and dispatches func_8001ABBC. */
#include "psx_types.h"

typedef struct {
    u8 data[0x30];
} Entry48;

extern Entry48 D_80062C38[];
extern s32 D_800BA1B4;
extern void func_8001ABBC(s32, s32, void *, s32, s32);

void func_8001AAD0(u32 sel, s32 arg1) {
    s32 idx;

    switch (sel) {
    case 0: idx = 9; break;
    case 1: idx = 10; break;
    case 2: idx = 11; break;
    case 3: idx = 12; break;
    case 4: idx = 13; break;
    case 5: idx = 14; break;
    case 6: idx = 15; break;
    case 7: idx = 16; break;
    case 8: idx = 17; break;
    case 9: idx = 18; break;
    case 10: idx = 19; break;
    case 11: idx = 20; break;
    default: idx = 0; break;
    }
    if (D_800BA1B4 == 5) {
        D_800BA1B4 = 0;
    }
    func_8001ABBC(3, arg1, &D_80062C38[idx], 0, 0);
}
