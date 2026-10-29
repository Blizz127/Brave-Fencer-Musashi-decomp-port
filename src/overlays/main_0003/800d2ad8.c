/* Owner re-derivation from retail assembly and m2c-assisted drafting. */
#include "psx_types.h"
#ifndef NULL
#define NULL ((void *)0)
#endif

extern s32 D_800DB530;
extern s32 D_800DB570;
extern s32 D_800DB5B4;

s32 *func_800D2AD8(s32 *arg0) {
    s32 *var_a1;
    s32 *var_a1_2;
    s32 *var_a1_3;
    s32 *var_a2;
    s32 *var_a2_2;
    s32 *var_a2_3;
    s32 temp_v0;
    s32 temp_v0_2;
    s32 temp_v0_3;
    s32 var_v1;
    s32 var_v1_2;
    s32 var_v1_3;

    var_a2 = arg0;
    var_a1 = &D_800DB530;
    var_v1 = 0xF;
    do {
        temp_v0 = *var_a1;
        var_a1 += 1;
        var_v1 -= 1;
        *var_a2 = temp_v0;
        var_a2 += 1;
    } while (var_v1 != -1);
    var_a2_2 = arg0 + 0x10;
    var_a1_2 = &D_800DB570;
    var_v1_2 = 0xF;
    do {
        temp_v0_2 = *var_a1_2;
        var_a1_2 += 1;
        var_v1_2 -= 1;
        *var_a2_2 = temp_v0_2;
        var_a2_2 += 1;
    } while (var_v1_2 != -1);
    var_a2_3 = arg0 + 0x20;
    var_a1_3 = &D_800DB5B4;
    var_v1_3 = 0x1F;
    do {
        temp_v0_3 = *var_a1_3;
        var_a1_3 += 1;
        var_v1_3 -= 1;
        *var_a2_3 = temp_v0_3;
        var_a2_3 += 1;
    } while (var_v1_3 != -1);
    return arg0;
}
