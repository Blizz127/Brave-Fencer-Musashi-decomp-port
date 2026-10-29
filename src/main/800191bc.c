/* Owner re-derivation from retail assembly (clean input set). */
/* Stores the sign-extended 16-bit argument into a global word. */
#include "psx_types.h"

extern s32 D_80078E30;

void func_800191BC(s16 arg0) {
    D_80078E30 = arg0;
}
