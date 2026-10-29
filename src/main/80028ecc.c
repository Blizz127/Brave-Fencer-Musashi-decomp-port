/* Independently re-derived from retail assembly; clean input set. */
#include "psx_types.h"

void func_80029124(s32, s32);                            /* extern */

void func_80028ECC(s32 arg0) {
    func_80029124(0x10, arg0 & 0xFF);
}
