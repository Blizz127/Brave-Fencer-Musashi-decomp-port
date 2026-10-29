/* Independently re-derived from retail assembly; clean input set. */
#include "psx_types.h"

void func_8002D4C8(s32, s32);                              /* extern */
extern s32 D_800747E4;
extern s32 D_800747E8;
extern s32 D_800BA1B4;

void func_8001931C(void) {
    if (D_800BA1B4 == 5) {
        if (D_800747E4 != 0) {
            func_8002D4C8(0x16, 0);
        }
        D_800BA1B4 = 0;
        D_800747E8 = 0;
        D_800747E4 = 0;
    }
}
