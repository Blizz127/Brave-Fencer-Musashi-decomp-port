/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

extern s32 D_801B751C;
extern void func_80179B74(s16 *a0);

void func_8017A0C4(s32 a0) {
    s16 buf;

    D_801B751C = a0;
    buf = 0x16;
    func_80179B74(&buf);
}
