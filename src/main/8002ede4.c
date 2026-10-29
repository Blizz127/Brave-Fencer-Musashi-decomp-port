/* Owner re-derivation from retail assembly and m2c-assisted drafting. */
#include "psx_types.h"
#ifndef NULL
#define NULL ((void *)0)
#endif

s32 func_8002EC10();                            /* extern */
s32 func_8002EE90();                            /* extern */
s32 func_80031CC8();                            /* extern */
s32 func_800385C0(s16);                         /* extern */
extern s16 D_800A4E9A;
extern u16 D_800A4EA2;
extern u8 D_800A4EE6;
extern s16 D_800A4EF6;

void func_8002EDE4(void) {
    func_8002EE90();
    func_80031CC8();
    func_8002EC10();
    if (D_800A4EA2 & 4) {
        func_800385C0(D_800A4E9A);
        D_800A4EA2 = 0;
    }
    D_800A4EF6 = 1;
    D_800A4EE6 &= 0xF9;
}
