/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

extern s32 D_801BD6A4;
extern s32 D_801BD6A8;
extern s32 D_801BD6D0;
extern s32 D_801BD6D4;

void func_8013CB20(void) {
    s32 t0 = D_801BD6A8;
    s32 t1 = D_801BD6A4;
    D_801BD6A8 = 1;
    D_801BD6A4 = 0;
    D_801BD6D0 = t0;
    D_801BD6D4 = t1;
}
