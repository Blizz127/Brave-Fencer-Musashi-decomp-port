/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

extern u8 D_801BE9D2;
extern u8 D_801BEA46;
extern u8 D_801BE8FA;
extern u8 D_801BE9B4;
extern u8 D_801BE8D8;
extern u8 D_801BE8F9;
extern s32 D_801BE9B8;

/* Steps each of three current/target byte pairs one unit toward its target,
 * then marks the state active. */
void func_8013D330(void) {
    u8 cur;
    u8 target;

    cur = D_801BE9D2;
    target = D_801BEA46;
    if (cur != target) {
        D_801BE9D2 = (cur < target) ? cur + 1 : cur - 1;
    }

    cur = D_801BE8FA;
    target = D_801BE9B4;
    if (cur != target) {
        D_801BE8FA = (cur < target) ? cur + 1 : cur - 1;
    }

    cur = D_801BE8D8;
    target = D_801BE8F9;
    if (cur != target) {
        D_801BE8D8 = (cur < target) ? cur + 1 : cur - 1;
    }

    D_801BE9B8 = 1;
}
