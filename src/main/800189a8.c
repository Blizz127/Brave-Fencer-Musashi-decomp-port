/* Exact retail word export for [800189A8,80018A20); EXE and assembly verified. */
#include "controller_report.h"

/* Decompiled by m2c from main.s, then verified byte-exact against
 * retail by tools/match_function.py. Statement order adjusted: the
 * flag store leads so its constant load lands at the top. */

extern s32 D_800A5E78;

void func_800189A8(void) {
    PadRecord *var_s0;
    u32 var_s1;

    D_800A5E78 = 1;
    var_s1 = 0;
    var_s0 = D_80078D98;
    do {
        if (func_80018A20((s32)var_s1) != 0) {
            func_80018C64(var_s0);
        }
        func_80018CE8(var_s0);
        func_80018E9C(var_s0);
        var_s1 += 1;
        var_s0++;
    } while (var_s1 < 2U);
}
