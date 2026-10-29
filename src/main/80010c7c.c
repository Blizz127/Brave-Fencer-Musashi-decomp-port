/* Independently re-derived from retail assembly; clean input set. */
#include "psx_types.h"

void func_80010AE0(s32);                     /* extern */
void func_80011778();                            /* extern */
s32 func_80019A24(s32 *, s32, s32, s32); /* extern */
extern s32 D_80072C70;
extern u8 D_800AE838[];

void func_80010C7C(void) {
    s32 sp10;

    sp10 = func_80019A24(D_800AE838, D_80072C70, 0, 0);
    if (sp10 != 0) {
        func_80010AE0(0x3E0);
        func_80011778();
    }
}
