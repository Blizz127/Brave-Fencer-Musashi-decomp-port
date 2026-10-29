/* Independently re-derived from retail assembly; clean input set. */
#include "psx_types.h"

void func_80010AE0(s32);                     /* extern */
void func_800116E0();                            /* extern */
void func_80011778();                            /* extern */
void func_80014338();                            /* extern */
s32 func_80019A24(s32 *, s32, s32, s32 *);  /* extern */
void func_8002D4C8(s32, s32);            /* extern */
extern s32 D_80072C70;
extern s32 D_800A2E20;
extern s32 D_800AE848;
extern s16 D_800B99DA;
extern s32 D_800C7C60;
extern s32 *D_800C7C64;

void func_80010ED4(void) {
    D_800C7C60 = 0x57;
    D_800C7C64 = &D_800A2E20;
    if (func_80019A24(&D_800AE848, D_80072C70, 0, &D_800C7C60) != 0) {
        func_8002D4C8(2, 0);
        func_80010AE0(0x3E0);
        func_80014338();
        func_800116E0();
        D_800B99DA = 0;
        func_80011778();
    }
}
