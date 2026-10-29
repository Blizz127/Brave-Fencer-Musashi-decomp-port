/* Owner re-derivation from retail assembly (clean input set). */
/* Replaces every table entry equal to arg0 with arg1. */
#include "psx_types.h"

extern s32 D_8006AEE8;
extern s32 D_80078F10[];

void func_80036FB0(s32 from, s32 to) {
    s32 i;

    for (i = 0; i < D_8006AEE8; i++) {
        if (D_80078F10[i] == from) {
            D_80078F10[i] = to;
        }
    }
}
