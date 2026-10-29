/* Owner re-derivation from retail assembly (clean input set). */
/* Starts music track `id` via func_8002FF0C (first flushing a pending stream if needed); track 26 also triggers sound request 40. Returns the start result. */
#include "psx_types.h"

extern void func_8002D240(s32);
extern s32 func_8002FF0C(s32, s32);
extern void func_80037D74(void);
extern s32 func_8003C4F0(s32);
extern u8 D_8006AEF4;
extern s16 D_800A46CC;

s32 func_8002FD14(s32 id, s32 arg) {
    s32 result;

    if (D_800A46CC == 0) {
        func_80037D74();
        if (func_8003C4F0(0) != 0) {
            D_8006AEF4 &= 0xFE;
            result = func_8002FF0C(id, arg);
        } else {
            result = 0;
        }
    } else {
        result = func_8002FF0C(id, arg);
    }
    if (result != 0) {
        if (id == 26) {
            func_8002D240(40);
            return result;
        }
        return result;
    }
    return 0;
}
