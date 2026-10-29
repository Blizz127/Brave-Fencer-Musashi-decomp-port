/* Independently re-derived from retail assembly; clean input set. */
#include "psx_types.h"

void func_80034DFC(s16);                         /* extern */
extern s32 D_8006AEE8;
extern s32 D_80078F10;
extern s32 D_800A63E8;
extern s32 D_800C6D28;

void func_80036F18(void) {
    s32 *var_s0;
    s32 var_s1;
    s32 bound;
    volatile s32 reserved[2];

    if (D_8006AEE8 != 0) {
        D_800C6D28 = D_800A63E8;
        var_s1 = 0;
        if (D_8006AEE8 > 0) {
            var_s0 = &D_80078F10;
            do {
                var_s1 += 1;
                func_80034DFC(*(s16 *)var_s0);
                bound = D_8006AEE8;
                *var_s0 = 0;
                var_s0 += 1;
            } while (var_s1 < bound);
        }
        D_8006AEE8 = 0;
    }
}
