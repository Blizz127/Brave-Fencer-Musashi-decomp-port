/* Independently re-derived from retail assembly; clean input set. */
#include "psx_types.h"

void func_80036F18();                                  /* extern */
extern s8 D_800A4F1A;
extern s32 D_800C6D28;

void func_80036EE8(void) {
    func_80036F18();
    D_800C6D28 = 0;
    D_800A4F1A = 0;
}
