/* Owner re-derivation from retail assembly (clean input set). */
/* If a precondition holds, queues an update from the difference of two table entries and returns 1; else 0. */
#include "psx_types.h"

typedef struct {
    u16 f0;
    u16 f2;
    u8 pad4[12];
} Row;

extern Row D_80063900[];
extern s32 func_8002A5C4(s32 amount);
extern s32 func_8002A728(void);
extern void func_8016E918(s32, s16);
extern void func_8002D4C8(s32, s32);
extern void func_8002AAAC(void);

/* Callers pass the amount in a0; it goes straight on to func_8002A5C4, which reads it. */
s32 func_8002A544(s32 amount) {
    s32 idx;

    if (func_8002A5C4(amount) != 0) {
        idx = func_8002A728();
        func_8016E918(3, D_80063900[idx].f2 - D_80063900[idx - 1].f2);
        func_8002D4C8(0xB2C, 0);
        func_8002AAAC();
        return 1;
    }
    return 0;
}
