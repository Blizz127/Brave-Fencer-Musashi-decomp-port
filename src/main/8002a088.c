/* func_8002A088: if the precondition holds, queue an update from two adjacent table entries; returns 1, else 0. */
#include "psx_types.h"

typedef struct {
    u16 f;
    u8 pad[14];
} Row;

extern Row D_800638FA[];
extern s32 func_8002A108(s32);
extern s32 func_8002A26C(void);
extern void func_8016E918(s32, s16);
extern void func_8002D4C8(s32, s32);
extern void func_8002AAAC(void);

s32 func_8002A088(s32 step) {
    s32 idx;

    if (func_8002A108(step) != 0) {
        idx = func_8002A26C();
        func_8016E918(0, D_800638FA[idx].f - D_800638FA[idx - 1].f);
        func_8002D4C8(0xB2C, 0);
        func_8002AAAC();
        return 1;
    }
    return 0;
}
