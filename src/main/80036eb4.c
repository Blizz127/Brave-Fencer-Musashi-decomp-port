/* Independently re-derived from retail assembly; clean input set. */
#include "psx_types.h"

void func_80036F18();                            /* extern */
extern u8 D_800A4F1A;
extern s32 D_800C6D28;

void func_80036EB4(void) {
    if (D_800A4F1A == 0) {
        func_80036F18();
        D_800C6D28 = 0;
    }
}
