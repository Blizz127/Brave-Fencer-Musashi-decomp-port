/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

typedef s32 (*Fn)(s32);

extern s32 func_80029504(void);
extern Fn D_800D375C;
extern s32 func_8001AAA0(s32 a0);
extern s32 D_8012704C;
extern s32 D_801151F8;
extern s32 D_80126AE8;
extern s32 D_80127500;
extern s32 D_801201F0;
extern s32 D_80127050;
extern s32 D_800A654C;

s32 func_800D0CE0(void) {
    s32 s0;
    s32 r = func_80029504();
    Fn fn = D_800D375C;

    if (fn == 0) {
        s0 = 0;
    } else {
        s0 = fn(r);
    }
    if (func_8001AAA0(s0) != 0) {
        D_8012704C = 0;
        D_801151F8 = 0;
        D_80126AE8 = 0;
        D_80127500 = 0;
        D_8012704C = 0;
        D_801201F0 = 0;
        D_80127050 = 0;
        D_800A654C = s0;
        return 1;
    }
    return 0;
}
