/* Owner re-derivation from retail assembly (clean input set). */
/* Finishes the current stream: waits for the pending read (func_8003C4F0) if one is outstanding, then clears the stream flags and restarts the streamer; returns 1 when done. */
#include "psx_types.h"

extern void func_800373D0(void);
extern s32 func_8003C4F0(s32);
extern u8 D_8006AEF4;
extern u8 D_80076242[];

s32 func_8003775C(void) {
    if (D_80076242[0] != 0) {
        if (func_8003C4F0(0) != 0) {
            D_80076242[0] = 0;
            D_8006AEF4 &= 0xFC;
            func_800373D0();
            return 1;
        }
        return 0;
    }
    D_8006AEF4 &= 0xFC;
    func_800373D0();
    return 1;
}
