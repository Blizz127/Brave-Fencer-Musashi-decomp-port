/* Independently re-derived from retail assembly; clean input set. */
#include "psx_types.h"

void func_80029344();                            /* extern */
extern s32 D_80078E90;

void func_8002931C(s32 arg0) {
    D_80078E90 = arg0;
    func_80029344();
}
