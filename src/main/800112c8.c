/* Owner re-derivation from retail assembly (clean input set). */
/* Calls a check routine with a fixed table and a global; if it succeeds, runs a follow-up routine. */
#include "psx_types.h"

extern u8 D_800AE870[];
extern s32 D_80072C70;
extern s32 func_80019A24(void *, s32, s32, s32);
extern void func_80011778(void);

void func_800112C8(void) {
    if (func_80019A24(D_800AE870, D_80072C70, 0, 0) != 0) {
        func_80011778();
    }
}
