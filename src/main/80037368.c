/* Independently re-derived from retail assembly; clean input set. */
#include "psx_types.h"

extern u8 D_80076220;
extern u8 D_80076250[];
extern u8 D_80076251[];

u8 *func_80037368(s32 *arg0) {
    s32 var_a1;
    s32 var_v1;
    u8 *var_a2;

    var_a1 = 0;
    var_a2 = &D_80076220;
    var_v1 = 0;
    do {
        *var_a2 = 0;
        var_a2 += 1;
        if (D_80076251[var_v1] != 0) {
            (&D_80076220)[var_a1] = D_80076250[var_v1];
            var_a1 += 1;
        }
        var_v1 += 0x10;
    } while (var_v1 < 0x50);
    *arg0 = var_a1;
    return &D_80076220;
}
