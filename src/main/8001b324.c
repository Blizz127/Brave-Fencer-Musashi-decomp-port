/* Independently re-derived from retail assembly; clean input set. */
#include "psx_types.h"

void func_8001B34C();                                  /* extern */
extern s32 D_800AE7EC;

void func_8001B324(void) {
    D_800AE7EC = 0;
    func_8001B34C();
}
