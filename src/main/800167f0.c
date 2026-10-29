/* Owner re-derivation from retail assembly (clean input set). */
/* Step the fade level by a table delta and redraw the fade at the new level. */
#include "psx_types.h"

typedef struct {
    u8 pad[0x19E];
    u16 level;
} State;

extern void func_80016224(s32, s32);
extern u8 D_80062BA0[];
extern State D_800AF630;
extern u16 D_800AF7CE;
extern s32 D_800B9A18;

s32 func_800167F0(u16 arg0) {
    State *state = &D_800AF630;
    s32 i;
    u32 n;
    u32 x;

    i = arg0 & 3;
    if (i == 0) {
        D_800AF7CE += D_80062BA0[D_800B9A18];
    } else {
        D_800AF7CE += D_80062BA0[i];
    }
    if (state->level >= 0xFF) {
        n = 0xFF;
    } else {
        n = state->level;
    }
    if (arg0 < 4) {
        x = 0xFF - n;
    } else {
        x = n;
    }
    func_80016224(x & 0xFF, 0);
    return state->level >= 0xFF;
}
