/* Independently re-derived from retail assembly; clean input set. */
#include "psx_types.h"

void func_80053AF8(s16, s16, s16);                     /* extern */
extern s16 D_800AE7E0;
extern s16 D_800AE7E2;
extern s16 D_800AE7E4;

void func_80021120(s32 *arg0) {
    func_80053AF8(D_800AE7E0, D_800AE7E2, D_800AE7E4);
    *arg0 += 1;
}
