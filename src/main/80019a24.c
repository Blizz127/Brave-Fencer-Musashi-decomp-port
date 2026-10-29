/* Owner re-derivation from retail assembly (clean input set). */
/* Starts a new streaming request for a data block unless the streamer is busy or a different block is already active; returns the request status. */
#include "psx_types.h"

extern void func_80019AF8(s32);
extern s32 func_80034B98(void);
extern s32 D_800AE708;
extern s32 D_800AE720;
extern s32 D_800AE724;
extern s32 D_800AE72C;
extern s32 *D_800AE7A4;
extern s32 D_800AE7A8;
extern s32 D_800AE7B0;

s32 func_80019A24(s32 *data, s32 mode, s32 dest, s32 wait) {
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
