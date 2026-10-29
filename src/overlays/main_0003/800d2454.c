/* Independently re-derived from retail assembly; clean input set.
 * Returns func_800D2A24's result: the caller (800D1F68) tests v0 after the
 * jal, and the retail words leave it there (both forms match byte-exact). */
#include "psx_types.h"

void func_80043088(s32, s32);                            /* extern */
void func_80046ABC(s32, s32, s32, s32, s32);               /* extern */
s32 func_800D2A24(s32);                                /* extern */
void func_800D2AA0(s32);                                 /* extern */
void func_800D2D44(s32);                                 /* extern */
extern s32 D_800D946C;

typedef struct { s32 pad; s32 x; s32 y; } Args;

s32 func_800D2454(s32 arg0, s32 arg1, Args *arg2) {
    func_800D2AA0(0);
    func_800D2D44(arg1);
    func_80043088(D_800D946C, 0x20);
    func_80046ABC(arg2->x, arg2->y, -1, 0, 0);
    return func_800D2A24(arg0);
}
