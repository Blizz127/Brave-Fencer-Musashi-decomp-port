/* Owner re-derivation from retail assembly (clean input set). */
/* Advances the streamed-data loader state machine: start the request, then poll the read/decode/finish steps until state 3 (done). */
#include "psx_types.h"

extern void func_80019AF8(s32);
extern s32 func_8001B0D4(s32 *, s16);
extern s32 func_8001B394(s32);
extern s32 func_8001B7C4(s32 *);
extern s32 func_80034B98(void);
extern s32 D_800747E4;
extern s16 D_800A6430;
extern s32 D_800A6550;
extern s32 D_800AE640;
extern s32 *D_800AE6E4;
extern s32 D_800AE708;
extern s32 D_800AE720;
extern s32 D_800AE724;
extern s32 D_800AE72C;
extern s32 *D_800AE7A4;
extern s32 D_800AE7A8;
extern s32 D_800AE7B0;
extern u32 D_800BA1B4;
extern s32 D_800BA318;
extern s32 D_800C6D2C;

static inline s32 start_stream(s32 *data, s32 mode, s32 dest, s32 wait) {
    s32 id;

    if (func_80034B98() != 0) {
        return 0;
    }
    if (D_800AE708 != 0) {
        id = *data;
        if (id != D_800AE708) {
            return 0;
        }
    } else {
        id = *data;
    }
    D_800AE72C = mode;
    D_800AE7A8 = dest;
    D_800AE7A4 = data;
    D_800AE724 = wait;
    D_800AE720 = 0;
    D_800AE708 = id;
    if (wait == 0) {
        D_800AE720 = 1;
    }
    func_80019AF8(0);
    return D_800AE7B0;
}

void func_8001AF34(void) {
    func_80034B98();
    switch (D_800BA1B4) {
    case 0:
    case 3:
        break;
    case 1:
        if (start_stream(D_800AE6E4, D_800BA318, D_800C6D2C, D_800A6550) != 0) {
            D_800BA1B4 = 3;
        }
        break;
    case 2:
        if (func_8001B394(D_800AE640) != 0) {
            D_800BA1B4 = 3;
        }
        break;
    case 4:
        if (func_8001B7C4(D_800AE6E4) != 0) {
            D_800BA1B4 = 3;
        }
        break;
    case 5:
        if (func_8001B0D4(D_800AE6E4, D_800A6430) != 0) {
            D_800747E4 = 0;
            D_800BA1B4 = 3;
        }
        break;
    }
}
