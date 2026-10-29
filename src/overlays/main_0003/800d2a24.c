/* Owner re-derivation from retail assembly (clean input set). */
/* Polls a status query until its status byte reports bit 0x10 (returns 1), or until the timeout check func_8004657C(0x1C0) fires (returns 0). */
#include "psx_types.h"

extern s32 func_800435CC(s32, s32, u8 *);
extern s32 func_8004657C(s32);
extern u8 D_800EC9E8[];

s32 func_800D2A24(s32 arg0) {
    s32 result;

retry:
    do {
        result = func_800435CC(2, arg0, D_800EC9E8);
        if (D_800EC9E8[0] & 0x10) {
            return 1;
        }
    } while (result == 0);
    if (func_8004657C(0x1C0) == 0) {
        goto retry;
    }
    return 0;
}
