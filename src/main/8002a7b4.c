/* Owner re-derivation from retail assembly (clean input set). */
/* If a precondition helper succeeds, queues an effect using the difference of two table entries and returns 1, else 0. */
#include "psx_types.h"


extern u16 D_80063906[][8];
extern s32 func_8002A834(s32 amount);
extern s32 func_8002A998(void);
extern void func_8002AAAC(void);
extern void func_8002D4C8(s32 a, s32 b);
extern void func_8016E918(s32 a, s16 b);

/* Callers pass the amount in a0; it goes straight on to func_8002A834, which reads it. */
s32 func_8002A7B4(s32 amount) {
    s32 idx;

    if (func_8002A834(amount) != 0) {
        idx = func_8002A998();
        func_8016E918(2, D_80063906[idx][0] - D_80063906[idx - 1][0]);
        func_8002D4C8(0xB2C, 0);
        func_8002AAAC();
        return 1;
    }
    return 0;
}
