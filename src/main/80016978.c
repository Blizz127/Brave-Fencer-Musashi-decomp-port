/* func_80016978: advance the fade counter D_800AF7CE by a per-mode step (mode & 3, or the D_800B9A18 entry
 * when 0), clamp the fade level at state+0x19E to 0xFF, apply it (inverted for modes < 4) via func_80016450,
 * and return 1 once the level is >= 0xFF. D_800AF7CE is the same word as D_800AF630+0x19E.
 * Idioms: the counter update sits inside each arm (gcc cross-jumps the identical tails), which keeps the
 * state+0x19E reload in its own block after the store; the clamp is an if/else that reloads the field
 * (CSE reuses the first load, giving the retail `lhu v0` + copy into the level register). */
#include "psx_types.h"

extern u8 D_80062BA0[];
extern s32 D_800B9A18;
typedef struct {
    u8 pad[0x19E];
    u16 fade;
} State;
extern State D_800AF630;
extern u16 D_800AF7CE;
extern void func_80016450(u8 grey, u16 depth);

s32 func_80016978(u16 mode) {
    State *state = &D_800AF630;
    u32 k;
    u32 level;
    u32 v;

    k = mode & 3;
    if (k == 0) {
        D_800AF7CE += D_80062BA0[D_800B9A18];
    } else {
        D_800AF7CE += D_80062BA0[k];
    }
    if (state->fade >= 0xFF) {
        level = 0xFF;
    } else {
        level = state->fade;
    }
    if (mode < 4) {
        v = 0xFF - level;
    } else {
        v = level;
    }
    func_80016450(v & 0xFF, 0);
    return state->fade >= 0xFF;
}
